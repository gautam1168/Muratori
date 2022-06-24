#include "handmade.hpp"
#include "handmade_math.hpp"
#include "handmade_world.cpp"
#include "handmade_random.hpp"
#include "handmade_sim_region.cpp"
// Platform -> Game

// Game -> Platform
void GameOutputSound(game_sound_buffer *SoundBuffer, game_state *GameState)
{
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
  {
    // TODO(gaurav): Implement sine
#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
    int16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

#if 0
    GameState->tSine += 2.0f * Pi32 * 1.0f/(real32)WavePeriod;
#endif
  }
}

inline uint32 SafeTruncateUint64(uint64 Value)
{
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

#pragma pack(push, 1)
struct bitmap_header
{
  uint16 FileType;
  uint32 FileSize;
  uint16 Reserved1;
  uint16 Reserved2;
  uint32 BitmapOffset;
  uint32 Size;
  uint32 Width;
  uint32 Height;
  uint16 Planes;
  uint16 BitsPerPixel;
  uint32 Compression;
  uint32 ImageSize;
  uint32 XPixelsPerM;
  uint32 YPixelsPerM;
  uint32 ColorsUsed;
  uint32 ImportantColors;
  uint32 RedMask;
  uint32 GreenMask;
  uint32 BlueMask;
  uint32 AlphaMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
  loaded_bitmap Result = {};
  // Byte order is RR GG BB AA
  // In little endian AABBGGRR
  debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
  if (ReadResult.ContentSize != 0)
  {
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    Result.Pixels = Pixels;
    Result.Width = Header->Width;
    Result.Height = Header->Height;

    Assert((Header->AlphaMask | Header->GreenMask | Header->BlueMask | Header->RedMask) == 0xFFFFFFFF);
    uint32 RedShift = 0;
    uint32 BlueShift = 0;
    uint32 GreenShift = 0;
    uint32 AlphaShift = 0;

    Assert(FindLowestSetBit(&RedShift, Header->RedMask));
    Assert(FindLowestSetBit(&GreenShift, Header->GreenMask));
    Assert(FindLowestSetBit(&BlueShift, Header->BlueMask));
    Assert(FindLowestSetBit(&AlphaShift, Header->AlphaMask));

    uint32 *SourceDest = Pixels;
    for (uint32 Y = 0;
         Y < Header->Height;
         Y++)
    {
      for (uint32 X = 0;
           X < Header->Width;
           X++)
      {
        uint32 C = *SourceDest;
        *SourceDest++ = ((((C >> AlphaShift) & 0xFF) << 24) |
                         (((C >> RedShift) & 0xFF) << 16) |
                         (((C >> GreenShift) & 0xFF) << 8) |
                         (((C >> BlueShift) & 0xFF)));
      }
    }
  }
  return Result;
}

// Fills rectangle from [low, high) i.e. excludes the upperbound
internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              v2 vMin, v2 vMax,
              real32 R, real32 G, real32 B)
{
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;

  int32 MinX = RoundReal32ToInt32(vMin.X);
  int32 MinY = RoundReal32ToInt32(vMin.Y);
  int32 MaxX = RoundReal32ToInt32(vMax.X);
  int32 MaxY = RoundReal32ToInt32(vMax.Y);

  if (MinX < 0)
  {
    MinX = 0;
  }

  if (MinY < 0)
  {
    MinY = 0;
  }

  if (MaxX > Buffer->Width)
  {
    MaxX = Buffer->Width;
  }

  if (MaxY > Buffer->Height)
  {
    MaxY = Buffer->Height;
  }

  uint8 *Row = (uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch;

  uint32 Color = (0xFF000000 |
                  RoundReal32ToUInt32(R * 255.0f) << 16 |
                  RoundReal32ToUInt32(G * 255.0f) << 8 |
                  RoundReal32ToUInt32(B * 255.0f));

  for (int Y = MinY; Y < MaxY; Y++)
  {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = MinX; X < MaxX; X++)
    {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
  }
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, 
  real32 RealX, real32 RealY
 )
{
  int32 MinX = RoundReal32ToInt32(RealX);
  int32 MinY = RoundReal32ToInt32(RealY);
  int32 MaxX = MinX + Bitmap->Width;
  int32 MaxY = MinY + Bitmap->Height;

  int32 SourceOffsetX = 0;
  if (MinX < 0)
  {
    SourceOffsetX = -MinX;
    MinX = 0;
  }

  int32 SourceOffsetY = 0;
  if (MinY < 0)
  {
    SourceOffsetY = -MinY;
    MinY = 0;
  }

  if (MaxX > Buffer->Width)
  {
    MaxX = Buffer->Width;
  }

  if (MaxY > Buffer->Height)
  {
    MaxY = Buffer->Height;
  }

  uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
  SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
  uint8 *DestRow = (uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch;
  for (int32 Y = MinY;
       Y < MaxY;
       Y++)
  {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = SourceRow;
    for (int32 X = MinX;
         X < MaxX;
         X++)
    {
      real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
      real32 SR = (real32)((*Source >> 16) & 0xFF);
      real32 SG = (real32)((*Source >> 8) & 0xFF);
      real32 SB = (real32)(*Source & 0xFF);

      real32 DR = (real32)((*Dest >> 16) & 0xFF);
      real32 DG = (real32)((*Dest >> 8) & 0xFF);
      real32 DB = (real32)(*Dest & 0xFF);

      real32 R = (1.0f - A) * DR + A * SR;
      real32 G = (1.0f - A) * DG + A * SG;
      real32 B = (1.0f - A) * DB + A * SB;

      *Dest = (TruncateReal32ToInt32(R) << 16) |
              (TruncateReal32ToInt32(G) << 8) |
              TruncateReal32ToInt32(B);
      Dest++;
      Source++;
    }
    DestRow += Buffer->Pitch;
    SourceRow -= Bitmap->Width;
  }
}

inline low_entity *
GetLowEntity(game_state *GameState, uint32 LowIndex)
{
  low_entity *Result = 0;
  entity Entity = {};
  if (LowIndex > 0 && (LowIndex < GameState->LowEntityCount))
  {
    Result = GameState->LowEntities + LowIndex;
  }
  return Result;
}

inline v2
GetCameraSpaceP(game_state *GameState, low_entity *LowEntity)
{
  world_difference Diff = Subtract(GameState->World, &LowEntity->P, &GameState->CameraP);
  v2 Result = Diff.dXY;
  return Result;
}

struct add_low_entity_result {
  uint32 LowIndex;
  low_entity *Low;
};

internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type Type, world_position *P)
{
  Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
  uint32 EntityIndex = GameState->LowEntityCount++;

  low_entity *LowEntity = GameState->LowEntities + EntityIndex;
  *LowEntity = {};
  LowEntity->Type = Type;
  ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, LowEntity, 0, P);

  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.LowIndex = EntityIndex;

  return Result;
}

