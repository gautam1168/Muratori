#include "handmade.hpp"
#include "handmade_math.hpp"
#include "handmade_world.cpp"
#include "handmade_random.hpp"
// Platform -> Game

// Game -> Platform
void GameOutputSound(game_sound_buffer *SoundBuffer, game_state *GameState) {
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/GameState->ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;  ++SampleIndex) {
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


inline uint32 SafeTruncateUint64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF);
  return (uint32)Value;
}

#pragma pack(push, 1)
struct bitmap_header {
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
  if (ReadResult.ContentSize != 0) {
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
        *SourceDest++ = (
          (((C >> AlphaShift) & 0xFF) << 24) |
          (((C >> RedShift) & 0xFF) << 16) |
          (((C >> GreenShift) & 0xFF) << 8) |
          (((C >> BlueShift) & 0xFF))
        );
      }
    }
  }
  return Result;
}

// Fills rectangle from [low, high) i.e. excludes the upperbound
internal void 
DrawRectangle(game_offscreen_buffer *Buffer, 
  v2 vMin, v2 vMax, 
  real32 R, real32 G, real32 B) {
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;

  int32 MinX = RoundReal32ToInt32(vMin.X);
  int32 MinY = RoundReal32ToInt32(vMin.Y);
  int32 MaxX = RoundReal32ToInt32(vMax.X);
  int32 MaxY = RoundReal32ToInt32(vMax.Y);

  if (MinX < 0) {
    MinX = 0;
  }

  if (MinY < 0) {
    MinY = 0;
  }

  if (MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }

  if (MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }

  uint8 *Row = (uint8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch;

  uint32 Color = (
    0xFF000000 |
    RoundReal32ToUInt32(R * 255.0f) << 16 |
    RoundReal32ToUInt32(G * 255.0f) << 8 |
    RoundReal32ToUInt32(B * 255.0f)
  );

  for (int Y = MinY; Y < MaxY; Y++) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = MinX; X < MaxX; X++) {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
  }
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY, int32 AlignX = 0, int32 AlignY = 0) 
{
  RealX -= (real32)AlignX;
  RealY -= (real32)AlignY;

  int32 MinX = RoundReal32ToInt32(RealX);
  int32 MinY = RoundReal32ToInt32(RealY);
  int32 MaxX = MinX + Bitmap->Width;
  int32 MaxY = MinY + Bitmap->Height;

  int32 SourceOffsetX = 0;
  if (MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }

  int32 SourceOffsetY = 0;
  if (MinY < 0) {
    SourceOffsetY = -MinY;
    MinY = 0;
  }

  if (MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }

  if (MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }

  uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
  SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
  uint8 *DestRow = (uint8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch;
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
 
      real32 R = (1.0f - A)*DR + A * SR;
      real32 G = (1.0f - A)*DG + A * SG;
      real32 B = (1.0f - A)*DB + A * SB;

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

inline high_entity * 
MakeEntityHighFrequency(game_state *GameState, low_entity *LowEntity, uint32 LowIndex, v2 CameraSpaceP)
{
  high_entity *HighEntity = 0;
  Assert(LowEntity->HighEntityIndex == 0);
  if (LowEntity->HighEntityIndex == 0)
  {
    if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities_))
    {
      uint32 HighIndex = GameState->HighEntityCount++;
      HighEntity = &GameState->HighEntities_[HighIndex];

      HighEntity->P = CameraSpaceP;
      HighEntity->dP = v2{0, 0};
      HighEntity->ChunkZ = LowEntity->P.ChunkZ;
      HighEntity->FacingDirection = 0;
      HighEntity->LowEntityIndex = LowIndex;
      LowEntity->HighEntityIndex = HighIndex;
    }
    else
    {
      InvalidCodePath;
    }
  }
  return HighEntity;
}

