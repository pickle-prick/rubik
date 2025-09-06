// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Scalar Ops

internal F32 mix_1f32(F32 a, F32 b, F32 t) {
  F32 c = (a + (b - a) * Clamp(0.f, t, 1.f));
  return c;
}

internal F64 mix_1f64(F64 a, F64 b, F64 t) {
  F64 c = (a + (b - a) * Clamp(0.0, t, 1.0));
  return c;
}

////////////////////////////////
//~ rjf: Vector Ops

internal Vec2F32 vec_2f32(F32 x, F32 y) {
  Vec2F32 v = {x, y};
  return v;
}
internal Vec2F32 add_2f32(Vec2F32 a, Vec2F32 b) {
  Vec2F32 c = {a.x + b.x, a.y + b.y};
  return c;
}
internal Vec2F32 sub_2f32(Vec2F32 a, Vec2F32 b) {
  Vec2F32 c = {a.x - b.x, a.y - b.y};
  return c;
}
internal Vec2F32 mul_2f32(Vec2F32 a, Vec2F32 b) {
  Vec2F32 c = {a.x * b.x, a.y * b.y};
  return c;
}
internal Vec2F32 div_2f32(Vec2F32 a, Vec2F32 b) {
  Vec2F32 c = {a.x / b.x, a.y / b.y};
  return c;
}
internal Vec2F32 scale_2f32(Vec2F32 v, F32 s) {
  Vec2F32 c = {v.x * s, v.y * s};
  return c;
}
internal Vec2F32 negate_2f32(Vec2F32 v) {
  Vec2F32 c = {-v.x, -v.y};
  return c;
}
internal F32 dot_2f32(Vec2F32 a, Vec2F32 b) {
  F32 c = a.x * b.x + a.y * b.y;
  return c;
}
internal F32 length_squared_2f32(Vec2F32 v) {
  F32 c = v.x * v.x + v.y * v.y;
  return c;
}
internal F32 length_2f32(Vec2F32 v) {
  F32 c = sqrt_f32(v.x * v.x + v.y * v.y);
  return c;
}
internal Vec2F32 normalize_2f32(Vec2F32 v) {
  v = scale_2f32(v, 1.f / length_2f32(v));
  return v;
}
internal Vec2F32 mix_2f32(Vec2F32 a, Vec2F32 b, F32 t) {
  Vec2F32 c = {mix_1f32(a.x, b.x, t), mix_1f32(a.y, b.y, t)};
  return c;
}

internal Vec2S64 vec_2s64(S64 x, S64 y) {
  Vec2S64 v = {x, y};
  return v;
}
internal Vec2S64 add_2s64(Vec2S64 a, Vec2S64 b) {
  Vec2S64 c = {a.x + b.x, a.y + b.y};
  return c;
}
internal Vec2S64 sub_2s64(Vec2S64 a, Vec2S64 b) {
  Vec2S64 c = {a.x - b.x, a.y - b.y};
  return c;
}
internal Vec2S64 mul_2s64(Vec2S64 a, Vec2S64 b) {
  Vec2S64 c = {a.x * b.x, a.y * b.y};
  return c;
}
internal Vec2S64 div_2s64(Vec2S64 a, Vec2S64 b) {
  Vec2S64 c = {a.x / b.x, a.y / b.y};
  return c;
}
internal Vec2S64 scale_2s64(Vec2S64 v, S64 s) {
  Vec2S64 c = {v.x * s, v.y * s};
  return c;
}
internal S64 dot_2s64(Vec2S64 a, Vec2S64 b) {
  S64 c = a.x * b.x + a.y * b.y;
  return c;
}
internal S64 length_squared_2s64(Vec2S64 v) {
  S64 c = v.x * v.x + v.y * v.y;
  return c;
}
internal S64 length_2s64(Vec2S64 v) {
  S64 c = (S64)sqrt_f64((F64)(v.x * v.x + v.y * v.y));
  return c;
}
internal Vec2S64 normalize_2s64(Vec2S64 v) {
  v = scale_2s64(v, (S64)(1.f / length_2s64(v)));
  return v;
}
internal Vec2S64 mix_2s64(Vec2S64 a, Vec2S64 b, F32 t) {
  Vec2S64 c = {(S64)mix_1f32((F32)a.x, (F32)b.x, t), (S64)mix_1f32((F32)a.y, (F32)b.y, t)};
  return c;
}

internal Vec2S32 vec_2s32(S32 x, S32 y) {
  Vec2S32 v = {x, y};
  return v;
}
internal Vec2S32 add_2s32(Vec2S32 a, Vec2S32 b) {
  Vec2S32 c = {a.x + b.x, a.y + b.y};
  return c;
}
internal Vec2S32 sub_2s32(Vec2S32 a, Vec2S32 b) {
  Vec2S32 c = {a.x - b.x, a.y - b.y};
  return c;
}
internal Vec2S32 mul_2s32(Vec2S32 a, Vec2S32 b) {
  Vec2S32 c = {a.x * b.x, a.y * b.y};
  return c;
}
internal Vec2S32 div_2s32(Vec2S32 a, Vec2S32 b) {
  Vec2S32 c = {a.x / b.x, a.y / b.y};
  return c;
}
internal Vec2S32 scale_2s32(Vec2S32 v, S32 s) {
  Vec2S32 c = {v.x * s, v.y * s};
  return c;
}
internal S32 dot_2s32(Vec2S32 a, Vec2S32 b) {
  S32 c = a.x * b.x + a.y * b.y;
  return c;
}
internal S32 length_squared_2s32(Vec2S32 v) {
  S32 c = v.x * v.x + v.y * v.y;
  return c;
}
internal S32 length_2s32(Vec2S32 v) {
  S32 c = (S32)sqrt_f32((F32)v.x * (F32)v.x + (F32)v.y * (F32)v.y);
  return c;
}
internal Vec2S32 normalize_2s32(Vec2S32 v) {
  v = scale_2s32(v, (S32)(1.f / length_2s32(v)));
  return v;
}
internal Vec2S32 mix_2s32(Vec2S32 a, Vec2S32 b, F32 t) {
  Vec2S32 c = {(S32)mix_1f32((F32)a.x, (F32)b.x, t),
    (S32)mix_1f32((F32)a.y, (F32)b.y, t)};
  return c;
}

internal Vec2S16 vec_2s16(S16 x, S16 y) {
  Vec2S16 v = {x, y};
  return v;
}
internal Vec2S16 add_2s16(Vec2S16 a, Vec2S16 b) {
  Vec2S16 c = {(S16)(a.x + b.x), (S16)(a.y + b.y)};
  return c;
}
internal Vec2S16 sub_2s16(Vec2S16 a, Vec2S16 b) {
  Vec2S16 c = {(S16)(a.x - b.x), (S16)(a.y - b.y)};
  return c;
}
internal Vec2S16 mul_2s16(Vec2S16 a, Vec2S16 b) {
  Vec2S16 c = {(S16)(a.x * b.x), (S16)(a.y * b.y)};
  return c;
}
internal Vec2S16 div_2s16(Vec2S16 a, Vec2S16 b) {
  Vec2S16 c = {(S16)(a.x / b.x), (S16)(a.y / b.y)};
  return c;
}
internal Vec2S16 scale_2s16(Vec2S16 v, S16 s) {
  Vec2S16 c = {(S16)(v.x * s), (S16)(v.y * s)};
  return c;
}
internal S16 dot_2s16(Vec2S16 a, Vec2S16 b) {
  S16 c = a.x * b.x + a.y * b.y;
  return c;
}
internal S16 length_squared_2s16(Vec2S16 v) {
  S16 c = v.x * v.x + v.y * v.y;
  return c;
}
internal S16 length_2s16(Vec2S16 v) {
  S16 c = (S16)sqrt_f32((F32)(v.x * v.x + v.y * v.y));
  return c;
}
internal Vec2S16 normalize_2s16(Vec2S16 v) {
  v = scale_2s16(v, (S16)(1.f / length_2s16(v)));
  return v;
}
internal Vec2S16 mix_2s16(Vec2S16 a, Vec2S16 b, F32 t) {
  Vec2S16 c = {(S16)mix_1f32((F32)a.x, (F32)b.x, t),
    (S16)mix_1f32((F32)a.y, (F32)b.y, t)};
  return c;
}

