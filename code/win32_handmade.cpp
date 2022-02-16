#include <windows.h>
#include <stdint.h>

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

struct win32_window_dimensions {
  int Width;
  int Height;
};

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

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height) {

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

internal void Win32DisplayBufferInWindow(HDC DeviceContext, win32_window_dimensions WinDims, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height) {
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
    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - X;
      int Height = Paint.rcPaint.bottom - Y;

      win32_window_dimensions windims = GetWindowDimensions(Window);

      Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer, X, Y, Width, Height);

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

  WNDCLASS WindowClass = {};

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

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
      int XOffset = 0, YOffset = 0;
      MSG Message;
      Running = true;
      while(Running) {
        BOOL MessageResult = PeekMessageA(&Message, 0, 0, 0, PM_REMOVE);
        if (MessageResult) {
          if (Message.message == WM_QUIT) {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }  
        RenderWeirdGradient(GlobalBackBuffer, XOffset++, YOffset++);
        {
          HDC DeviceContext = GetDC(WindowHandle);
          win32_window_dimensions windims = GetWindowDimensions(WindowHandle);
          Win32DisplayBufferInWindow(DeviceContext, windims, GlobalBackBuffer, 0, 0, windims.Width, windims.Height);
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