inline high_entity *
MakeEntityHighFrequency(game_state *GameState, uint32 LowIndex)
{
  low_entity *LowEntity = GameState->LowEntities + LowIndex;
  high_entity *HighEntity = 0;
  if (LowEntity->HighEntityIndex)
  {
    HighEntity = GameState->HighEntities_ + LowEntity->HighEntityIndex;
  }
  else 
  {
    v2 CameraSpaceP = GetCameraSpaceP(GameState, LowEntity);
    HighEntity = MakeEntityHighFrequency(GameState, LowEntity, LowIndex, CameraSpaceP);
  }
  return HighEntity;
}

inline entity
GetHighEntity(game_state *GameState, uint32 LowIndex)
{
  entity Result = {};
  
  if(LowIndex > 0 && (LowIndex < GameState->LowEntityCount))
  {
    Result.LowIndex = LowIndex;
    Result.Low = GameState->LowEntities + LowIndex;
    Result.High = MakeEntityHighFrequency(GameState, LowIndex);
  }

  return Result;
}

inline void
MakeEntityLowFrequency(game_state *GameState, uint32 LowIndex) 
{
  low_entity *EntityLow = &GameState->LowEntities[LowIndex];
  uint32 HighIndex = EntityLow->HighEntityIndex;
  if (HighIndex)
  {
    uint32 LastHighEntityIndex = GameState->HighEntityCount - 1;
    if (HighIndex != LastHighEntityIndex)
    {
      high_entity *LastEntity = GameState->HighEntities_ + LastHighEntityIndex;
      high_entity *DelEntity = GameState->HighEntities_ + HighIndex;
      *DelEntity = *LastEntity;
      GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
    }
    --GameState->HighEntityCount;
    EntityLow->HighEntityIndex = 0;
  }
}

inline bool
ValidateEntityPairs(game_state *GameState)
{
  bool Valid = true;
  for (uint32 HighEntityIndex = 1;
       HighEntityIndex < GameState->HighEntityCount;
       HighEntityIndex++)
  {
    high_entity *High = GameState->HighEntities_ + HighEntityIndex;
    Valid = Valid && (GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
  }
  return Valid;
}

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 HighFrequencyBounds) 
{
  for (uint32 HighEntityIndex = 1;
    HighEntityIndex < GameState->HighEntityCount;
    )
  {
    high_entity *High = GameState->HighEntities_ + HighEntityIndex;
    High->P += Offset;
    if (IsInRectangle(HighFrequencyBounds, High->P))
    {
      HighEntityIndex++;
    }
    else
    {
      Assert(GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
      MakeEntityLowFrequency(GameState, High->LowEntityIndex);
    }
  }
}

internal uint32
AddLowEntity(game_state *GameState, entity_type Type, world_position *P) 
{
  Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
  uint32 EntityIndex = GameState->LowEntityCount++;

  low_entity *LowEntity = GameState->LowEntities + EntityIndex;
  *LowEntity = {};
  LowEntity->Type = Type;
  if (P) 
  {
    LowEntity->P = *P;
    ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, 0, P);
  }

  return EntityIndex;
}

internal uint32
AddPlayer(game_state *GameState) 
{
  world_position P = GameState->CameraP;
  uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero, &P);
  low_entity *Entity = GetLowEntity(GameState, EntityIndex);
  Entity->Height = 0.5f;
  Entity->Width = 1.0f;
  Entity->Collides = true;

  if (GameState->CamerFollowingEntityIndex == 0) {
    GameState->CamerFollowingEntityIndex = EntityIndex;
  }
  return EntityIndex;
}

internal uint32
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) 
{
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  uint32 EntityIndex = AddLowEntity(GameState, EntityType_Wall, &P);
  low_entity *Entity = GetLowEntity(GameState, EntityIndex);
  Entity->Height = GameState->World->TileSideInMeters;
  Entity->Width = Entity->Height;
  Entity->Collides = true;

  return EntityIndex;
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
MovePlayer(game_state *GameState, entity Entity, real32 dt, v2 ddP)
{
  world *World = GameState->World;
  real32 ddPLength = LengthSq(ddP);
  if (ddPLength > 1.0f)  {
    ddP = (1.0f/SquareRoot(ddPLength)) * ddP;
  }

  real32 PlayerSpeed = 50.0f;
  ddP *= PlayerSpeed;
  ddP += -8.0f * Entity.High->dP;

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
  ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex, &Entity.Low->P, &NewP);
  Entity.Low->P = NewP;
}

