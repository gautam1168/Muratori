#if !defined(HANDMADE_SIM_REGION_HPP)

struct sim_entity 
{
  uint32 StorageIndex;
  v2 P;
  uint32 ChunkZ;
  real32 Z;
  real32 dZ;
};

struct sim_region 
{
  world *World;
  world_position Origin;
  rectangle2 RegionBounds;
  uint32 MaxEntityCount;
  uint32 EntityCount;
  sim_entity *Entities;
};

#define HANDMADE_SIM_REGION_HPP
#endif