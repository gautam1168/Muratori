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
ReCanonicalizePosition(tile_map *TileMap, tile_map_position Pos) { 
  tile_map_position Result = Pos;
  ReCanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
  ReCanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);
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

inline tile_chunk * 
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ) {
  tile_chunk *TileChunk = 0;
  if (TileChunkX < TileMap->TileChunkCountX && TileChunkX >= 0 && 
    TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY && 
    TileChunkZ >= 0 && TileChunkZ < TileMap->TileChunkCountZ) {
    TileChunk = &TileMap->TileChunks[
      TileChunkX + 
      TileChunkY * TileMap->TileChunkCountX + 
      TileChunkZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY];
  }
  return TileChunk;
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
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
  Assert(TileChunk);
  if (!TileChunk->Tiles) 
  {
    uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
    TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
    for (uint32 TileIndex = 0;
      TileIndex < TileCount;
      TileIndex++)
    {
      TileChunk->Tiles[TileIndex] = 1;
    }
  }
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

  v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
    (real32)A->AbsTileY - (real32)B->AbsTileY};
  real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

  Result.dXY = TileMap->TileSideInMeters * dTileXY + (A->Offset - B->Offset);
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