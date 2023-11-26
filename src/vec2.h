#pragma once

#include <iostream>
#include <cmath>

struct vec2 {
    vec2()
        : x(0), y(0)
    {}

    vec2(float x, float y) 
        : x(x), y(y)
    {}

    float magnitude() {
        return std::sqrt(x*x + y*y);
    }

    vec2 normalized() {
        float mag = std::sqrt(x*x + y*y);
        return vec2(x/mag, y/mag);
    }

    vec2 perpendicular() {
        return vec2(-y, x);
    }

    float x, y;
};

vec2 operator+(const vec2& lhs, const vec2& rhs) {
    return vec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

void operator+=(vec2& lhs, const vec2& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
}

vec2 operator-(const vec2& lhs, const vec2& rhs) {
    return vec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

vec2 operator*(const vec2& lhs, const vec2& rhs) {
    return vec2(lhs.x * rhs.x, lhs.y * rhs.y);
}

vec2 operator*(const float lhs, const vec2& rhs) {
    return vec2(lhs * rhs.x, lhs * rhs.y);
}

vec2 operator*(const vec2& lhs, const float rhs) {
    return vec2(lhs.x * rhs, lhs.y * rhs);
}

vec2 operator/(const vec2& lhs, const vec2& rhs) {
    return vec2(lhs.x / rhs.x, lhs.y / rhs.y);
}

std::ostream& operator<<(std::ostream& lhs, const vec2& rhs) {
    lhs << "(" << rhs.x << "," << rhs.y << ")";
    return lhs;
}