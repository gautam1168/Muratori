#if !defined(WIN32_HANDMADE_HPP)
struct win32_offscreen_buffer {
  BITMAPINFO BitmapInfo;
  void* Memory;
  int Width;
  int Height;
  int BytesPerPixel; 
  int Pitch;
};

struct win32_window_dimensions {
  int Width;
  int Height;
};

struct win32_sound_output {
  int SamplesPerSecond;
  uint32 RunningSampleIndex;
  int BytesPerSample;
  int SecondaryBufferSize;
  DWORD SafteyBytes;
  real32 tSine;
  int LatencySampleCount;
};

struct win32_debug_time_marker {
  DWORD FlipPlayCursor;
  DWORD FlipWriteCursor;

  DWORD OutputLocation;
  DWORD OutputByteCount;

  DWORD OutputPlayCursor;
  DWORD OutputWriteCursor;
};

#define WIN32_HANDMADE_HPP
#endif