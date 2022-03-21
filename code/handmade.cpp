#include "handmade.hpp"

// Platform -> Game

// Game -> Platform
void GameOutputSound(game_sound_buffer *SoundBuffer, game_state *GameState) {
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/GameState->ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;  ++SampleIndex) {
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    GameState->tSine += 2.0f * Pi32 * 1.0f/(real32)WavePeriod;
  }
}

void RenderWeirdGradient(game_offscreen_buffer *Buffer, game_state *GameState) {
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

internal void RenderPlayer(game_offscreen_buffer *Buffer, game_state *GameState) {
  int Top = GameState->PlayerY;
  int Bottom = GameState->PlayerY + 10;

  for (int X = GameState->PlayerX; X < GameState->PlayerX + 10; X++) {
    uint8 *Pixel = ((uint8 *)Buffer->Memory + X * Buffer->BytesPerPixel + Top*Buffer->Pitch);
    for (int Y = Top; Y < Bottom; Y++) {
      *(uint32 *)Pixel = 0xFFFFFFFF;
      Pixel += Buffer->Pitch;
    }
  }
}

inline uint32 SafeTruncateUint64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {

    char *FileName = __FILE__;
    debug_read_file_result FileData = Memory->DEBUGPlatformReadEntireFile(FileName);
    if (FileData.Contents) {
      Memory->DEBUGPlatformWriteEntireFile("out.txt", FileData.ContentSize, FileData.Contents);
      Memory->DEBUGPlatformFreeFileMemory(FileData.Contents);
      FileData.ContentSize = 0;
    }

    GameState->ToneHz = 256;
    GameState->tSine = 0.0f;
    GameState->PlayerX = 100;
    GameState->PlayerY = 100;
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
      // GameState->GreenOffset += 1;
      GameState->PlayerY -= 10;
    }
    GameState->PlayerX += (int)(4.0f*(Controller->MoveRight.HalfTransitionCount));
    GameState->PlayerY += (int)(4.0f*(Controller->MoveDown.HalfTransitionCount));
    GameState->PlayerX -= (int)(4.0f*(Controller->MoveLeft.HalfTransitionCount));
    GameState->PlayerY -= (int)(4.0f*(Controller->MoveUp.HalfTransitionCount));
  } 
  
  RenderWeirdGradient(Buffer, GameState);
  RenderPlayer(Buffer, GameState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameOutputSound(SoundBuffer, GameState);
}