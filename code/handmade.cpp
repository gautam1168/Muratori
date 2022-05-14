#include "handmade.hpp"
#include "handmade_math.hpp"
#include "handmade_tile.cpp"
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

inline high_entity * 
MakeEntityHighFrequency(game_state *GameState, uint32 LowIndex)
{
  high_entity *HighEntity = 0;
  low_entity *LowEntity = &GameState->LowEntities[LowIndex];
  if (LowEntity->HighEntityIndex)
  {
    HighEntity = GameState->HighEntities_ + LowEntity->HighEntityIndex;
  }
  else 
  {
    if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities_))
    {
      uint32 HighIndex = GameState->HighEntityCount++;
      HighEntity = &GameState->HighEntities_[HighIndex];
      tile_map_difference Diff = Subtract(GameState->World->TileMap, &LowEntity->P, &GameState->CameraP);
      HighEntity->P = Diff.dXY;
      HighEntity->dP = v2{0, 0};
      HighEntity->AbsTileZ = LowEntity->P.AbsTileZ;
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

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 CameraBounds) 
{
  for (uint32 EntityIndex = 1;
    EntityIndex < GameState->HighEntityCount;
    )
  {
    high_entity *High = GameState->HighEntities_ + EntityIndex;
    High->P += Offset;
    if (IsInRectangle(CameraBounds, High->P))
    {
      EntityIndex++;
    }
    else
    {
      MakeEntityLowFrequency(GameState, High->LowEntityIndex);
    }
  }
}

internal uint32
AddLowEntity(game_state *GameState, entity_type Type) 
{
  Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
  uint32 EntityIndex = GameState->LowEntityCount++;

  GameState->LowEntities[EntityIndex] = {};
  GameState->LowEntities[EntityIndex].Type = Type;
  return EntityIndex;
}

internal uint32
AddPlayer(game_state *GameState) 
{
  uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero);
  low_entity *Entity = GetLowEntity(GameState, EntityIndex);
  Entity->P.AbsTileX = 8;
  Entity->P.AbsTileY = 5;
  Entity->P.Offset_.X = 0;
  Entity->P.Offset_.Y = 0;
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
  uint32 EntityIndex = AddLowEntity(GameState, EntityType_Wall);
  low_entity *Entity = GetLowEntity(GameState, EntityIndex);
  Entity->P.AbsTileX = AbsTileX;
  Entity->P.AbsTileY = AbsTileY;
  Entity->P.AbsTileZ = AbsTileZ;
  Entity->Height = GameState->World->TileMap->TileSideInMeters;
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
  tile_map *TileMap = GameState->World->TileMap;
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
      Entity.High->AbsTileZ += HitLow->dAbsTileZ;
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

  Entity.Low->P = MapIntoTileSpace(GameState->World->TileMap, GameState->CameraP, Entity.High->P);
}

