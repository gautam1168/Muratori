#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static;
#define local_persist static;
#define global_variable static;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer {
  BITMAPINFO BitmapInfo;
  void* Memory;
  int Width;
  int Height;
  int BytesPerPixel; 
  int Pitch;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int XOffset = 0, YOffset = 0;

struct win32_window_dimensions {
  int Width;
  int Height;
};

/*
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(xInputGetStateStub) {
  return 0;
}
global_variable x_input_get_state *XInputGetState_ = xInputGetStateStub;
#define XInputGetState XInputGetState_;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(xInputSetStateStub) {
  return 0;
}
global_variable x_input_set_state *XInputSetState_ = xInputSetStateStub;
#define XInputSetState XInputSetState_;
*/

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
DWORD WINAPI xInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE *pState) {
  return 0;
}
global_variable x_input_get_state *XInputGetState_ = xInputGetStateStub;
#define XInputGetState XInputGetState_

typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
DWORD WINAPI xInputSetStateStub(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration) {
  return 0;
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

win32_window_dimensions GetWindowDimensions(HWND Window) {
  win32_window_dimensions Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset) {
  uint8* Row = (uint8*) Buffer.Memory;
  for (int Y = 0; Y < Buffer.Height; ++Y) {
    uint32* Pixel = (uint32*) Row;
    for (int X = 0; X < Buffer.Width; ++X) {
      uint8 Blue = (X + XOffset);
      uint8 Green = (Y + YOffset);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += Buffer.Pitch;
  }
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

  RenderWeirdGradient(GlobalBackBuffer, 0, 0);
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
        XOffset -= 10;
      } else if (VKCode == 'S') {

      } else if (VKCode == 'D') {
        XOffset += 10;
      } else if (VKCode == 'Q') {

      } else if (VKCode == 'E') {

      } else if (VKCode == VK_UP) {

      } else if (VKCode == VK_DOWN) {

      } else if (VKCode == VK_LEFT) {

      } else if (VKCode == VK_RIGHT) {

      } else if (VKCode == VK_SPACE) {

      } else if (VKCode == VK_ESCAPE) {

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
  
  if (RegisterClassA(&WindowClass)) {
    HWND WindowHandle = CreateWindowExA(
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

    if (WindowHandle) {
      Running = true;
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

        DWORD dwResult;    
        for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
        {
            XINPUT_STATE state;
            ZeroMemory( &state, sizeof(XINPUT_STATE) );

            // Simply get the state of the controller from XInput.
            dwResult = XInputGetState( i, &state );

            if( dwResult == ERROR_SUCCESS )
            {
              // Controller is connected
              XINPUT_GAMEPAD *Pad = &state.Gamepad;
              bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP); 

              if (DPadUp) {
                XOffset++;
              }
            }
            else
            {
                // Controller is not connected
            }
        }

        RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset++);
        win32_window_dimensions windims = GetWindowDimensions(WindowHandle);
        // AcqRelease DeviceContext
        {
          HDC DeviceContext = GetDC(WindowHandle);
          Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer);
          ReleaseDC(WindowHandle, DeviceContext);
        }
      }
    } else {
      OutputDebugString("No windowhandle was obtained from createWindow");
    }
    
  } else {
    OutputDebugString("Registration of window failed");
  }
  

  return 0;
}