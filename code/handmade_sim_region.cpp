internal sim_entity * AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source);
internal sim_entity_hash *
GetHashFromStorageIndex(sim_region*SimRegion, uint32 StorageIndex)
{
  Assert(StorageIndex);
  sim_entity_hash *Result = 0;
  uint32 HashValue = StorageIndex;
  for (uint32 Offset = 0;
       Offset < ArrayCount(SimRegion->Hash);
       Offset++)
  {
    // The bitwise & here makes the whole index wrap around and check the whole
    // array. This only works if the length of the array is a power of 2
    sim_entity_hash *Entry = SimRegion->Hash + ((HashValue + Offset) & (ArrayCount(SimRegion->Hash) - 1));
    if (Entry->Index == StorageIndex || Entry->Index == 0)
    {
      Result = Entry;
      break;
    }
    }
  return Result;
}

internal void
MapStorageIndexToEntity(sim_region *SimRegion, uint32 StorageIndex, sim_entity *Entity)
{
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  Assert((Entry->Index == 0) || (Entry->Index == StorageIndex));
  Entry->Index = StorageIndex;
  Entry->Ptr = Entity;
}

inline void
LoadEntityReference(game_state *GameState, sim_region *SimRegion, entity_reference *Ref)
{
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
  if (Entry->Ptr == 0)
  {
    AddEntity(GameState, SimRegion, Ref->Index, GetLowEntity(GameState, Ref->Index));
  }
  Ref->Ptr = Entry->Ptr;
}

inline void
StoreEntityReference(entity_reference *Ref)
{
  if (Ref->Ptr != 0)
  {
    Ref->Index = Ref->Ptr->StorageIndex;
  }
}

inline sim_entity *
GetEntityByStorageIndex(sim_region *SimRegion, uint32 StorageIndex)
{
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  sim_entity *Result = Entry->Ptr;
  return Result;
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source)
{
  Assert(StorageIndex);
  sim_entity *Entity = 0;
  if (SimRegion->EntityCount < SimRegion->MaxEntityCount)
  {
    Entity = SimRegion->Entities + SimRegion->EntityCount++;
    MapStorageIndexToEntity(SimRegion, StorageIndex, Entity);
    if (Source)
    {
      *Entity = Source->Sim;
      LoadEntityReference(GameState, SimRegion, &Entity->Sword);
    }
    Entity->StorageIndex = StorageIndex;
  }
  else
  {
    InvalidCodePath;
  }
  return Entity;
}

inline v2
GetSimSpaceP(sim_region *SimRegion, stored_entity *Stored)
{
  world_difference Diff = Subtract(SimRegion->World, &Stored->P, &SimRegion->Origin);
  v2 Result = Diff.dXY;
  return Result;
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v2 *SimP)
{
  sim_entity *Dest = AddEntity(GameState, SimRegion, StorageIndex, Source);
  if (Dest) 
  {
    if (SimP)
    {
      Dest->P = *SimP; 
    }
    else
    {
      Dest->P = GetSimSpaceP(SimRegion, Source);
    }
  }
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState, world *World, world_position Origin, rectangle2 Bounds) 
{

  sim_region *SimRegion = PushStruct(SimArena, sim_region);

  SimRegion->World = World;
  SimRegion->Origin = Origin;
  SimRegion->Bounds = Bounds;

  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

  world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin,
                                               GetMinCorner(SimRegion->Bounds));
  world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin,
                                               GetMaxCorner(SimRegion->Bounds));

  for (int32 ChunkY = MinChunkP.ChunkY;
       ChunkY <= MaxChunkP.ChunkY;
       ChunkY++)
  {
    for (int32 ChunkX = MinChunkP.ChunkX;
         ChunkX <= MaxChunkP.ChunkX;
         ChunkX++)
    {
      world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, SimRegion->Origin.ChunkZ,
                                         &GameState->WorldArena);
      if (Chunk)
      {
        for (world_entity_block *Block = &Chunk->FirstBlock;
             Block;
             Block = Block->Next)
        {

          for (uint32 EntityIndexIndex = 0;
               EntityIndexIndex < Block->EntityCount;
               ++EntityIndexIndex)
          {

            uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndexIndex];
            low_entity *Low = GameState->LowEntities + LowEntityIndex;
            v2 SimSpaceP = GetSimSpaceP(SimRegion, Low);
            if (IsInRectangle(SimRegion->RegionBounds, SimSpaceP))
            {
              AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
            }
          }
        }
      }
    }
  }
}



