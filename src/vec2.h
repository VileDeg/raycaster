#ifndef VEC2_H
#define VEC2_H

//#include <math.h>
#include <stdio.h>

#define VEC2_BUF_LEN 16
#define VEC2_EPS 0.000001f

// Defines the struct: e.g., Vec2f or Vec2i
#define VEC2_DEF(_P, _Type) \
    typedef struct { \
        _Type x, y; \
    } Vec2##_P;

// Defines the implementation functions with unique names per type
#define VEC2_IMPL(_P, _Type, _C, _TypeC, _Format) \
\
    static inline _Type vec2##_P##_magnitude(Vec2##_P v) { \
        return (_Type)sqrt((double)(v.x * v.x + v.y * v.y)); \
    } \
\
    static inline Vec2##_P vec2##_P##_normalized(Vec2##_P v) { \
        double mag = sqrt((double)(v.x * v.x + v.y * v.y)); \
        if (mag < VEC2_EPS) return v; \
        return (Vec2##_P){ (_Type)(v.x / mag), (_Type)(v.y / mag) }; \
    } \
\
    static inline Vec2##_P vec2##_P##_perpendicular(Vec2##_P v) { \
        return (Vec2##_P){ -v.y, v.x }; \
    } \
\
    static inline Vec2##_P vec2##_P##_inv(Vec2##_P v) { \
        return (Vec2##_P){ -v.x, -v.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_add(Vec2##_P v, Vec2##_P vv) { \
        return (Vec2##_P){ v.x + vv.x, v.y + vv.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_adds(Vec2##_P v, _Type s) { \
        return (Vec2##_P){ v.x + s, v.y + s }; \
    } \
\
    static inline Vec2##_P vec2##_P##_sub(Vec2##_P v, Vec2##_P vv) { \
        return (Vec2##_P){ v.x - vv.x, v.y - vv.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_subs(Vec2##_P v, _Type s) { \
        return (Vec2##_P){ v.x - s, v.y - s }; \
    } \
\
    static inline Vec2##_P vec2##_P##_mul(Vec2##_P v, Vec2##_P vv) { \
        return (Vec2##_P){ v.x * vv.x, v.y * vv.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_muls(Vec2##_P v, _Type s) { \
        return (Vec2##_P){ v.x * s, v.y * s }; \
    } \
\
    static inline Vec2##_P vec2##_P##_div(Vec2##_P v, Vec2##_P vv) { \
        return (Vec2##_P){ v.x / vv.x, v.y / vv.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_divs(Vec2##_P v, _Type s) { \
        return (Vec2##_P){ v.x / s, v.y / s }; \
    } \
\
    static inline Vec2##_C vec2##_P##_to_##_C(Vec2##_P v) { \
        return (Vec2##_C){ (_TypeC)v.x, (_TypeC)v.y }; \
    } \
\
    static inline Vec2##_P vec2##_P##_from_##_C(Vec2##_C v) { \
        return (Vec2##_P){ (_Type)v.x, (_Type)v.y }; \
    } \
\
    static inline const char* vec2##_P##_str(Vec2##_P v) { \
        static char buf[VEC2_BUF_LEN]; \
        snprintf(buf, VEC2_BUF_LEN, _Format, v.x, v.y); \
        return buf; \
    }

#endif // VEC2_H