internal void
SetCamera(game_state *GameState, world_position NewCameraP) {
  
  world *World = GameState->World;
  Assert(ValidateEntityPairs(GameState));
  world_difference dCameraP = Subtract(World, &NewCameraP, &GameState->CameraP);
  GameState->CameraP = NewCameraP;

  uint32 TileSpanX = 17 * 3;
  uint32 TileSpanY = 9 * 3;
  rectangle2 CameraBounds = RectCenterDim(
      // TileMap->TileSideInMeters * v2{(real32)NewCameraP.AbsTileX, (real32)NewCameraP.AbsTileY},
      v2{0, 0},
      World->TileSideInMeters * v2{(real32)TileSpanX, (real32)TileSpanY});

  v2 EntityOffsetForFrame = -dCameraP.dXY;
  OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

  Assert(ValidateEntityPairs(GameState));
  world_position MinChunkP = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraBounds));
  world_position MaxChunkP = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraBounds));

  for (uint32 ChunkY = MinChunkP.ChunkY;
       ChunkY <= MaxChunkP.ChunkY;
       ChunkY++)
  {
    for (uint32 ChunkX = MinChunkP.ChunkX;
       ChunkX <= MaxChunkP.ChunkX;
       ChunkX++)
    {
      world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
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
            if (Low->HighEntityIndex == 0)
            {
              v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
              if (IsInRectangle(CameraBounds, CameraSpaceP))
              {
                MakeEntityHighFrequency(GameState, Low, LowEntityIndex, CameraSpaceP);
              }
            }
          }
        }
      }
    }
  }

  Assert(ValidateEntityPairs(GameState));
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  int32 TileSideInPixels = 60;
  real32 MetersToPixels = 0; 
  if (Memory->IsInitialized) {
    world *World = GameState->World;
    MetersToPixels = (real32)TileSideInPixels/(real32)World->TileSideInMeters;
  }

  if (!Memory->IsInitialized) {
    // Reserve entity slot 0 for null entity
    AddLowEntity(GameState, EntityType_Null, 0);
    GameState->HighEntityCount = 1;

    GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

    GameState->Tree = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_back.bmp");
    
    hero_bitmaps *Bitmap = &GameState->HeroBitmaps[0];
    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_right.bmp");
    Bitmap->AlignX = 30;
    Bitmap->AlignY = 55;
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_back.bmp");
    Bitmap->AlignX = 30;
    Bitmap->AlignY = 50;
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_left.bmp");
    Bitmap->AlignX = 30;
    Bitmap->AlignY = 50;
    Bitmap++;

    Bitmap->Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/hero_front.bmp");
    Bitmap->AlignX = 30;
    Bitmap->AlignY = 50;

    

    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;

    InitializeWorld(World, 1.4f);

    MetersToPixels = (real32)TileSideInPixels/(real32)World->TileSideInMeters;
    uint32 RandomNumberIndex = 0;

    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    uint32 ScreenBaseX = (INT16_MAX / TilesPerWidth) / 2;
    uint32 ScreenBaseY = (INT16_MAX / TilesPerHeight) / 2;
    uint32 ScreenBaseZ = INT16_MAX / 2;
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
      ScreenIndex < 2000;
      ScreenIndex++) 
    {
      Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
      uint32 RandomChoice;
      if (DoorUp || DoorDown) 
      {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
      } else {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
      }
      bool CreatedZDoor = false;
      if (RandomChoice == 2)
      {
        CreatedZDoor = true;
        if (AbsTileZ == ScreenBaseZ) {
          DoorUp = true;
        } else {
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
          uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

          uint32 TileValue = 1;

          if ((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight / 2)))) {
            TileValue = 2;
          }

          if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight / 2)))) {
            TileValue = 2;
          }

          if ((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth / 2)))) {
            TileValue = 2;
          }

          if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2)))) {
            TileValue = 2;
          }

          if (TileX == 10 && TileY == 6) {
            if (DoorUp) {
              TileValue = 3;
            } 
            if (DoorDown) {
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
        if (AbsTileZ == ScreenBaseZ) {
          AbsTileZ = ScreenBaseZ + 1;
        } else {
          AbsTileZ = ScreenBaseZ;
        }
      } else if (RandomChoice == 1) {
        ScreenX += 1;
      } else {
        ScreenY += 1;
      }
    }
    world_position NewCameraP = {};
    NewCameraP = ChunkPositionFromTilePosition(GameState->World, (ScreenBaseX * TilesPerWidth) + (17 / 2), (ScreenBaseY * TilesPerHeight) + (9 / 2), ScreenBaseZ);
    SetCamera(GameState, NewCameraP);

    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&TileMap-> GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  world *World = GameState->World;
  // Assert(TileMap);
  real32 LowerLeftX = (real32)(-1.0f * TileSideInPixels/2.0f);
  real32 LowerLeftY = (real32)Buffer->Height;

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
    if (LowIndex == 0) {
      if (Controller->Start.EndedDown)
      {
        uint32 EntityIndex = AddPlayer(GameState); //AddEntity(GameState);
        GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
      }
    }
    else 
    {
      entity ControllingEntity = GetHighEntity(GameState, LowIndex);
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
      MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
    }
  } 

  v2 EntityOffsetForFrame = {};
  entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CamerFollowingEntityIndex);
  if (CameraFollowingEntity.High)
  {
    world_position NewCameraP = GameState->CameraP;
    NewCameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;

    if (CameraFollowingEntity.High->P.X > (9.0f * World->TileSideInMeters))
    {
      NewCameraP.ChunkX += 17;
    }

    if (CameraFollowingEntity.High->P.X < (-9.0f * World->TileSideInMeters))
    {
      NewCameraP.ChunkX -= 17;
    }

    if (CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters))
    {
      NewCameraP.ChunkY += 9;
    }

    if (CameraFollowingEntity.High->P.Y < (-5.0f * World->TileSideInMeters))
    {
      NewCameraP.ChunkY -= 9;
    }

    SetCamera(GameState, NewCameraP);
  }