internal void
InitHitPoints(low_entity *Entity, uint32 HitPointCount) 
{
  Assert(HitPointCount <= ArrayCount(Entity->HitPoint));
  Entity->HitPointMax = HitPointCount;
  for (uint32 HitPointIndex = 0;
       HitPointIndex < HitPointCount;
       ++HitPointIndex)
  {
      hit_point *HitPoint = Entity->HitPoint + HitPointIndex;
      HitPoint->Flags = 0;
      HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
  }
}

internal add_low_entity_result
AddSword(game_state *GameState) 
{
  add_low_entity_result EntityResult = AddLowEntity(GameState, EntityType_Sword, 0);
  EntityResult.Low->Height = 0.5f;
  EntityResult.Low->Width = 1.0f;
  EntityResult.Low->Collides = false;
  return EntityResult;
}

internal add_low_entity_result
AddPlayer(game_state *GameState)
{
  world_position P = GameState->CameraP;
  add_low_entity_result EntityResult = AddLowEntity(GameState, EntityType_Hero, &P);
  low_entity *Entity = EntityResult.Low;

  Entity->Height = 0.5f;
  Entity->Width = 1.0f;
  Entity->Collides = true;

  InitHitPoints(Entity, 3);
  add_low_entity_result Sword = AddSword(GameState);
  Entity->SwordLowIndex = Sword.LowIndex;

  if (GameState->CamerFollowingEntityIndex == 0)
  {
    GameState->CamerFollowingEntityIndex = EntityResult.LowIndex;
  }
  return EntityResult;
}

internal add_low_entity_result
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result EntityResult = AddLowEntity(GameState, EntityType_Wall, &P);
  low_entity *Entity = EntityResult.Low;
  Entity->Height = GameState->World->TileSideInMeters;
  Entity->Width = Entity->Height;
  Entity->Collides = true;

  return EntityResult;
}