internal void
EndSim(sim_region *Region, game_state *GameState) 
{
  sim_entity *Entity = Region->Entities;
  for (uint32 EntityIndex = 0;
       EntityIndex < Region->EntityCount;
       ++EntityIndex, ++Entity)
  {
    low_entity *Stored = GameState->LowEntities + Entity->StorageIndex;

    Stored->Sim = *Entity;
    StoreEntityReference(&Stored->Sim.Sword);

    world_position NewP = MapIntoChunkSpace(GameState->World,
                                            Region->Origin, Entity->P);
    ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                         Entity.StorageIndex, Stored, &Stored->P, &NewP);
    if (Entity->StorageIndex == GameState->CamerFollowingEntityIndex)
    {
      world_position NewCameraP = GameState->CameraP;
      NewCameraP.ChunkZ = Stored->P.ChunkZ;

#if 0
      if (CameraFollowingEntity->P.X > (9.0f * World->TileSideInMeters))
      {
        NewCameraP.Offset_.X += 17.0f * World->TileSideInMeters;
      }

      if (CameraFollowingEntity.High->P.X < (-9.0f * World->TileSideInMeters))
      {
        NewCameraP.Offset_.X -= 17.0f * World->TileSideInMeters;
      }

      if (CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters))
      {
        NewCameraP.Offset_.Y += 9.0f * World->TileSideInMeters;
      }

      if (CameraFollowingEntity.High->P.Y < (-5.0f * World->TileSideInMeters))
      {
        NewCameraP.Offset_.Y -= 9.0f * World->TileSideInMeters;
      }
#else
      NewCameraP = Stored.P;
#endif
    }
  }
}

internal bool
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
  bool Hit = false;
  real32 tEpsilon = 0.01f;
  if (PlayerDeltaX != 0.0f)
  {
    real32 tResult = (WallX - RelX) / PlayerDeltaX;
    real32 Y = RelY + tResult * PlayerDeltaY;
    if (tResult >= 0.0f && *tMin > tResult)
    {
      if ((Y >= MinY) && (Y <= MaxY))
      {
        *tMin = Maximum(0.0f, tResult - tEpsilon);
        Hit = true;
      }
    }
  }
  return Hit;
}

internal void
MoveEntity(sim_region *Region, sim_entity Entity, real32 dt, move_spec *MoveSpec, v2 ddP)
{
  world *World = Region->World;
  if (MoveSpec->UnitMaxAccelVector)
  {
    real32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
      ddP = (1.0f / SquareRoot(ddPLength)) * ddP;
    }
  }

  ddP *= MoveSpec->Speed;
  ddP += -MoveSpec->Drag * Entity->dP;

  v2 OldPlayerP = Entity->P;
  v2 PlayerDelta = dt * Entity->dP + 0.5f * Square(dt) * ddP;
  Entity->dP = Entity->dP + dt * ddP;

  v2 NewPlayerP = OldPlayerP + PlayerDelta;

  real32 tRemaining = 1.0f;
  for (uint32 Iteration = 0;
       (Iteration < 4) && (tRemaining > 0.0f);
       Iteration++)
  {

    real32 tMin = 1.0f;
    v2 WallNormal = {};
    sim_entity *HitEntity = 0;
    if (Entity.Low->Collides)
    {
      for (uint32 TestHighEntityIndex = 1;
           TestHighEntityIndex < Region->EntityCount;
           TestHighEntityIndex++)
      {
        sim_entity *TestEntity = Region->Entities + TestHighEntityIndex;
        if (TestHighEntityIndex != Entity.Low->HighEntityIndex)
        {
          if (TestEntity.Low->Collides)
          {
            real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
            real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;
            v2 MinCorner = -0.5f * v2{DiameterW, DiameterH};
            v2 MaxCorner = 0.5f * v2{DiameterW, DiameterH};

            v2 Rel = Entity->P - TestEntity->P;

            if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                         &tMin, MinCorner.Y, MaxCorner.Y))
            {
              WallNormal = v2{-1.0f, 0.0f};
              HitEntity = TestEntity;
            }

            if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                         &tMin, MinCorner.Y, MaxCorner.Y))
            {
              WallNormal = v2{1.0f, 0.0f};
              HitEntity = TestEntity;
            }

            if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                         &tMin, MinCorner.X, MaxCorner.X))
            {

              WallNormal = v2{0.0f, -1.0f};
              HitEntity = TestEntity;
            }

            if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                         &tMin, MinCorner.X, MaxCorner.X))
            {
              WallNormal = v2{0.0f, 1.0f};
              HitEntity = TestEntity;
            }
          }
        }
      }
    }

    Entity->P += tMin * PlayerDelta;
    if (HitEntity)
    {
      Entity->dP = Entity->dP - 1 * Inner(Entity->dP, WallNormal) * WallNormal;
      PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;
      tRemaining -= tMin * tRemaining;
      // Entity->AbsTileZ += HitLow->dAbsTileZ;
    }
    else
    {
      break;
    }
  }

  if (AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
  {
    if (Entity->dP.X > 0)
    {
      Entity->FacingDirection = 0;
    }
    else
    {
      Entity->FacingDirection = 2;
    }
  }
  else if (AbsoluteValue(Entity->dP.X) < AbsoluteValue(Entity->dP.Y))
  {
    if (Entity->dP.Y > 0)
    {
      Entity->FacingDirection = 1;
    }
    else
    {
      Entity->FacingDirection = 3;
    }
  }
}