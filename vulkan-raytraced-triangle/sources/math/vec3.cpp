#include "math/vec3.hpp"

#include <cmath>

// TODO: assert "operator/"

/* Vec3 */
Vec3::Vec3() { }
Vec3::Vec3(const Vec3& other) { *this = other; }
Vec3::Vec3(const float& v1, const float& v2, const float& v3)
    : data{v1, v2, v3} { }
Vec3::~Vec3() noexcept { }

Vec3& Vec3::operator=(const Vec3& other) {
    x = other.x;
    y = other.y;
    z = other.z;

    return *this;
}

Vec3& Vec3::operator+=(const Vec3& other) {
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;
}
Vec3& Vec3::operator-=(const Vec3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
}

Vec3 Vec3::operator+(const Vec3& other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z
    };
}
Vec3 Vec3::operator-(const Vec3& other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z
    };
}
Vec3 Vec3::operator*(const float& scalar) const {
    return {
        x * scalar,
        y * scalar,
        z * scalar
    };
}
Vec3 Vec3::operator/(const float& scalar) const {
    return {
        x / scalar,
        y / scalar,
        z / scalar
    };
}

float Vec3::dot(const Vec3& other) const {
    return {
        x * other.x +
        y * other.y +
        z * other.z
    };
}
Vec3 Vec3::cross(const Vec3& other) const {
    return {
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    };
}

/* Vec3 static */
float Vec3::length(const Vec3& v) {
    return std::sqrt(
        v.x * v.x +
        v.y * v.y +
        v.z * v.z
    );
}
Vec3 Vec3::normalize(const Vec3& v) { return { v / Vec3::length(v) }; }
float Vec3::dot(const Vec3& v1, const Vec3& v2) {
    return {
        v1.x * v2.x +
        v1.y * v2.y +
        v1.z * v2.z
    };
}
Vec3 Vec3::cross(const Vec3& v1, const Vec3& v2) {
    return {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
}