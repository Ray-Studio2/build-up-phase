#pragma once

class Vec2 {
    public:
        Vec2();
        Vec2(const Vec2&);
        Vec2(const float&, const float&);
        ~Vec2() noexcept;

        Vec2& operator=(const Vec2&);

        Vec2& operator*=(const float&);

    public:
        union {
            float data[2]{ };

            struct {
                float x;
                float y;
            };
            struct {
                float u;
                float v;
            };
        };
};