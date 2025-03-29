#include "math/vec2.hpp"

Vec2::Vec2() { }
Vec2::Vec2(const Vec2& other) { *this = other; }
Vec2::Vec2(const float& v1, const float& v2)
    : data{v1, v2} { }
Vec2::~Vec2() noexcept { }

Vec2& Vec2::operator=(const Vec2& other) {
    x = other.x;
    y = other.y;

    return *this;
}

Vec2& Vec2::operator*=(const float& scalar) {
    x *= scalar;
    y *= scalar;

    return *this;
}