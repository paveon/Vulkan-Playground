#ifndef VULKAN_MATHLIB_H
#define VULKAN_MATHLIB_H

#include <cmath>
#include <cassert>
#include <emmintrin.h>
#include <pmmintrin.h>

#define _USE_MATH_DEFINES

constexpr double TWO_PI = M_PI * 2.0f;
constexpr double PI_RAD = M_PI / 180.0f;

namespace math {
   struct mat4x4;


   struct vec2 {
      float x, y;

   public:
      vec2() : x(0), y(0) {}

      vec2(float x, float y) : x(x), y(y) {}
   };


   struct vec3 {
      float x, y, z;

   public:
      vec3() : x(1.0f), y(1.0f), z(1.0f) {}

      vec3(float x, float y, float z) : x(x), y(y), z(z) {}

      vec3 operator/(float divisor) const {
         auto result = _mm_div_ps((__m128) *this, _mm_set_ps1(divisor));
         result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
         return *reinterpret_cast<vec3*>(&result);
      }

      vec3 operator-(const vec3& v) const {
         auto result = _mm_sub_ps((__m128) *this, (__m128) v);
         result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
         return *reinterpret_cast<vec3*>(&result);
      }

      vec3 operator*(const vec3& v) const {
         auto result = _mm_mul_ps((__m128) *this, (__m128) v);
         result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
         return *reinterpret_cast<vec3*>(&result);
      }

      explicit operator __m128() const;

      float& operator[](int index) {
         assert(index >= 0 && index < 3);
         switch (index) {
            default:
            case 0:
               return x;
            case 1:
               return y;
            case 2:
               return z;
         }
      }
   };

   inline vec3::operator __m128() const {
      return _mm_set_ps(x, y, z, 0);
   }

   inline vec3 operator*(const vec3& v, float multiplier) {
      auto result = _mm_mul_ps((__m128) v, _mm_set_ps1(multiplier));
      result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
      return *reinterpret_cast<vec3*>(&result);
   }

   inline vec3 operator*(float multiplier, const vec3& v) { return operator*(v, multiplier); }

   inline float dot(const vec3& v1, const vec3& v2) {
      auto tmp = _mm_mul_ps((__m128) v1, (__m128) v2);
      auto aux = reinterpret_cast<float*>(&tmp);
      return aux[1] + aux[2] + aux[3];
   }


   struct vec4 {
      float x, y, z, w;

   public:
      vec4() : x(1.0f), y(1.0f), z(1.0f), w(1.0f) {}

      vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

      vec4(const vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}


      vec4 operator+(const vec4& v) {
         return vec4(x + v.x, y + v.y, z + v.z, w + v.w);
      }

      vec4 operator*(const mat4x4& mat);

      explicit operator __m128() const {
         return *reinterpret_cast<const __m128*>(&x);
      }

      float& operator[](int index) {
         assert(index >= 0 && index < 4);
         switch (index) {
            default:
            case 0:
               return x;
            case 1:
               return y;
            case 2:
               return z;
            case 3:
               return w;
         }
      }

      float operator[](int index) const {
         assert(index >= 0 && index < 4);
         switch (index) {
            default:
            case 0:
               return x;
            case 1:
               return y;
            case 2:
               return z;
            case 3:
               return w;
         }
      }
   };

   inline float dot(const vec4& v1, const vec4& v2) {
      auto tmp = _mm_mul_ps((__m128) v1, (__m128) v2);
      auto aux = reinterpret_cast<float*>(&tmp);
      return aux[0] + aux[1] + aux[2] + aux[3];
   }


   inline vec3 cross(const vec3& a, const vec3& b) {
      __m128 a_tmp(a);
      a_tmp = _mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(0, 1, 2, 3));
      __m128 b_tmp(b);
      b_tmp = _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(0, 1, 2, 3));
      __m128 a_yzx = _mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(3, 0, 2, 1));
      __m128 b_yzx = _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(3, 0, 2, 1));
      __m128 c = _mm_sub_ps(_mm_mul_ps(a_tmp, b_yzx), _mm_mul_ps(a_yzx, b_tmp));
      c = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
      auto tmp = reinterpret_cast<float*>(&c);
      return vec3(tmp[0], tmp[1], tmp[2]);

