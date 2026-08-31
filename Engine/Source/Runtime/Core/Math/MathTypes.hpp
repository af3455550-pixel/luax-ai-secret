#pragma once

#include <cmath>
#include <iostream>
#include <array>
#include <string>
#include <sstream>
#include <algorithm>

namespace Apex::Math {

    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI / 180.0f;
    constexpr float RAD2DEG = 180.0f / PI;

    struct Vec2 {
        float x{0.0f}, y{0.0f};

        Vec2() = default;
        Vec2(float x_, float y_) : x(x_), y(y_) {}

        Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
        Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
        Vec2 operator*(float s) const { return {x * s, y * s}; }
        Vec2 operator/(float s) const { return {x / s, y / s}; }

        float Length() const { return std::sqrt(x * x + y * y); }
        Vec2 Normalized() const {
            float l = Length();
            return l > 1e-6f ? (*this / l) : Vec2(0, 0);
        }
    };

    struct Vec3 {
        float x{0.0f}, y{0.0f}, z{0.0f};

        Vec3() = default;
        Vec3(float val) : x(val), y(val), z(val) {}
        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
        Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
        Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
        Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

        float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
        Vec3 Cross(const Vec3& o) const {
            return {
                y * o.z - z * o.y,
                z * o.x - x * o.z,
                x * o.y - y * o.x
            };
        }

        float LengthSq() const { return x * x + y * y + z * z; }
        float Length() const { return std::sqrt(LengthSq()); }

        Vec3 Normalized() const {
            float l = Length();
            return l > 1e-6f ? (*this / l) : Vec3(0, 0, 0);
        }

        static Vec3 Zero() { return {0.0f, 0.0f, 0.0f}; }
        static Vec3 One() { return {1.0f, 1.0f, 1.0f}; }
        static Vec3 Up() { return {0.0f, 1.0f, 0.0f}; }
        static Vec3 Forward() { return {0.0f, 0.0f, 1.0f}; }
        static Vec3 Right() { return {1.0f, 0.0f, 0.0f}; }

        std::string ToString() const {
            std::ostringstream ss;
            ss << "(" << x << ", " << y << ", " << z << ")";
            return ss.str();
        }
    };

    struct Vec4 {
        float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};

