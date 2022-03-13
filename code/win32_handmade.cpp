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

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName) {
  debug_read_file_result Result = {};
  HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize)) {
      uint32 FileSize32 = SafeTruncateUint64(FileSize.QuadPart);
      Result.Contents = VirtualAlloc(0, FileSize32, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if (Result.Contents) {
        DWORD BytesRead;
        if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && BytesRead == FileSize32) {
          Result.ContentSize = FileSize32;
        } else {
          DEBUGPlatformFreeFileMemory(Result.Contents);
          Result.Contents = 0;
          Result.ContentSize = 0;
        }
      }
    }
    CloseHandle(FileHandle);
  }
  return Result;
}

internal void DEBUGPlatformFreeFileMemory(void *Memory) {
  if (Memory) {
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory) {
  bool Result = false;
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {
    DWORD BytesWritten;
    if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
      Result = BytesWritten == MemorySize;
    }
  }
  return Result;
}

internal void
Win32LoadXInput() {
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (XInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

internal void Win32ClearBuffer(win32_sound_output *SoundOutputL) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(SecondaryBuffer->Lock(0, SoundOutputL->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
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

internal void Win32ProcessKeyboardMessage(game_button_state *state, bool isDown) {
  Assert(state->EndedDown != isDown);
  state->EndedDown = isDown;
  state->HalfTransitionCount++;
}

internal real32 Win32ProcessXInputStickValue(SHORT stickValue, SHORT deadZone) {
  real32 X = 0;
  if (stickValue < -deadZone) {
    X = (real32)stickValue / 32768.0f;
  } else if (stickValue > deadZone) {
    X = (real32)stickValue / 32767.0f;
  }
  return X;
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
      Assert(!"Keyboard message came in through a non-dispatch message!");
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

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController) {
  MSG Message;
  while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
    switch (Message.message) {
    case WM_QUIT:
    {
      Running = false;
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      uint32 VKCode = (int32)Message.wParam;
      uint32 WasDown = ((Message.lParam & (1 << 30)) != 0);
      uint32 IsDown = ((Message.lParam & (1 << 31)) == 0);
      if (WasDown != IsDown)
      {
        if (VKCode == 'W')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
        }
        else if (VKCode == 'A')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
        }
        else if (VKCode == 'S')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
        }
        else if (VKCode == 'D')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
        }
        else if (VKCode == 'Q')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
        }
        else if (VKCode == 'E')
        {
          Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
        }
        else if (VKCode == VK_UP)
        {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
        }
        else if (VKCode == VK_DOWN)
        {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
        }
        else if (VKCode == VK_LEFT)
        {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
        }
        else if (VKCode == VK_RIGHT)
        {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
        }
        else if (VKCode == VK_ESCAPE)
        {
          Running = false;
        }
        else if (VKCode == VK_SPACE)
        {
        }

        bool AltKeyWasDown = (Message.lParam & (1 << 29));
        if ((VKCode == VK_F4) && AltKeyWasDown)
        {
          Running = false;
        }
      }
    }
    break;
    default:
    {
      TranslateMessage(&Message);
      DispatchMessage(&Message);
    }
    }
  }
}

global_variable LARGE_INTEGER PerfCountFrequency;

inline LARGE_INTEGER Win32GetWallClock() {
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
  real32 Result = ((real32)end.QuadPart - (real32)start.QuadPart) / (real32)PerfCountFrequency.QuadPart;
  return Result;
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, int X, int Top, int Bottom, uint32 Color) {
  uint8 *Pixel = (uint8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch;
  for (int Y = Top; Y < Bottom; Y++) {
    *((uint32 *)Pixel) = Color;
    Pixel += BackBuffer->Pitch;
  }
}

internal void Win32DebugSyncDisplay(
    win32_offscreen_buffer *BackBuffer,
    int MarkerCount,
    win32_debug_time_marker *Markers,
    win32_sound_output *soundOutput,
    real32 TargetSecondsPerFrame)
{

  int Pad = 16;
  int Top = Pad;
  int Bottom = BackBuffer->Height - Pad;
  real32 c = (real32)(BackBuffer->Width - 2 * Pad) / (real32)soundOutput->SecondaryBufferSize;

  for (int MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++) {
    win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
    // Assert(ThisMarker->PlayCursor < soundOutput->SecondaryBufferSize);
    int X = Pad + (int)(c * (real32)ThisMarker->PlayCursor);
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, 0xFFFFFFFF);
    X = Pad + (int)(c * (real32)ThisMarker->WriteCursor);
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, 0xFFFF0000);
  }
}

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz/2)
#define FramesOfAudioLatency 2

