#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

#include "handmade.hpp"
#include "handmade.cpp"

#include "win32_handmade.hpp"

global_variable bool Running, SoundIsPlaying;
global_variable win32_offscreen_buffer GlobalBackBuffer;
// global_variable int XOffset = 0, YOffset = 0;
global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;
global_variable win32_sound_output SoundOutput;

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
DWORD WINAPI xInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE *pState) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = xInputGetStateStub;
#define XInputGetState XInputGetState_

typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
DWORD WINAPI xInputSetStateStub(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = xInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput() {
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (XInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

internal void Win32ClearBuffer(win32_sound_output *SoundOutput) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(SecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
    uint8 *DestSample = (uint8 *)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++) {
      *DestSample++ = 0;
    }

    DestSample = (uint8 *)Region2;
    for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++) {
      *DestSample++ = 0;
    }
    SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal void Win32FillSoundBuffer(uint32 BytesPerSample, uint32 *RunningSampleIndex, DWORD ByteToLock, DWORD BytesToWrite, game_sound_buffer *SourceBuffer) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0 ))) {
    int16 *SourceSamples = SourceBuffer->Samples;

    int16 *SampleOut = (int16 *)Region1;
    DWORD Region1SampleCount = Region1Size/BytesPerSample;
    for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
      *(SampleOut++) = *(SourceSamples++);
      *(SampleOut++) = *(SourceSamples++);
      *RunningSampleIndex += 1;
    }

    SampleOut = (int16 *)Region2;
    DWORD Region2SampleCount = Region2Size/BytesPerSample;
    for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
      *(SampleOut++) = *(SourceSamples++);
      *(SampleOut++) = *(SourceSamples++);
      *RunningSampleIndex += 1;

    }
    SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal void Wind32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if (DSoundLibrary) {
    direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample)/8;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,  &PrimaryBuffer, 0))) {
          if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {

          } else {
            OutputDebugString("Failed to set wave format");
          }
        }
      }
      DSBUFFERDESC BufferDescription= {};
      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwFlags = 0;
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;
      if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0))) {
        OutputDebugString("Created secondary buffer");
      } else {
        OutputDebugString("Failed to create secondary buffer");
      }
    }
  }
}

win32_window_dimensions GetWindowDimensions(HWND Window) {
  win32_window_dimensions Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}

