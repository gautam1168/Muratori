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
  int32 MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
  int32 MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  int32 TileSideInPixels = 60;
  real32 MetersToPixels = 0; 
  if (Memory->IsInitialized) {
    tile_map *TileMap = GameState->World->TileMap;
    MetersToPixels = (real32)TileSideInPixels/(real32)TileMap->TileSideInMeters;
  }

  if (!Memory->IsInitialized) {
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

    GameState->CameraP.AbsTileX = 17/2;
    GameState->CameraP.AbsTileY = 9/2;
    GameState->CameraP.AbsTileZ = 0;

    GameState->PlayerP.AbsTileX = 1;
    GameState->PlayerP.AbsTileY = 1;
    GameState->PlayerP.Offset.X = 0.5f;
    GameState->PlayerP.Offset.Y = 0.5f;
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
      ScreenIndex < 100;
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

    Memory->IsInitialized = true;
  }

  // tile_map *TileMap = GetTileMap(&TileMap-> GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY); 
  tile_map *TileMap = GameState->World->TileMap;
  // Assert(TileMap);
  real32 LowerLeftX = (real32)(-1.0f * TileSideInPixels/2.0f);
  real32 LowerLeftY = (real32)Buffer->Height;

  tile_map_position OldPlayerP = GameState->PlayerP;
  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {

    } else {
      v2 ddPlayer = {};
      if (Controller->MoveUp.EndedDown) {
        GameState->HeroFacingDirection = 1;
        ddPlayer.Y = 1.0f;
      }
      if (Controller->MoveDown.EndedDown) {
        GameState->HeroFacingDirection = 3;
        ddPlayer.Y = -1.0f;
      }
      if (Controller->MoveLeft.EndedDown) {
        GameState->HeroFacingDirection = 2;
        ddPlayer.X = -1.0f;
      }
      if (Controller->MoveRight.EndedDown) {
        GameState->HeroFacingDirection = 0;
        ddPlayer.X = 1.0f;
      }

      real32 PlayerSpeed = 120.0f;
      if (Controller->ActionUp.EndedDown) {
        PlayerSpeed = 320.0f;
      }

      if ((ddPlayer.X != 0.0f) && (ddPlayer.Y != 0.0f)) {
        ddPlayer *= 0.707106f;
      }

      ddPlayer *= PlayerSpeed;
      ddPlayer += -10.0f*GameState->dPlayerP;

      tile_map_position NewPlayerP = GameState->PlayerP;

      v2 PlayerDelta = Input->dtForFrame * GameState->dPlayerP  
        + 0.5f * Square(Input->dtForFrame) * ddPlayer;
      NewPlayerP.Offset = PlayerDelta + GameState->PlayerP.Offset;

      GameState->dPlayerP = GameState->dPlayerP + Input->dtForFrame * ddPlayer;

      NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

#if 1
      tile_map_position PlayerLeft = NewPlayerP;
      PlayerLeft.Offset.X -= 0.5f*PlayerWidth;
      PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

      tile_map_position PlayerRight = NewPlayerP;
      PlayerRight.Offset.X += 0.5f*PlayerWidth;
      PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);

      bool Collided = false;
      tile_map_position ColP = {};
      if (!IsTileMapPointEmpty(TileMap, NewPlayerP)) {
        ColP = NewPlayerP;
        Collided = true;
      }
      if (!IsTileMapPointEmpty(TileMap, PlayerLeft)) {
        ColP = PlayerLeft;
        Collided = true;
      }
      if (!IsTileMapPointEmpty(TileMap, PlayerRight)) {
        ColP = PlayerRight;
        Collided = true;
      }
      if (Collided) 
      {
        v2 R = {};
        // Left wall
        if (ColP.AbsTileX < GameState->PlayerP.AbsTileX) {
          R = v2{1, 0};
        }
        // Right wall
        if (ColP.AbsTileX > GameState->PlayerP.AbsTileX) {
          R = v2{-1, 0};
        }
        // Top wall
        if (ColP.AbsTileY > GameState->PlayerP.AbsTileY) {
          R = v2{0, 1};
        }
        // Bottom wall
        if (ColP.AbsTileY < GameState->PlayerP.AbsTileY) {
          R = v2{0, -1};
        }
        GameState->dPlayerP = GameState->dPlayerP - 1 * Inner(GameState->dPlayerP, R) * R;
      }
      else 
      {
        GameState->PlayerP = NewPlayerP;
      }