//      __m128 product = _mm_sub_ps(
//              _mm_mul_ps(_mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(3, 0, 2, 1)),
//                         _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(3, 1, 0, 2))),
//              _mm_mul_ps(_mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(3, 1, 0, 2)),
//                         _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(3, 0, 2, 1)))
//      );
//      auto tmp = reinterpret_cast<float*>(&product);
//      return vec3(tmp[0], tmp[1], tmp[2]);
   }


   inline vec4 operator*(const vec4& v, float multiplier) {
      auto result = _mm_mul_ps((__m128) v, _mm_set_ps1(multiplier));
      return *reinterpret_cast<vec4*>(&result);

      //return vec4(v.x * multiplier, v.y * multiplier, v.z * multiplier, v.w * multiplier);
   }

   inline vec4 operator*(float multiplier, const vec4& v) { return operator*(v, multiplier); }


   struct mat4x4 {
      vec4 value[4];

   public:
      mat4x4() : value{vec4(1.0f, 0.0f, 0.0f, 0.0f),
                       vec4(0.0f, 1.0f, 0.0f, 0.0f),
                       vec4(0.0f, 0.0f, 1.0f, 0.0f),
                       vec4(0.0f, 0.0f, 0.0f, 1.0f)} {}

      mat4x4(const vec4& row1, const vec4& row2, const vec4& row3, const vec4& row4)
              : value{row1, row2, row3, row4} {}

      explicit mat4x4(float init) : value{vec4(init, 0.0f, 0.0f, 0.0f),
                                          vec4(0.0f, init, 0.0f, 0.0f),
                                          vec4(0.0f, 0.0f, init, 0.0f),
                                          vec4(0.0f, 0.0f, 0.0f, init)} {}


      vec4& operator[](int index) {
         assert(index >= 0 && index < 4);
         return value[index];
      }

      vec4 operator[](int index) const {
         assert(index >= 0 && index < 4);
         return value[index];
      }

//      vec4 col(int index) const {
//         assert(index >= 0 && index < 4);
//         return vec4(value[0][index], value[1][index], value[2][index], value[3][index]);
//      }
   };

   typedef mat4x4 mat4;


   inline mat4x4 operator*(const mat4x4& a, const mat4x4& b) {
      const vec4& aRow0 = a[0];
      const vec4& aRow1 = a[1];
      const vec4& aRow2 = a[2];
      const vec4& aRow3 = a[3];
      const vec4& bRow0 = b[0];
      const vec4& bRow1 = b[1];
      const vec4& bRow2 = b[2];
      const vec4& bRow3 = b[3];

      return {
              (aRow0[0] * bRow0) + (aRow0[1] * bRow1) + (aRow0[2] * bRow2) + (aRow0[3] * bRow3),
              (aRow1[0] * bRow0) + (aRow1[1] * bRow1) + (aRow1[2] * bRow2) + (aRow1[3] * bRow3),
              (aRow2[0] * bRow0) + (aRow2[1] * bRow1) + (aRow2[2] * bRow2) + (aRow2[3] * bRow3),
              (aRow3[0] * bRow0) + (aRow3[1] * bRow1) + (aRow3[2] * bRow2) + (aRow3[3] * bRow3)
      };
   }


   inline vec4 vec4::operator*(const mat4x4& mat) {
      return (x * mat[0]) + (y * mat[1]) + (z * mat[2]) + (w * mat[3]);
   }


   inline float radians(float degrees) { return degrees * PI_RAD; }

   inline float magnitude(vec3 v) {
      __m128 aux(v);
      aux = _mm_mul_ps(aux, aux);
      auto tmp = reinterpret_cast<float*>(&aux);
      return sqrtf(tmp[1] + tmp[2] + tmp[3]);

      //return sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
   }

   inline vec3 normalize(vec3 v) {
      float m = magnitude(v);
      assert(m > 0);
      return v / m;
   }

   inline mat4x4 rotate(mat4x4 mat, float angle, vec3 axis) {
      float c = std::cos(angle);
      float s = std::sin(angle);
      axis = normalize(axis);
      vec3 temp((1.0f - c) * axis);

      //Rodrigues' rotation matrix
      //http://mathworld.wolfram.com/RodriguesRotationFormula.html
      mat4x4 rotate;
      rotate[0][0] = axis[0] * temp[0] + c;
      rotate[0][1] = axis[0] * temp[1] - (axis[2] * s);
      rotate[0][3] = axis[0] * temp[2] + (axis[1] * s);

      rotate[1][0] = axis[1] * temp[0] + (axis[2] * s);
      rotate[1][1] = axis[1] * temp[1] + c;
      rotate[1][3] = axis[1] * temp[2] - (axis[0] * s);

      rotate[2][0] = axis[2] * temp[0] - (axis[1] * s);
      rotate[2][1] = axis[2] * temp[1] + (axis[0] * s);
      rotate[2][3] = axis[2] * temp[2] + c;

      return mat * rotate;

//      mat4x4 result;
//      result[0] = mat.col(0) * rotate[0][0] + mat.col(1) * rotate[0][1] + mat.col(2) * rotate[0][2];
//      result[1] = mat.col(0) * rotate[1][0] + mat.col(1) * rotate[1][1] + mat.col(2) * rotate[1][2];
//      result[2] = mat.col(0) * rotate[2][0] + mat.col(1) * rotate[2][1] + mat.col(2) * rotate[2][2];
//      result[3] = mat[3];
//
//      return result;
   }


   inline mat4x4 lookAt(vec3 cameraPos, vec3 cameraTarget, vec3 up) {
      vec3 cameraDirection(normalize(cameraPos - cameraTarget));
      vec3 cameraRight(normalize(cross(up, cameraDirection)));
      vec3 cameraUp(cross(cameraDirection, cameraRight));

      return {
              vec4(cameraRight.x, cameraUp.x, cameraDirection.x, 0),
              vec4(cameraRight.y, cameraUp.y, cameraDirection.y, 0),
              vec4(cameraRight.z, cameraUp.z, cameraDirection.z, 0),
              vec4(-dot(cameraRight, cameraPos), -dot(cameraUp, cameraPos), -dot(cameraDirection, cameraPos), 1)
      };
   }


   inline mat4x4 perspective(float fovY, float aspectRatio, float zNear, float zFar) {
      assert(aspectRatio != 0);
      assert(zFar != zNear);

      float tanHalfFovY = std::tan(fovY / 2.0f);

      mat4x4 perspectiveMat(0);
      perspectiveMat[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
      perspectiveMat[1][1] = 1.0f / (tanHalfFovY);
      perspectiveMat[2][2] = - (zFar + zNear) / (zFar - zNear);
      perspectiveMat[2][3] = -1.0f;
      perspectiveMat[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
      return perspectiveMat;
   }

}


#endif //VULKAN_MATHLIB_H