internal add_low_entity_result
AddMonster(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) 
{
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result EntityResult = AddLowEntity(GameState, EntityType_Monster, &P);
  EntityResult.Low->Height = 0.5f;
  EntityResult.Low->Width = 1.0f;
  EntityResult.Low->Collides = true;
  InitHitPoints(EntityResult.Low, 3);
  return EntityResult;
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) 
{
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result EntityResult = AddLowEntity(GameState, EntityType_Familiar, &P);
  EntityResult.Low->Height = 0.5f;
  EntityResult.Low->Width = 1.0f;
  EntityResult.Low->Collides = false;

  return EntityResult;
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

struct move_spec 
{
  bool UnitMaxAccelVector;
  real32 Speed;
  real32 Drag;
};

inline move_spec
DefaultMoveSpec()
{
  move_spec Result;
  Result.UnitMaxAccelVector = false;
  Result.Speed = 1.0f;
  Result.Drag = 0.0f;
  return Result;
}

internal void
MoveEntity(game_state *GameState, entity Entity, real32 dt, move_spec *MoveSpec, v2 ddP)
{
  world *World = GameState->World;
  if (MoveSpec->UnitMaxAccelVector)
  {
    real32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
      ddP = (1.0f / SquareRoot(ddPLength)) * ddP;
    }
  }

  ddP *= MoveSpec->Speed;
  ddP += -MoveSpec->Drag * Entity.High->dP;

  v2 OldPlayerP = Entity.High->P;
  v2 PlayerDelta = dt * Entity.High->dP + 0.5f * Square(dt) * ddP;
  Entity.High->dP = Entity.High->dP + dt * ddP;

  v2 NewPlayerP = OldPlayerP + PlayerDelta;

  real32 tRemaining = 1.0f;
  for (uint32 Iteration = 0;
       (Iteration < 4) && (tRemaining > 0.0f);
       Iteration++)
  {

    real32 tMin = 1.0f;
    v2 WallNormal = {};
    uint32 HitHighEntityIndex = 0;
    if (Entity.Low->Collides)
    {
      for (uint32 TestHighEntityIndex = 1;
           TestHighEntityIndex < GameState->HighEntityCount;
           TestHighEntityIndex++)
      {
        if (TestHighEntityIndex != Entity.Low->HighEntityIndex)
        {
          entity TestEntity;
          TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
          TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
          TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;
          if (TestEntity.Low->Collides)
          {
            real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
            real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;
            v2 MinCorner = -0.5f * v2{DiameterW, DiameterH};
            v2 MaxCorner = 0.5f * v2{DiameterW, DiameterH};

            v2 Rel = Entity.High->P - TestEntity.High->P;

            if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                         &tMin, MinCorner.Y, MaxCorner.Y))
            {
              WallNormal = v2{-1.0f, 0.0f};
              HitHighEntityIndex = TestHighEntityIndex;
            }

            if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                         &tMin, MinCorner.Y, MaxCorner.Y))
            {
              WallNormal = v2{1.0f, 0.0f};
              HitHighEntityIndex = TestHighEntityIndex;
            }

            if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                         &tMin, MinCorner.X, MaxCorner.X))
            {

              WallNormal = v2{0.0f, -1.0f};
              HitHighEntityIndex = TestHighEntityIndex;
            }

            if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                         &tMin, MinCorner.X, MaxCorner.X))
            {

              WallNormal = v2{0.0f, 1.0f};
              HitHighEntityIndex = TestHighEntityIndex;
            }
          }
        }
      }
    }

    Entity.High->P += tMin * PlayerDelta;
    if (HitHighEntityIndex)
    {
      Entity.High->dP = Entity.High->dP - 1 * Inner(Entity.High->dP, WallNormal) * WallNormal;
      PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;
      tRemaining -= tMin * tRemaining;
      high_entity *HitHigh = GameState->HighEntities_ + HitHighEntityIndex;
      low_entity *HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
      // Entity.High->AbsTileZ += HitLow->dAbsTileZ;
    }
    else
    {
      break;
    }
  }

  if (AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
  {
    if (Entity.High->dP.X > 0)
    {
      Entity.High->FacingDirection = 0;
    }
    else
    {
      Entity.High->FacingDirection = 2;
    }
  }
  else if (AbsoluteValue(Entity.High->dP.X) < AbsoluteValue(Entity.High->dP.Y))
  {
    if (Entity.High->dP.Y > 0)
    {
      Entity.High->FacingDirection = 1;
    }
    else
    {
      Entity.High->FacingDirection = 3;
    }
  }

  world_position NewP = MapIntoChunkSpace(GameState->World, GameState->CameraP, Entity.High->P);
  ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex, Entity.Low, &Entity.Low->P, &NewP);
}

