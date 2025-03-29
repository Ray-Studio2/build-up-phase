#pragma once

class Vec4 {
    public:
        Vec4();
        Vec4(const Vec4&);
        Vec4(const float&, const float&, const float&, const float&);
        ~Vec4() noexcept;

        Vec4& operator=(const Vec4&);

    public:
        union {
            float data[4]{ };

            struct {
                float x;
                float y;
                float z;
                float w;
            };
            struct {
                float r;
                float g;
                float b;
                float a;
            };
        };
};