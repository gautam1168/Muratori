#if !defined(HANDMADE_HPP)

struct game_offscreen_buffer {
  void* Memory;
  int Width;
  int Height;
  int Pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

struct game_sound_buffer {
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

#define HANDMADE_HPP
#endif