#if !defined(HANDMADE_MATH_HPP)

struct v2
{
  union {
    struct {
      real32 X, Y;
    };
    real32 E[2];
  };

  inline v2 &operator*=(real32 m);
  inline v2 &operator+=(v2 A);
};

inline v2 operator*(real32 m, v2 A) {
  v2 Result;

  Result.X = m*A.X;
  Result.Y = m*A.Y;
  return Result;
}

inline v2 &v2::
operator*=(real32 m) {
  *this = m * *this;
  return *this;
}

inline v2 operator-(v2 A) {
  v2 Result;

  Result.X = -A.X;
  Result.Y = -A.Y;
  return Result;
}

inline v2 operator+(v2 A, v2 B) 
{
  v2 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  return Result;
}

inline v2 &v2::operator+=(v2 A) {
  *this = A + *this;
  return *this;
}

inline v2 operator-(v2 A, v2 B) 
{
  v2 Result;
  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  return Result;
}

inline real32
Square(real32 A) {
  real32 Result = A * A;
  return Result;
}

inline real32
Inner(v2 A, v2 B) {
  real32 Result = A.X * B.X + A.Y * B.Y;
  return Result;
}

inline real32
LengthSq(v2 A) {
  real32 Result;
  Result = Inner(A, A);
  return Result;
}

#define HANDMADE_MATH_HPP
#endif