#include "handmade.hpp"

// Platform -> Game

// Game -> Platform
internal void GameOutputSound(game_sound_buffer *SoundBuffer) {
  local_persist real32 tSine = 0.0f;
  int16 ToneVolume = 3000;
  int ToneHz = 256;
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
      uint8 Blue = (X + GameState->BlueOffset);
      uint8 Green = (Y + GameState->GreenOffset);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += Buffer->Pitch;
  }
}

internal game_state * GameStartup() {
  game_state *GameState = new game_state;
  if (GameState) {
    GameState->BlueOffset = 0;
    GameState->GreenOffset = 0;
    GameState->ToneHz = 256;
  }
  return GameState;
}

internal void GameShutDown(game_state *GameState) {
  delete GameState;
}


inline uint32 SafeTruncateUint64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

internal void GameUpdateAndRender(
  game_memory *Memory,
  game_input *Input,
  game_offscreen_buffer *Buffer, 
  game_sound_buffer *SoundBuffer
) {

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {

    char *FileName = __FILE__;
    debug_read_file_result FileData = DEBUGPlatformReadEntireFile(FileName);
    if (FileData.Contents) {
      DEBUGPlatformFreeFileMemory(FileData.Contents);
      FileData.ContentSize = 0;
    }

    GameState->ToneHz = 256;
    Memory->IsInitialized = true;
  }
  
  game_controller_input *Input0 = &Input->Controllers[0];

  if (Input0->IsAnalog) {
    GameState->ToneHz = 256 + (int)(128.0f*(Input0->EndX));
    GameState->BlueOffset = (int)4.0f*(Input0->EndY);
  } else {

  }

  if (Input0->Down.EndedDown) {
    GameState->GreenOffset += 1;
  }
  
  
  GameOutputSound(SoundBuffer);
  RenderWeirdGradient(Buffer, GameState);
}