        Vec4() = default;
        Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
        Vec4(const Vec3& v, float w_ = 1.0f) : x(v.x), y(v.y), z(v.z), w(w_) {}
    };

    struct Quat {
        float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};

        Quat() = default;
        Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

        static Quat Identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

        static Quat FromEuler(float pitch, float yaw, float roll) {
            float p = pitch * DEG2RAD * 0.5f;
            float y = yaw * DEG2RAD * 0.5f;
            float r = roll * DEG2RAD * 0.5f;

            float sinP = std::sin(p); float cosP = std::cos(p);
            float sinY = std::sin(y); float cosY = std::cos(y);
            float sinR = std::sin(r); float cosR = std::cos(r);

            Quat q;
            q.w = cosR * cosP * cosY + sinR * sinP * sinY;
            q.x = sinR * cosP * cosY - cosR * sinP * sinY;
            q.y = cosR * sinP * cosY + sinR * cosP * sinY;
            q.z = cosR * cosP * sinY - sinR * sinP * cosY;
            return q;
        }

        static Quat FromAxisAngle(const Vec3& axis, float angleRadians) {
            float half = angleRadians * 0.5f;
            float s = std::sin(half);
            Vec3 norm = axis.Normalized();
            return { norm.x * s, norm.y * s, norm.z * s, std::cos(half) };
        }

        Vec3 Rotate(const Vec3& v) const {
            Vec3 u(x, y, z);
            float s = w;
            return u * 2.0f * u.Dot(v)
                 + v * (s * s - u.Dot(u))
                 + u.Cross(v) * 2.0f * s;
        }
    };

    struct Mat4 {
        float m[16]{
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };

        Mat4() = default;

        static Mat4 Identity() {
            return Mat4();
        }

        static Mat4 Translation(const Vec3& t) {
            Mat4 res;
            res.m[12] = t.x;
            res.m[13] = t.y;
            res.m[14] = t.z;
            return res;
        }

        static Mat4 Scale(const Vec3& s) {
            Mat4 res;
            res.m[0] = s.x;
            res.m[5] = s.y;
            res.m[10] = s.z;
            return res;
        }

        static Mat4 FromTRS(const Vec3& pos, const Quat& rot, const Vec3& scale) {
            // Rotation matrix from Quat
            float x2 = rot.x + rot.x,  y2 = rot.y + rot.y,  z2 = rot.z + rot.z;
            float xx = rot.x * x2,   xy = rot.x * y2,   xz = rot.x * z2;
            float yy = rot.y * y2,   yz = rot.y * z2,   zz = rot.z * z2;
            float wx = rot.w * x2,   wy = rot.w * y2,   wz = rot.w * z2;

            Mat4 res;
            res.m[0] = (1.0f - (yy + zz)) * scale.x;
            res.m[1] = (xy + wz) * scale.x;
            res.m[2] = (xz - wy) * scale.x;
            res.m[3] = 0.0f;

            res.m[4] = (xy - wz) * scale.y;
            res.m[5] = (1.0f - (xx + zz)) * scale.y;
            res.m[6] = (yz + wx) * scale.y;
            res.m[7] = 0.0f;

            res.m[8] = (xz + wy) * scale.z;
            res.m[9] = (yz - wx) * scale.z;
            res.m[10] = (1.0f - (xx + yy)) * scale.z;
            res.m[11] = 0.0f;

            res.m[12] = pos.x;
            res.m[13] = pos.y;
            res.m[14] = pos.z;
            res.m[15] = 1.0f;
            return res;
        }

        Mat4 operator*(const Mat4& o) const {
            Mat4 res;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    res.m[c * 4 + r] =
                        m[0 * 4 + r] * o.m[c * 4 + 0] +
                        m[1 * 4 + r] * o.m[c * 4 + 1] +
                        m[2 * 4 + r] * o.m[c * 4 + 2] +
                        m[3 * 4 + r] * o.m[c * 4 + 3];
                }
            }
            return res;
        }

        Vec3 TransformPoint(const Vec3& p) const {
            return {
                m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
                m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
                m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]
            };
        }

        static Mat4 Perspective(float fovRad, float aspect, float nearZ, float farZ) {
            Mat4 res;
            std::fill(std::begin(res.m), std::end(res.m), 0.0f);
            float tanHalfFov = std::tan(fovRad * 0.5f);
            res.m[0] = 1.0f / (aspect * tanHalfFov);
            res.m[5] = 1.0f / tanHalfFov;
            res.m[10] = -(farZ + nearZ) / (farZ - nearZ);
            res.m[11] = -1.0f;
            res.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
            res.m[15] = 0.0f;
            return res;
        }

        static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
            Vec3 f = (target - eye).Normalized();
            Vec3 s = f.Cross(up).Normalized();
            Vec3 u = s.Cross(f);

            Mat4 res = Identity();
            res.m[0] = s.x;  res.m[4] = s.y;  res.m[8] = s.z;
            res.m[1] = u.x;  res.m[5] = u.y;  res.m[9] = u.z;
            res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z;
            res.m[12] = -s.Dot(eye);
            res.m[13] = -u.Dot(eye);
            res.m[14] = f.Dot(eye);
            return res;
        }
    };

    struct Transform {
        Vec3 position{0.0f, 0.0f, 0.0f};
        Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
        Vec3 scale{1.0f, 1.0f, 1.0f};

        Mat4 ToMatrix() const {
            return Mat4::FromTRS(position, rotation, scale);
        }
    };

    struct AABB {
        Vec3 min{-0.5f, -0.5f, -0.5f};
        Vec3 max{0.5f, 0.5f, 0.5f};

        Vec3 Center() const { return (min + max) * 0.5f; }
        Vec3 Extents() const { return (max - min) * 0.5f; }

        bool Intersects(const AABB& o) const {
            return (min.x <= o.max.x && max.x >= o.min.x) &&
                   (min.y <= o.max.y && max.y >= o.min.y) &&
                   (min.z <= o.max.z && max.z >= o.min.z);
        }
    };

    struct Ray {
        Vec3 origin{0.0f, 0.0f, 0.0f};
        Vec3 direction{0.0f, 0.0f, 1.0f};

        Vec3 GetPoint(float distance) const {
            return origin + direction * distance;
        }
    };

} // namespace Apex::Math
