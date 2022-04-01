#include "handmade.hpp"

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

inline int32 RoundReal32ToInt32(real32 realvalue) {
  return (int32)(realvalue + 0.5f);
}

inline uint32 RoundReal32ToUInt32(real32 realvalue) {
  return (uint32)(realvalue + 0.5f);
}

inline int32 TruncateReal32ToInt32(real32 realvalue) {
  int32 truncated;
  if (realvalue > 0) {
    truncated = (int32)realvalue;
  } else {
    truncated = (int32)(realvalue - 1);
  }
  return truncated;
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

inline uint32 
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TileX, int32 TileY) {
  Assert(TileMap);
  Assert((TileX >= 0 && TileY >= 0 && TileX < World->CountX && TileY < World->CountY));
  return TileMap->Tiles[TileX + TileY * World->CountX];
}

inline tile_map * 
GetTileMap(world *World, int32 TileMapX, int32 TileMapY) {
  tile_map *TileMap = 0;
  if (TileMapX < World->TileMapCountX && TileMapX >= 0 && TileMapY >= 0 && TileMapY < World->TileMapCountY) {
    TileMap = &World->TileMaps[TileMapX + TileMapY * World->TileMapCountX];
  }
  return TileMap;
}

internal bool 
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY) {
  bool Empty = false;

  if ((TestTileX >= 0) && (TestTileX < World->CountX) &&  
      (TestTileY >= 0) && (TestTileY < World->CountY)) {
    uint32 TilemapValue = TileMap->Tiles[World->CountX * TestTileY + TestTileX];
    Empty = (TilemapValue == 0);
  }
  return Empty;
}

internal canonical_position
GetCanonicalPosition(world * World, raw_position Pos) { 
  canonical_position CanPos = {};

  real32 dX = Pos.X - World->UpperLeftX;
  real32 dY = Pos.Y - World->UpperLeftY;

  CanPos.TileX = TruncateReal32ToInt32((Pos.X - World->UpperLeftX) / World->TileWidth);
  CanPos.TileY = TruncateReal32ToInt32((Pos.Y - World->UpperLeftY) / World->TileHeight);

  CanPos.X = dX - CanPos.TileX*World->TileWidth;
  CanPos.Y = dY - CanPos.TileY*World->TileHeight;

  CanPos.TileMapX = Pos.TileMapX;
  CanPos.TileMapY = Pos.TileMapY;

  Assert(CanPos.X >= 0);
  Assert(CanPos.Y >= 0);
  Assert(CanPos.X < World->TileWidth);
  Assert(CanPos.Y < World->TileHeight);

  if (CanPos.TileX < 0) {
    CanPos.TileX += World->CountX;
    CanPos.TileMapX = Pos.TileMapX - 1;
  }

  if (CanPos.TileY < 0) {
    CanPos.TileY += World->CountY;
    CanPos.TileMapY = Pos.TileMapY - 1;
  }

  if (CanPos.TileX >= World->CountX) {
    CanPos.TileX -= World->CountX;
    CanPos.TileMapX = Pos.TileMapX + 1;
  }

  if (CanPos.TileY >= World->CountY) {
    CanPos.TileY -= World->CountY;
    CanPos.TileMapY = Pos.TileMapY + 1;
  }
  return CanPos;
}

internal bool 
IsWorldPointEmpty(world *World, raw_position Pos) {
  bool Empty = false;

  
  canonical_position CanPos = GetCanonicalPosition(World, Pos); 

  tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
  Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
  return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
#define TILEMAP_MAX_X 16
#define TILEMAP_MAX_Y 9

  uint32 Tiles00[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 0 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  uint32 Tiles01[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 }
  };


  uint32 Tiles10[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 0, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   1, 0, 0, 1 },
    { 1, 1, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  uint32 Tiles11[TILEMAP_MAX_Y][TILEMAP_MAX_X] = {
    { 1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1 },
    { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 }
  };

  tile_map TileMaps[2][2] = {};
  TileMaps[0][0].Tiles = (uint32 *)Tiles00;
  TileMaps[1][0].Tiles = (uint32 *)Tiles01;
  TileMaps[0][1].Tiles = (uint32 *)Tiles10;
  TileMaps[1][1].Tiles = (uint32 *)Tiles11;

  world World;
  World.TileMapCountX = 2;
  World.TileMapCountY = 2; 
  World.TileMaps = (tile_map *)TileMaps;
  World.CountX = TILEMAP_MAX_X;
  World.CountY = TILEMAP_MAX_Y;
  World.UpperLeftX = -30;
  World.UpperLeftY = 0;
  World.TileWidth = 60;
  World.TileHeight = 60;

  real32 PlayerWidth = 0.75f * World.TileWidth;
  real32 PlayerHeight = World.TileHeight;

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    GameState->PlayerX = 150;
    GameState->PlayerY = 150;
    Memory->IsInitialized = true;
  }

  tile_map *TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY); 
  Assert(TileMap);

  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog) {

    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;
      if (Controller->MoveUp.EndedDown) {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveDown.EndedDown) {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveLeft.EndedDown) {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown) {
        dPlayerX = 1.0f;
      }
      dPlayerX *= 128.0f;
      dPlayerY *= 128.0f;
      real32 NewPlayerX = GameState->PlayerX +  Input->dtForFrame * dPlayerX;
      real32 NewPlayerY = GameState->PlayerY + Input->dtForFrame * dPlayerY;

      raw_position PlayerPos = {
        GameState->PlayerTileMapX,
        GameState->PlayerTileMapY,
        NewPlayerX,
        NewPlayerY
      };

      raw_position PlayerLeft = {
        GameState->PlayerTileMapX,
        GameState->PlayerTileMapY,
        NewPlayerX - 0.5f * PlayerWidth,
        NewPlayerY
      };

      raw_position PlayerRight = {
        GameState->PlayerTileMapX,
        GameState->PlayerTileMapY,
        NewPlayerX + 0.5f * PlayerWidth,
        NewPlayerY
      };
      
      if (IsWorldPointEmpty(&World, PlayerPos) &&
          IsWorldPointEmpty(&World, PlayerLeft) &&
          IsWorldPointEmpty(&World, PlayerRight)
          ) {
        canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);
        GameState->PlayerTileMapX = CanPos.TileMapX;
        GameState->PlayerTileMapY = CanPos.TileMapY;

        GameState->PlayerX = World.UpperLeftX + CanPos.TileX*World.TileWidth + CanPos.X;
        GameState->PlayerY = World.UpperLeftY + CanPos.TileY*World.TileHeight + CanPos.Y;
      }
    }
  } 

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)(Buffer->Width + World.UpperLeftX), 
    (real32)(Buffer->Height + World.UpperLeftY), 1.0f, 0.0f, 1.0f);
  for (int Row = 0; Row < 9; Row++) {
    for (int Column = 0; Column < 16; Column++) {
      uint32 TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
      real32 Gray = 0.5f;
      if (TileID == 1) {
        Gray = 1.0f;
      }
      real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileWidth;
      real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileHeight;
      real32 MaxX = MinX + World.TileWidth;
      real32 MaxY = MinY + World.TileHeight;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerMinX = GameState->PlayerX - (0.5f * PlayerWidth);
  real32 PlayerMinY = GameState->PlayerY - World.TileHeight;
  real32 PlayerMaxX = PlayerMinX + PlayerWidth;
  real32 PlayerMaxY = PlayerMinY + PlayerHeight;
  DrawRectangle(Buffer, PlayerMinX, PlayerMinY, PlayerMaxX, PlayerMaxY, 1.0f, 1.0f, 0.0f);
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