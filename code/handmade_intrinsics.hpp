#if !defined(HANDMADE_INTINSICS_HPP)
#include "math.h"
inline int32 RoundReal32ToInt32(real32 realvalue) {
  return (int32)roundf(realvalue);
}

inline uint32 RoundReal32ToUInt32(real32 realvalue) {
  return (uint32)roundf(realvalue);
}

inline int32 TruncateReal32ToInt32(real32 realvalue) {
  return (int32)realvalue;
}

inline int32 FloorReal32ToInt32(real32 realvalue) {
  return (int32)floorf(realvalue);
}

inline real32 
Sin(real32 Angle) {
  return sinf(Angle);
}

inline real32 
Cos(real32 Angle) {
  return cosf(Angle);
}

inline bool
FindLowestSetBit(uint32 *Index, uint32 Mask) {
  bool Found = false;
#if COMPILER_MSVC
  Found = _BitScanForward(Index, Mask);
#else
  for (uint32 Test = 0;
    Test < 32;
    Test++)
  {
    if (Mask & (1 << Test)) {
      Found = true;
      *Index = Test;
      break;
    }
  }
#endif
  return Found;
}


// inline real32 
// ATan2(real32 Y, real32 X) {
//   return atan2(Y, X);
// }

#define HANDMADE_INTRINSICS_HPP
#endif