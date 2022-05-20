inline void
ReCanonicalizeCoord(world *World, uint32 *Tile, real32 *TileRel) {

  // Number of tiles to move out from current tile
  int32 Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);

  *Tile += Offset;
  *TileRel -= Offset*World->TileSideInMeters;

  Assert(*TileRel >= -0.5f * World->TileSideInMeters);
  Assert(*TileRel <= 0.5f * World->TileSideInMeters);
}

internal world_position
MapIntoTileSpace(world *World, world_position BasePos, v2 Offset) { 
  world_position Result = BasePos;
  Result.Offset_ += Offset;
  ReCanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
  ReCanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);
  return Result;
}



#define TILE_CHUNK_SAFE_MARGIN 256

inline world_chunk * 
GetTileChunk(world *World, uint32 ChunkX, uint32 ChunkY, uint32 ChunkZ,
    memory_arena *Arena = 0) {

  Assert(ChunkX > TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkY > TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
  Assert(ChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
  Assert(ChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));

  uint32 HashValue = 19 * ChunkX + 7 * ChunkY + 3 * ChunkZ;
  uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(World->ChunkHash));

  world_chunk *Chunk = World->ChunkHash + HashSlot;
  
  do {
    if ((ChunkX == Chunk->ChunkX) &&
        (ChunkY == Chunk->ChunkY) &&
        (ChunkZ == Chunk->ChunkZ))
    {
      break;
    }

    if (Arena && Chunk->ChunkX != 0 && !Chunk->NextInHash) 
    {
      Chunk->NextInHash = PushStruct(Arena, world_chunk);
      Chunk->ChunkX = 0;
      Chunk = Chunk->NextInHash;
    }

    if (Arena && Chunk->ChunkX == 0)
    {
      uint32 TileCount = World->ChunkDim * World->ChunkDim;

      Chunk->ChunkX = ChunkX;
      Chunk->ChunkY = ChunkY;
      Chunk->ChunkZ = ChunkZ;

      Chunk->NextInHash = 0;
      break;
    } else if (!Arena && Chunk->ChunkX == 0) {
      Chunk = 0;
    }
  } while (Chunk);
  return Chunk;
}

#if 0
inline world_chunk_position 
GetChunkPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  world_chunk_position Result = {};
  Result.ChunkX = AbsTileX >> World->ChunkShift;
  Result.ChunkY = AbsTileY >> World->ChunkShift;
  Result.ChunkZ = AbsTileZ;
  Result.RelTileX = AbsTileX & World->ChunkMask;
  Result.RelTileY = AbsTileY & World->ChunkMask;
  return Result;
}
#endif

internal void
InitializeWorld(world *World, real32 TileSideInMeters)
{
  World->ChunkShift = 4;
  World->ChunkMask = (1 << World->ChunkShift) - 1;
  World->ChunkDim = (1 << World->ChunkShift);
  World->TileSideInMeters = TileSideInMeters;

  for (uint32 TileChunkIndex = 0;
    TileChunkIndex < ArrayCount(World->ChunkHash);
    TileChunkIndex++)
  {
    World->ChunkHash[TileChunkIndex].ChunkX = 0;
  }
}



inline bool
AreOnSameTile(world_position *A, world_position *B) {
  bool Result = (
    A->AbsTileX == B->AbsTileX &&
    A->AbsTileY == B->AbsTileY &&
    A->AbsTileZ == B->AbsTileZ
  );

  return Result;
}

inline world_difference 
Subtract(world *World, world_position *A, world_position *B) {
  world_difference Result;

  // v2 dTileXY = {(real32)((real64)A->AbsTileX - (real64)B->AbsTileX),
  //   (real32)((real64)A->AbsTileY - (real64)B->AbsTileY)};
  // real32 dTileZ = (real32)((real64)A->AbsTileZ - (real64)B->AbsTileZ);

  v2 dTileXY = {(real32)((real64)A->AbsTileX - (real64)B->AbsTileX),
    (real32)((real64)A->AbsTileY - (real64)B->AbsTileY)};
  real32 dTileZ = (real32)((real64)A->AbsTileZ - (real64)B->AbsTileZ);

  Result.dXY = World->TileSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
  Result.dZ = World->TileSideInMeters * dTileZ;

  return Result;
}

inline world_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  world_position Result = {};
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;

  return Result;
}
/*
#if 0
inline uint32 
GetTileValueUnchecked(world *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  uint32 TileChunkValue = 0;
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  if (TileChunk && TileChunk->Tiles) {
    TileChunkValue = TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim];
  }
  return TileChunkValue;
}

inline void 
SetTileValueUnchecked(world *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  if (TileChunk && TileChunk->Tiles) {
    TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim] = TileValue;
  }
}

internal void 
SetTileValue(world *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY, uint32 TileValue) {
  if (TileChunk) {
    SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
  }
}

internal uint32 
GetTileValue(world *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY) {
  uint32 TileChunkValue = 0;

  if (TileChunk) {
    TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
  }
  return TileChunkValue;
}

internal uint32 
GetTileValue(world *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
  uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
  return TileChunkValue;
}

internal uint32 
GetTileValue(world *TileMap, world_position Position) {
  uint32 TileChunkValue = GetTileValue(TileMap, Position.AbsTileX, Position.AbsTileY, Position.AbsTileZ);
  return TileChunkValue;
}

internal bool
IsTileValueEmpty(uint32 TileValue) {
  bool Empty = ((TileValue == 1) ||
                (TileValue == 3) ||
                (TileValue == 4));
  return Empty;
}

internal bool 
IsTileMapPointEmpty(world *TileMap, world_position CanPos) {
  uint32 TileChunkValue = GetTileValue(TileMap, CanPos);
  bool Empty = IsTileValueEmpty(TileChunkValue);
  return Empty;
}

internal void
SetTileValue(memory_arena *Arena, world *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ, Arena);
  SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}
#endif
*/