internal Vec3F32 vec_3f32(F32 x, F32 y, F32 z) {
  Vec3F32 v = {x, y, z};
  return v;
}
internal Vec3F32 add_3f32(Vec3F32 a, Vec3F32 b) {
  Vec3F32 c = {a.x + b.x, a.y + b.y, a.z + b.z};
  return c;
}
internal Vec3F32 sub_3f32(Vec3F32 a, Vec3F32 b) {
  Vec3F32 c = {a.x - b.x, a.y - b.y, a.z - b.z};
  return c;
}
internal Vec3F32 mul_3f32(Vec3F32 a, Vec3F32 b) {
  Vec3F32 c = {a.x * b.x, a.y * b.y, a.z * b.z};
  return c;
}
internal Vec3F32 div_3f32(Vec3F32 a, Vec3F32 b) {
  Vec3F32 c = {a.x / b.x, a.y / b.y, a.z / b.z};
  return c;
}
internal Vec3F32 scale_3f32(Vec3F32 v, F32 s) {
  Vec3F32 c = {v.x * s, v.y * s, v.z * s};
  return c;
}
internal Vec3F32 negate_3f32(Vec3F32 v) {
  Vec3F32 c = {-v.x, -v.y, -v.z};
  return c;
}
internal F32 dot_3f32(Vec3F32 a, Vec3F32 b) {
  F32 c = a.x * b.x + a.y * b.y + a.z * b.z;
  return c;
}
internal F32 length_squared_3f32(Vec3F32 v) {
  F32 c = v.x * v.x + v.y * v.y + v.z * v.z;
  return c;
}
internal F32 length_3f32(Vec3F32 v) {
  F32 c = sqrt_f32(v.x * v.x + v.y * v.y + v.z * v.z);
  return c;
}
internal Vec3F32 normalize_3f32(Vec3F32 v) {
  v = scale_3f32(v, 1.f / length_3f32(v));
  return v;
}
internal Vec3F32 mix_3f32(Vec3F32 a, Vec3F32 b, F32 t) {
  Vec3F32 c = {mix_1f32(a.x, b.x, t), mix_1f32(a.y, b.y, t),
    mix_1f32(a.z, b.z, t)};
  return c;
}
internal Vec3F32 slerp_3f32(Vec3F32 a, Vec3F32 b, F32 t)
{
  F32 dot = dot_3f32(a, b);
  // Clamp dot to prevent numerical instability
  dot = Clamp(-1.f, dot, 1.f);

  // if vectors are almost the same, linear interpolation is enough
  if(dot > 0.9995f)
  {
    return mix_3f32(a,b,t);
  }

  F32 theta = acosf(dot);
  F32 sin_theta = sinf(theta);
  F32 coeff0 = sinf((1.f-t) * theta) / sin_theta;
  F32 coeff1 = sinf(t*theta) / sin_theta;
  return add_3f32(scale_3f32(a, coeff0), scale_3f32(b, coeff1));
}
internal Vec3F32 cross_3f32(Vec3F32 a, Vec3F32 b) {
  Vec3F32 c = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x};
  return c;
}

internal Vec3S32 vec_3s32(S32 x, S32 y, S32 z) {
  Vec3S32 v = {x, y, z};
  return v;
}
internal Vec3S32 add_3s32(Vec3S32 a, Vec3S32 b) {
  Vec3S32 c = {a.x + b.x, a.y + b.y, a.z + b.z};
  return c;
}
internal Vec3S32 sub_3s32(Vec3S32 a, Vec3S32 b) {
  Vec3S32 c = {a.x - b.x, a.y - b.y, a.z - b.z};
  return c;
}
internal Vec3S32 mul_3s32(Vec3S32 a, Vec3S32 b) {
  Vec3S32 c = {a.x * b.x, a.y * b.y, a.z * b.z};
  return c;
}
internal Vec3S32 div_3s32(Vec3S32 a, Vec3S32 b) {
  Vec3S32 c = {a.x / b.x, a.y / b.y, a.z / b.z};
  return c;
}
internal Vec3S32 scale_3s32(Vec3S32 v, S32 s) {
  Vec3S32 c = {v.x * s, v.y * s, v.z * s};
  return c;
}
internal S32 dot_3s32(Vec3S32 a, Vec3S32 b) {
  S32 c = a.x * b.x + a.y * b.y + a.z * b.z;
  return c;
}
internal S32 length_squared_3s32(Vec3S32 v) {
  S32 c = v.x * v.x + v.y * v.y + v.z * v.z;
  return c;
}
internal S32 length_3s32(Vec3S32 v) {
  S32 c = (S32)sqrt_f32((F32)(v.x * v.x + v.y * v.y + v.z * v.z));
  return c;
}
internal Vec3S32 normalize_3s32(Vec3S32 v) {
  v = scale_3s32(v, (S32)(1.f / length_3s32(v)));
  return v;
}
internal Vec3S32 mix_3s32(Vec3S32 a, Vec3S32 b, F32 t) {
  Vec3S32 c = 
  {
    (S32)mix_1f32((F32)a.x, (F32)b.x, t),
    (S32)mix_1f32((F32)a.y, (F32)b.y, t),
    (S32)mix_1f32((F32)a.z, (F32)b.z, t)
  };
  return c;
}
internal Vec3S32 cross_3s32(Vec3S32 a, Vec3S32 b) {
  Vec3S32 c = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x};
  return c;
}

