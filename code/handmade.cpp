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
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  Assert(TileChunk);
  Assert(TileX < World->ChunkDim && TileY < World->ChunkDim);
  return TileChunk->Tiles[TileX + TileY * World->ChunkDim];
}

inline tile_chunk * 
GetTileChunk(world *World, int32 TileChunkX, int32 TileChunkY) {
  tile_chunk *TileChunk = 0;
  if (TileChunkX < World->TileChunkCountX && TileChunkX >= 0 && TileChunkY >= 0 && TileChunkY < World->TileChunkCountY) {
    TileChunk = &World->TileChunks[TileChunkX + TileChunkY * World->TileChunkCountX];
  }
  return TileChunk;
}

internal uint32 
GetTileValue(world *World, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY) {
  uint32 TileChunkValue = 0;

  if (TileChunk) {
    TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
  }
  return TileChunkValue;
}

inline void
ReCanonicalizeCoord(world *World, uint32 *Tile, real32 *TileRel) {

  // Number of tiles to move out from current tile
  int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);

  *Tile += Offset;
  *TileRel -= Offset*World->TileSideInMeters;

  Assert(*TileRel >= 0);
  Assert(*TileRel < World->TileSideInMeters);
}

internal world_position
ReCanonicalizePosition(world *World, world_position Pos) { 
  world_position Result = Pos;
  ReCanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
  ReCanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
  return Result;
}

inline tile_chunk_position 
GetChunkPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY) {
  tile_chunk_position Result = {};
  Result.TileChunkX = AbsTileX >> World->ChunkShift;
  Result.TileChunkY = AbsTileY >> World->ChunkShift;
  Result.RelTileX = AbsTileX & World->ChunkMask;
  Result.RelTileY = AbsTileY & World->ChunkMask;
  return Result;
}

internal uint32 
GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
  uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
  return TileChunkValue;
}

internal bool 
IsWorldPointEmpty(world *World, world_position CanPos) {
  uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
  bool Empty = TileChunkValue == 0;
  return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
#define TILEMAP_MAX_X 256
#define TILEMAP_MAX_Y 256

  uint32 TempTiles[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1},
    { 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1, 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 1},
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1, 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1},
    { 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1},
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1},
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1}
  };

  world World;
  World.ChunkShift = 8;
  World.ChunkMask = 0xFF;
  World.ChunkDim = 256;

  tile_chunk TileChunk;
  TileChunk.Tiles = (uint32 *)TempTiles;
  World.TileChunks = &TileChunk;

  World.TileChunkCountX = 1;
  World.TileChunkCountY = 1; 


  World.TileSideInMeters = 1.4f;
  World.TileSideInPixels = 60;
  World.MetersToPixels = (real32)World.TileSideInPixels/(real32)World.TileSideInMeters;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  real32 LowerLeftX = (real32)(-1.0f * World.TileSideInPixels/2.0f);
  real32 LowerLeftY = (real32)Buffer->Height;

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    GameState->PlayerP.AbsTileX = 3;
    GameState->PlayerP.AbsTileY = 3;
    GameState->PlayerP.TileRelX = 0.5f;
    GameState->PlayerP.TileRelY = 0.5f;
    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  // Assert(TileMap);

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
      dPlayerX *= 5.0f;
      dPlayerY *= 5.0f;

      world_position NewPlayerP = GameState->PlayerP;

      NewPlayerP.TileRelX = GameState->PlayerP.TileRelX +  Input->dtForFrame * dPlayerX;
      NewPlayerP.TileRelY = GameState->PlayerP.TileRelY + Input->dtForFrame * dPlayerY;
      NewPlayerP = ReCanonicalizePosition(&World, NewPlayerP);

      world_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
      PlayerLeft = ReCanonicalizePosition(&World, PlayerLeft);

      world_position PlayerRight = NewPlayerP;
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
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)(LowerLeftX + Buffer->Width), 
    LowerLeftY, 1.0f, 0.0f, 1.0f);

  real32 CenterX = 0.5f*(real32)Buffer->Width;
  real32 CenterY = 0.5f*(real32)Buffer->Height;

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

      uint32 TileID = GetTileValue(&World, Column, Row);
      real32 Gray = 0.5f;
      if (TileID == 1) {
        Gray = 1.0f;
      } else if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY) {
        Gray = 0.0f;
      }
      real32 MinX = CenterX + ((real32)RelColumn) * World.TileSideInPixels;
      real32 MinY = CenterY - ((real32)RelRow) * World.TileSideInPixels;
      real32 MaxX = MinX + World.TileSideInPixels;
      real32 MaxY = MinY - World.TileSideInPixels;
      DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
    }
  }

  // Draw player
  real32 PlayerLeft = CenterX + GameState->PlayerP.TileRelX*World.MetersToPixels 
    - 0.5f*PlayerWidth*World.MetersToPixels;

  real32 PlayerTop = CenterY - GameState->PlayerP.TileRelY*World.MetersToPixels 
    - PlayerHeight*World.MetersToPixels;

  DrawRectangle(Buffer, 
    PlayerLeft, PlayerTop, 
    PlayerLeft + PlayerWidth*World.MetersToPixels, 
    PlayerTop + PlayerHeight*World.MetersToPixels, 
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