#if 1
  DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else 
  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif

  real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;

  // Draw player
  for (uint32 HighEntityIndex = 0;
       HighEntityIndex < GameState->HighEntityCount;
       ++HighEntityIndex)
  {
    high_entity *HighEntity = GameState->HighEntities_ + HighEntityIndex;
    low_entity *LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;
    real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * HighEntity->P.X;
    real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * HighEntity->P.Y;

    HighEntity->P += EntityOffsetForFrame;

    real32 PlayerLeft = PlayerGroundPointX - 0.5f * LowEntity->Width * MetersToPixels;
    real32 PlayerTop = PlayerGroundPointY - 0.5f * LowEntity->Height * MetersToPixels;

    if (LowEntity->Type == EntityType_Hero)
    {
      DrawRectangle(Buffer,
                    v2{PlayerLeft, PlayerTop},
                    v2{PlayerLeft + LowEntity->Width * MetersToPixels,
                       PlayerTop + LowEntity->Height * MetersToPixels},
                    1.0f, 1.0f, 0.0f);

      hero_bitmaps HeroBitmap = GameState->HeroBitmaps[HighEntity->FacingDirection];
      DrawBitmap(Buffer, &HeroBitmap.Body, PlayerGroundPointX, PlayerGroundPointY, HeroBitmap.AlignX, HeroBitmap.AlignY);
    }
    else
    {
      DrawRectangle(Buffer,
                    v2{PlayerLeft, PlayerTop},
                    v2{PlayerLeft + LowEntity->Width * MetersToPixels,
                       PlayerTop + LowEntity->Height * MetersToPixels},
                    1.0f, 1.0f, 0.0f);
      DrawBitmap(Buffer, &GameState->Tree, PlayerGroundPointX, PlayerGroundPointY, 0, 0);
    }
  }
}



extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
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