#else
      uint32 MinTileX = 0;
      uint32 MinTileY = 0;
      uint32 OnePastMaxTileX = 0;
      uint32 OnePastMaxTileY = 0;
      uint32 AbsTileZ = GameState->PlayerP.AbsTileZ;
      tile_map_position BestPlayerP = GameState->PlayerP;
      real32 BestDistanceSq = LengthSq(PlayerDelta);
      for (uint32 AbsTileY = MinTileY;
        AbsTileY != OnePastMaxTileY;
        AbsTileY++)
      {
        for (uint32 AbsTileX = MinTileX;
          AbsTileX != OnePastMaxTileX;
          AbsTileX++)
        {
          tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
          uint32 TileValue = GetTileValue(TileMap, TestTileP);
          if (IsTileValueEmpty(TileValue)) {
            v2 MinCorner = -0.5f * v2{TileMap.TileSideInMeters, TTileMap.TileSideInMeters};
            v2 MaxCorner =  0.5f * v2{TileMap.TileSideInMeters, TTileMap.TileSideInMeters};

            tile_map_difference RelNewPlayerP = Subtract(TileMap, &TestTileP, &NewPlayerP);
            v2 TestP = ClosestPointInRectangle(MinCorner, MaxCorner, RelNewPlayerP);

          }
        }
      }
#endif
    }
  } 

  if (!AreOnSameTile(&OldPlayerP, &GameState->PlayerP)) {
    uint32 NewTileValue = GetTileValue(TileMap, GameState->PlayerP);
    if (NewTileValue == 3)
    {
      ++GameState->PlayerP.AbsTileZ;
    }
    else if (NewTileValue == 4)
    {
      --GameState->PlayerP.AbsTileZ;
    }
  }
    
  GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;

  tile_map_difference Difference = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);
  if (Difference.dXY.X > (9.0f * TileMap->TileSideInMeters)) {
    GameState->CameraP.AbsTileX += 17;
  }

  if (Difference.dXY.X < (-9.0f * TileMap->TileSideInMeters)) {
    GameState->CameraP.AbsTileX -= 17;
  }

  if (Difference.dXY.Y > (5.0f * TileMap->TileSideInMeters)) {
    GameState->CameraP.AbsTileY += 9;
  }

  if (Difference.dXY.Y < (-5.0f * TileMap->TileSideInMeters)) {
    GameState->CameraP.AbsTileY -= 9;
  }
  Difference = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);

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
        v2 Cen = {ScreenCenterX - GameState->CameraP.Offset.X*MetersToPixels + ((real32)RelColumn) * TileSideInPixels,
        ScreenCenterY + GameState->CameraP.Offset.Y*MetersToPixels - ((real32)RelRow) * TileSideInPixels};
        v2 Min = Cen - TileSide;
        v2 Max = Cen + TileSide;
        DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
      }
    }
  }

  // Draw player

  real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*Difference.dXY.X;
  real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*Difference.dXY.Y;

  real32 PlayerLeft = PlayerGroundPointX - 0.5f*PlayerWidth*MetersToPixels;
  real32 PlayerTop = PlayerGroundPointY - PlayerHeight*MetersToPixels;

  // DrawRectangle(Buffer, 
  //   PlayerLeft, PlayerTop, 
  //   PlayerLeft + PlayerWidth*MetersToPixels, 
  //   PlayerTop + PlayerHeight*MetersToPixels, 
  //   1.0f, 1.0f, 0.0f);

  hero_bitmaps HeroBitmap = GameState->HeroBitmaps[GameState->HeroFacingDirection];
  DrawBitmap(Buffer, &HeroBitmap.Body, PlayerGroundPointX, PlayerGroundPointY, HeroBitmap.AlignX, HeroBitmap.AlignY);
  
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