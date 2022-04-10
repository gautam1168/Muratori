#include "handmade.hpp"
#include "handmade_tile.cpp"
// Platform -> Game

// Game -> Platform
void GameOutputSound(game_sound_buffer *SoundBuffer, game_state *GameState) {
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/GameState->ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;  ++SampleIndex) {
    // TODO(gaurav): Implement sine 
#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
    int16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

#if 0
    GameState->tSine += 2.0f * Pi32 * 1.0f/(real32)WavePeriod;
#endif
  }
}

// Fills rectangle from [low, high) i.e. excludes the upperbound
internal void DrawRectangle(game_offscreen_buffer *Buffer, 
  real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, 
  real32 R, real32 G, real32 B) {
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;

  int32 MinX = RoundReal32ToInt32(RealMinX);
  int32 MinY = RoundReal32ToInt32(RealMinY);
  int32 MaxX = RoundReal32ToInt32(RealMaxX);
  int32 MaxY = RoundReal32ToInt32(RealMaxY);

  if (MinX < 0) {
    MinX = 0;
  }

  if (MinY < 0) {
    MinY = 0;
  }

  if (MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }

  if (MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }

  uint8 *Row = (uint8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch;

  uint32 Color = (
    0xFF000000 |
    RoundReal32ToUInt32(R * 255.0f) << 16 |
    RoundReal32ToUInt32(G * 255.0f) << 8 |
    RoundReal32ToUInt32(B * 255.0f)
  );

  for (int Y = MinY; Y < MaxY; Y++) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = MinX; X < MaxX; X++) {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
  }
}

inline uint32 SafeTruncateUint64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  if (!Memory->IsInitialized) {
    GameState->PlayerP.AbsTileX = 1;
    GameState->PlayerP.AbsTileY = 0;
    GameState->PlayerP.TileRelX = 0.5f;
    GameState->PlayerP.TileRelY = 0.5f;
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

    tile_map *TileMap = World->TileMap;

    TileMap->ChunkShift = 8;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = 256;

    TileMap->TileChunkCountX = 4;
    TileMap->TileChunkCountY = 4; 

    TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY, tile_chunk);

    for (uint32 Y = 0; 
      Y < TileMap->TileChunkCountY;
      Y++) 
    {
      for (uint32 X = 0;
        X < TileMap->TileChunkCountX; 
        X++)
      {
        TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Tiles = 
          PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
      }
    }

    TileMap->TileSideInMeters = 1.4f;
    TileMap->TileSideInPixels = 60;
    TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels/(real32)TileMap->TileSideInMeters;



    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    for (uint32 ScreenY = 0; 
      ScreenY < 32;
      ScreenY++) {
        for (uint32 ScreenX = 0;
          ScreenX < 32;
          ScreenX++) {
            for (uint32 TileY = 0; 
              TileY < TilesPerHeight; 
              TileY++) {
              for (uint32 TileX = 0; 
                TileX < TilesPerWidth; 
                TileX++) {
                uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
                SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY,
                  (TileX == TileY) && (TileX % 2 == 0) ? 1 : 0
                );
              }
            }
        }
    }

    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&TileMap-> GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  tile_map *TileMap = GameState->World->TileMap;
  // Assert(TileMap);
  real32 LowerLeftX = (real32)(-1.0f * TileMap->TileSideInPixels/2.0f);
  real32 LowerLeftY = (real32)Buffer->Height;

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {

    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;
      if (Controller->MoveUp.EndedDown) {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveDown.EndedDown) {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveLeft.EndedDown) {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown) {
        dPlayerX = 1.0f;
      }
      real32 PlayerSpeed = 5.0f;
      if (Controller->ActionUp.EndedDown) {
        PlayerSpeed = 20.0f;
      }
      dPlayerX *= PlayerSpeed;
      dPlayerY *= PlayerSpeed;

      tile_map_position NewPlayerP = GameState->PlayerP;

      NewPlayerP.TileRelX = GameState->PlayerP.TileRelX +  Input->dtForFrame * dPlayerX;
      NewPlayerP.TileRelY = GameState->PlayerP.TileRelY + Input->dtForFrame * dPlayerY;
      NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

      tile_map_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
      PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

      tile_map_position PlayerRight = NewPlayerP;
      PlayerRight.TileRelX += 0.5f*PlayerWidth;
      PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);
      
      if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
          IsTileMapPointEmpty(TileMap, PlayerLeft) &&
          IsTileMapPointEmpty(TileMap, PlayerRight)
          ) {
        GameState->PlayerP = NewPlayerP;
      }
    }
  } 

  // Clear color
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)(LowerLeftX + Buffer->Width), 
    LowerLeftY, 1.0f, 0.0f, 1.0f);

  real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;

  // Draw tiles
  for (int RelRow = -10; 
    RelRow < 10; 
    RelRow++) 
  {
    for (int32 RelColumn = -20;
      RelColumn < 20;
      RelColumn++) 
    {
      uint32 Column = RelColumn + GameState->PlayerP.AbsTileX;
      uint32 Row = RelRow + GameState->PlayerP.AbsTileY;

      uint32 TileID = GetTileValue(TileMap, Column, Row);
      real32 Gray = 0.5f;
      if (TileID == 1) {
        Gray = 1.0f;
      } else if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY) {
        Gray = 0.0f;
      }
      real32 CenX = ScreenCenterX - GameState->PlayerP.TileRelX*TileMap->MetersToPixels + ((real32)RelColumn) * TileMap->TileSideInPixels;
      real32 CenY = ScreenCenterY + GameState->PlayerP.TileRelY*TileMap->MetersToPixels - ((real32)RelRow) * TileMap->TileSideInPixels;
      real32 MinX = CenX - 0.5f * TileMap->TileSideInPixels;
      real32 MinY = CenY - 0.5f * TileMap->TileSideInPixels;
      real32 MaxX = CenX + 0.5f * TileMap->TileSideInPixels;
      real32 MaxY = CenY + 0.5f * TileMap->TileSideInPixels;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  // Draw player
  real32 PlayerLeft = ScreenCenterX - 0.5f*PlayerWidth*TileMap->MetersToPixels;

  real32 PlayerTop = ScreenCenterY - PlayerHeight*TileMap->MetersToPixels;

  DrawRectangle(Buffer, 
    PlayerLeft, PlayerTop, 
    PlayerLeft + PlayerWidth*TileMap->MetersToPixels, 
    PlayerTop + PlayerHeight*TileMap->MetersToPixels, 
    1.0f, 1.0f, 0.0f);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameState->ToneHz = 400;
  GameOutputSound(SoundBuffer, GameState);
}

/*
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
*/