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
  ReCanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
  ReCanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);
  return Result;
}

inline uint32 
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY) {
  Assert(TileChunk);
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  return TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim];
}

inline void 
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
  Assert(TileChunk);
  Assert(TileX < TileMap->ChunkDim && TileY < TileMap->ChunkDim);
  TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim] = TileValue;
}

inline tile_chunk * 
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY) {
  tile_chunk *TileChunk = 0;
  if (TileChunkX < TileMap->TileChunkCountX && TileChunkX >= 0 && TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY) {
    TileChunk = &TileMap->TileChunks[TileChunkX + TileChunkY * TileMap->TileChunkCountX];
  }
  return TileChunk;
}

inline tile_chunk_position 
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY) {
  tile_chunk_position Result = {};
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
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
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
  uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
  return TileChunkValue;
}

internal bool 
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanPos) {
  uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
  bool Empty = TileChunkValue == 0;
  return Empty;
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, uint32 TileX, uint32 TileY, uint32 TileValue) {
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, TileX, TileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
  Assert(TileChunk);
  SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}