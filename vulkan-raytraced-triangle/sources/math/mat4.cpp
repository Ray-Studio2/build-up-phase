#include "math/mat4.hpp"

/* Row4 */
Mat4::Row4::Row4() { }
Mat4::Row4::Row4(const Row4& other) { *this = other; }
Mat4::Row4::Row4(const float& v1, const float& v2, const float& v3, const float& v4)
    : mData{v1, v2, v3, v4} { }
Mat4::Row4::~Row4() noexcept { }

Mat4::Row4& Mat4::Row4::operator=(const Row4& other) {
    mData[0] = other.mData[0];
    mData[1] = other.mData[1];
    mData[2] = other.mData[2];
    mData[3] = other.mData[3];

    return *this;
}

float& Mat4::Row4::operator[](const unsigned int& idx) { return mData[idx]; }
const float& Mat4::Row4::operator[](const unsigned int& idx) const { return mData[idx]; }

/* Mat4 */
Mat4::Mat4() { }
Mat4::Mat4(const Mat4& other) { *this = other; }
Mat4::Mat4(const Row4& r1, const Row4& r2, const Row4& r3, const Row4& r4)
    : mRow{r1, r2, r3, r4} { }
Mat4::~Mat4() noexcept { }

Mat4& Mat4::operator=(const Mat4& other) {
    mRow[0] = other.mRow[0];
    mRow[1] = other.mRow[1];
    mRow[2] = other.mRow[2];
    mRow[3] = other.mRow[3];

    return *this;
}

Mat4::Row4& Mat4::operator[](const unsigned int& idx) { return mRow[idx]; }
const Mat4::Row4& Mat4::operator[](const unsigned int& idx) const { return mRow[idx]; }

Mat4& Mat4::operator*=(const Mat4& other) { return (*this = ((*this) * other)); }

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;

    for (unsigned int i = 0; i < 4; ++i) {
        for (unsigned int j = 0; j < 4; ++j) {
            float sum{ };

            for (unsigned int k = 0; k < 4; ++k)
                sum += (mRow[i][k] * other.mRow[k][j]);

            result[i][j] = sum;
        }
    }

    return result;
}

Mat4 Mat4::transpose() const {
    return {
        { mRow[0][0], mRow[1][0], mRow[2][0], mRow[3][0] },
        { mRow[0][1], mRow[1][1], mRow[2][1], mRow[3][1] },
        { mRow[0][2], mRow[1][2], mRow[2][2], mRow[3][2] },
        { mRow[0][3], mRow[1][3], mRow[2][3], mRow[3][3] }
    };
}

/* Mat4 static */
Mat4 Mat4::identity() noexcept {
    return {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
}
Mat4 Mat4::translate(const Vec3& pos) noexcept {
    return {
        { 1.0f, 0.0f, 0.0f, pos.x },
        { 0.0f, 1.0f, 0.0f, pos.y },
        { 0.0f, 0.0f, 1.0f, pos.z },
        { 0.0f, 0.0f, 0.0f,  1.0f }
    };
}
Mat4 Mat4::scale(const Vec3& scale) noexcept {
    return {
        { scale.x,    0.0f,    0.0f, 0.0f },
        {    0.0f, scale.y,    0.0f, 0.0f },
        {    0.0f,    0.0f, scale.z, 0.0f },
        {    0.0f,    0.0f,    0.0f, 1.0f }
    };
}
Mat4 Mat4::view(const Vec3& pos, const Vec3& look) noexcept {
    const Vec3 up { 0.0f, 1.0f, 0.0f };

    Vec3 z = Vec3::normalize(look - pos);
    Vec3 x = Vec3::normalize(z.cross(up));
    Vec3 y = x.cross(z);

    return {
        {  x.x,  x.y,  x.z, -x.dot(pos) },
        {  y.x,  y.y,  y.z, -y.dot(pos) },
        { -z.x, -z.y, -z.z,  z.dot(pos) },
        { 0.0f, 0.0f, 0.0f,        1.0f }
    };
}
Mat4 Mat4::projection(const float& fovY, const float& aspect, const float& near, const float& far) noexcept {
    
}