internal void
SimCameraRegion(game_state *GameState)
{
  world *World = GameState->World;
  uint32 TileSpanX = 17 * 3;
  uint32 TileSpanY = 9 * 3;
  rectangle2 CameraBounds = RectCenterDim(
      // TileMap->TileSideInMeters * v2{(real32)NewCameraP.AbsTileX, (real32)NewCameraP.AbsTileY},
      v2{0, 0},
      World->TileSideInMeters * v2{(real32)TileSpanX, (real32)TileSpanY});

  sim_region *SimRegion = BeginSim(SimArena, GameState->World, GameState->CameraP,
                                   CameraBounds);

  EndSim(SimRegion, GameState);
}

// inline void
// UpdateEntity(GameState, Entity, dt) {

// }

inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap, v2 Offset, real32 OffsetZ, 
  v2 Align, v2 Dim, v4 Color)
{
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;
  Piece->Bitmap = Bitmap;
  Piece->Offset = Group->GameState->MetersToPixels*v2{Offset.X, -Offset.Y} - Align;
  Piece->OffsetZ = Group->GameState->MetersToPixels*OffsetZ;
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->A = Color.A;
  Piece->Dim = Dim;
}

inline void
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap, v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f)
{
  PushPiece(Group, Bitmap, Offset, OffsetZ, Align, v2{0, 0}, v4{0, 0, 0, 1.0f});
}

inline void
PushRect(entity_visible_piece_group *Group, v2 Offset, real32 OffsetZ, v2 Dim, v4 Color)
{
  PushPiece(Group, 0, Offset, OffsetZ, v2{0, 0}, Dim, Color);
}

inline entity
EntityFromHighIndex(game_state *GameState, uint32 HightEntityIndex) 
{
  entity Result = {};
  if (HightEntityIndex) 
  {
    Assert(HightEntityIndex < ArrayCount(GameState->HighEntities_));

    Result.High = GameState->HighEntities_ + HightEntityIndex;
    Result.LowIndex = Result.High->LowEntityIndex;
    Result.Low = GameState->LowEntities + Result.LowIndex;
  }
  return Result;
}

inline void
UpdateFamiliar(game_state *GameState, entity Entity, real32 dt)
{
  entity ClosestHero = {};
  real32 ClosestHeroD = Square(10.0f); //Max search radius in meter
  for (uint32 HighEntityIndex = 1;
       HighEntityIndex < GameState->HighEntityCount;
       HighEntityIndex++)
  {
    entity TestEntity = EntityFromHighIndex(GameState, HighEntityIndex);

    if (TestEntity.Low->Type == EntityType_Hero) 
    {
      real32 TestDSq = LengthSq(TestEntity.High->P - Entity.High->P);
      if (ClosestHeroD > TestDSq) {
        ClosestHeroD = TestDSq;
        ClosestHero = TestEntity;
      }
    }
  }

  v2 ddP = {};
  if (ClosestHero.High && (ClosestHeroD > 3.0f)) {
    real32 Acceleration = 0.5f;
    real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroD);
    ddP = OneOverLength * (ClosestHero.High->P - Entity.High->P);
  }

  move_spec MoveSpec = DefaultMoveSpec();
  MoveSpec.UnitMaxAccelVector = true;
  MoveSpec.Speed = 50.0f;
  MoveSpec.Drag = 8.0f;
  MoveEntity(GameState, Entity, dt, &MoveSpec, ddP);
}

inline void
UpdateMonster(game_state *GameState, entity Entity, real32 dt)
{
}

