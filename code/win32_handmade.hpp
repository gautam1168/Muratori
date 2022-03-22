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

typedef struct win32_game_code {
  HMODULE GameCodeDll;
  FILETIME DLLLastWriteTime;
  game_update_and_render *UpdateAndRender;
  game_get_sound_samples *GetSoundSamples;
  bool isValid;
} win32_game_code;

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
typedef struct win32_state {
  uint64 TotalGameMemorySize;
  void *GameMemoryBlock;

  HANDLE RecordingHandle;
  int InputRecordingIndex; 

  HANDLE PlaybackHandle;
  int InputPlayingIndex;

  char EXEFilename[WIN32_STATE_FILE_NAME_COUNT];
  char *OnePastLastExeFilenameSlash;
} win32_state;

#define WIN32_HANDMADE_HPP
#endif