internal void InitializeGlobalBackBuffer(win32_offscreen_buffer* Buffer, int Width, int Height) {
  if (Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

  Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
  Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->Width;
  Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->Height;
  Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
  Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
  Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = Buffer->BytesPerPixel * Width * Height;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // RenderWeirdGradient(GlobalBackBuffer, 0, 0);
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, win32_window_dimensions WinDims, win32_offscreen_buffer Buffer) {
  StretchDIBits(
    DeviceContext,
    0, 0, WinDims.Width, WinDims.Height,
    0, 0, Buffer.Width, Buffer.Height,
    Buffer.Memory, 
    &Buffer.BitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

internal void Win32ProcessXInputDigitalButton(
  DWORD XInputButtonState,
  game_button_state *OldState, DWORD ButtonBit,
  game_button_state *NewState
) {
  NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
  NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

LRESULT CALLBACK Win32MainWindowCallback(
  HWND   Window,
  UINT   Message,
  WPARAM wParam,
  LPARAM lParam
) {
  LRESULT Result = 0;
  switch(Message) {
    case WM_SIZE:
    {
      OutputDebugString("WM_SIZE\n");
    }
    break;
    case WM_DESTROY:
    {
      Running = false;
      OutputDebugString("WM_DESTROY\n");
    }
    break;
    case WM_CLOSE:
    {
      Running = false;
      OutputDebugString("WM_CLOSE\n");
    }
    break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugString("WM_ACTIVATEAPP\n");
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      uint32 VKCode = wParam;
      bool wasDown = (lParam & (1 << 30)) != 0;
      bool IsDown = (lParam & (1 << 30)) == 0;
      if (VKCode == 'W') {

      } else if (VKCode == 'A') {
      } else if (VKCode == 'S') {

      } else if (VKCode == 'D') {
      } else if (VKCode == 'Q') {

      } else if (VKCode == 'E') {

      } else if (VKCode == VK_UP) {

      } else if (VKCode == VK_DOWN) {

      } else if (VKCode == VK_LEFT) {

      } else if (VKCode == VK_RIGHT) {

      } else if (VKCode == VK_SPACE) {

      } else if (VKCode == VK_ESCAPE) {

      }

      bool AltKeyWasDown = (lParam & (1 << 29)) != 0; 
      if (VKCode == VK_F4) {
        Running = false;
      }
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      win32_window_dimensions windims = GetWindowDimensions(Window);
      Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer);
      EndPaint(Window, &Paint);
    }
    break;
    default:
    {
      OutputDebugString("default\n");
      Result = DefWindowProc(Window, Message, wParam, lParam);
    }
  }
  return Result;
}

int WINAPI wWinMain(
  HINSTANCE Instance, 
  HINSTANCE PrevInstance, 
  PWSTR CommandLine, 
  int ShowCode) {

  Win32LoadXInput();

  WNDCLASS WindowClass = {};

  InitializeGlobalBackBuffer(&GlobalBackBuffer, 1280, 720);

  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon = ;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  
  LARGE_INTEGER PerfCountFrequency;
  QueryPerformanceFrequency(&PerfCountFrequency);

  int64 LastCycleCount = __rdtsc();

  if (RegisterClassA(&WindowClass)) {
    HWND Window = CreateWindowExA(
      0,
      WindowClass.lpszClassName,
      "Handmade Hero",
      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0,
      0,
      Instance,
      0
    );

    if (Window) {
      Running = true;

      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = sizeof(int16)*2;
      SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

      Wind32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
      Win32ClearBuffer(&SoundOutput);
      SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

      game_input Input[2] = {};
      game_input *NewInput = &Input[0];
      game_input *OldInput = &Input[1];
      LARGE_INTEGER LastCounter;
      QueryPerformanceCounter(&LastCounter);

      LPVOID BaseAddress;

#if HANDMADE_INTERNAL
  BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
  BaseAddress = 0;
#endif

      game_memory GameMemory = {};
      GameMemory.PermanentStorageSize = Megabytes(64);
      GameMemory.TransientStorageSize = Gigabytes((uint64)4);
      uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
      GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_COMMIT, PAGE_READWRITE);

      GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

      if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
        while(Running) {
          MSG Message;


          BOOL MessageResult = PeekMessageA(&Message, 0, 0, 0, PM_REMOVE);
          if (MessageResult) {
            if (Message.message == WM_QUIT) {
              Running = false;
            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
          }  
          int MaxControllerCount = XUSER_MAX_COUNT;
          if (MaxControllerCount > ArrayCount(NewInput->Controllers)) {
            MaxControllerCount = ArrayCount(NewInput->Controllers);
          }
          DWORD dwResult;    
          for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
          {
              game_controller_input *OldController = &OldInput->Controllers[i];
              game_controller_input *NewController = &NewInput->Controllers[i];
              XINPUT_STATE state;
              ZeroMemory( &state, sizeof(XINPUT_STATE) );

              // Simply get the state of the controller from XInput.
              dwResult = XInputGetState( i, &state );

              if( dwResult == ERROR_SUCCESS )
              {
                // Controller is connected
                XINPUT_GAMEPAD *Pad = &state.Gamepad;
                bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP); 
                bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                NewController->IsAnalog = true;
                NewController->StartX = OldController->EndX;
                NewController->StartY = OldController->EndY;

                real32 X;
                if (Pad->sThumbLX < 0) {
                  X = (real32)Pad->sThumbLX / 32768.0f;
                } else {
                  X = (real32)Pad->sThumbLX / 32767.0f;
                }
                NewController->MinX = NewController->MaxX = X;

                real32 Y;
                if (Pad->sThumbLY < 0) {
                  Y = (real32)Pad->sThumbLY / 32768.0f;
                } else {
                  Y = (real32)Pad->sThumbLY / 32767.0f;
                }
                NewController->MinY = NewController->MaxY = Y;

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A,
                  &NewController->Down);
                
                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B,
                  &NewController->Right);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X,
                  &NewController->Left);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y,
                  &NewController->Up);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                  &NewController->LeftShoulder);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                  &NewController->RightShoulder);

                // if (DPadUp) {
                //   XOffset++;
                // }
              }
              else
              {
                  // Controller is not connected
              }
          }

          DWORD ByteToLock;
          DWORD TargetCursor;
          DWORD BytesToWrite;
          DWORD PlayCursor;
          DWORD WriteCursor;
          bool SoundIsvalid = false;

          if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
            ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

            TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize));

            if (ByteToLock > PlayCursor) {
              BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
              BytesToWrite += PlayCursor; 
            } else {
              BytesToWrite = PlayCursor - ByteToLock;
            }
            SoundIsvalid = true;
          }

          game_offscreen_buffer Buffer = {};
          Buffer.Memory = GlobalBackBuffer.Memory;
          Buffer.Height = GlobalBackBuffer.Height;
          Buffer.Width = GlobalBackBuffer.Width;
          Buffer.Pitch = GlobalBackBuffer.Pitch;

          game_sound_buffer SoundBuffer = {};
          SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
          SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
          SoundBuffer.Samples = Samples;
          int16 *SourceSamples = Samples;

          GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

          if (SoundIsvalid) {
            Win32FillSoundBuffer(SoundOutput.BytesPerSample, &SoundOutput.RunningSampleIndex, ByteToLock, BytesToWrite, &SoundBuffer);
          }

          win32_window_dimensions windims = GetWindowDimensions(Window);
          // AcqRelease DeviceContext
          {
            HDC DeviceContext = GetDC(Window);
            Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer);
            ReleaseDC(Window, DeviceContext);
          }
          LARGE_INTEGER EndCounter;
          QueryPerformanceCounter(&EndCounter);

          
          int64 EndCycleCount = __rdtsc();

          int64 CyclesElapsed = EndCycleCount - LastCycleCount;
          int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
          int32 TimeElapsedMs = (int32)(1000*CounterElapsed/PerfCountFrequency.QuadPart);
          int32 FPS = PerfCountFrequency.QuadPart / CounterElapsed;
          int32 MCPF = (int32)(CyclesElapsed/(1000 * 1000));
#if 0        
          char Buffer[256];
          wsprintf(Buffer, "Milliseconds/frame: %dms, FPS: %d, MCPF: %dmc/f\n", TimeElapsedMs, FPS, MCPF);
          OutputDebugString(Buffer);
          LastCounter = EndCounter;
          LastCycleCount = EndCycleCount;

#endif
          game_input *Temp;
          Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;
        }
      } else {
        OutputDebugString("Could not obtain game memory");
      } 
    } else {
      OutputDebugString("No windowhandle was obtained from createWindow");
    }
    
  } else {
    OutputDebugString("Registration of window failed");
  }
  
  return 0;
}