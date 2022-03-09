#if !defined(HANDMADE_HPP)

/*
  HANDMADE_INTERNAL
    - 0  for release
    - 1  for devs

  HANDMADE_SLOW
    - 0 performance
    - 1 debug asserts
*/

#if HANDMADE_SLOW
#define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(count) ((count) * 1024)
#define Megabytes(count) (Kilobytes(count) * 1024)
#define Gigabytes(count) (Megabytes(count) * 1024)
#define Terabytes(count) (Gigabytes(count) * 1024)

struct game_offscreen_buffer {
  void* Memory;
  int Width;
  int Height;
  int Pitch;
};

struct game_sound_buffer {
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

struct game_button_state {
  int HalfTransitionCount;
  bool EndedDown;
};

struct game_controller_input {
  bool IsAnalog;

  real32 StickAverageX;
  real32 StickAverageY;
    
  union {
    game_button_state Buttons[10];
    struct {
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionLeft;
      game_button_state ActionRight;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
};

struct game_memory {
  bool IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage; // Required to be 0 on allocation
  uint64 TransientStorageSize;
  void *TransientStorage;
};

struct game_input {
  game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));

  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}

struct debug_read_file_result {
  uint32 ContentSize;
  void *Contents;
};

struct game_state {
  int GreenOffset;
  int BlueOffset;
  int ToneHz;
};

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer);


#define HANDMADE_HPP
#endif