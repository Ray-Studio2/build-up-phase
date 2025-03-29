#include "components/transform.hpp"

Transform::Transform() { }
Transform::~Transform() noexcept { }

void Transform::pos(const Vec3& pos) { mPos = pos; }
void Transform::scale(const Vec3& scale) { mScale = scale; }
void Transform::angle(const Vec3& angle) { mAngle = angle; }

Vec3 Transform::pos() const { return mPos; }
Vec3 Transform::scale() const { return mScale; }
Vec3 Transform::angle() const { return mAngle; }