#pragma once

#include "math/vec3.hpp"

class Camera {
    Camera(const Camera&) = delete;
    Camera(Camera&&) noexcept = delete;

    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) noexcept = delete;

    public:
        Camera();
        ~Camera() noexcept;

        void pos(const Vec3&);
        void dir(const Vec3&);
        void sensitivity(const float&);

        Vec3 pos() const;
        Vec3 dir() const;
        float sensitivity() const;

    protected:
        Vec3 mPos;
        Vec3 mLook;

        float mSens{ 0.5f };
};