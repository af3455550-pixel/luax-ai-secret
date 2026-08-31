#pragma once
#include "core/Math.hpp"
#include <vector>

namespace Fireline {

class Terrain {
public:
    Terrain(int width=512, int height=512, float cellSize=2.0f)
        : w(width), h(height), cellSize(cellSize) {
        heights.resize(w*h, 0);
        generateProcedural();
    }

    void generateProcedural() {
        // Simple fractal noise for mountains, valleys
        for(int y=0;y<h;y++){
            for(int x=0;x<w;x++){
                float fx = (float)x / w * 4.0f;
                float fy = (float)y / h * 4.0f;
                float h1 = std::sin(fx*2.3f)*std::cos(fy*1.7f)*50;
                float h2 = std::sin(fx*5.1f + fy*2.2f)*15;
                float h3 = std::cos(fx*0.8f)*std::sin(fy*0.9f)*80;
                float valley = std::exp(-std::pow((fx-2.0f),2)*2)* -20;
                float ridge = std::sin(fx*0.5f)*30;
                heights[y*w+x] = h1 + h2 + h3 + valley + ridge + 100;
                // Ensure some flat areas for villages
                if(x>w*0.4 && x<w*0.6 && y>h*0.4 && y<h*0.6) {
                    heights[y*w+x] *= 0.3f;
                    heights[y*w+x] += 20;
                }
            }
        }
    }

    float getHeight(int x,int y) const {
        if(x<0||x>=w||y<0||y>=h) return 0;
        return heights[y*w+x];
    }

    float getHeightWorld(float wx,float wz) const {
        int ix = (int)((wx / cellSize) + w*0.5f);
        int iz = (int)((wz / cellSize) + h*0.5f);
        return getHeight(ix,iz);
    }

    Vec3 getNormal(int x,int y) const {
        float hl = getHeight(x-1,y);
        float hr = getHeight(x+1,y);
        float hd = getHeight(x,y-1);
        float hu = getHeight(x,y+1);
        Vec3 n{hl-hr, 2.0f*cellSize, hd-hu};
        return n.normalized();
    }

    float getSlopeDegrees(int x,int y) const {
        Vec3 n = getNormal(x,y);
        // slope = angle between normal and up
        float dot = clamp(n.dot(Vec3::up()), -1,1);
        float angle = std::acos(dot) * RAD2DEG;
        return angle;
    }

    float getSlopeFactor(int x,int y, const Vec2& fireSpreadDir) const {
        // Slope increases fire spread uphill
        Vec3 n = getNormal(x,y);
        // Project normal to horizontal plane to get uphill direction
        Vec2 uphill{-n.x, -n.z};
        if(uphill.length() < 0.001f) return 1.0f;
        uphill = uphill.normalized();
        float alignment = uphill.dot(fireSpreadDir.normalized());
        float slopeDeg = getSlopeDegrees(x,y);
        // Rothermel slope factor approximation
        float slopeRad = slopeDeg * DEG2RAD;
        float factor = 1.0f + std::tan(slopeRad) * 1.5f * std::max(0.0f, alignment);
        return clamp(factor, 0.2f, 5.0f);
    }

    Vec3 worldToGrid(Vec3 world) const {
        int gx = (int)((world.x / cellSize) + w*0.5f);
        int gz = (int)((world.z / cellSize) + h*0.5f);
        return Vec3{(float)gx, getHeight(gx,gz), (float)gz};
    }

    Vec3 gridToWorld(int gx,int gz) const {
        float wx = (gx - w*0.5f)*cellSize;
        float wz = (gz - h*0.5f)*cellSize;
        return Vec3{wx, getHeight(gx,gz), wz};
    }

    int width() const { return w; }
    int height() const { return h; }
    float getCellSize() const { return cellSize; }

private:
    int w,h;
    float cellSize;
    std::vector<float> heights;
};

} // namespace Fireline