internal void
SetCamera(game_state *GameState, tile_map_position NewCameraP) {
  
  tile_map *TileMap = GameState->World->TileMap;
  tile_map_difference dCameraP = Subtract(TileMap, &NewCameraP, &GameState->CameraP);
  GameState->CameraP = NewCameraP;

  uint32 TileSpanX = 17 * 3;
  uint32 TileSpanY = 9 * 3;
  rectangle2 CameraBounds = RectCenterDim(
      // TileMap->TileSideInMeters * v2{(real32)NewCameraP.AbsTileX, (real32)NewCameraP.AbsTileY},
      v2{0, 0},
      TileMap->TileSideInMeters * v2{(real32)TileSpanX, (real32)TileSpanY});

  v2 EntityOffsetForFrame = -dCameraP.dXY;
  OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

  uint32 MinTileX = NewCameraP.AbsTileX - TileSpanX / 2;
  uint32 MinTileY = NewCameraP.AbsTileY - TileSpanY / 2;
  uint32 MaxTileX = NewCameraP.AbsTileX + TileSpanX / 2;
  uint32 MaxTileY = NewCameraP.AbsTileY + TileSpanY / 2;
  for (uint32 EntityIndex = 1; 
    EntityIndex < GameState->LowEntityCount;
    ++EntityIndex)
  {

    low_entity *Low = GameState->LowEntities + EntityIndex;
    if (Low->HighEntityIndex == 0)
    {
      if (Low->P.AbsTileZ == NewCameraP.AbsTileZ &&
          Low->P.AbsTileX >= MinTileX &&
          Low->P.AbsTileX <= MaxTileX &&
          Low->P.AbsTileY >= MinTileY &&
          Low->P.AbsTileY <= MaxTileY)
      {
        MakeEntityHighFrequency(GameState, EntityIndex);
      }
    }
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  int32 TileSideInPixels = 60;
  real32 MetersToPixels = 0; 
  if (Memory->IsInitialized) {
    tile_map *TileMap = GameState->World->TileMap;
    MetersToPixels = (real32)TileSideInPixels/(real32)TileMap->TileSideInMeters;
  }

  if (!Memory->IsInitialized) {
    // Reserve entity slot 0 for null entity
    AddLowEntity(GameState, EntityType_Null);
    GameState->HighEntityCount = 1;

    GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
    
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
    World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

    tile_map *TileMap = World->TileMap;

    TileMap->ChunkShift = 4;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);

    TileMap->TileChunkCountX = 128;
    TileMap->TileChunkCountY = 128; 
    TileMap->TileChunkCountZ = 2;

    TileMap->TileChunks = PushArray(
      &GameState->WorldArena, 
      TileMap->TileChunkCountX*TileMap->TileChunkCountY*TileMap->TileChunkCountZ, 
      tile_chunk
    );

    TileMap->TileSideInMeters = 1.4f;
    MetersToPixels = (real32)TileSideInPixels/(real32)TileMap->TileSideInMeters;
    uint32 RandomNumberIndex = 0;

    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    uint32 ScreenX = 0;
    uint32 ScreenY = 0;
    uint32 AbsTileZ = 0;

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
      } else {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
      }
      bool CreatedZDoor = false;
      if (RandomChoice == 2)
      {
        CreatedZDoor = true;
        if (AbsTileZ == 0) {
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

          SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ,
            TileValue
          );

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
        if (AbsTileZ == 0) {
          AbsTileZ = 1;
        } else {
          AbsTileZ = 0;
        }
      } else if (RandomChoice == 1) {
        ScreenX += 1;
      } else {
        ScreenY += 1;
      }
    }
    tile_map_position NewCameraP = {};
    NewCameraP.AbsTileX = 17/2;
    NewCameraP.AbsTileY = 9/2;
    NewCameraP.AbsTileZ = 0;
    SetCamera(GameState, NewCameraP);

    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&TileMap-> GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  tile_map *TileMap = GameState->World->TileMap;
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
    tile_map_position NewCameraP = GameState->CameraP;
    NewCameraP.AbsTileZ = CameraFollowingEntity.Low->P.AbsTileZ;

    if (CameraFollowingEntity.High->P.X > (9.0f * TileMap->TileSideInMeters))
    {
      NewCameraP.AbsTileX += 17;
    }

    if (CameraFollowingEntity.High->P.X < (-9.0f * TileMap->TileSideInMeters))
    {
      NewCameraP.AbsTileX -= 17;
    }

    if (CameraFollowingEntity.High->P.Y > (5.0f * TileMap->TileSideInMeters))
    {
      NewCameraP.AbsTileY += 9;
    }

    if (CameraFollowingEntity.High->P.Y < (-5.0f * TileMap->TileSideInMeters))
    {
      NewCameraP.AbsTileY -= 9;
    }

    SetCamera(GameState, NewCameraP);
  }

  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

  real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;

  // Draw tiles
  for (int RelRow = -10; 
    RelRow < 10; 
    RelRow++) 
  {
    for (int32 RelColumn = -20;
      RelColumn < 20;
      RelColumn++) 
    {
      uint32 Column = RelColumn + GameState->CameraP.AbsTileX;
      uint32 Row = RelRow + GameState->CameraP.AbsTileY;
      uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);

      real32 Gray = 0.5f;
      if (TileID > 1) {
        if (TileID == 2) {
          Gray = 1.0f;
        } else if (Column == GameState->CameraP.AbsTileX && Row == GameState->CameraP.AbsTileY) {
          Gray = 0.0f;
        }

        if (TileID > 2) {
          Gray = 0.25f;
        }

        v2 TileSide = { 0.5f * TileSideInPixels, 0.5f * TileSideInPixels };
        v2 Cen = {ScreenCenterX - GameState->CameraP.Offset_.X*MetersToPixels + ((real32)RelColumn) * TileSideInPixels,
        ScreenCenterY + GameState->CameraP.Offset_.Y*MetersToPixels - ((real32)RelRow) * TileSideInPixels};
        v2 Min = Cen - 0.9f * TileSide;
        v2 Max = Cen + 0.9f * TileSide;
        DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
      }
    }
  }

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