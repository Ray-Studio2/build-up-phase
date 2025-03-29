#pragma once

#include "math/vec3.hpp"

class Transform {
    Transform(const Transform&) = delete;
    Transform(Transform&&) noexcept = delete;

    Transform& operator=(const Transform&) = delete;
    Transform& operator=(Transform&&) noexcept = delete;

    public:
        Transform();
        ~Transform() noexcept;

        void pos(const Vec3&);
        void scale(const Vec3&);
        void angle(const Vec3&);

        Vec3 pos() const;
        Vec3 scale() const;
        Vec3 angle() const;

    private:
        Vec3 mPos;
        Vec3 mScale;
        Vec3 mAngle;
};