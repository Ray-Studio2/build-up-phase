#pragma once

class Vec3 {
    public:
        Vec3();
        Vec3(const Vec3&);
        Vec3(const float&, const float&, const float&);
        ~Vec3() noexcept;

        Vec3& operator=(const Vec3&);

        Vec3& operator+=(const Vec3&);
        Vec3& operator-=(const Vec3&);

        Vec3 operator+(const Vec3&) const;
        Vec3 operator-(const Vec3&) const;
        Vec3 operator*(const float&) const;
        Vec3 operator/(const float&) const;

        float dot(const Vec3&) const;
        Vec3 cross(const Vec3&) const;

        static float length(const Vec3&);
        static Vec3 normalize(const Vec3&);

        static float dot(const Vec3&, const Vec3&);
        static Vec3 cross(const Vec3&, const Vec3&);

    public:
        union {
            float data[3]{ };

            struct {
                float x;
                float y;
                float z;
            };
            struct {
                float r;
                float g;
                float b;
            };
            struct {
                float u;
                float v;
                float w;
            };
        };
};