internal Vec4F32 vec_4f32(F32 x, F32 y, F32 z, F32 w) {
  Vec4F32 v = {x, y, z, w};
  return v;
}
internal Vec4F32 add_4f32(Vec4F32 a, Vec4F32 b) {
  Vec4F32 c = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
  return c;
}
internal Vec4F32 sub_4f32(Vec4F32 a, Vec4F32 b) {
  Vec4F32 c = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
  return c;
}
internal Vec4F32 mul_4f32(Vec4F32 a, Vec4F32 b) {
  Vec4F32 c = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
  return c;
}
internal Vec4F32 div_4f32(Vec4F32 a, Vec4F32 b) {
  Vec4F32 c = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
  return c;
}
internal Vec4F32 scale_4f32(Vec4F32 v, F32 s) {
  Vec4F32 c = {v.x * s, v.y * s, v.z * s, v.w * s};
  return c;
}
internal F32 dot_4f32(Vec4F32 a, Vec4F32 b) {
  F32 c = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  return c;
}
internal F32 length_squared_4f32(Vec4F32 v) {
  F32 c = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
  return c;
}
internal F32 length_4f32(Vec4F32 v) {
  F32 c = sqrt_f32(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
  return c;
}
internal Vec4F32 normalize_4f32(Vec4F32 v) {
  v = scale_4f32(v, 1.f / length_4f32(v));
  return v;
}
internal Vec4F32 mix_4f32(Vec4F32 a, Vec4F32 b, F32 t) {
  Vec4F32 c = {mix_1f32(a.x, b.x, t), mix_1f32(a.y, b.y, t), mix_1f32(a.z, b.z, t), mix_1f32(a.w, b.w, t)};
  return c;
}

internal Vec4S32 vec_4s32(S32 x, S32 y, S32 z, S32 w) {
  Vec4S32 v = {x, y, z, w};
  return v;
}
internal Vec4S32 add_4s32(Vec4S32 a, Vec4S32 b) {
  Vec4S32 c = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
  return c;
}
internal Vec4S32 sub_4s32(Vec4S32 a, Vec4S32 b) {
  Vec4S32 c = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
  return c;
}
internal Vec4S32 mul_4s32(Vec4S32 a, Vec4S32 b) {
  Vec4S32 c = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
  return c;
}
internal Vec4S32 div_4s32(Vec4S32 a, Vec4S32 b) {
  Vec4S32 c = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
  return c;
}
internal Vec4S32 scale_4s32(Vec4S32 v, S32 s) {
  Vec4S32 c = {v.x * s, v.y * s, v.z * s, v.w * s};
  return c;
}
internal S32 dot_4s32(Vec4S32 a, Vec4S32 b) {
  S32 c = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  return c;
}
internal S32 length_squared_4s32(Vec4S32 v) {
  S32 c = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
  return c;
}
internal S32 length_4s32(Vec4S32 v) {
  S32 c =
    (S32)sqrt_f32((F32)(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w));
  return c;
}
internal Vec4S32 normalize_4s32(Vec4S32 v) {
  v = scale_4s32(v, (S32)(1.f / length_4s32(v)));
  return v;
}
internal Vec4S32 mix_4s32(Vec4S32 a, Vec4S32 b, F32 t) {
  Vec4S32 c = 
  {
    (S32)mix_1f32((F32)a.x, (F32)b.x, t),
    (S32)mix_1f32((F32)a.y, (F32)b.y, t),
    (S32)mix_1f32((F32)a.z, (F32)b.z, t),
    (S32)mix_1f32((F32)a.w, (F32)b.w, t)
  };
  return c;
}

////////////////////////////////
//~ rjf: Matrix Ops

internal Mat2x2F32 mat_2x2f32(F32 diagonal)
{
  Mat2x2F32 ret = {0};
  ret.v[0][0] = 1.;
  ret.v[1][1] = 1.;
  return ret;
}

internal Vec2F32 transform_2x2f32(Mat2x2F32 M, Vec2F32 a)
{
  Vec2F32 ret = {0};

  U64 i,j;
  for(i = 0; i < 2; i++)
  {
    ret.v[i] = M.v[0][i]*a.v[0] + M.v[1][i]*a.v[1];
  }
  return ret;
}

internal Mat2x2F32 inverse_2x2f32(Mat2x2F32 M)
{
  Mat2x2F32 ret = {0};
  F32 a = M.v[0][0];
  F32 b = M.v[1][0];
  F32 c = M.v[0][1];
  F32 d = M.v[1][1];
  F32 ad_min_bc_recp = 1.0/(a*d-b*c);

  ret.v[0][0] =  d*ad_min_bc_recp;
  ret.v[0][1] = -c*ad_min_bc_recp;
  ret.v[1][0] = -b*ad_min_bc_recp;
  ret.v[1][1] =  a*ad_min_bc_recp;
  return ret;
}

internal Mat3x3F32 mat_3x3f32(F32 diagonal) {
  Mat3x3F32 result = {0};
  result.v[0][0] = diagonal;
  result.v[1][1] = diagonal;
  result.v[2][2] = diagonal;
  return result;
}

internal Mat3x3F32 make_translate_3x3f32(Vec2F32 delta) {
  Mat3x3F32 mat = mat_3x3f32(1.f);
  mat.v[2][0] = delta.x;
  mat.v[2][1] = delta.y;
  return mat;
}

internal Mat3x3F32 make_scale_3x3f32(Vec2F32 scale) {
  Mat3x3F32 mat = mat_3x3f32(1.f);
  mat.v[0][0] = scale.x;
  mat.v[1][1] = scale.y;
  return mat;
}

internal Mat3x3F32 mul_3x3f32(Mat3x3F32 a, Mat3x3F32 b)
{
  Mat3x3F32 c = {0};
  U64 i,j;
  for(j = 0; j < 3; j += 1)
  {
    for(i = 0; i < 3; i += 1)
    {
      c.v[i][j] = a.v[0][j] * b.v[i][0] + a.v[1][j] * b.v[i][1] + a.v[2][j] * b.v[i][2];
    }
  }
  return c;
}

internal Mat3x3F32 mul_3x3f32_rmajor(Mat3x3F32 a, Mat3x3F32 b)
{
  Mat3x3F32 ret = {0};
  U64 i,j;
  for(i = 0; i < 3; i++)
  {
    for(j = 0; j < 3; j++)
    {
      ret.v[i][j] = a.v[i][0]*b.v[0][j] + a.v[i][1]*b.v[1][j] + a.v[i][2]*b.v[2][j];
    }
  }
  return ret;
}

internal Mat3x3F32 transpose_3x3f32(Mat3x3F32 m)
{
  Mat3x3F32 c = {0};
  U64 i,j;
  for(i = 0; i < 3; i++)
  {
    for(j = 0; j < 3; j++)
    {
      c.v[i][j] = m.v[j][i];
    }
  }
  return c;
}

internal Vec3F32 transform_3x3f32_rmajor(Mat3x3F32 a, Vec3F32 b)
{
  Vec3F32 ret = {0};
  U64 i,j;
  for(i = 0; i < 3; i++)
  {
    ret.v[i] = a.v[i][0] * b.v[0] + a.v[i][1] * b.v[1] + a.v[i][2] * b.v[2];
  }
  return ret;
}

internal Mat3x3F32 star_3x3f32_rmajor(Vec3F32 a)
{
  Mat3x3F32 ret = {0};

  ret.v[0][1] = -a.v[2];
  ret.v[0][2] = a.v[1];

  ret.v[1][0] = a.v[2];
  ret.v[1][2] = -a.v[0];

  ret.v[2][0] = -a.v[1];
  ret.v[2][1] = a.v[0];

  return ret;
}

internal Mat3x3F32 mat3x3f32_from_quat_rmajor(QuatF32 q)
{
  Mat3x3F32 ret = {0};
  F32 s = q.w;
  F32 x = q.x;
  F32 y = q.y;
  F32 z = q.z;

  ret.v[0][0] = 1 - 2*y*y - 2*z*z;
  ret.v[0][1] = 2*x*y - 2*s*z;
  ret.v[0][2] = 2*x*z + 2*s*y;

  ret.v[1][0] = 2*x*y + 2*s*z;
  ret.v[1][1] = 1 - 2*x*x - 2*z*z;
  ret.v[1][2] = 2*y*z - 2*s*x;

  ret.v[2][0] = 2*x*z - 2*s*y;
  ret.v[2][1] = 2*y*z + 2*s*x;
  ret.v[2][2] = 1 - 2*x*x - 2*y*y;
  return ret;
}

internal Mat4x4F32 mat_4x4f32(F32 diagonal) {
  Mat4x4F32 result = {0};
  result.v[0][0] = diagonal;
  result.v[1][1] = diagonal;
  result.v[2][2] = diagonal;
  result.v[3][3] = diagonal;
  return result;
}

internal Mat4x4F32 make_translate_4x4f32(Vec3F32 delta) {
  Mat4x4F32 result = mat_4x4f32(1.f);
  result.v[3][0] = delta.x;
  result.v[3][1] = delta.y;
  result.v[3][2] = delta.z;
  return result;
}

internal Mat4x4F32 make_scale_4x4f32(Vec3F32 scale) {
  Mat4x4F32 result = mat_4x4f32(1.f);
  result.v[0][0] = scale.x;
  result.v[1][1] = scale.y;
  result.v[2][2] = scale.z;
  return result;
}

internal Mat4x4F32 make_perspective_4x4f32(F32 fov, F32 aspect_ratio,
    F32 near_z, F32 far_z) 
{
  Mat4x4F32 result = mat_4x4f32(1.f);
  F32 tan_theta_over_2 = tan_f32(fov / 2);
  result.v[0][0] = 1.f / tan_theta_over_2;
  result.v[1][1] = aspect_ratio / tan_theta_over_2;
  result.v[2][3] = 1.f;
  result.v[2][2] = -(near_z + far_z) / (near_z - far_z);
  result.v[3][2] = (2.f * near_z * far_z) / (near_z - far_z);
  result.v[3][3] = 0.f;
  return result;
}

internal Mat4x4F32 make_perspective_vulkan_4x4f32(F32 fov, F32 aspect_ratio,
    F32 near_z, F32 far_z)
{
  Mat4x4F32 result = {0};
  F32 a = 1.f / tan_f32(fov / 2.f);
  result.v[0][0] = a / aspect_ratio;
  result.v[0][1] = 0.f;
  result.v[0][2] = 0.f;
  result.v[0][3] = 0.f;

  result.v[1][0] = 0.f;
  result.v[1][1] = a;
  result.v[1][2] = 0.f;
  result.v[1][3] = 0.f;

  result.v[2][0] = 0.f;
  result.v[2][1] = 0.f;
  result.v[2][2] = far_z / (far_z - near_z);
  result.v[2][3] = 1.f;

  result.v[3][0] = 0.f;
  result.v[3][1] = 0.f;
  result.v[3][2] = -(far_z * near_z) / (far_z - near_z);;
  result.v[3][3] = 0.f;
  return result;
}

internal Mat4x4F32 make_orthographic_4x4f32(F32 left, F32 right, F32 bottom,
    F32 top, F32 near_z, F32 far_z) {
  Mat4x4F32 result = mat_4x4f32(1.f);

  result.v[0][0] = 2.f / (right - left);
  result.v[1][1] = 2.f / (top - bottom);
  result.v[2][2] = 2.f / (far_z - near_z);
  result.v[3][3] = 1.f;

  result.v[3][0] = (left + right) / (left - right);
  result.v[3][1] = (bottom + top) / (bottom - top);
  result.v[3][2] = (near_z + far_z) / (near_z - far_z);

  return result;
}

internal Mat4x4F32 make_orthographic_vulkan_4x4f32(F32 left, F32 right, F32 bottom,
    F32 top, F32 near_z, F32 far_z)
{
  Mat4x4F32 result = {0};

  result.v[0][0] = 2.f / (right-left);
  result.v[0][1] = 0.f;
  result.v[0][2] = 0.f;
  result.v[0][3] = 0.f;

  result.v[1][0] = 0.f;
  result.v[1][1] = 2.f / (bottom - top);
  result.v[1][2] = 0.f;
  result.v[1][3] = 0.f;

  result.v[2][0] = 0.f;
  result.v[2][1] = 0.f;
  result.v[2][2] = 1.f / (far_z - near_z);
  result.v[2][3] = 0.f;

  result.v[3][0] = -(right+left) / (right-left);
  result.v[3][1] = -(bottom+top) / (bottom-top);
  result.v[3][2] = -near_z / (far_z-near_z);
  result.v[3][3] = 1.f;

  return result;
}

internal Mat4x4F32 make_look_at_4x4f32(Vec3F32 eye, Vec3F32 center,
    Vec3F32 up)
{
  Mat4x4F32 result;
  Vec3F32 f = normalize_3f32(sub_3f32(eye, center));
  Vec3F32 s = normalize_3f32(cross_3f32(f, up));
  Vec3F32 u = cross_3f32(s, f);
  result.v[0][0] = s.x;
  result.v[0][1] = u.x;
  result.v[0][2] = -f.x;
  result.v[0][3] = 0.0f;
  result.v[1][0] = s.y;
  result.v[1][1] = u.y;
  result.v[1][2] = -f.y;
  result.v[1][3] = 0.0f;
  result.v[2][0] = s.z;
  result.v[2][1] = u.z;
  result.v[2][2] = -f.z;
  result.v[2][3] = 0.0f;
  result.v[3][0] = -dot_3f32(s, eye);
  result.v[3][1] = -dot_3f32(u, eye);
  result.v[3][2] = dot_3f32(f, eye);
  result.v[3][3] = 1.0f;
  return result;
}

internal Mat4x4F32 make_look_at_vulkan_4x4f32(Vec3F32 eye, Vec3F32 center,
    Vec3F32 up)
{
  Mat4x4F32 result;
  Vec3F32 f = normalize_3f32(sub_3f32(center, eye)); // forward
  Vec3F32 s = normalize_3f32(cross_3f32(f, up)); // side(right)
  Vec3F32 u = cross_3f32(f, s); // up
  result.v[0][0] = s.x;
  result.v[0][1] = u.x;
  result.v[0][2] = f.x;
  result.v[0][3] = 0;

  result.v[1][0] = s.y;
  result.v[1][1] = u.y;
  result.v[1][2] = f.y;
  result.v[1][3] = 0;

  result.v[2][0] = s.z;
  result.v[2][1] = u.z;
  result.v[2][2] = f.z;
  result.v[2][3] = 0;

  result.v[3][0] = -dot_3f32(s, eye);
  result.v[3][1] = -dot_3f32(u, eye);
  result.v[3][2] = -dot_3f32(f, eye);
  result.v[3][3] = 1.0f;
  return result;
}

internal Mat4x4F32 make_rotate_4x4f32(Vec3F32 axis, F32 turns) {
  Mat4x4F32 result = mat_4x4f32(1.f);
  axis = normalize_3f32(axis);
  F32 sin_theta = sin_f32(turns);
  F32 cos_theta = cos_f32(turns);
  F32 cos_value = 1.f - cos_theta;
  result.v[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
  result.v[0][1] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
  result.v[0][2] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);
  result.v[1][0] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
  result.v[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
  result.v[1][2] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);
  result.v[2][0] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
  result.v[2][1] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
  result.v[2][2] = (axis.z * axis.z * cos_value) + cos_theta;
  return result;
}

internal Mat4x4F32 mul_4x4f32(Mat4x4F32 a, Mat4x4F32 b) {
  Mat4x4F32 c = {0};
  for (int j = 0; j < 4; j += 1) {
    for (int i = 0; i < 4; i += 1) {
      c.v[i][j] =
        (a.v[0][j] * b.v[i][0] + a.v[1][j] * b.v[i][1] +
         a.v[2][j] * b.v[i][2] + a.v[3][j] * b.v[i][3]);
    }
  }
  return c;
}

internal Vec4F32 transform_4x4f32(Mat4x4F32 a, Vec4F32 b)
{
  // clock_t start_time = clock();
  Vec4F32 c = {0};
#if 0
  {
    for(U64 i = 0; i < 4; i++)
    {
      F32 acc = 0;
      for(U64 j = 0; j < 4; j++)
      {
        acc += a.v[j][i] * b.v[j];
      }
      c.v[i] = acc;
    }
  }
#else
  {
    // Store elements of vector b in temporary variables
    F32 b0 = b.v[0];
    F32 b1 = b.v[1];
    F32 b2 = b.v[2];
    F32 b3 = b.v[3];

    // Manually unroll the loops to eliminate loop overhead
    c.v[0] = a.v[0][0] * b0 + a.v[1][0] * b1 + a.v[2][0] * b2 + a.v[3][0] * b3;
    c.v[1] = a.v[0][1] * b0 + a.v[1][1] * b1 + a.v[2][1] * b2 + a.v[3][1] * b3;
    c.v[2] = a.v[0][2] * b0 + a.v[1][2] * b1 + a.v[2][2] * b2 + a.v[3][2] * b3;
    c.v[3] = a.v[0][3] * b0 + a.v[1][3] * b1 + a.v[2][3] * b2 + a.v[3][3] * b3;
  }
#endif
  // double time_taken = ((double)(clock()-start_time)) / CLOCKS_PER_SEC;
  return c;
}

internal Mat4x4F32 scale_4x4f32(Mat4x4F32 m, F32 scale) {
  for (int j = 0; j < 4; j += 1) {
    for (int i = 0; i < 4; i += 1) {
      m.v[i][j] *= scale;
    }
  }
  return m;
}

internal Mat4x4F32 transpose_4x4f32(Mat4x4F32 m) {
  Mat4x4F32 c = {0};
  for (U64 i = 0; i < 4; i += 1) {
    for (U64 j = 0; j < 4; j += 1) {
      c.v[i][j] = m.v[j][i];
    }
  }
  return c;
}

internal Mat4x4F32 inverse_4x4f32(Mat4x4F32 m) {
  F32 coef00 = m.v[2][2] * m.v[3][3] - m.v[3][2] * m.v[2][3];
  F32 coef02 = m.v[1][2] * m.v[3][3] - m.v[3][2] * m.v[1][3];
  F32 coef03 = m.v[1][2] * m.v[2][3] - m.v[2][2] * m.v[1][3];
  F32 coef04 = m.v[2][1] * m.v[3][3] - m.v[3][1] * m.v[2][3];
  F32 coef06 = m.v[1][1] * m.v[3][3] - m.v[3][1] * m.v[1][3];
  F32 coef07 = m.v[1][1] * m.v[2][3] - m.v[2][1] * m.v[1][3];
  F32 coef08 = m.v[2][1] * m.v[3][2] - m.v[3][1] * m.v[2][2];
  F32 coef10 = m.v[1][1] * m.v[3][2] - m.v[3][1] * m.v[1][2];
  F32 coef11 = m.v[1][1] * m.v[2][2] - m.v[2][1] * m.v[1][2];
  F32 coef12 = m.v[2][0] * m.v[3][3] - m.v[3][0] * m.v[2][3];
  F32 coef14 = m.v[1][0] * m.v[3][3] - m.v[3][0] * m.v[1][3];
  F32 coef15 = m.v[1][0] * m.v[2][3] - m.v[2][0] * m.v[1][3];
  F32 coef16 = m.v[2][0] * m.v[3][2] - m.v[3][0] * m.v[2][2];
  F32 coef18 = m.v[1][0] * m.v[3][2] - m.v[3][0] * m.v[1][2];
  F32 coef19 = m.v[1][0] * m.v[2][2] - m.v[2][0] * m.v[1][2];
  F32 coef20 = m.v[2][0] * m.v[3][1] - m.v[3][0] * m.v[2][1];
  F32 coef22 = m.v[1][0] * m.v[3][1] - m.v[3][0] * m.v[1][1];
  F32 coef23 = m.v[1][0] * m.v[2][1] - m.v[2][0] * m.v[1][1];

  Vec4F32 fac0 = {coef00, coef00, coef02, coef03};
  Vec4F32 fac1 = {coef04, coef04, coef06, coef07};
  Vec4F32 fac2 = {coef08, coef08, coef10, coef11};
  Vec4F32 fac3 = {coef12, coef12, coef14, coef15};
  Vec4F32 fac4 = {coef16, coef16, coef18, coef19};
  Vec4F32 fac5 = {coef20, coef20, coef22, coef23};

  Vec4F32 vec0 = {m.v[1][0], m.v[0][0], m.v[0][0], m.v[0][0]};
  Vec4F32 vec1 = {m.v[1][1], m.v[0][1], m.v[0][1], m.v[0][1]};
  Vec4F32 vec2 = {m.v[1][2], m.v[0][2], m.v[0][2], m.v[0][2]};
  Vec4F32 vec3 = {m.v[1][3], m.v[0][3], m.v[0][3], m.v[0][3]};

  Vec4F32 inv0 =
    add_4f32(sub_4f32(mul_4f32(vec1, fac0), mul_4f32(vec2, fac1)),
        mul_4f32(vec3, fac2));
  Vec4F32 inv1 =
    add_4f32(sub_4f32(mul_4f32(vec0, fac0), mul_4f32(vec2, fac3)),
        mul_4f32(vec3, fac4));
  Vec4F32 inv2 =
    add_4f32(sub_4f32(mul_4f32(vec0, fac1), mul_4f32(vec1, fac3)),
        mul_4f32(vec3, fac5));
  Vec4F32 inv3 =
    add_4f32(sub_4f32(mul_4f32(vec0, fac2), mul_4f32(vec1, fac4)),
        mul_4f32(vec2, fac5));

  Vec4F32 sign_a = {+1, -1, +1, -1};
  Vec4F32 sign_b = {-1, +1, -1, +1};

  Mat4x4F32 inverse;
  for (U32 i = 0; i < 4; i += 1) {
    inverse.v[0][i] = inv0.v[i] * sign_a.v[i];
    inverse.v[1][i] = inv1.v[i] * sign_b.v[i];
    inverse.v[2][i] = inv2.v[i] * sign_a.v[i];
    inverse.v[3][i] = inv3.v[i] * sign_b.v[i];
  }

  Vec4F32 row0 = {inverse.v[0][0], inverse.v[1][0], inverse.v[2][0],
    inverse.v[3][0]};
  Vec4F32 m0 = {m.v[0][0], m.v[0][1], m.v[0][2], m.v[0][3]};
  Vec4F32 dot0 = mul_4f32(m0, row0);
  F32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

  F32 one_over_det = 1 / dot1;

  return scale_4x4f32(inverse, one_over_det);
}

internal Mat4x4F32 derotate_4x4f32(Mat4x4F32 mat) {
  Vec3F32 scale = {
    length_3f32(v3f32(mat.v[0][0], mat.v[0][1], mat.v[0][2])),
    length_3f32(v3f32(mat.v[1][0], mat.v[1][1], mat.v[1][2])),
    length_3f32(v3f32(mat.v[2][0], mat.v[2][1], mat.v[2][2])),
  };
  mat.v[0][0] = scale.x;
  mat.v[1][0] = 0.f;
  mat.v[2][0] = 0.f;
  mat.v[0][1] = 0.f;
  mat.v[1][1] = scale.y;
  mat.v[2][1] = 0.f;
  mat.v[0][2] = 0.f;
  mat.v[1][2] = 0.f;
  mat.v[2][2] = scale.z;
  return mat;
}

////////////////////////////////
//~ rjf: Quaterion Ops

internal QuatF32 make_indentity_quat_f32()
{
  return v4f32(0,0,0,1.0f);
}
internal QuatF32 make_rotate_quat_f32(Vec3F32 axis, F32 turns)
{
  axis = normalize_3f32(axis);
  QuatF32 result;
  F32 half_turns = turns/2.0f;
  MemoryCopy(&result, &axis, sizeof(Vec3F32));
  result.w = cos_f32(half_turns);
  F32 s = sin_f32(half_turns);
  result.x *= s;
  result.y *= s;
  result.z *= s;
  return result;
}
internal Mat4x4F32 mat_4x4f32_from_quat_f32(QuatF32 q)
{
  Mat4x4F32 result;
  F32 a = q.v[3];
  F32 b = q.v[0];
  F32 c = q.v[1];
  F32 d = q.v[2];
  F32 a2 = a*a;
  F32 b2 = b*b;
  F32 c2 = c*c;
  F32 d2 = d*d;

  result.v[0][0] = a2 + b2 - c2 - d2;
  result.v[0][1] = 2.f * (b*c + a*d);
  result.v[0][2] = 2.f * (b*d - a*c);
  result.v[0][3] = 0.f;

  result.v[1][0] = 2 * (b*c - a*d);
  result.v[1][1] = a2 - b2 + c2 - d2;
  result.v[1][2] = 2.f * (c*d + a*b);
  result.v[1][3] = 0.f;

  result.v[2][0] = 2.f * (b*d + a*c);
  result.v[2][1] = 2.f * (c*d - a*b);
  result.v[2][2] = a2 - b2 - c2 + d2;
  result.v[2][3] = 0.f;

  result.v[3][0] = result.v[3][1] = result.v[3][2] = 0.f;
  result.v[3][3] = 1.f;
  return result;
}
internal QuatF32 mul_quat_f32(QuatF32 const q, QuatF32 const p)
{
  QuatF32 result;
  Vec3F32 w, tmp; 

  Vec3F32 q3 = v3f32(q.x, q.y, q.z);
  Vec3F32 p3 = v3f32(p.x, p.y, p.z);

  // q⋅p=(w1​w2​−v1​⋅v2​,w1​v2​+w2​v1​+v1​×v2​)
  tmp = cross_3f32(q3, p3);
  w = scale_3f32(q3, p.v[3]);
  w = add_3f32(w, scale_3f32(p3, q.v[3]));
  tmp = add_3f32(tmp, w);
  MemoryCopy(&result, &tmp, sizeof(Vec3F32));
  result.v[3] = q.v[3] * p.v[3] - dot_3f32(q3,p3);
  return result;
}
internal QuatF32 conj_quat_f32(QuatF32 q)
{
  QuatF32 result;
  for(U64 i = 0; i < 3; i++)
  {
    result.v[i] = -q.v[i];
  }
  result.v[3] = q.v[3];
  return result;
}
internal Vec3F32 mul_quat_f32_v3f32(QuatF32 q, Vec3F32 v)
{
  /*
   * Method by Fabian 'ryg' Giessen (of Farbrausch)
   * t = 2 * cross(q.xyz, v)
   * v' = v + q.w * t + cross(q.xyz, t)
   */
  Vec3F32 result;

  Vec3F32 t;
  Vec3F32 q_xyz = {q.x, q.y, q.z};
  Vec3F32 u = {q.x, q.y, q.z};

  t = cross_3f32(q_xyz, v);
  t = scale_3f32(t, 2);

  u = cross_3f32(q_xyz, t);
  t = scale_3f32(t, q.v[3]);

  result = add_3f32(v, t);
  result = add_3f32(result, u);
  return result;
}
internal QuatF32 quat_f32_from_4x4f32(Mat4x4F32 M)
{
  QuatF32 q;
  F32 trace = M.v[0][0] + M.v[1][1] + M.v[2][2];

  if(trace > 0.0f)
  {
    F32 s = sqrt_f32(trace + 1.0f) * 2.0f; // s = 4 * q.w
    q.v[3] = 0.25f * s;                      // w
    q.v[0] = (M.v[1][2] - M.v[2][1]) / s;    // x
    q.v[1] = (M.v[2][0] - M.v[0][2]) / s;    // y
    q.v[2] = (M.v[0][1] - M.v[1][0]) / s;    // z
  }
  else if((M.v[0][0] > M.v[1][1]) && (M.v[0][0] > M.v[2][2]))
  {
    F32 s = sqrt_f32(1.0f + M.v[0][0] - M.v[1][1] - M.v[2][2]) * 2.0f; // s = 4 * q.x
    q.v[0] = 0.25f * s;                                                 // x
    q.v[1] = (M.v[0][1] + M.v[1][0]) / s;                               // y
    q.v[2] = (M.v[0][2] + M.v[2][0]) / s;                               // z
    q.v[3] = (M.v[1][2] - M.v[2][1]) / s;                               // w
  }
  else if(M.v[1][1] > M.v[2][2])
  {
    F32 s = sqrt_f32(1.0f + M.v[1][1] - M.v[0][0] - M.v[2][2]) * 2.0f; // s = 4 * q.y
    q.v[0] = (M.v[0][1] + M.v[1][0]) / s;                               // x
    q.v[1] = 0.25f * s;                                                 // y
    q.v[2] = (M.v[1][2] + M.v[2][1]) / s;                               // z
    q.v[3] = (M.v[2][0] - M.v[0][2]) / s;                               // w
  }
  else
  {
    F32 s = sqrt_f32(1.0f + M.v[2][2] - M.v[0][0] - M.v[1][1]) * 2.0f; // s = 4 * q.z
    q.v[0] = (M.v[0][2] + M.v[2][0]) / s;                               // x
    q.v[1] = (M.v[1][2] + M.v[2][1]) / s;                               // y
    q.v[2] = 0.25f * s;                                                 // z
    q.v[3] = (M.v[0][1] - M.v[1][0]) / s;                               // w
  }

  // Normalize the quaternion to account for numerical errors
  F32 magnitude = sqrt_f32(q.v[0]*q.v[0] + q.v[1]*q.v[1] + q.v[2]*q.v[2] + q.v[3]*q.v[3]);
  q.v[0] /= magnitude;
  q.v[1] /= magnitude;
  q.v[2] /= magnitude;
  q.v[3] /= magnitude;

  return q;
}
internal QuatF32 mix_quat_f32(QuatF32 qa, QuatF32 qb, F32 t)
{
#if 0
  // NOTE: interpolation between two quaterions mean spherical linear
  // http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
  QuatF32 ret;
  // Calculate angle between them.
  F32 cos_half_theta = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;
  // if qa=qb or qa=-qb then theta = 0 and we can return qa
  if (abs_f32(cos_half_theta) >= 1.0){
    ret.w = qa.w;
    ret.x = qa.x;
    ret.y = qa.y;
    ret.z = qa.z;
    return ret;
  }
  // Calculate temporary values.
  F32 half_theta = acosf(cos_half_theta);
  F32 sin_half_theta = sqrtf(1.0 - cos_half_theta*cos_half_theta);
  // if theta = 180 degrees then result is not fully defined
  // we could rotate around any axis normal to qa or qb
  if (abs_f32(sin_half_theta) < 0.001){ // fabs is floating point absolute
    ret.w = (qa.w * 0.5 + qb.w * 0.5);
    ret.x = (qa.x * 0.5 + qb.x * 0.5);
    ret.y = (qa.y * 0.5 + qb.y * 0.5);
    ret.z = (qa.z * 0.5 + qb.z * 0.5);
    return ret;
  }
  F32 ratioA = sinf((1 - t) * half_theta) / sin_half_theta;
  F32 ratioB = sinf(t * half_theta) / sin_half_theta; 
  //calculate Quaternion.
  ret.w = (qa.w * ratioA + qb.w * ratioB);
  ret.x = (qa.x * ratioA + qb.x * ratioB);
  ret.y = (qa.y * ratioA + qb.y * ratioB);
  ret.z = (qa.z * ratioA + qb.z * ratioB);
  ret = normalize_4f32(ret);
  return ret;
#elif 0
  QuatF32 ret;
  F32 dot = qa.x * qb.x + qa.y*qb.y + qa.z*qb.z + qa.w*qb.w;

  F32 theta,st,sut,sout,coeff1,coeff2;
  // algorithm adapted from Shoemake's paper
  t /= 2.0;
  theta = (F32)acos(dot);
  if(theta<0.f) theta = -theta;

  st = (F32)sin(theta);
  sut = (F32)sin(t*theta);
  sout = (F32)sin((1-t)*theta);
  coeff1 = sout/st;
  coeff2 = sut/st;

  ret.x = coeff1*qa.x + coeff2*qb.x;
  ret.y = coeff1*qa.y + coeff2*qb.y;
  ret.z = coeff1*qa.z + coeff2*qb.z;
  ret.w = coeff1*qa.w + coeff2*qb.w;
  ret = normalize_4f32(ret);
  return ret;
#else
  QuatF32 ret;

  // Ensure both quaternions are normalized
  // qa = normalize_4f32(qa);
  // qb = normalize_4f32(qb);

  // Compute the dot product
  F32 dot = qa.x * qb.x + qa.y * qb.y + qa.z * qb.z + qa.w * qb.w;

  // Adjust quaternions if necessary to ensure shortest path
  if (dot < 0.0f) {
    dot = -dot;
    qb.x = -qb.x;
    qb.y = -qb.y;
    qb.z = -qb.z;
    qb.w = -qb.w;
  }

  // Handle small angle approximation
  if (dot > 0.9995f) { // Use LERP for very small angles
    ret.x = qa.x + t * (qb.x - qa.x);
    ret.y = qa.y + t * (qb.y - qa.y);
    ret.z = qa.z + t * (qb.z - qa.z);
    ret.w = qa.w + t * (qb.w - qa.w);
    ret = normalize_4f32(ret);
    return ret;
  }

  // Perform SLERP for other cases
  F32 theta_0 = acosf(dot);      // Angle between quaternions
  F32 sin_theta_0 = sinf(theta_0);
  F32 theta = theta_0 * t;       // Interpolated angle
  F32 sin_theta = sinf(theta);

  F32 ratioA = sinf(theta_0 - theta) / sin_theta_0;
  F32 ratioB = sin_theta / sin_theta_0;

  ret.x = ratioA * qa.x + ratioB * qb.x;
  ret.y = ratioA * qa.y + ratioB * qb.y;
  ret.z = ratioA * qa.z + ratioB * qb.z;
  ret.w = ratioA * qa.w + ratioB * qb.w;

  return ret;
#endif
}

////////////////////////////////
//~ rjf: Range Ops

internal Rng1U32 rng_1u32(U32 min, U32 max) {
  Rng1U32 r = {min, max};
  if (r.min > r.max) {
    Swap(U32, r.min, r.max);
  }
  return r;
}
internal Rng1U32 shift_1u32(Rng1U32 r, U32 x) {
  r.min += x;
  r.max += x;
  return r;
}
internal Rng1U32 pad_1u32(Rng1U32 r, U32 x) {
  r.min -= x;
  r.max += x;
  return r;
}
internal U32 center_1u32(Rng1U32 r) {
  U32 c = (r.min + r.max) / 2;
  return c;
}
internal B32 contains_1u32(Rng1U32 r, U32 x) {
  B32 c = (r.min <= x && x < r.max);
  return c;
}
internal U32 dim_1u32(Rng1U32 r) {
  U32 c = r.max - r.min;
  return c;
}
internal Rng1U32 union_1u32(Rng1U32 a, Rng1U32 b) {
  Rng1U32 c = {Min(a.min, b.min), Max(a.max, b.max)};
  return c;
}
internal Rng1U32 intersect_1u32(Rng1U32 a, Rng1U32 b) {
  Rng1U32 c = {Max(a.min, b.min), Min(a.max, b.max)};
  return c;
}
internal U32 clamp_1u32(Rng1U32 r, U32 v) {
  v = Clamp(r.min, v, r.max);
  return v;
}

internal Rng1S32 rng_1s32(S32 min, S32 max) {
  Rng1S32 r = {min, max};
  if (r.min > r.max) {
    Swap(S32, r.min, r.max);
  }
  return r;
}
internal Rng1S32 shift_1s32(Rng1S32 r, S32 x) {
  r.min += x;
  r.max += x;
  return r;
}
internal Rng1S32 pad_1s32(Rng1S32 r, S32 x) {
  r.min -= x;
  r.max += x;
  return r;
}
internal S32 center_1s32(Rng1S32 r) {
  S32 c = (r.min + r.max) / 2;
  return c;
}
internal B32 contains_1s32(Rng1S32 r, S32 x) {
  B32 c = (r.min <= x && x < r.max);
  return c;
}
internal S32 dim_1s32(Rng1S32 r) {
  S32 c = r.max - r.min;
  return c;
}
internal Rng1S32 union_1s32(Rng1S32 a, Rng1S32 b) {
  Rng1S32 c = {Min(a.min, b.min), Max(a.max, b.max)};
  return c;
}
internal Rng1S32 intersect_1s32(Rng1S32 a, Rng1S32 b) {
  Rng1S32 c = {Max(a.min, b.min), Min(a.max, b.max)};
  return c;
}
internal S32 clamp_1s32(Rng1S32 r, S32 v) {
  v = Clamp(r.min, v, r.max);
  return v;
}

internal Rng1U64 rng_1u64(U64 min, U64 max) {
  Rng1U64 r = {min, max};
  if (r.min > r.max) {
    Swap(U64, r.min, r.max);
  }
  return r;
}
internal Rng1U64 shift_1u64(Rng1U64 r, U64 x) {
  r.min += x;
  r.max += x;
  return r;
}
internal Rng1U64 pad_1u64(Rng1U64 r, U64 x) {
  r.min -= x;
  r.max += x;
  return r;
}
internal U64 center_1u64(Rng1U64 r) {
  U64 c = (r.min + r.max) / 2;
  return c;
}
internal B32 contains_1u64(Rng1U64 r, U64 x) {
  B32 c = (r.min <= x && x < r.max);
  return c;
}
internal U64 dim_1u64(Rng1U64 r) {
  U64 c = r.max - r.min;
  return c;
}
internal Rng1U64 union_1u64(Rng1U64 a, Rng1U64 b) {
  Rng1U64 c = {Min(a.min, b.min), Max(a.max, b.max)};
  return c;
}
internal Rng1U64 intersect_1u64(Rng1U64 a, Rng1U64 b) {
  Rng1U64 c = {Max(a.min, b.min), Min(a.max, b.max)};
  return c;
}
internal U64 clamp_1u64(Rng1U64 r, U64 v) {
  v = Clamp(r.min, v, r.max);
  return v;
}

internal Rng1S64 rng_1s64(S64 min, S64 max) {
  Rng1S64 r = {min, max};
  if (r.min > r.max) {
    Swap(S64, r.min, r.max);
  }
  return r;
}
internal Rng1S64 shift_1s64(Rng1S64 r, S64 x) {
  r.min += x;
  r.max += x;
  return r;
}
internal Rng1S64 pad_1s64(Rng1S64 r, S64 x) {
  r.min -= x;
  r.max += x;
  return r;
}
internal S64 center_1s64(Rng1S64 r) {
  S64 c = (r.min + r.max) / 2;
  return c;
}
internal B32 contains_1s64(Rng1S64 r, S64 x) {
  B32 c = (r.min <= x && x < r.max);
  return c;
}
internal S64 dim_1s64(Rng1S64 r) {
  S64 c = r.max - r.min;
  return c;
}
internal Rng1S64 union_1s64(Rng1S64 a, Rng1S64 b) {
  Rng1S64 c = {Min(a.min, b.min), Max(a.max, b.max)};
  return c;
}
internal Rng1S64 intersect_1s64(Rng1S64 a, Rng1S64 b) {
  Rng1S64 c = {Max(a.min, b.min), Min(a.max, b.max)};
  return c;
}
internal S64 clamp_1s64(Rng1S64 r, S64 v) {
  v = Clamp(r.min, v, r.max);
  return v;
}

internal Rng1F32 rng_1f32(F32 min, F32 max) {
  Rng1F32 r = {min, max};
  if (r.min > r.max) {
    Swap(F32, r.min, r.max);
  }
  return r;
}
internal Rng1F32 shift_1f32(Rng1F32 r, F32 x) {
  r.min += x;
  r.max += x;
  return r;
}
internal Rng1F32 pad_1f32(Rng1F32 r, F32 x) {
  r.min -= x;
  r.max += x;
  return r;
}
internal F32 center_1f32(Rng1F32 r) {
  F32 c = (r.min + r.max) / 2;
  return c;
}
internal B32 contains_1f32(Rng1F32 r, F32 x) {
  B32 c = (r.min <= x && x < r.max);
  return c;
}
internal F32 dim_1f32(Rng1F32 r) {
  F32 c = r.max - r.min;
  return c;
}
internal Rng1F32 union_1f32(Rng1F32 a, Rng1F32 b) {
  Rng1F32 c = {Min(a.min, b.min), Max(a.max, b.max)};
  return c;
}
internal Rng1F32 intersect_1f32(Rng1F32 a, Rng1F32 b) {
  Rng1F32 c = {Max(a.min, b.min), Min(a.max, b.max)};
  return c;
}
internal F32 clamp_1f32(Rng1F32 r, F32 v) {
  v = Clamp(r.min, v, r.max);
  return v;
}

internal Rng2S16 rng_2s16(Vec2S16 min, Vec2S16 max) {
  Rng2S16 r = {min, max};
  return r;
}
internal Rng2S16 shift_2s16(Rng2S16 r, Vec2S16 x) {
  r.min = add_2s16(r.min, x);
  r.max = add_2s16(r.max, x);
  return r;
}
internal Rng2S16 pad_2s16(Rng2S16 r, S16 x) {
  Vec2S16 xv = {x, x};
  r.min = sub_2s16(r.min, xv);
  r.max = add_2s16(r.max, xv);
  return r;
}
internal Vec2S16 center_2s16(Rng2S16 r) {
  Vec2S16 c = {(S16)((r.min.x + r.max.x) / 2),
    (S16)((r.min.y + r.max.y) / 2)};
  return c;
}
internal B32 contains_2s16(Rng2S16 r, Vec2S16 x) {
  B32 c = (r.min.x <= x.x && x.x < r.max.x && r.min.y <= x.y &&
      x.y < r.max.y);
  return c;
}
internal Vec2S16 dim_2s16(Rng2S16 r) {
  Vec2S16 dim = {(S16)(r.max.x - r.min.x), (S16)(r.max.y - r.min.y)};
  return dim;
}
internal Rng2S16 union_2s16(Rng2S16 a, Rng2S16 b) {
  Rng2S16 c;
  c.p0.x = Min(a.min.x, b.min.x);
  c.p0.y = Min(a.min.y, b.min.y);
  c.p1.x = Max(a.max.x, b.max.x);
  c.p1.y = Max(a.max.y, b.max.y);
  return c;
}
internal Rng2S16 intersect_2s16(Rng2S16 a, Rng2S16 b) {
  Rng2S16 c;
  c.p0.x = Max(a.min.x, b.min.x);
  c.p0.y = Max(a.min.y, b.min.y);
  c.p1.x = Min(a.max.x, b.max.x);
  c.p1.y = Min(a.max.y, b.max.y);
  return c;
}
internal Vec2S16 clamp_2s16(Rng2S16 r, Vec2S16 v) {
  v.x = Clamp(r.min.x, v.x, r.max.x);
  v.y = Clamp(r.min.y, v.y, r.max.y);
  return v;
}

internal Rng2S32 rng_2s32(Vec2S32 min, Vec2S32 max) {
  Rng2S32 r = {min, max};
  return r;
}
internal Rng2S32 shift_2s32(Rng2S32 r, Vec2S32 x) {
  r.min = add_2s32(r.min, x);
  r.max = add_2s32(r.max, x);
  return r;
}
internal Rng2S32 pad_2s32(Rng2S32 r, S32 x) {
  Vec2S32 xv = {x, x};
  r.min = sub_2s32(r.min, xv);
  r.max = add_2s32(r.max, xv);
  return r;
}
internal Vec2S32 center_2s32(Rng2S32 r) {
  Vec2S32 c = {(r.min.x + r.max.x) / 2, (r.min.y + r.max.y) / 2};
  return c;
}
internal B32 contains_2s32(Rng2S32 r, Vec2S32 x) {
  B32 c = (r.min.x <= x.x && x.x < r.max.x && r.min.y <= x.y &&
      x.y < r.max.y);
  return c;
}
internal Vec2S32 dim_2s32(Rng2S32 r) {
  Vec2S32 dim = {r.max.x - r.min.x, r.max.y - r.min.y};
  return dim;
}
internal Rng2S32 union_2s32(Rng2S32 a, Rng2S32 b) {
  Rng2S32 c;
  c.p0.x = Min(a.min.x, b.min.x);
  c.p0.y = Min(a.min.y, b.min.y);
  c.p1.x = Max(a.max.x, b.max.x);
  c.p1.y = Max(a.max.y, b.max.y);
  return c;
}
internal Rng2S32 intersect_2s32(Rng2S32 a, Rng2S32 b) {
  Rng2S32 c;
  c.p0.x = Max(a.min.x, b.min.x);
  c.p0.y = Max(a.min.y, b.min.y);
  c.p1.x = Min(a.max.x, b.max.x);
  c.p1.y = Min(a.max.y, b.max.y);
  return c;
}
internal Vec2S32 clamp_2s32(Rng2S32 r, Vec2S32 v) {
  v.x = Clamp(r.min.x, v.x, r.max.x);
  v.y = Clamp(r.min.y, v.y, r.max.y);
  return v;
}

internal Rng2S64 rng_2s64(Vec2S64 min, Vec2S64 max) {
  Rng2S64 r = {min, max};
  return r;
}
internal Rng2S64 shift_2s64(Rng2S64 r, Vec2S64 x) {
  r.min = add_2s64(r.min, x);
  r.max = add_2s64(r.max, x);
  return r;
}
internal Rng2S64 pad_2s64(Rng2S64 r, S64 x) {
  Vec2S64 xv = {x, x};
  r.min = sub_2s64(r.min, xv);
  r.max = add_2s64(r.max, xv);
  return r;
}
internal Vec2S64 center_2s64(Rng2S64 r) {
  Vec2S64 c = {(r.min.x + r.max.x) / 2, (r.min.y + r.max.y) / 2};
  return c;
}
internal B32 contains_2s64(Rng2S64 r, Vec2S64 x) {
  B32 c = (r.min.x <= x.x && x.x < r.max.x && r.min.y <= x.y &&
      x.y < r.max.y);
  return c;
}
internal Vec2S64 dim_2s64(Rng2S64 r) {
  Vec2S64 dim = {r.max.x - r.min.x, r.max.y - r.min.y};
  return dim;
}
internal Rng2S64 union_2s64(Rng2S64 a, Rng2S64 b) {
  Rng2S64 c;
  c.p0.x = Min(a.min.x, b.min.x);
  c.p0.y = Min(a.min.y, b.min.y);
  c.p1.x = Max(a.max.x, b.max.x);
  c.p1.y = Max(a.max.y, b.max.y);
  return c;
}
internal Rng2S64 intersect_2s64(Rng2S64 a, Rng2S64 b) {
  Rng2S64 c;
  c.p0.x = Max(a.min.x, b.min.x);
  c.p0.y = Max(a.min.y, b.min.y);
  c.p1.x = Min(a.max.x, b.max.x);
  c.p1.y = Min(a.max.y, b.max.y);
  return c;
}
internal Vec2S64 clamp_2s64(Rng2S64 r, Vec2S64 v) {
  v.x = Clamp(r.min.x, v.x, r.max.x);
  v.y = Clamp(r.min.y, v.y, r.max.y);
  return v;
}

internal Rng2F32 rng_2f32(Vec2F32 min, Vec2F32 max) {
  Rng2F32 r = {min, max};
  return r;
}
internal Rng2F32 shift_2f32(Rng2F32 r, Vec2F32 x) {
  r.min = add_2f32(r.min, x);
  r.max = add_2f32(r.max, x);
  return r;
}
internal Rng2F32 pad_2f32(Rng2F32 r, F32 x) {
  Vec2F32 xv = {x, x};
  r.min = sub_2f32(r.min, xv);
  r.max = add_2f32(r.max, xv);
  return r;
}
internal Vec2F32 center_2f32(Rng2F32 r) {
  Vec2F32 c = {(r.min.x + r.max.x) / 2, (r.min.y + r.max.y) / 2};
  return c;
}
internal B32 contains_2f32(Rng2F32 r, Vec2F32 x) {
  B32 c = (r.min.x <= x.x && x.x < r.max.x && r.min.y <= x.y && x.y < r.max.y);
  return c;
}
internal B32 contains_22f32(Rng2F32 a, Rng2F32 b) {
  B32 c = b.x0 >= a.x0 &&
          b.x1 <= a.x1 &&
          b.y0 >= a.y0 &&
          b.y1 <= a.y1;
  return c;
};
internal Vec2F32 dim_2f32(Rng2F32 r) {
  Vec2F32 dim = {r.max.x - r.min.x, r.max.y - r.min.y};
  return dim;
}
internal Rng2F32 union_2f32(Rng2F32 a, Rng2F32 b) {
  Rng2F32 c;
  c.p0.x = Min(a.min.x, b.min.x);
  c.p0.y = Min(a.min.y, b.min.y);
  c.p1.x = Max(a.max.x, b.max.x);
  c.p1.y = Max(a.max.y, b.max.y);
  return c;
}
internal Rng2F32 intersect_2f32(Rng2F32 a, Rng2F32 b) {
  Rng2F32 c;
  c.p0.x = Max(a.min.x, b.min.x);
  c.p0.y = Max(a.min.y, b.min.y);
  c.p1.x = Min(a.max.x, b.max.x);
  c.p1.y = Min(a.max.y, b.max.y);
  return c;
}
internal B32 overlaps_2f32(Rng2F32 a, Rng2F32 b) {
  return !(a.p1.x <= b.p0.x || a.p0.x >= b.p1.x ||
           a.p1.y <= b.p0.y || a.p0.y >= b.p1.y);
}

internal Vec2F32 clamp_2f32(Rng2F32 r, Vec2F32 v) {
  v.x = Clamp(r.min.x, v.x, r.max.x);
  v.y = Clamp(r.min.y, v.y, r.max.y);
  return v;
}

////////////////////////////////
//~ rjf: Color Operations

//- rjf: hsv <-> rgb

internal Vec3F32
hsv_from_rgb(Vec3F32 rgb)
{
  F32 c_max = Max(rgb.x, Max(rgb.y, rgb.z));
  F32 c_min = Min(rgb.x, Min(rgb.y, rgb.z));
  F32 delta = c_max - c_min;
  F32 h = ((delta == 0.f) ? 0.f :
           (c_max == rgb.x) ? mod_f32((rgb.y - rgb.z)/delta + 6.f, 6.f) :
           (c_max == rgb.y) ? (rgb.z - rgb.x)/delta + 2.f :
           (c_max == rgb.z) ? (rgb.x - rgb.y)/delta + 4.f :
           0.f);
  F32 s = (c_max == 0.f) ? 0.f : (delta/c_max);
  F32 v = c_max;
  Vec3F32 hsv = {h/6.f, s, v};
  return hsv;
}

internal Vec3F32
rgb_from_hsv(Vec3F32 hsv)
{
  F32 h = mod_f32(hsv.x * 360.f, 360.f);
  F32 s = hsv.y;
  F32 v = hsv.z;
  
  F32 c = v*s;
  F32 x = c*(1.f - abs_f32(mod_f32(h/60.f, 2.f) - 1.f));
  F32 m = v - c;
  
  F32 r = 0;
  F32 g = 0;
  F32 b = 0;
  
  if ((h >= 0.f && h < 60.f) || (h >= 360.f && h < 420.f)){
    r = c;
    g = x;
    b = 0;
  }
  else if (h >= 60.f && h < 120.f){
    r = x;
    g = c;
    b = 0;
  }
  else if (h >= 120.f && h < 180.f){
    r = 0;
    g = c;
    b = x;
  }
  else if (h >= 180.f && h < 240.f){
    r = 0;
    g = x;
    b = c;
  }
  else if (h >= 240.f && h < 300.f){
    r = x;
    g = 0;
    b = c;
  }
  else if ((h >= 300.f && h <= 360.f) || (h >= -60.f && h <= 0.f)){
    r = c;
    g = 0;
    b = x;
  }
  
  Vec3F32 rgb = {r + m, g + m, b + m};
  return(rgb);
}

internal Vec4F32
hsva_from_rgba(Vec4F32 rgba)
{
  Vec3F32 rgb = v3f32(rgba.x, rgba.y, rgba.z);
  Vec3F32 hsv = hsv_from_rgb(rgb);
  Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
  return hsva;
}

internal Vec4F32
rgba_from_hsva(Vec4F32 hsva)
{
  Vec3F32 hsv = v3f32(hsva.x, hsva.y, hsva.z);
  Vec3F32 rgb = rgb_from_hsv(hsv);
  Vec4F32 rgba = v4f32(rgb.x, rgb.y, rgb.z, hsva.w);
  return rgba;
}

//- rjf: srgb <-> linear

internal Vec3F32
linear_from_srgb(Vec3F32 srgb)
{
  Vec3F32 result;
  for EachElement(idx, srgb.v)
  {
    result.v[idx] = (srgb.v[idx] < 0.0404482362771082f ? srgb.v[idx] / 12.92f : pow_f32((srgb.v[idx] + 0.055f) / 1.055f, 2.4f));
  }
  return result;
}

internal Vec3F32
srgb_from_linear(Vec3F32 linear)
{
  Vec3F32 result;
  for EachElement(idx, linear.v)
  {
    result.v[idx] = (0 <= linear.v[idx] && linear.v[idx] < 0.00313066844250063) ? linear.v[idx]*12.92f : 1.05f * pow_f32(linear.v[idx], 1.f/2.4f) - 0.055f;
  }
  return result;
}

internal Vec4F32
linear_from_srgba(Vec4F32 srgba)
{
  Vec4F32 result;
  result.xyz = linear_from_srgb(srgba.xyz);
  result.w = srgba.w;
  return result;
}

internal Vec4F32
srgba_from_linear(Vec4F32 linear)
{
  Vec4F32 result;
  result.xyz = srgb_from_linear(linear.xyz);
  result.w = linear.w;
  return result;
}

//- rjf: oklab <-> linear

internal Vec3F32
oklab_from_linear(Vec3F32 linear)
{
  F32 l = (0.4122214708f * linear.x + 0.5363325363f * linear.y + 0.0514459929f * linear.z);
	F32 m = (0.2119034982f * linear.x + 0.6806995451f * linear.y + 0.1073969566f * linear.z);
	F32 s = (0.0883024619f * linear.x + 0.2817188376f * linear.y + 0.6299787005f * linear.z);
  F32 l_ = cbrt_f32(l);
  F32 m_ = cbrt_f32(m);
  F32 s_ = cbrt_f32(s);
  Vec3F32 result;
  result.x = 0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_;
  result.y = 1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_;
  result.z = 0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_;
  return result;
}

internal Vec3F32
linear_from_oklab(Vec3F32 oklab)
{
  F32 l_ = oklab.x + 0.3963377774f * oklab.y + 0.2158037573f * oklab.z;
  F32 m_ = oklab.x - 0.1055613458f * oklab.y - 0.0638541728f * oklab.z;
  F32 s_ = oklab.x - 0.0894841775f * oklab.y - 1.2914855480f * oklab.z;
  F32 l = l_*l_*l_;
  F32 m = m_*m_*m_;
  F32 s = s_*s_*s_;
  Vec3F32 result;
  result.x = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
  result.y = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
  result.z = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
  return result;
}

internal Vec4F32
oklab_from_lineara(Vec4F32 lineara)
{
  Vec4F32 result;
  result.xyz = oklab_from_linear(lineara.xyz);
  result.w = lineara.w;
  return result;
}

internal Vec4F32
lineara_from_oklab(Vec4F32 oklab)
{
  Vec4F32 result;
  result.xyz = linear_from_oklab(oklab.xyz);
  result.w = oklab.w;
  return result;
}

//- rjf: rgba <-> u32

internal U32
u32_from_rgba(Vec4F32 rgba)
{
  U32 result = 0;
  result |= ((U32)((U8)(rgba.x*255.f))) << 24;
  result |= ((U32)((U8)(rgba.y*255.f))) << 16;
  result |= ((U32)((U8)(rgba.z*255.f))) <<  8;
  result |= ((U32)((U8)(rgba.w*255.f))) <<  0;
  return result;
}

internal Vec4F32
rgba_from_u32(U32 hex)
{
  Vec4F32 result = v4f32(((hex&0xff000000)>>24)/255.f,
                         ((hex&0x00ff0000)>>16)/255.f,
                         ((hex&0x0000ff00)>> 8)/255.f,
                         ((hex&0x000000ff)>> 0)/255.f);
  return result;
}

////////////////////////////////
//~ rjf: Miscellaneous Ops

// thread_static U32 gseed = 0;

// internal U32
// rand_u32(void)
// {
//   // TODO(k): this way of accumlating gseed is not quite right, the mod operation will not be uniformly distrubuted
//   U32 seed = gseed++;
//   seed = (seed << 13) ^ seed;
//   return ((seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff);
// }

// internal F32
// rand_f32(void)
// {
//   // rand_u32() returns values in range [0, 2147483647], which is 2^31 - 1. Dividing by 2^31 (2147483648.0f) ensures the result stays below 1.0.
//   // This gives you a uniform float in [0.0, ~0.9999999]`, never reaching exactly 1.0—which is ideal for most randomness use cases (like in noise generation or sampling).
//   U32 r = rand_u32();
//   F32 ret =  (F32)r/2147483648.0f;
//   return ret;
// }

// internal void
// gsrand(U32 s)
// {
//   gseed = s;
// }

////////////////////////////////
//~ rjf: List Type Functions

internal void rng1s64_list_push(Arena *arena, Rng1S64List *list, Rng1S64 rng) {
  Rng1S64Node *n = push_array(arena, Rng1S64Node, 1);
  MemoryCopyStruct(&n->v, &rng);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal Rng1S64Array rng1s64_array_from_list(Arena *arena, Rng1S64List *list) {
  Rng1S64Array arr = {0};
  arr.count = list->count;
  arr.v = push_array_no_zero(arena, Rng1S64, arr.count);
  U64 idx = 0;
  for (Rng1S64Node *n = list->first; n != 0; n = n->next) {
    arr.v[idx] = n->v;
    idx += 1;
  }
  return arr;
}
