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

inline v2
V2(real32 x, real32 y)
{
  v2 Result;
  Result.X = x;
  Result.Y = y;
  return Result;
}

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
SquareRoot(real32 A) {
  real32 Result = sqrtf(A);
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

struct rectangle2 {
  v2 Min;
  v2 Max;
};

inline v2
GetMinCorner(rectangle2 Rect)
{
  v2 Result = Rect.Min;
  return Result;
}

inline v2
GetMaxCorner(rectangle2 Rect)
{
  v2 Result = Rect.Max;
  return Result;
}

inline v2
GetCenter(rectangle2 Rect)
{
  v2 Result = 0.5f * (Rect.Max + Rect.Min);
  return Result;
}

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim) {
  rectangle2 Result = {};
  Result.Min = Center - HalfDim;
  Result.Max = Center + HalfDim;
  return Result;
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim) {
  rectangle2 Result = RectCenterHalfDim(Center, 0.5f * Dim);
  return Result;
}

inline bool 
IsInRectangle(rectangle2 Rectangle, v2 Test) {
  bool Result = ((Test.X >= Rectangle.Min.X) &&
                 (Test.Y >= Rectangle.Min.Y) &&
                 (Test.X < Rectangle.Max.X) &&
                 (Test.Y < Rectangle.Max.Y));
  return Result;
}

#define HANDMADE_MATH_HPP
#endif