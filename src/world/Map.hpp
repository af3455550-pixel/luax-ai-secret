#pragma once
#include "core/Math.hpp"
#include "simulation/Terrain.hpp"
#include <vector>
#include <string>

namespace Fireline {

enum class MapFeatureType {
    FOREST_DENSE,
    FOREST_SPARSE,
    MOUNTAIN,
    VALLEY,
    DIRT_ROAD,
    MAIN_ROAD,
    RIVER,
    LAKE,
    BRIDGE,
    WATCHTOWER,
    VILLAGE,
    ISOLATED_HOUSE,
    FARM,
    SUPPLY_POST,
    FIRE_STATION,
    INDUSTRIAL,
    HYDRANT
};

struct MapFeature {
    MapFeatureType type;
    Vec3 position;
    float radius;
    std::string name;
    bool isFlammable = true;
    bool isCritical = false;
    MapFeature() = default;
    MapFeature(MapFeatureType t, Vec3 p, float r, const std::string& n, bool fl, bool cr)
        : type(t), position(p), radius(r), name(n), isFlammable(fl), isCritical(cr) {}
};

class GameMap {
public:
    GameMap(int terrainW=512, int terrainH=512) {
        // Generate features procedurally
        generateFeatures();
    }

    void generateFeatures() {
        features.clear();
        // Fire station at center
        features.push_back(MapFeature{MapFeatureType::FIRE_STATION, Vec3{0,0,0}, 30, "Quartel Bombeiros Pedra Alta", false, true});

        // Villages
        features.push_back(MapFeature{MapFeatureType::VILLAGE, Vec3{-80,0,-250}, 80, "Aldeia Pedra Alta", true, true});
        features.push_back(MapFeature{MapFeatureType::VILLAGE, Vec3{300,0,200}, 60, "Fazenda Santa Clara", true, true});
        features.push_back(MapFeature{MapFeatureType::VILLAGE, Vec3{-200,0,300}, 40, "Posto Vigia Norte", false, false});

        // Isolated houses
        for(int i=0;i<12;i++){
            float x = randRange(-400,400);
            float z = randRange(-400,400);
            if(std::sqrt(x*x+z*z) < 100) continue;
            features.push_back(MapFeature{MapFeatureType::ISOLATED_HOUSE, Vec3{x,0,z}, 15, "Casa Isolada " + std::to_string(i+1), true, true});
        }

        // Watchtowers on high ground
        features.push_back(MapFeature{MapFeatureType::WATCHTOWER, Vec3{400,0,-300}, 10, "Torre Serra da Bruma", false, false});
        features.push_back(MapFeature{MapFeatureType::WATCHTOWER, Vec3{-350,0,50}, 10, "Torre Ponte do Lobo", false, false});
        features.push_back(MapFeature{MapFeatureType::WATCHTOWER, Vec3{50,0,400}, 10, "Torre Base", false, false});

        // Roads
        // Main road - horizontal
        for(int i=-5;i<=5;i++){
            features.push_back(MapFeature{MapFeatureType::MAIN_ROAD, Vec3{(float)i*100,0,0}, 20, "EN-342", false, false});
        }
        // Dirt roads branching
        for(int i=0;i<8;i++){
            float angle = randRange(0, 360)*DEG2RAD;
            float dist = randRange(50,350);
            Vec3 pos{std::cos(angle)*dist,0,std::sin(angle)*dist};
            features.push_back(MapFeature{MapFeatureType::DIRT_ROAD, pos, 8, "Estrada Terra " + std::to_string(i), false, false});
        }

        // Rivers and lakes
        features.push_back(MapFeature{MapFeatureType::RIVER, Vec3{150,0,-100}, 25, "Rio Seco", false, false});
        features.push_back(MapFeature{MapFeatureType::LAKE, Vec3{250,0,350}, 60, "Lago Escondido", false, false});

        // Bridges
        features.push_back(MapFeature{MapFeatureType::BRIDGE, Vec3{150,0,-100}, 15, "Ponte Rio Seco", false, true});

        // Farms
        for(int i=0;i<5;i++){
            Vec3 pos{randRange(-300,300),0,randRange(-300,300)};
            features.push_back(MapFeature{MapFeatureType::FARM, pos, 35, "Fazenda " + std::to_string(i), true, false});
        }

        // Supply posts
        for(int i=0;i<6;i++){
            Vec3 pos{randRange(-400,400),0,randRange(-400,400)};
            features.push_back(MapFeature{MapFeatureType::SUPPLY_POST, pos, 12, "Posto Abastecimento " + std::to_string(i), false, false});
        }

        // Industrial near forest
        features.push_back(MapFeature{MapFeatureType::INDUSTRIAL, Vec3{-100,0,500}, 50, "Zona Industrial Sul", true, true});

        // Hydrants near villages and station
        for(int i=0;i<20;i++){
            Vec3 pos{randRange(-200,200),0,randRange(-200,200)};
            features.push_back(MapFeature{MapFeatureType::HYDRANT, pos, 2, "Hidrante " + std::to_string(i), false, false});
        }

        // Forests
        for(int i=0;i<15;i++){
            Vec3 pos{randRange(-500,500),0,randRange(-500,500)};
            float r = randRange(40,120);
            auto type = (randFloat()<0.6f) ? MapFeatureType::FOREST_DENSE : MapFeatureType::FOREST_SPARSE;
            features.push_back(MapFeature{type, pos, r, (type==MapFeatureType::FOREST_DENSE?"Floresta Densa ":"Floresta ") + std::to_string(i), true, false});
        }
    }

    std::vector<MapFeature> getFeaturesInRadius(Vec3 pos, float radius) const {
        std::vector<MapFeature> result;
        for(auto& f : features){
            if((f.position - pos).length() <= radius + f.radius) result.push_back(f);
        }
        return result;
    }

    MapFeature* getNearestFeature(Vec3 pos, MapFeatureType type) {
        MapFeature* best = nullptr;
        float bestDist = 1e9f;
        for(auto& f : features){
            if(f.type != type) continue;
            float d = (f.position - pos).length();
            if(d < bestDist){ bestDist=d; best=&f; }
        }
        return best;
    }

    std::vector<MapFeature> features;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
