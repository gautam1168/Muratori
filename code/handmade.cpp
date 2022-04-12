#include "handmade.hpp"
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

// Fills rectangle from [low, high) i.e. excludes the upperbound
internal void DrawRectangle(game_offscreen_buffer *Buffer, 
  real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, 
  real32 R, real32 G, real32 B) {
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;

  int32 MinX = RoundReal32ToInt32(RealMinX);
  int32 MinY = RoundReal32ToInt32(RealMinY);
  int32 MaxX = RoundReal32ToInt32(RealMaxX);
  int32 MaxY = RoundReal32ToInt32(RealMaxY);

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
};
#pragma pack(pop)

internal uint32 *
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
  uint32 *Result = 0;
  debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
  if (ReadResult.ContentSize != 0) {
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    Result = Pixels;
  }
  return Result;
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
    GameState->PixelPointer = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
    GameState->PlayerP.AbsTileX = 1;
    GameState->PlayerP.AbsTileY = 1;
    GameState->PlayerP.TileRelX = 0.5f;
    GameState->PlayerP.TileRelY = 0.5f;
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

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {

    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;
      if (Controller->MoveUp.EndedDown) {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveDown.EndedDown) {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveLeft.EndedDown) {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown) {
        dPlayerX = 1.0f;
      }
      real32 PlayerSpeed = 5.0f;
      if (Controller->ActionUp.EndedDown) {
        PlayerSpeed = 20.0f;
      }
      dPlayerX *= PlayerSpeed;
      dPlayerY *= PlayerSpeed;

      tile_map_position NewPlayerP = GameState->PlayerP;

      NewPlayerP.TileRelX = GameState->PlayerP.TileRelX +  Input->dtForFrame * dPlayerX;
      NewPlayerP.TileRelY = GameState->PlayerP.TileRelY + Input->dtForFrame * dPlayerY;
      NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

      tile_map_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
      PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

      tile_map_position PlayerRight = NewPlayerP;
      PlayerRight.TileRelX += 0.5f*PlayerWidth;
      PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);
      
      if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
          IsTileMapPointEmpty(TileMap, PlayerLeft) &&
          IsTileMapPointEmpty(TileMap, PlayerRight)
          ) {
        if (!AreOnSameTile(&GameState->PlayerP, &NewPlayerP)) {
          uint32 NewTileValue = GetTileValue(TileMap, NewPlayerP);
          if (NewTileValue == 3)
          {
            ++NewPlayerP.AbsTileZ;
          }
          else if (NewTileValue == 4)
          {
            --NewPlayerP.AbsTileZ;
          }
        }
        GameState->PlayerP = NewPlayerP;
      }
    }
  } 

  // Clear color
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)(LowerLeftX + Buffer->Width), 
    LowerLeftY, 1.0f, 0.0f, 1.0f);

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
      uint32 Column = RelColumn + GameState->PlayerP.AbsTileX;
      uint32 Row = RelRow + GameState->PlayerP.AbsTileY;
      uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);

      real32 Gray = 0.5f;
      if (TileID > 0) {
        if (TileID == 2) {
          Gray = 1.0f;
        } else if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY) {
          Gray = 0.0f;
        }

        if (TileID > 2) {
          Gray = 0.25f;
        }

        real32 CenX = ScreenCenterX - GameState->PlayerP.TileRelX*MetersToPixels + ((real32)RelColumn) * TileSideInPixels;
        real32 CenY = ScreenCenterY + GameState->PlayerP.TileRelY*MetersToPixels - ((real32)RelRow) * TileSideInPixels;
        real32 MinX = CenX - 0.5f * TileSideInPixels;
        real32 MinY = CenY - 0.5f * TileSideInPixels;
        real32 MaxX = CenX + 0.5f * TileSideInPixels;
        real32 MaxY = CenY + 0.5f * TileSideInPixels;
        DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
      }
    }
  }

  // Draw player
  real32 PlayerLeft = ScreenCenterX - 0.5f*PlayerWidth*MetersToPixels;

  real32 PlayerTop = ScreenCenterY - PlayerHeight*MetersToPixels;

  DrawRectangle(Buffer, 
    PlayerLeft, PlayerTop, 
    PlayerLeft + PlayerWidth*MetersToPixels, 
    PlayerTop + PlayerHeight*MetersToPixels, 
    1.0f, 1.0f, 0.0f);

#if 0
  uint32 *Source = GameState->PixelPointer;
  uint32 *Dest = (uint32 *)Buffer->Memory;
  for (int32 Y = 0;
    Y < Buffer->Height;
    Y++)
  {
    for (int32 X = 0;
      X < Buffer->Width;
      X++) 
    {
      *Dest++ = *Source++;
    }
  }
#endif
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