#ifndef _VEC2_H_
#define _VEC2_H_

#include <math.h>
#include <stdio.h>

#define _vec2_buf_len 16

typedef struct {
    float x, y;
} Vec2;

float vec2_magnitude(Vec2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

#define eps 0.000001f

Vec2 vec2_normalized(Vec2 v) {
    float mag = sqrt(v.x * v.x + v.y * v.y);
    // Avoid dividing by zero. If vector is (0, 0) do nothing
    if (fabs(v.x - 0.f) < eps && fabs(v.y - 0.f) < eps) {
        return v;
    }
    return (Vec2){ v.x/mag, v.y/mag };
}


Vec2 vec2_perpendicular(Vec2 v) {
    return (Vec2){ -v.y, v.x };
}


Vec2 vec2_add(Vec2 v, Vec2 vv) {
    return (Vec2){ v.x+vv.x, v.y+vv.y };
}

Vec2 vec2_addi(Vec2 v, int i) {
    return (Vec2){ v.x+i, v.y+i };
}

Vec2 vec2_addf(Vec2 v, int f) {
    return (Vec2){ v.x+f, v.y+f };
}


Vec2 vec2_sub(Vec2 v, Vec2 vv) {
    return (Vec2){ v.x-vv.x, v.y-vv.y };
}

Vec2 vec2_subf(Vec2 v, float f) {
    return (Vec2){ v.x-f, v.y-f };
}


Vec2 vec2_mul(Vec2 v, Vec2 vv) {
    return (Vec2){ v.x*vv.x, v.y*vv.y };
}

Vec2 vec2_muli(Vec2 v, int i) {
    return (Vec2){ v.x*i, v.y*i };
}

Vec2 vec2_mulf(Vec2 v, float f) {
    return (Vec2){ v.x*f, v.y*f };
}


Vec2 vec2_div(Vec2 v, Vec2 vv) {
    return (Vec2){ v.x/vv.x, v.y/vv.y };
}

Vec2 vec2_divf(Vec2 v, float f) {
    return (Vec2){ v.x/f, v.y/f };
}

Vec2 vec2_fdiv(float f, Vec2 v) {
    return (Vec2){ f/v.x, f/v.y };
}


char* vec2_str(Vec2 v) {
    static char _vec2_buf[_vec2_buf_len];

    snprintf(_vec2_buf, _vec2_buf_len, "(%.2f, %.2f)", v.x, v.y);
    return _vec2_buf;
}

#endif // _VEC2_H_