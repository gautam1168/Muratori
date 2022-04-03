#if !defined(HANDMADE_INTINSICS_HPP)
#include "math.h"
inline int32 RoundReal32ToInt32(real32 realvalue) {
  return (int32)(realvalue + 0.5f);
}

inline uint32 RoundReal32ToUInt32(real32 realvalue) {
  return (uint32)(realvalue + 0.5f);
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

// inline real32 
// ATan2(real32 Y, real32 X) {
//   return atan2(Y, X);
// }

#define HANDMADE_INTRINSICS_HPP
#endif