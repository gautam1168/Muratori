#include "handmade.hpp"

// Platform -> Game

// Game -> Platform
internal void GameOutputSound(game_sound_buffer *SoundBuffer, int ToneHz) {
  local_persist real32 tSine = 0.0f;
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;  ++SampleIndex) {
    real32 SineValue = sinf(tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    tSine += 2.0f * Pi32 * 1.0f/(real32)WavePeriod;
  }
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, game_state *GameState) {
  uint8* Row = (uint8*) Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32* Pixel = (uint32*) Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8 Blue = (uint8)(X + GameState->BlueOffset);
      uint8 Green = (uint8)(Y + GameState->GreenOffset);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += Buffer->Pitch;
  }
}

inline uint32 SafeTruncateUint64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

internal void GameUpdateAndRender(
  game_memory *Memory,
  game_input *Input,
  game_offscreen_buffer *Buffer
) {

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {

    char *FileName = __FILE__;
    debug_read_file_result FileData = DEBUGPlatformReadEntireFile(FileName);
    if (FileData.Contents) {
      DEBUGPlatformWriteEntireFile("out.txt", FileData.ContentSize, FileData.Contents);
      DEBUGPlatformFreeFileMemory(FileData.Contents);
      FileData.ContentSize = 0;
    }

    GameState->ToneHz = 256;
    Memory->IsInitialized = true;
  }

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {
      GameState->ToneHz = 256 + (int)(128.0f*(Controller->StickAverageX));
      GameState->BlueOffset = (int)(4.0f*(Controller->StickAverageY));
    } else {
      if (Controller->MoveLeft.EndedDown) {
        GameState->BlueOffset -= 1;
      }
      if (Controller->MoveRight.EndedDown) {
        GameState->BlueOffset += 1;
      }
    }

    if (Controller->ActionDown.EndedDown) {
      GameState->GreenOffset += 1;
    }
  } 
  
  RenderWeirdGradient(Buffer, GameState);
}

internal void GameGetSoundSamples(game_memory *Memory, game_sound_buffer *SoundBuffer) {
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameOutputSound(SoundBuffer, GameState->ToneHz);
}