inline void
UpdateSword(game_state *GameState, entity Entity, real32 dt)
{
  move_spec MoveSpec = DefaultMoveSpec();
  MoveSpec.UnitMaxAccelVector = false;
  MoveSpec.Speed = 0.0f;
  MoveSpec.Drag = 0.0f;
  v2 OldP = Entity.High->P;
  MoveEntity(GameState, Entity, dt, &MoveSpec, V2(0, 0));
  real32 DistanceTravelled = Length(Entity.High->P - OldP);
  Entity.Low->DistanceRemaining -= DistanceTravelled;
  if (Entity.Low->DistanceRemaining < 0.0f) 
  {
    ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                         Entity.LowIndex, Entity.Low, &Entity.Low->P, 0);
  }
}

inline void
DrawHitPoints(low_entity *LowEntity, entity_visible_piece_group *PieceGroup) 
{
if (LowEntity->HitPointMax >= 1)
  {
    v2 HealthDim = {0.2f, 0.2f};
    real32 SpacingX = 1.5f * HealthDim.X;
    v2 HitP = {-0.5f * (LowEntity->HitPointMax - 1) * SpacingX, -0.2f};
    v2 dHitP = {SpacingX, 0.0f};
    for (uint32 HealthIndex = 0;
         HealthIndex < LowEntity->HitPointMax;
         ++HealthIndex)
    {
      hit_point *Hitpoint = LowEntity->HitPoint + HealthIndex;
      v4 Color = {1.0f, 0.0f, 0.0f, 0.0f};
      if (Hitpoint->FilledAmount == 0)
      {
        Color.R = 0.2f;
        Color.G = 0.2f;
        Color.B = 0.2f;
      }
      PushRect(PieceGroup, HitP, 0, HealthDim, Color);
      HitP += dHitP;
    }
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  if (!Memory->IsInitialized)
  {
    // Reserve entity slot 0 for null entity
    AddLowEntity(GameState, EntityType_Null, 0);
    GameState->HighEntityCount = 1;

    GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

    GameState->Tree = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/onetree.bmp");
    GameState->Monster = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/monster_stand1.bmp");
    GameState->Sword = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/weapon.bmp");

    GameState->FamiliarBitmaps[0] = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/familiar_left.bmp");
    GameState->FamiliarBitmaps[1] = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/familiar_right.bmp");

    hero_bitmaps *Bitmap = &GameState->HeroBitmaps[0];
    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_right.bmp");
    Bitmap->Align = v2{30, 55};
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_back.bmp");
    Bitmap->Align = v2{30, 55};
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_left.bmp");
    Bitmap->Align = v2{30, 55};
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_front.bmp");
    Bitmap->Align = v2{30, 55};

    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));

    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;

    InitializeWorld(World, 1.4f);

    int32 TileSideInPixels = 60;
    GameState->MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;
    uint32 RandomNumberIndex = 0;

    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    uint32 ScreenBaseX = 0;
    uint32 ScreenBaseY = 0;
    uint32 ScreenBaseZ = 0;
    uint32 ScreenX = ScreenBaseX;
    uint32 ScreenY = ScreenBaseY;
    uint32 AbsTileZ = ScreenBaseZ;

    bool DoorLeft = false;
    bool DoorRight = false;
    bool DoorTop = false;
    bool DoorBottom = false;
    bool DoorUp = false;
    bool DoorDown = false;

    for (uint32 ScreenIndex = 0;
         ScreenIndex < 2;
         ScreenIndex++)
    {
      Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
      uint32 RandomChoice;
      if (DoorUp || DoorDown)
      {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
      }
      else
      {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
      }
      bool CreatedZDoor = false;
      if (RandomChoice == 2)
      {
        CreatedZDoor = true;
        if (AbsTileZ == ScreenBaseZ)
        {
          DoorUp = true;
        }
        else
        {
          DoorDown = 0;
        }
      }
      else if (RandomChoice == 1)
      {
        DoorRight = true;
      }
      else
      {
        DoorTop = true;
      }

      for (uint32 TileY = 0;
           TileY < TilesPerHeight;
           TileY++)
      {
        for (uint32 TileX = 0;
             TileX < TilesPerWidth;
             TileX++)
        {
          uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
          uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

          uint32 TileValue = 1;

          if ((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight / 2))))
          {
            TileValue = 2;
          }

          if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight / 2))))
          {
            TileValue = 2;
          }

          if ((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth / 2))))
          {
            TileValue = 2;
          }

          if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2))))
          {
            TileValue = 2;
          }

          if (TileX == 10 && TileY == 6)
          {
            if (DoorUp)
            {
              TileValue = 3;
            }
            if (DoorDown)
            {
              TileValue = 4;
            }
          }

          if (TileValue == 2)
          {
            AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
          }
        }
      }

      DoorLeft = DoorRight;
      DoorBottom = DoorTop;
      if (CreatedZDoor)
      {
        DoorDown = !DoorDown;
        DoorUp = !DoorUp;
      }
      else
      {
        DoorUp = false;
        DoorDown = false;
      }

      DoorRight = false;
      DoorTop = false;

      if (RandomChoice == 2)
      {
        if (AbsTileZ == ScreenBaseZ)
        {
          AbsTileZ = ScreenBaseZ + 1;
        }
        else
        {
          AbsTileZ = ScreenBaseZ;
        }
      }
      else if (RandomChoice == 1)
      {
        ScreenX += 1;
      }
      else
      {
        ScreenY += 1;
      }
    }
    world_position NewCameraP = {};
    uint32 CameraTileX = (ScreenBaseX * TilesPerWidth) + (17/2);
    uint32 CameraTileY = (ScreenBaseY * TilesPerHeight) + (9 / 2);
    NewCameraP = ChunkPositionFromTilePosition(GameState->World, 
      CameraTileX,
      CameraTileY,
      ScreenBaseZ
    );

    AddMonster(GameState, CameraTileX + 2, CameraTileY + 2, ScreenBaseZ);
    AddFamiliar(GameState, CameraTileX - 2, CameraTileY + 2, ScreenBaseZ);

    SetCamera(GameState, NewCameraP);

    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&TileMap-> GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
  world *World = GameState->World;
  // Assert(TileMap);

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
    if (LowIndex == 0)
    {
      if (Controller->Start.EndedDown)
      {
        uint32 EntityIndex = AddPlayer(GameState).LowIndex; // AddEntity(GameState);
        GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
      }
    }
    else
    {
      entity ControllingEntity = ForceEntityIntoHigh(GameState, LowIndex);
      v2 ddP = {};

      if (Controller->IsAnalog)
      {
        ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
      }
      else
      {
        if (Controller->MoveUp.EndedDown)
        {
          ddP.Y = 1.0f;
        }
        if (Controller->MoveDown.EndedDown)
        {
          ddP.Y = -1.0f;
        }
        if (Controller->MoveLeft.EndedDown)
        {
          ddP.X = -1.0f;
        }
        if (Controller->MoveRight.EndedDown)
        {
          ddP.X = 1.0f;
        }
      }

      v2 dSword = {};
      if (Controller->ActionUp.EndedDown)
      {
        dSword = V2(0.0f, 1.0f);
      }
      if (Controller->ActionDown.EndedDown)
      {
        dSword = V2(0.0f, -1.0f);
      }
      if (Controller->ActionLeft.EndedDown)
      {
        dSword = V2(-1.0f, 0.0f);
      }
      if (Controller->ActionRight.EndedDown)
      {
        dSword = V2(1.0f, 0.0f);
      }

      move_spec MoveSpec = DefaultMoveSpec();
      MoveSpec.UnitMaxAccelVector = true;
      MoveSpec.Speed = 50.0f;
      MoveSpec.Drag = 8.0f;
      MoveEntity(GameState, ControllingEntity, Input->dtForFrame, &MoveSpec, ddP);
      if (dSword.X != 0 || dSword.Y != 0) {
        low_entity *LowSword = GetLowEntity(GameState, ControllingEntity.Low->SwordLowIndex);
        if (LowSword) 
        {
          world_position SwordP = ControllingEntity.Low->P;
          ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                               ControllingEntity.Low->SwordLowIndex, LowSword, 0,
                               &SwordP);
          entity Sword = ForceEntityIntoHigh(GameState, ControllingEntity.Low->SwordLowIndex);
          Sword.Low->DistanceRemaining = 5.0f;
          Sword.High->dP = 5.0f * dSword;
        }
      }
    }
  }

  

  uint32 TileSpanX = 17 * 3;
  uint32 TileSpanY = 9 * 3;
  rectangle2 CameraBounds = RectCenterDim(
      // TileMap->TileSideInMeters * v2{(real32)NewCameraP.AbsTileX, (real32)NewCameraP.AbsTileY},
      v2{0, 0},
      World->TileSideInMeters * v2{(real32)TileSpanX, (real32)TileSpanY});

  sim_region *SimRegion = BeginSim(SimArena, GameState->World, GameState->CameraP,
                                  CameraBounds);


