inline void
ReCanonicalizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel) {

  // Number of tiles to move out from current tile
  int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);

  *Tile += Offset;
  *TileRel -= Offset*TileMap->TileSideInMeters;

  Assert(*TileRel >= -0.5f * TileMap->TileSideInMeters);
  Assert(*TileRel <= 0.5f * TileMap->TileSideInMeters);
}

internal tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position BasePos, v2 Offset) { 
  tile_map_position Result = BasePos;
  Result.Offset_ += Offset;
  ReCanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
  ReCanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);
  return Result;
}

inline uint32 
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  uint32 TileChunkValue = 0;
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  if (TileChunk && TileChunk->Tiles) {
    TileChunkValue = TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim];
  }
  return TileChunkValue;
}

inline void 
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  if (TileChunk && TileChunk->Tiles) {
    TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim] = TileValue;
  }
}

#define TILE_CHUNK_SAFE_MARGIN 256

inline tile_chunk * 
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ,
    memory_arena *Arena = 0) {

  Assert(TileChunkX > TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY > TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ > TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
  Assert(TileChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
  Assert(TileChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));

  uint32 HashValue = 19 * TileChunkX + 7 * TileChunkY + 3 * TileChunkZ;
  uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
  Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));

  tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
  
  do {
    if ((TileChunkX == Chunk->TileChunkX) &&
        (TileChunkY == Chunk->TileChunkY) &&
        (TileChunkZ == Chunk->TileChunkZ))
    {
      break;
    }

    if (Arena && Chunk->TileChunkX != 0 && !Chunk->NextInHash) 
    {
      Chunk->NextInHash = PushStruct(Arena, tile_chunk);
      Chunk->TileChunkX = 0;
      Chunk = Chunk->NextInHash;
    }

    if (Arena && Chunk->TileChunkX == 0)
    {
      uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;

      Chunk->TileChunkX = TileChunkX;
      Chunk->TileChunkY = TileChunkY;
      Chunk->TileChunkZ = TileChunkZ;

      Chunk->Tiles = PushArray(Arena, TileCount, uint32); 
      for (uint32 TileIndex= 0; 
      TileIndex < TileCount;
      TileIndex++) 
      {
        Chunk->Tiles[TileIndex] = 1;
      }
      Chunk->NextInHash = 0;
      break;
    } else if (!Arena && Chunk->TileChunkX == 0) {
      Chunk = 0;
    }
  } while (Chunk);
  return Chunk;
}

inline tile_chunk_position 
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_chunk_position Result = {};
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.TileChunkZ = AbsTileZ;
  Result.RelTileX = AbsTileX & TileMap->ChunkMask;
  Result.RelTileY = AbsTileY & TileMap->ChunkMask;
  return Result;
}

internal uint32 
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY) {
  uint32 TileChunkValue = 0;

  if (TileChunk) {
    TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
  }
  return TileChunkValue;
}

internal void
InitializeTileMap(tile_map *TileMap, real32 TileSideInMeters)
{
  TileMap->ChunkShift = 4;
  TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
  TileMap->ChunkDim = (1 << TileMap->ChunkShift);
  TileMap->TileSideInMeters = TileSideInMeters;

  for (uint32 TileChunkIndex = 0;
    TileChunkIndex < ArrayCount(TileMap->TileChunkHash);
    TileChunkIndex++)
  {
    TileMap->TileChunkHash[TileChunkIndex].TileChunkX = 0;
  }
}

internal void 
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY, uint32 TileValue) {
  if (TileChunk) {
    SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
  }
}

internal uint32 
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
  uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
  return TileChunkValue;
}

internal uint32 
GetTileValue(tile_map *TileMap, tile_map_position Position) {
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
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanPos) {
  uint32 TileChunkValue = GetTileValue(TileMap, CanPos);
  bool Empty = IsTileValueEmpty(TileChunkValue);
  return Empty;
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ, Arena);
  SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}

inline bool
AreOnSameTile(tile_map_position *A, tile_map_position *B) {
  bool Result = (
    A->AbsTileX == B->AbsTileX &&
    A->AbsTileY == B->AbsTileY &&
    A->AbsTileZ == B->AbsTileZ
  );

  return Result;
}

inline tile_map_difference 
Subtract(tile_map *TileMap, tile_map_position *A, tile_map_position *B) {
  tile_map_difference Result;

  // v2 dTileXY = {(real32)((real64)A->AbsTileX - (real64)B->AbsTileX),
  //   (real32)((real64)A->AbsTileY - (real64)B->AbsTileY)};
  // real32 dTileZ = (real32)((real64)A->AbsTileZ - (real64)B->AbsTileZ);

  v2 dTileXY = {(real32)((real64)A->AbsTileX - (real64)B->AbsTileX),
    (real32)((real64)A->AbsTileY - (real64)B->AbsTileY)};
  real32 dTileZ = (real32)((real64)A->AbsTileZ - (real64)B->AbsTileZ);

  Result.dXY = TileMap->TileSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
  Result.dZ = TileMap->TileSideInMeters * dTileZ;

  return Result;
}

inline tile_map_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_map_position Result = {};
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;

  return Result;
}