int WINAPI wWinMain(
  HINSTANCE Instance, 
  HINSTANCE PrevInstance, 
  PWSTR CommandLine, 
  int ShowCode) {

  // int MonitorRefreshHz = 60;
  // int GameUpdateHz = MonitorRefreshHz / 2;
  real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz ;

  UINT DesiredSchedulerMS = 1;
  bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

  Win32LoadXInput();

  WNDCLASS WindowClass = {};

  InitializeGlobalBackBuffer(&GlobalBackBuffer, 1280, 720);

  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon = ;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  
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
      SoundOutput.LatencySampleCount = FramesOfAudioLatency * (SoundOutput.SamplesPerSecond / GameUpdateHz);

      Wind32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
      Win32ClearBuffer(&SoundOutput);
      SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

      LARGE_INTEGER LastCounter = Win32GetWallClock();

      LPVOID BaseAddress;

#if HANDMADE_INTERNAL
  // BaseAddress = (LPVOID)Terabytes((uint64)2); // Memory allocation fails
  BaseAddress = 0;
  int DebugLastMarkerIndex = 0;
  win32_debug_time_marker DebugLastTimeMarkers[GameUpdateHz / 2] = {0};
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
        game_input Input[2] = {};
        game_input *NewInput = &Input[0];
        game_input *OldInput = &Input[1];

        DWORD LastPlayCursor = 0;
        bool SoundIsvalid = false;

        while(Running) {
          game_controller_input *OldKeyboardController = GetController(OldInput, 0);
          game_controller_input *NewKeyboardController = GetController(NewInput, 0);
          game_controller_input ZeroController = {};
          *NewKeyboardController = ZeroController;

          for (int i = 0; i < ArrayCount(NewKeyboardController->Buttons); i++) {
            NewKeyboardController->Buttons[i].EndedDown = OldKeyboardController->Buttons[i].EndedDown;
          }

          Win32ProcessPendingMessages(NewKeyboardController);
            
          // Poll XInput devices
          int MaxControllerCount = XUSER_MAX_COUNT + 1;
          if (MaxControllerCount > ArrayCount(NewInput->Controllers)) {
            MaxControllerCount = ArrayCount(NewInput->Controllers);
          }
          DWORD dwResult;    
          for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
          {
              DWORD OurControllerIndex = i + 1; // 0 is keyboard
              game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
              game_controller_input *NewController = GetController(NewInput, OurControllerIndex);
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
                NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
                  NewController->StickAverageY = 1.0f;
                }

                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
                  NewController->StickAverageY = -1.0f;
                }

                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                  NewController->StickAverageX = -1.0f;
                }

                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
                  NewController->StickAverageX = 1.0f;
                }

                real32 Threshold = 0.5f;

                Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0,
                                                &OldController->MoveLeft, 1,
                                                &NewController->MoveLeft);

                Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0,
                                                &OldController->MoveRight, 1,
                                                &NewController->MoveRight);

                Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0,
                                                &OldController->MoveUp, 1,
                                                &NewController->MoveUp);

                Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0,
                                                &OldController->MoveDown, 1,
                                                &NewController->MoveDown);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A,
                  &NewController->ActionDown);
                
                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B,
                  &NewController->ActionRight);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                  &NewController->ActionLeft);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                  &NewController->ActionUp);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                  &NewController->LeftShoulder);

                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                  &NewController->RightShoulder);

                // Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                //   &NewController->Start);
                  
                // Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                //   &NewController->Back);

                // if (DPadUp) {
                //   XOffset++;
                // }
              }
              else
              {
                  // Controller is not connected
              }
          }

          DWORD ByteToLock = 0;
          DWORD TargetCursor;
          DWORD BytesToWrite = 0;

          if (SoundIsvalid) {
            ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

            TargetCursor = ((LastPlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize));

            if (ByteToLock > LastPlayCursor) {
              BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
              BytesToWrite += LastPlayCursor; 
            } else {
              BytesToWrite = LastPlayCursor - ByteToLock;
            }
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

          LARGE_INTEGER WorkCounter = Win32GetWallClock();
          real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
          real32 SecondsElapsedForFrame = WorkSecondsElapsed;
          if (SecondsElapsedForFrame < TargetSecondsPerFrame) {
            while(SecondsElapsedForFrame < TargetSecondsPerFrame) {
              if (SleepIsGranular) {
                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                Sleep(SleepMS);
              }
              SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            }
          } else {
            OutputDebugString("Frame missed?");
          }

          LARGE_INTEGER EndCounter = Win32GetWallClock();
          LastCounter = EndCounter;

          win32_window_dimensions windims = GetWindowDimensions(Window);
          // AcqRelease DeviceContext
          {
            HDC DeviceContext = GetDC(Window);
#if HANDMADE_INTERNAL
            Win32DebugSyncDisplay(&GlobalBackBuffer,
                                  ArrayCount(DebugLastTimeMarkers),
                                  DebugLastTimeMarkers,
                                  &SoundOutput,
                                  TargetSecondsPerFrame);
#endif
            Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer);
            ReleaseDC(Window, DeviceContext);
          }

          win32_debug_time_marker Marker = {};
          if(SecondaryBuffer->GetCurrentPosition(&Marker.PlayCursor, &Marker.WriteCursor) == DS_OK) {
            LastPlayCursor = Marker.PlayCursor;
            if (!SoundIsvalid) {
              SoundOutput.RunningSampleIndex = Marker.WriteCursor / SoundOutput.BytesPerSample;
              SoundIsvalid = true;
            }
          } else {
            SoundIsvalid = false;
          }

#if HANDMADE_INTERNAL
          {
            DebugLastTimeMarkers[DebugLastMarkerIndex++] = Marker;
            if (DebugLastMarkerIndex > ArrayCount(DebugLastTimeMarkers)) {
              DebugLastMarkerIndex = 0;
            }
          }
#endif
#if 0        
          int32 TimeElapsedMs = (int32)(1000*CounterElapsed/PerfCountFrequency.QuadPart);
          int32 FPS = (int32)(PerfCountFrequency.QuadPart / CounterElapsed);
          int32 MCPF = (int32)(CyclesElapsed/(1000 * 1000));
          char Buffer[256];
          wsprintf(Buffer, "Milliseconds/frame: %dms, FPS: %d, MCPF: %dmc/f\n", TimeElapsedMs, FPS, MCPF);
          OutputDebugString(Buffer);
#endif
          game_input *Temp;
          Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;

          int64 EndCycleCount = __rdtsc();
          int64 CyclesElapsed = EndCycleCount - LastCycleCount;
          LastCycleCount = EndCycleCount;
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