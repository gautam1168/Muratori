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
  uint32 ChunkX;
  uint32 ChunkY;
  uint32 ChunkZ;

  v2 Offset_;
};

struct world_entity_block
{
  uint32 EntityCount;
  uint32 LowEntityIndex[16];
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
  real32 TileSideInMeters;
  real32 ChunkSideInMeters;
  world_entity_block *FirstFree;

  world_chunk ChunkHash[4096];
};

#define HANDMADE_WORLD_HPP
#endif