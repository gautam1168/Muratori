#if !defined(HANDMADE_TILE_HPP)

#include "handmade_math.hpp"
struct tile_map_difference {
  v2 dXY;
  real32 dZ;
};

struct tile_map_position {
  // Fixed point tile locations. High bits are tile chunk index and low bits are tile index in the chunk
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;

  v2 Offset_;
};

struct tile_chunk_position {
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;

  uint32 RelTileX;
  uint32 RelTileY;
};

struct tile_chunk {
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;

  uint32 *Tiles;
  tile_chunk *NextInHash;
};

struct tile_map {
  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;

  real32 TileSideInMeters;

  tile_chunk TileChunkHash[4096];
};


#define HANDMADE_TILE_HPP
#endif