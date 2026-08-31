#pragma once
#include "FireCell.hpp"
#include "Weather.hpp"
#include "Terrain.hpp"
#include <vector>
#include <functional>

namespace Fireline {

class FireSimulation {
public:
    FireSimulation(int width=256, int height=256, float cellSize=2.0f)
        : w(width), h(height), cellSize(cellSize), terrain(width,height,cellSize) {
        cells.resize(w*h);
        initializeVegetation();
    }

    void initializeVegetation() {
        for(int y=0;y<h;y++){
            for(int x=0;x<w;x++){
                Vec3 wp = terrain.gridToWorld(x,y);
                // Determine vegetation based on height and position
                float height = terrain.getHeight(x,y);
                float slope = terrain.getSlopeDegrees(x,y);
                VegetationType vt;

                if(height < 30) {
                    vt = (randFloat()<0.7f) ? VegetationType::GRASS_DRY : VegetationType::SHRUB_LOW;
                } else if(height < 60) {
                    float r = randFloat();
                    if(r<0.3f) vt = VegetationType::SHRUB_DENSE;
                    else if(r<0.6f) vt = VegetationType::GRASS_GREEN;
                    else vt = VegetationType::UNDERGROWTH;
                } else if(slope > 30) {
                    vt = VegetationType::BRUSH_DEAD;
                } else {
                    float r = randFloat();
                    if(r<0.25f) vt = VegetationType::PINE_MATURE;
                    else if(r<0.5f) vt = VegetationType::PINE_YOUNG;
                    else if(r<0.75f) vt = VegetationType::OAK;
                    else vt = VegetationType::EUCALYPTUS;
                }
                // Central village area less dense
                float dx = x - w*0.5f, dy = y - h*0.5f;
                if(std::sqrt(dx*dx+dy*dy) < 20) {
                    vt = VegetationType::GRASS_GREEN;
                }

                cells[y*w+x] = FireCell(x,y,vt,wp);
            }
        }
    }

    FireCell& getCell(int x,int y) {
        static FireCell dummy;
        if(x<0||x>=w||y<0||y>=h) return dummy;
        return cells[y*w+x];
    }
    const FireCell& getCell(int x,int y) const {
        if(x<0||x>=w||y<0||y>=h) {
            static FireCell dummy;
            return dummy;
        }
        return cells[y*w+x];
    }

    void ignite(int x,int y, float initialHeat=500.0f) {
        if(x<0||x>=w||y<0||y>=h) return;
        auto& c = cells[y*w+x];
        if(c.fuelRemaining > 0.1f && !c.hasFirebreak) {
            c.applyHeat(initialHeat);
            c.state = FireState::COMBUSTION;
            c.intensity = 200.0f;
        }
    }

    void igniteWorld(Vec3 worldPos, float radius=5.0f) {
        Vec3 grid = terrain.worldToGrid(worldPos);
        int gx = (int)grid.x, gz = (int)grid.z;
        int radCells = (int)(radius / cellSize);
        for(int dy=-radCells; dy<=radCells; ++dy){
            for(int dx=-radCells; dx<=radCells; ++dx){
                if(dx*dx+dy*dy <= radCells*radCells) {
                    ignite(gx+dx, gz+dy, 400+randRange(0,300));
                }
            }
        }
    }

    void applyWater(int x,int y,float liters,float radius=1.0f) {
        int rad = (int)(radius / cellSize);
        if(rad<1) rad=1;
        for(int dy=-rad; dy<=rad; ++dy){
            for(int dx=-rad; dx<=rad; ++dx){
                int nx=x+dx, ny=y+dy;
                if(nx<0||nx>=w||ny<0||ny>=h) continue;
                float dist = std::sqrt(float(dx*dx+dy*dy));
                float factor = 1.0f - (dist/(rad+1));
                if(factor>0) {
                    cells[ny*w+nx].applyWater(liters*factor);
                }
            }
        }
    }

    void applyWaterWorld(Vec3 worldPos,float liters,float radius=3.0f){
        Vec3 grid = terrain.worldToGrid(worldPos);
        applyWater((int)grid.x,(int)grid.z,liters,radius);
    }

    void createFirebreak(int x,int y,int width=2){
        for(int dy=-width; dy<=width; ++dy){
            for(int dx=-width; dx<=width; ++dx){
                int nx=x+dx, ny=y+dy;
                if(nx<0||nx>=w||ny<0||ny>=h) continue;
                auto& c = cells[ny*w+nx];
                c.hasFirebreak = true;
                c.fuelRemaining = 0.0f;
                c.flammability = 0.0f;
            }
        }
    }

    void createFirebreakWorld(Vec3 worldPos,float radius=4.0f){
        Vec3 grid = terrain.worldToGrid(worldPos);
        int rad = (int)(radius/cellSize);
        createFirebreak((int)grid.x,(int)grid.z,rad);
    }

