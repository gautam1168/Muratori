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

  real32 StartX;
  real32 StartY;
  real32 MinX;
  real32 MinY;
  real32 MaxX;
  real32 MaxY;
  real32 EndX;
  real32 EndY;
  
  union {
    game_button_state Buttons[6];
    struct {
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
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
  game_controller_input Controllers[4];
};

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer);

struct game_state {
  int GreenOffset;
  int BlueOffset;
  int ToneHz;
};

#define HANDMADE_HPP
#endif