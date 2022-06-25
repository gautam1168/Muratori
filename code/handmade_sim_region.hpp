#if !defined(HANDMADE_SIM_REGION_HPP)
struct move_spec 
{
  bool UnitMaxAccelVector;
  real32 Speed;
  real32 Drag;
};

enum entity_type {
  EntityType_Null,
  EntityType_Hero,
  EntityType_Wall,
  EntityType_Familiar,
  EntityType_Monster,
  EntityType_Sword
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point 
{
   uint8 Flags;
   uint8 FilledAmount;
};

struct sim_entity;
union entity_reference {
  sim_entity *Ptr;
  uint32 Index;
};

struct sim_entity 
{
  uint32 StorageIndex;
  entity_type Type;

  v2 P;
  uint32 ChunkZ;

  real32 Z;
  real32 dZ;

  v2 dP;
  real32 Width, Height;

  uint32 FacingDirection;
  real32 tBob;

  bool Collides;
  int32 dAbsTileZ; // for stairs

  uint32 HitPointMax;
  hit_point HitPoint[16];

  entity_reference Sword;
  real32 DistanceRemaining;
};

struct sim_entity_hash
{
  uint32 Index;
  sim_entity *Ptr;
};

struct sim_region 
{
  world *World;
  world_position Origin;
  rectangle2 RegionBounds;
  uint32 MaxEntityCount;
  uint32 EntityCount;
  sim_entity *Entities;
  sim_entity_hash Hash[4096];
};

#define HANDMADE_SIM_REGION_HPP
#endif