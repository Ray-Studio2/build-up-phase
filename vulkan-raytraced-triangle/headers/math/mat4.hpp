#pragma once

#include "math/vec3.hpp"

class Mat4 {
    private:
        class Row4 {
            public:
                Row4();
                Row4(const Row4&);
                Row4(const float&, const float&, const float&, const float&);
                ~Row4() noexcept;

                Row4& operator=(const Row4&);

                float& operator[](const unsigned int&);
                const float& operator[](const unsigned int&) const;

            private:
                float mData[4]{ };
        };

    public:
        Mat4();
        Mat4(const Mat4&);
        Mat4(const Row4&, const Row4&, const Row4&, const Row4&);
        ~Mat4() noexcept;

        Mat4& operator=(const Mat4&);

        Row4& operator[](const unsigned int&);
        const Row4& operator[](const unsigned int&) const;

        Mat4& operator*=(const Mat4&);

        Mat4 operator*(const Mat4&) const;

        Mat4 transpose() const;

        static Mat4 identity() noexcept;
        static Mat4 translate(const Vec3&) noexcept;
        static Mat4 scale(const Vec3&) noexcept;
        static Mat4 rotate(const Vec3&, const float&) noexcept;
        static Mat4 view(const Vec3&, const Vec3&) noexcept;
        static Mat4 projection(const float&, const float&, const float&, const float&) noexcept;

    private:
        Row4 mRow[4];
};