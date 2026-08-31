#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Fireline {

constexpr float PI = 3.14159265359f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    Vec2 operator/(float s) const { return {x/s, y/s}; }
    float length() const { return std::sqrt(x*x + y*y); }
    Vec2 normalized() const { float l = length(); return l>0 ? *this/l : Vec2{0,0}; }
    float dot(const Vec2& o) const { return x*o.x + y*o.y; }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }
    Vec3 operator-() const { return {-x,-y,-z}; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float lengthSq() const { return x*x + y*y + z*z; }
    Vec3 normalized() const { float l = length(); return l>1e-6f ? *this/l : Vec3{0,0,0}; }
    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3 cross(const Vec3& o) const { return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x}; }
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b-a)*t; }
    static Vec3 up() { return {0,1,0}; }
    static Vec3 forward() { return {0,0,-1}; }
};

struct Vec4 {
    float x=0,y=0,z=0,w=0;
    Vec4() = default;
    Vec4(float x_,float y_,float z_,float w_) : x(x_),y(y_),z(z_),w(w_) {}
    Vec4(const Vec3& v,float w_) : x(v.x),y(v.y),z(v.z),w(w_) {}
};

struct Mat4 {
    float m[16] = {0};
    Mat4() { identity(); }
    void identity() {
        for(int i=0;i<16;i++) m[i]=0;
        m[0]=m[5]=m[10]=m[15]=1.0f;
    }
    static Mat4 Identity() { Mat4 r; r.identity(); return r; }
    static Mat4 Translation(const Vec3& t) {
        Mat4 r; r.identity();
        r.m[12]=t.x; r.m[13]=t.y; r.m[14]=t.z;
        return r;
    }
    static Mat4 Scale(const Vec3& s) {
        Mat4 r; r.identity();
        r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z;
        return r;
    }
    static Mat4 RotationX(float rad) {
        Mat4 r; r.identity();
        float c=std::cos(rad), s=std::sin(rad);
        r.m[5]=c; r.m[6]=s;
        r.m[9]=-s; r.m[10]=c;
        return r;
    }
    static Mat4 RotationY(float rad) {
        Mat4 r; r.identity();
        float c=std::cos(rad), s=std::sin(rad);
        r.m[0]=c; r.m[2]=-s;
        r.m[8]=s; r.m[10]=c;
        return r;
    }
    static Mat4 RotationZ(float rad) {
        Mat4 r; r.identity();
        float c=std::cos(rad), s=std::sin(rad);
        r.m[0]=c; r.m[1]=s;
        r.m[4]=-s; r.m[5]=c;
        return r;
    }
    static Mat4 Perspective(float fovDeg, float aspect, float nearP, float farP) {
        Mat4 r; for(int i=0;i<16;i++) r.m[i]=0;
        float f = 1.0f / std::tan(fovDeg * DEG2RAD * 0.5f);
        r.m[0]=f/aspect;
        r.m[5]=f;
        r.m[10]=(farP+nearP)/(nearP-farP);
        r.m[11]=-1;
        r.m[14]=(2*farP*nearP)/(nearP-farP);
        return r;
    }
    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center-eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        Mat4 r; r.identity();
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;
        r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12]=-s.dot(eye);
        r.m[13]=-u.dot(eye);
        r.m[14]=f.dot(eye);
        return r;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) {
            res.m[i*4+j]=0;
            for(int k=0;k<4;k++) res.m[i*4+j]+=m[i*4+k]*o.m[k*4+j];
        }
        return res;
    }
    Vec4 operator*(const Vec4& v) const {
        return {
            m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }
};

inline float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
inline float lerp(float a,float b,float t){ return a + (b-a)*t; }
inline float smoothstep(float edge0,float edge1,float x){
    float t = clamp((x-edge0)/(edge1-edge0),0.0f,1.0f);
    return t*t*(3-2*t);
}
inline float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
inline float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }

struct Color {
    float r=1,g=1,b=1,a=1;
    Color()=default;
    Color(float r_,float g_,float b_,float a_=1):r(r_),g(g_),b(b_),a(a_){}
    static Color lerp(const Color& c1,const Color& c2,float t){
        return { ::Fireline::lerp(c1.r,c2.r,t), ::Fireline::lerp(c1.g,c2.g,t), ::Fireline::lerp(c1.b,c2.b,t), ::Fireline::lerp(c1.a,c2.a,t)};
    }
};

} // namespace Fireline
