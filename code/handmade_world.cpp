inline bool
IsCanonical(world *World, real32 TileRel)
{
  bool Result = (TileRel >= -0.5f * World->ChunkSideInMeters) &&
                (TileRel <= 0.5f * World->ChunkSideInMeters);
  return Result;
}

inline bool
IsCanonical(world *World, v2 Offset)
{
  bool Result = IsCanonical(World, Offset.X) && IsCanonical(World, Offset.Y);
  return Result;
}

inline void
ReCanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel) {

  // Number of tiles to move out from current tile
  int32 Offset = RoundReal32ToInt32(*TileRel / World->ChunkSideInMeters);

  *Tile += Offset;
  *TileRel -= Offset*World->ChunkSideInMeters;

  Assert(IsCanonical(World, *TileRel));
}

internal world_position
MapIntoChunkSpace(world *World, world_position BasePos, v2 Offset) { 
  world_position Result = BasePos;
  Result.Offset_ += Offset;
  ReCanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
  ReCanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);
  return Result;
}



#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64) 
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

inline bool
AreOnSameChunk(world *World, world_position *A, world_position *B) {
  Assert(IsCanonical(World, A->Offset_));
  Assert(IsCanonical(World, B->Offset_));
  bool Result = (
    A->ChunkX == B->ChunkX &&
    A->ChunkY == B->ChunkY &&
    A->ChunkZ == B->ChunkZ
  );

  return Result;
}

inline world_chunk * 
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
    memory_arena *Arena = 0) {

  Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);

  int32 HashValue = 19 * ChunkX + 7 * ChunkY + 3 * ChunkZ;
  int32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(World->ChunkHash));

  world_chunk *Chunk = World->ChunkHash + HashSlot;
  
  do {
    if ((ChunkX == Chunk->ChunkX) &&
        (ChunkY == Chunk->ChunkY) &&
        (ChunkZ == Chunk->ChunkZ))
    {
      break;
    }

    if (Arena && Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED && !Chunk->NextInHash) 
    {
      Chunk->NextInHash = PushStruct(Arena, world_chunk);
      Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
      Chunk = Chunk->NextInHash;
    }

    if (Arena && Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED)
    {
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

internal void
InitializeWorld(world *World, real32 TileSideInMeters)
{
  World->TileSideInMeters = TileSideInMeters;
  World->ChunkSideInMeters = 16.0f * TileSideInMeters;
  World->FirstFree = 0;

  for (uint32 TileChunkIndex = 0;
    TileChunkIndex < ArrayCount(World->ChunkHash);
    TileChunkIndex++)
  {
    World->ChunkHash[TileChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
    World->ChunkHash[TileChunkIndex].FirstBlock.EntityCount = 0;
  }
}


inline world_difference 
Subtract(world *World, world_position *A, world_position *B) {
  world_difference Result;

  // v2 dTileXY = {(real32)((real64)A->AbsTileX - (real64)B->AbsTileX),
  //   (real32)((real64)A->AbsTileY - (real64)B->AbsTileY)};
  // real32 dTileZ = (real32)((real64)A->AbsTileZ - (real64)B->AbsTileZ);

  v2 dTileXY = {(real32)((real64)A->ChunkX - (real64)B->ChunkX),
    (real32)((real64)A->ChunkY - (real64)B->ChunkY)};
  real32 dTileZ = (real32)((real64)A->ChunkZ - (real64)B->ChunkZ);

  Result.dXY = World->ChunkSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
  Result.dZ = World->ChunkSideInMeters * dTileZ;

  return Result;
}

inline world_position
CenteredChunkPoint(uint32 ChunkX, uint32 ChunkY, uint32 ChunkZ) {
  world_position Result = {};
  Result.ChunkX = ChunkX;
  Result.ChunkY = ChunkY;
  Result.ChunkZ = ChunkZ;

  return Result;
}

inline world_position 
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
  world_position Result = {};
  Result.ChunkX = AbsTileX / TILES_PER_CHUNK;
  Result.ChunkY = AbsTileY / TILES_PER_CHUNK;
  Result.ChunkZ = AbsTileZ / TILES_PER_CHUNK;

  Result.Offset_.X = (real32)(AbsTileX - (Result.ChunkX * TILES_PER_CHUNK)) * World->TileSideInMeters;
  Result.Offset_.Y = (real32)(AbsTileY - (Result.ChunkY * TILES_PER_CHUNK)) * World->TileSideInMeters;
  return Result;
}

inline void
ChangeEntityLocation(memory_arena *Arena, world *World, uint32 LowEntityIndex,
                     world_position *OldP, world_position *NewP)
{
  if (OldP && AreOnSameChunk(World, NewP, OldP))
  {
    // Leave entity where it is
  }
  else 
  {
    if (OldP) {
      world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, Arena);
      Assert(Chunk);
      if (Chunk) 
      {
        bool NotFound = true;
        world_entity_block *FirstBlock = &Chunk->FirstBlock;
        for (world_entity_block *Block = &Chunk->FirstBlock;
             Block && NotFound;
             Block = Block->Next)
        {
          for (uint32 Index = 0;
               (Index < Block->EntityCount) &&  NotFound;
               Index++)
          {
            if (Block->LowEntityIndex[Index] == LowEntityIndex)
            {
              Block->LowEntityIndex[Index] = FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
              if (FirstBlock->EntityCount == 0)
              {
                world_entity_block *NextBlock = FirstBlock->Next;
                *FirstBlock = *NextBlock;
                NextBlock->Next = World->FirstFree;
                World->FirstFree = NextBlock;
              }
              NotFound = false;
            }
          }
        }
      }
    }
    world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
    world_entity_block *Block = &Chunk->FirstBlock;
    if (Block->EntityCount == ArrayCount(Block->LowEntityIndex))
    {
      world_entity_block *OldBlock = World->FirstFree;
      if (OldBlock)
      {
        World->FirstFree = OldBlock->Next;
      }
      else
      {
        OldBlock = PushStruct(Arena, world_entity_block);;
      }
      *OldBlock = *Block;
      Block->Next = OldBlock;
      Block->EntityCount = 0;
    }
    Assert(Block->EntityCount != ArrayCount(Block->LowEntityIndex));
    Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
  }
}