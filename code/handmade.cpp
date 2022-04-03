#include "handmade.hpp"
#include "handmade_intrinsics.hpp"
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

inline uint32 
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TileX, int32 TileY) {
  Assert(TileMap);
  Assert((TileX >= 0 && TileY >= 0 && TileX < World->CountX && TileY < World->CountY));
  return TileMap->Tiles[TileX + TileY * World->CountX];
}

inline tile_map * 
GetTileMap(world *World, int32 TileMapX, int32 TileMapY) {
  tile_map *TileMap = 0;
  if (TileMapX < World->TileMapCountX && TileMapX >= 0 && TileMapY >= 0 && TileMapY < World->TileMapCountY) {
    TileMap = &World->TileMaps[TileMapX + TileMapY * World->TileMapCountX];
  }
  return TileMap;
}

internal bool 
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY) {
  bool Empty = false;

  if ((TestTileX >= 0) && (TestTileX < World->CountX) &&  
      (TestTileY >= 0) && (TestTileY < World->CountY)) {
    uint32 TilemapValue = TileMap->Tiles[World->CountX * TestTileY + TestTileX];
    Empty = (TilemapValue == 0);
  }
  return Empty;
}

inline void
ReCanonicalizeCoord(world *World, int32 TileCount, int32 *TileMap, int32 *Tile, real32 *TileRel) {

  // Number of tiles to move out from current tile
  int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*World->TileSideInMeters;

  Assert(*TileRel >= 0);
  Assert(*TileRel < World->TileSideInMeters);

  if (*Tile < 0) {
    *Tile += TileCount;
    *TileMap = *TileMap - 1;
  }

  if (*Tile >= TileCount) {
    *Tile -= TileCount;
    *TileMap = *TileMap + 1;
  }
}

internal canonical_position
ReCanonicalizePosition(world *World, canonical_position Pos) { 
  canonical_position Result = Pos;
  ReCanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
  ReCanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);
  return Result;
}

internal bool 
IsWorldPointEmpty(world *World, canonical_position CanPos) {
  bool Empty = false;

  tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
  Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
  return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
#define TILEMAP_MAX_X 16
#define TILEMAP_MAX_Y 9

  uint32 Tiles00[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 0 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  uint32 Tiles01[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 }
  };


  uint32 Tiles10[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 0, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  uint32 Tiles11[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  tile_map TileMaps[2][2] = {};
  TileMaps[0][0].Tiles = (uint32 *)Tiles00;
  TileMaps[1][0].Tiles = (uint32 *)Tiles01;
  TileMaps[0][1].Tiles = (uint32 *)Tiles10;
  TileMaps[1][1].Tiles = (uint32 *)Tiles11;

  world World;
  World.TileSideInMeters = 1.4f;
  World.TileSideInPixels = 60;
  World.MetersToPixels = (real32)World.TileSideInPixels/(real32)World.TileSideInMeters;
  World.TileMapCountX = 2;
  World.TileMapCountY = 2; 
  World.TileMaps = (tile_map *)TileMaps;
  World.CountX = TILEMAP_MAX_X;
  World.CountY = TILEMAP_MAX_Y;
  World.UpperLeftY = 0;
  World.UpperLeftX = (real32)(-1.0f * World.TileSideInPixels/2.0f);

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    GameState->PlayerP.TileMapX = 0;
    GameState->PlayerP.TileMapY = 0;
    GameState->PlayerP.TileX = 3;
    GameState->PlayerP.TileY = 3;
    GameState->PlayerP.TileRelX = 0.0f;
    GameState->PlayerP.TileRelY = 0.0f;
    Memory->IsInitialized = true;
  }

  tile_map *TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  Assert(TileMap);

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {

    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;
      if (Controller->MoveUp.EndedDown) {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveDown.EndedDown) {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveLeft.EndedDown) {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown) {
        dPlayerX = 1.0f;
      }
      dPlayerX *= 3.0f;
      dPlayerY *= 3.0f;

      canonical_position NewPlayerP = GameState->PlayerP;

      NewPlayerP.TileRelX = GameState->PlayerP.TileRelX +  Input->dtForFrame * dPlayerX;
      NewPlayerP.TileRelY = GameState->PlayerP.TileRelY + Input->dtForFrame * dPlayerY;
      NewPlayerP = ReCanonicalizePosition(&World, NewPlayerP);

      canonical_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
      PlayerLeft = ReCanonicalizePosition(&World, PlayerLeft);

      canonical_position PlayerRight = NewPlayerP;
      PlayerRight.TileRelX += 0.5f*PlayerWidth;
      PlayerRight = ReCanonicalizePosition(&World, PlayerRight);
      
      if (IsWorldPointEmpty(&World, NewPlayerP) &&
          IsWorldPointEmpty(&World, PlayerLeft) &&
          IsWorldPointEmpty(&World, PlayerRight)
          ) {
        GameState->PlayerP = NewPlayerP;
      }
    }
  } 

  // Clear color
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)(Buffer->Width + World.UpperLeftX), 
    (real32)(Buffer->Height + World.UpperLeftY), 1.0f, 0.0f, 1.0f);

  // Draw tiles
  for (int Row = 0; Row < 9; Row++) {
    for (int Column = 0; Column < 16; Column++) {
      uint32 TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
      real32 Gray = 0.5f*(Row + Column)/(Row * Column);
      if (TileID == 1) {
        Gray = 1.0f;
      } else if (Column == GameState->PlayerP.TileX && Row == GameState->PlayerP.TileY) {
        Gray = 0.0f;
      }
      real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileSideInPixels;
      real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileSideInPixels;
      real32 MaxX = MinX + World.TileSideInPixels;
      real32 MaxY = MinY + World.TileSideInPixels;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  // Draw player
  real32 PlayerLeft = World.UpperLeftX 
    + GameState->PlayerP.TileX*World.TileSideInPixels 
    + GameState->PlayerP.TileRelX*World.TileSideInMeters 
    - 0.5f*PlayerWidth*World.MetersToPixels;

  real32 PlayerTop = World.UpperLeftY 
    + GameState->PlayerP.TileY*World.TileSideInPixels 
    + GameState->PlayerP.TileRelY*World.TileSideInMeters 
    - PlayerHeight*World.MetersToPixels;

  DrawRectangle(Buffer, 
    PlayerLeft, PlayerTop, 
    PlayerLeft + PlayerWidth*10, 
    PlayerTop + PlayerHeight*10, 
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