    void update(float dt, const WeatherState& weather) {
        time += dt;

        // Update each cell
        for(auto& cell : cells) {
            cell.update(dt, weather.temperature, weather.humidity);
        }

        // Fire spread - process combustion cells
        // We collect spread events to avoid immediate propagation in same frame affecting neighbors
        struct SpreadEvent {int sx,sy,dx,dy; float heat;};
        std::vector<SpreadEvent> spreads;
        spreads.reserve(1024);

        for(int y=1;y<h-1;y++){
            for(int x=1;x<w-1;x++){
                const auto& cell = cells[y*w+x];
                if(cell.state != FireState::COMBUSTION) continue;
                if(cell.intensity < 10.0f) continue;

                // Wind influence
                Vec2 windDir = weather.windDirection;
                float windSpeed = weather.getWindSpeedMS();

                // Check 8 neighbors
                for(int dy=-1; dy<=1; ++dy){
                    for(int dx=-1; dx<=1; ++dx){
                        if(dx==0 && dy==0) continue;
                        int nx=x+dx, ny=y+dy;
                        auto& ncell = cells[ny*w+nx];
                        if(!ncell.canIgnite()) continue;

                        Vec2 spreadDir{(float)dx,(float)dy};
                        spreadDir = spreadDir.normalized();

                        // Wind factor: alignment with wind
                        float windAlignment = windDir.dot(spreadDir);
                        float windFactor = 1.0f + windAlignment * (windSpeed * 0.3f);
                        windFactor = std::max(0.1f, windFactor);

                        // Slope factor
                        float slopeFactor = terrain.getSlopeFactor(x,y,spreadDir);

                        // Distance factor
                        float dist = (dx==0||dy==0) ? 1.0f : 1.414f;
                        float distanceFactor = 1.0f / dist;

                        // Vegetation factor
                        float vegFactor = ncell.vegProfile.combustionSpeed / 2.0f;

                        // Humidity factor
                        float humidityFactor = 1.0f - (ncell.moisture/100.0f)*0.8f;

                        // Temperature factor
                        float tempFactor = clamp((weather.temperature - 15)/30.0f, 0.2f, 2.0f);

                        // Combined probability
                        float baseProb = 0.02f * dt * 10.0f; // per second
                        float spreadProb = baseProb * windFactor * slopeFactor * distanceFactor * vegFactor * humidityFactor * tempFactor * (cell.intensity/100.0f);

                        // Ember spotting for high intensity + wind + eucalyptus/pine
                        if(cell.vegProfile.emberProduction > 0.5f && windSpeed > 5.0f && cell.intensity > 300) {
                            if(randFloat() < 0.001f * cell.vegProfile.emberProduction) {
                                int jumpX = (int)(windDir.x * randRange(5,20));
                                int jumpY = (int)(windDir.y * randRange(5,20));
                                int ex = x + jumpX, ey = y + jumpY;
                                if(ex>=0&&ex<w&&ey>=0&&ey<h) {
                                    auto& ecell = cells[ey*w+ex];
                                    if(ecell.canIgnite() && randFloat() < 0.3f) {
                                        spreads.push_back(SpreadEvent{x,y,ex,ey, 200.0f});
                                    }
                                }
                            }
                        }

                        if(randFloat() < spreadProb) {
                            float heatTransfer = cell.intensity * 0.5f * distanceFactor;
                            spreads.push_back(SpreadEvent{x,y,nx,ny,heatTransfer});
                        }
                    }
                }
            }
        }

        // Apply spread events
        for(auto& ev : spreads) {
            auto& target = cells[ev.dy*w+ev.dx];
            target.applyHeat(ev.heat);
        }

        // Update stats
        updateStats();
    }

    struct Stats {
        int normal=0, heating=0, smoke=0, combustion=0, burned=0, mopup=0;
        int totalBurning=0;
        float totalIntensity=0;
        float burnedAreaHa=0;
    } stats;

    void updateStats() {
        stats = {};
        for(auto& c : cells) {
            switch(c.state){
                case FireState::NORMAL: stats.normal++; break;
                case FireState::HEATING: stats.heating++; break;
                case FireState::SMOKE: stats.smoke++; break;
                case FireState::COMBUSTION: stats.combustion++; stats.totalBurning++; stats.totalIntensity+=c.intensity; break;
                case FireState::BURNED: stats.burned++; break;
                case FireState::MOPUP: stats.mopup++; stats.totalBurning++; break;
            }
        }
        stats.burnedAreaHa = (stats.burned + stats.mopup) * cellSize * cellSize / 10000.0f;
    }

    int width() const { return w; }
    int height() const { return h; }
    float getCellSize() const { return cellSize; }
    const Terrain& getTerrain() const { return terrain; }
    Terrain& getTerrain() { return terrain; }

    // For rendering / gameplay queries
    std::vector<FireCell>& getCells() { return cells; }
    const std::vector<FireCell>& getCells() const { return cells; }

    float time = 0;

private:
    int w,h;
    float cellSize;
    Terrain terrain;
    std::vector<FireCell> cells;

    float randFloat() { return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
