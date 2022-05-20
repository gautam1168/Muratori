#if !defined(HANDMADE_WORLD_HPP)

#include "handmade_math.hpp"
struct world_difference
{
  v2 dXY;
  real32 dZ;
};

struct world_position
{
  // Fixed point tile locations. High bits are tile chunk index and low bits are tile index in the chunk
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;

  v2 Offset_;
};

struct world_entity_block
{
  uint32 EntityCount;
  uint32 LowEntityIindex[16];
  world_entity_block *Next;
};

struct world_chunk
{
  uint32 ChunkX;
  uint32 ChunkY;
  uint32 ChunkZ;

  world_entity_block FirstBlock;

  world_chunk *NextInHash;
};

struct world
{
  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;

  real32 TileSideInMeters;

  world_chunk ChunkHash[4096];
};

#define HANDMADE_WORLD_HPP
#endif