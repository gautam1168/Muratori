#if !defined(HANDMADE_HPP)

struct game_offscreen_buffer {
  void* Memory;
  int Width;
  int Height;
  int Pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#define HANDMADE_HPP
#endif