#if 1
  DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif

  real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

  // Draw player
  entity_visible_piece_group PieceGroup;
  PieceGroup.GameState = GameState;
  entity *Entity = SimRegion->Entities;
  for (uint32 EntityIndex = 0;
       EntityIndex < SimRegion->EntityCount;
       ++EntityIndex)
  {

    PieceGroup.Count = 0;
    low_entity *LowEntity = GameState->LowEntities + HighEntity->StorageIndex;

    real32 dt = Input->dtForFrame;
    
    // UpdateEntity(GameState, Entity, dt);


    // real32 PlayerLeft = PlayerGroundPointX - 0.5f * LowEntity->Width * GameState->MetersToPixels;
    // real32 PlayerTop = PlayerGroundPointY - 0.5f * LowEntity->Height * GameState->MetersToPixels;

    hero_bitmaps HeroBitmap = GameState->HeroBitmaps[LowEntity->FacingDirection];

    switch (LowEntity->Type)
    {
      case EntityType_Hero: 
      {
        // DrawRectangle(Buffer,
        //             v2{PlayerLeft, PlayerTop},
        //             v2{PlayerLeft + LowEntity->Width * GameState->MetersToPixels,
        //                PlayerTop + LowEntity->Height * GameState->MetersToPixels},
        //             1.0f, 1.0f, 0.0f);
        PushBitmap(&PieceGroup, &HeroBitmap.Body, v2{0, 0}, 0, HeroBitmap.Align);
        DrawHitPoints(LowEntity, &PieceGroup);
      } break;
      case EntityType_Wall: 
      {
        PushBitmap(&PieceGroup, &GameState->Tree, v2{0, 0}, 0, HeroBitmap.Align); 
      } break;
      case EntityType_Sword: 
      {
        UpdateSword(GameState, Entity, dt);
        PushBitmap(&PieceGroup, &GameState->Sword, v2{0, 0}, 0, V2(16, 10)); 
      } break;
      case EntityType_Familiar: 
      {
        UpdateFamiliar(GameState, Entity, dt);
        loaded_bitmap FamiliarBitmap = GameState->FamiliarBitmaps[0];
        PushBitmap(&PieceGroup, &FamiliarBitmap, v2{0, 0}, 0, HeroBitmap.Align); 
      } break;
      case EntityType_Monster: 
      {
        UpdateMonster(GameState, Entity, dt);
        PushBitmap(&PieceGroup, &GameState->Monster, v2{0, 0}, 0, HeroBitmap.Align); 
        DrawHitPoints(LowEntity, &PieceGroup);
      } break;
      case EntityType_Null:
      break;
      default:
        InvalidCodePath;
    }

    real32 EntityGroundPointX = ScreenCenterX + (GameState->MetersToPixels *
                                                 Entity->P.X);
    real32 EntityGroundPointY = ScreenCenterY - (GameState->MetersToPixels *
                                                 Entity->P.Y);

    for (int32 PieceIndex = 0;
      PieceIndex < PieceGroup.Count;
      PieceIndex++
    )
    {
      entity_visible_piece *Piece = &PieceGroup.Pieces[PieceIndex];
      v2 Center = {EntityGroundPointX + Piece->Offset.X,
                   EntityGroundPointY + Piece->Offset.Y};
      if (Piece->Bitmap)
      {
        DrawBitmap(Buffer, Piece->Bitmap, Center.X, Center.Y);
      }
      else 
      {
        v2 HalfDim = GameState->MetersToPixels * 0.5f * Piece->Dim;
        DrawRectangle(Buffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
      }
    }
  }

  EndSim(SimRegion, GameState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameState->ToneHz = 400;
  GameOutputSound(SoundBuffer, GameState);
}

/*
void RenderWeirdGradient(game_offscreen_buffer *Buffer, game_state *GameState) {
  uint8* Row = (uint8*) Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32* Pixel = (uint32*) Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8 Blue = (uint8)(X + GameState->BlueOffset);
      uint8 Green = (uint8)(Y + GameState->GreenOffset);
      *Pixel++ = (Green << 8) | Blue;
    }
    Row += Buffer->Pitch;
  }
}
*/
