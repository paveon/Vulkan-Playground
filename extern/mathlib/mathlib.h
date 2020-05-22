#ifndef VULKAN_MATHLIB_H
#define VULKAN_MATHLIB_H

#include <cmath>
#include <cassert>
#include <emmintrin.h>
#include <pmmintrin.h>

#define _USE_MATH_DEFINES

#define MTH_DEPTH_ZERO_TO_ONE
#ifdef MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE
#undef MTH_DEPTH_ZERO_TO_ONE
#endif

#define MTH_RIGHT_HAND_SYSTEM
#ifdef MTH_LEFT_HAND_SYSTEM
#undef MTH_RIGHT_HAND_SYSTEM
#endif


//#define CLAMP(lower_bound, value, upper_bound) std::max((lower_bound), std::min((upper_bound), (value)))
template<typename T>
inline auto CLAMP(T lowerBound, T value, T upperBound) -> T {
    return std::max((lowerBound), std::min((upperBound), (value)));
}

constexpr double HALF_PI = M_PI_2;
constexpr double PI = M_PI;
constexpr double TWO_PI = M_PI * 2.0;
constexpr double PI_RAD = M_PI / 180.0;


constexpr float HALF_PI_F = M_PI_2;
constexpr float PI_F = M_PI;
constexpr float TWO_PI_F = M_PI * 2.0;
constexpr float PI_RAD_F = M_PI / 180.0;


namespace math {
    struct mat4x4;

    using Type = float;

    struct vec2 {
        Type x, y;

    public:
        vec2() : x(0), y(0) {}

        template<typename T>
        explicit vec2(T value) : x(static_cast<Type>(value)), y(static_cast<Type>(value)) {}

        template<typename T>
        vec2(T x, T y) : x(static_cast<Type>(x)), y(static_cast<Type>(y)) {}

        auto operator==(const vec2 &other) const -> bool { return x == other.x && y == other.y; }
    };


    struct vec3 {
        float x, y, z;

    public:
        vec3() : x(0.0f), y(0.0f), z(0.0f) {}

        explicit vec3(float value) : x(value), y(value), z(value) {}

        vec3(float x, float y, float z) : x(x), y(y), z(z) {}

        auto operator/(float divisor) const -> vec3 {
            auto result = _mm_div_ps((__m128) *this, _mm_set_ps1(divisor));
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec3 *>(&result);
        }

        auto operator-(const vec3 &v) const -> vec3 {
            auto result = _mm_sub_ps((__m128) *this, (__m128) v);
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec3 *>(&result);
        }

        auto operator+(const vec3 &v) const -> vec3 {
            auto result = _mm_add_ps((__m128) *this, (__m128) v);
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec3 *>(&result);
        }

        auto operator*(const vec3 &v) const -> vec3 {
            auto result = _mm_mul_ps((__m128) *this, (__m128) v);
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec3 *>(&result);
        }

        explicit operator __m128() const;

        auto operator[](int index) -> float & {
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

        auto operator==(const vec3 &other) const -> bool { return x == other.x && y == other.y && z == other.z; }
    };

    inline vec3::operator __m128() const {
        return _mm_set_ps(x, y, z, 0);
    }

    inline auto operator*(const vec3 &v, float multiplier) -> vec3 {
        auto result = _mm_mul_ps((__m128) v, _mm_set_ps1(multiplier));
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
        return *reinterpret_cast<vec3 *>(&result);
    }

    inline auto operator*(float multiplier, const vec3 &v) -> vec3 { return operator*(v, multiplier); }

    inline auto dot(const vec3 &v1, const vec3 &v2) -> float {
        auto tmp = _mm_mul_ps((__m128) v1, (__m128) v2);
        auto *aux = reinterpret_cast<float *>(&tmp);
        return aux[1] + aux[2] + aux[3];
    }


    struct vec4 {
        float x, y, z, w;

    public:
        vec4() : x(1.0f), y(1.0f), z(1.0f), w(1.0f) {}

        vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        vec4(const vec3 &v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

        auto operator-(const vec4 &v) const -> vec4 {
            auto result = _mm_sub_ps((__m128) *this, (__m128) v);
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec4 *>(&result);
        }

        auto operator+(const vec4 &v) const -> vec4 {
            //return vec4(x + v.x, y + v.y, z + v.z, w + v.w);

            auto result = _mm_add_ps((__m128) *this, (__m128) v);
            result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 1, 2, 3));
            return *reinterpret_cast<vec4 *>(&result);
        }

        auto operator*(const mat4x4 &mat) const -> vec4;

        explicit operator __m128() const {
            return *reinterpret_cast<const __m128 *>(&x);
        }

        auto operator[](int index) -> float & {
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

        auto operator[](int index) const -> float {
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

        auto operator==(const vec4 &other) const -> bool {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }
    };

    inline auto dot(const vec4 &v1, const vec4 &v2) -> float {
        auto tmp = _mm_mul_ps((__m128) v1, (__m128) v2);
        auto *aux = reinterpret_cast<float *>(&tmp);
        return aux[0] + aux[1] + aux[2] + aux[3];
    }


    inline auto cross(const vec3 &a, const vec3 &b) -> vec3 {
        __m128 a_tmp(a);
        a_tmp = _mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 b_tmp(b);
        b_tmp = _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 a_yzx = _mm_shuffle_ps(a_tmp, a_tmp, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 b_yzx = _mm_shuffle_ps(b_tmp, b_tmp, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 c = _mm_sub_ps(_mm_mul_ps(a_tmp, b_yzx), _mm_mul_ps(a_yzx, b_tmp));
        c = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
        auto *tmp = reinterpret_cast<float *>(&c);
        return vec3(tmp[0], tmp[1], tmp[2]);
    }


    inline auto operator*(const vec4 &v, float multiplier) -> vec4 {
        auto result = _mm_mul_ps((__m128) v, _mm_set_ps1(multiplier));
        return *reinterpret_cast<vec4 *>(&result);
    }

    inline auto operator*(float multiplier, const vec4 &v) -> vec4 { return operator*(v, multiplier); }


    struct mat4x4 {
        vec4 value[4];

    public:
        mat4x4() : value{vec4(1.0f, 0.0f, 0.0f, 0.0f),
                         vec4(0.0f, 1.0f, 0.0f, 0.0f),
                         vec4(0.0f, 0.0f, 1.0f, 0.0f),
                         vec4(0.0f, 0.0f, 0.0f, 1.0f)} {}

        mat4x4(const vec4 &row1, const vec4 &row2, const vec4 &row3, const vec4 &row4) : value{row1, row2, row3,
                                                                                               row4} {}

        explicit mat4x4(float init) : value{vec4(init, 0.0f, 0.0f, 0.0f),
                                            vec4(0.0f, init, 0.0f, 0.0f),
                                            vec4(0.0f, 0.0f, init, 0.0f),
                                            vec4(0.0f, 0.0f, 0.0f, init)} {}


        auto operator[](int index) -> vec4 & {
            assert(index >= 0 && index < 4);
            return value[index];
        }

        auto operator[](int index) const -> vec4 {
            assert(index >= 0 && index < 4);
            return value[index];
        }
    };

    typedef mat4x4 mat4;


    inline auto operator*(const mat4x4 &a, const mat4x4 &b) -> mat4x4 {
        const vec4 &aRow0 = a[0];
        const vec4 &aRow1 = a[1];
        const vec4 &aRow2 = a[2];
        const vec4 &aRow3 = a[3];
        const vec4 &bRow0 = b[0];
        const vec4 &bRow1 = b[1];
        const vec4 &bRow2 = b[2];
        const vec4 &bRow3 = b[3];

        return {
                (aRow0[0] * bRow0) + (aRow0[1] * bRow1) + (aRow0[2] * bRow2) + (aRow0[3] * bRow3),
                (aRow1[0] * bRow0) + (aRow1[1] * bRow1) + (aRow1[2] * bRow2) + (aRow1[3] * bRow3),
                (aRow2[0] * bRow0) + (aRow2[1] * bRow1) + (aRow2[2] * bRow2) + (aRow2[3] * bRow3),
                (aRow3[0] * bRow0) + (aRow3[1] * bRow1) + (aRow3[2] * bRow2) + (aRow3[3] * bRow3)
        };
    }


    inline auto vec4::operator*(const mat4x4 &mat) const -> vec4 {
        return (x * mat[0]) + (y * mat[1]) + (z * mat[2]) + (w * mat[3]);
    }


    inline auto radians(double degrees) -> double { return degrees * PI_RAD; }

    inline auto magnitude(vec3 v) -> float {
        __m128 aux(v);
        aux = _mm_mul_ps(aux, aux);
        auto *tmp = reinterpret_cast<float *>(&aux);
        return sqrtf(tmp[1] + tmp[2] + tmp[3]);
    }

    inline auto normalize(vec3 v) -> vec3 {
        float m = magnitude(v);
        assert(m > 0);
        return v / m;
    }

    inline auto rotate(mat4x4 mat, float angle, vec3 axis) -> mat4x4 {
        float c = std::cos(angle);
        float s = std::sin(angle);
        axis = normalize(axis);
        vec3 temp((1.0f - c) * axis);

        mat4x4 rotate(0);
        rotate[0][0] = axis[0] * temp[0] + c;
        rotate[1][0] = axis[0] * temp[1] - (axis[2] * s);
        rotate[2][0] = axis[0] * temp[2] + (axis[1] * s);

        rotate[0][1] = axis[1] * temp[0] + (axis[2] * s);
        rotate[1][1] = axis[1] * temp[1] + c;
        rotate[2][1] = axis[1] * temp[2] - (axis[0] * s);

        rotate[0][2] = axis[2] * temp[0] - (axis[1] * s);
        rotate[1][2] = axis[2] * temp[1] + (axis[0] * s);
        rotate[2][2] = axis[2] * temp[2] + c;
        rotate[3] = mat[3];

        //TODO: Might be necessary to flip the multiplication operands
        // when input matrices get more complicated since all my matrices
        // are transposed. This is to preserve the normal multiplication
        // order in shaders since my matrices are row major
        return rotate * mat;
    }


    inline auto lookAt(vec3 cameraPos, vec3 cameraTarget, vec3 up) -> mat4x4 {
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

    /* perspective matrix for Right Hand coordinate system with 0 to 1 depth clip space */
    inline auto perspectiveRH_ZO(float fovY, float aspectRatio, float zNear, float zFar) -> mat4x4 {
        assert(aspectRatio != 0);
        assert(zFar != zNear);

        float tanHalfFovY = std::tan(fovY / 2.0f);
        float negative_range = zNear - zFar;

        mat4x4 perspectiveMat(0);
        perspectiveMat[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
        perspectiveMat[1][1] = 1.0f / (tanHalfFovY);
        perspectiveMat[2][2] = zFar / negative_range;
        perspectiveMat[2][3] = -1.0f;
        perspectiveMat[3][2] = (zFar * zNear) / negative_range;
        return perspectiveMat;
    }

    /* perspective matrix for Right Hand coordinate system with -1 to 1 depth clip space */
    inline auto perspectiveRH_NO(float fovY, float aspectRatio, float zNear, float zFar) -> mat4x4 {
        assert(aspectRatio != 0);
        assert(zFar != zNear);

        float tanHalfFovY = std::tan(fovY / 2.0f);
        float range = zFar - zNear;

        mat4x4 perspectiveMat(0);
        perspectiveMat[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
        perspectiveMat[1][1] = 1.0f / (tanHalfFovY);
        perspectiveMat[2][2] = -(zFar + zNear) / range;
        perspectiveMat[3][2] = -1.0f;
        perspectiveMat[2][3] = (-2.0f * zFar * zNear) / range;
        return perspectiveMat;
    }

    inline auto perspective(float fovY, float aspectRatio, float zNear, float zFar) -> mat4x4 {
#ifdef MTH_RIGHT_HAND_SYSTEM
#ifdef MTH_DEPTH_ZERO_TO_ONE
        return perspectiveRH_ZO(fovY, aspectRatio, zNear, zFar);
#else
        return perspectiveRH_NO(fovY, aspectRatio, zNear, zFar);
#endif
#else
#ifdef MTH_DEPTH_ZERO_TO_ONE
        static_assert(false, "perspectiveLH_ZO not implemented yet");
#else
        static_assert(false, "perspectiveLH_NO not implemented yet");
#endif
#endif
    }

}


#endif //VULKAN_MATHLIB_H
