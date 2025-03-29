#include "components/camera.hpp"

Camera::Camera() { }
Camera::~Camera() noexcept { }

void Camera::pos(const Vec3& pos) { mPos = pos; }
void Camera::dir(const Vec3& dir) { mLook = dir; }
void Camera::sensitivity(const float& sens) { mSens = sens; }

Vec3 Camera::pos() const { return mPos; }
Vec3 Camera::dir() const { return mLook; }
float Camera::sensitivity() const { return mSens; }