#include "math/vec4.hpp"

/* Vec4 */
Vec4::Vec4() { }
Vec4::Vec4(const Vec4& other) { *this = other; }
Vec4::Vec4(const float& v1, const float& v2, const float& v3, const float& v4)
    : data{v1, v2, v3, v4} { }
Vec4::~Vec4() noexcept { }

Vec4& Vec4::operator=(const Vec4& other) {
    x = other.x;
    y = other.y;
    z = other.z;
    w = other.w;

    return *this;
}