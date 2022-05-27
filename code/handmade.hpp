#if !defined(HANDMADE_HPP)

#include <stdint.h>

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else 
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef char int8;
typedef short int16;
typedef long int32;
typedef long long int64;

// #define UINT32_MAX ULONG_MAX

typedef size_t memory_index;
/*
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
*/

typedef float real32;
typedef double real64;
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

#define InvalidCodePath Assert(!"InvalidCodePath")

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(count) ((count) * 1024)
#define Megabytes(count) (Kilobytes(count) * 1024)
#define Gigabytes(count) (Megabytes(count) * 1024)
#define Terabytes(count) (Gigabytes(count) * 1024)

struct thread_context {
  int Placeholder;
};

struct game_offscreen_buffer {
  void* Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
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
  bool IsConnected;
  bool IsAnalog;

  real32 StickAverageX;
  real32 StickAverageY;
    
  union {
    game_button_state Buttons[12];
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

      game_button_state Back;
      game_button_state Start;

      game_button_state Terminator;
    };
  };
};

struct debug_read_file_result {
  uint32 ContentSize;
  void *Contents;
};

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#include "handmade_intrinsics.hpp"
#include "handmade_world.hpp"

struct game_input {
  game_button_state MouseButtons[5];
  int32 MouseX, MouseY, MouseZ;
  real32 dtForFrame;
  game_controller_input Controllers[5];
};


struct memory_arena {
  memory_index Size;
  uint8 *Base;
  memory_index Used;
};

internal void 
InitializeArena(memory_arena *Arena, memory_index Size, uint8 *Base) {
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}


#define PushStruct(Arena, StructType) (StructType *)PushSize_(Arena, sizeof(StructType))
#define PushArray(Arena, Count, StructType) (StructType *)PushSize_(Arena, (Count) * sizeof(StructType))

internal void *
PushSize_(memory_arena *Arena, memory_index Size) {
  Assert((Arena->Used + Size) < Arena->Size);
  void *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return Result;
}

struct loaded_bitmap {
  int32 Width;
  int32 Height;
  uint32 *Pixels;
};

struct hero_bitmaps {
  int32 AlignX;
  int32 AlignY;
  loaded_bitmap Body;
};

enum entity_type {
  EntityType_Null,
  EntityType_Hero,
  EntityType_Wall
};

struct high_entity {
  bool Exists;
  v2 P;
  v2 dP;
  uint32 ChunkZ;
  uint32 FacingDirection;
  uint32 LowEntityIndex;
};

struct low_entity {
  entity_type Type;
  world_position P;
  real32 Width, Height;

  bool Collides;
  int32 dAbsTileZ; // for stairs
  uint32 HighEntityIndex;
};

struct entity {
  uint32 LowIndex;
  low_entity *Low;
  high_entity *High;
};

struct low_entity_chunk_reference
{
  world_chunk *TileChunk;
  uint32 EntityIndexInChunk;
};

struct game_state
{
  uint32 ToneHz;
  memory_arena WorldArena;
  world *World;
  uint32 CamerFollowingEntityIndex;
  world_position CameraP;

  uint32 PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];
  uint32 LowEntityCount;
  low_entity LowEntities[100000];

  uint32 HighEntityCount;
  high_entity HighEntities_[256];

  loaded_bitmap Backdrop;
  hero_bitmaps HeroBitmaps[4];

  loaded_bitmap Tree;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


struct game_memory {
  bool IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage; // Required to be 0 on allocation
  uint64 TransientStorageSize;
  void *TransientStorage;
  debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
  debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
  debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));

  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}



#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{}

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{}

// void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer);
// void GameGetSoundSamples(game_memory *Memory, game_sound_buffer *SoundBuffer);


#define HANDMADE_HPP
#endif
