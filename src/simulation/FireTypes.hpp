#pragma once
#include <string>

namespace Fireline {

enum class FireState {
    NORMAL = 0,       // Saudável
    HEATING = 1,      // Aquecimento - pré-ignição
    SMOKE = 2,        // Fumo - libertação de voláteis
    COMBUSTION = 3,   // Combustão ativa
    BURNED = 4,       // Queimado
    MOPUP = 5         // Rescaldo - brasas
};

inline std::string fireStateToString(FireState s) {
    switch(s) {
        case FireState::NORMAL: return "NORMAL";
        case FireState::HEATING: return "AQUECIMENTO";
        case FireState::SMOKE: return "FUMO";
        case FireState::COMBUSTION: return "COMBUSTAO";
        case FireState::BURNED: return "QUEIMADO";
        case FireState::MOPUP: return "RESCALDO";
    }
    return "UNKNOWN";
}

enum class VegetationType {
    GRASS_DRY = 0,
    GRASS_GREEN = 1,
    SHRUB_LOW = 2,
    SHRUB_DENSE = 3,
    PINE_YOUNG = 4,
    PINE_MATURE = 5,
    OAK = 6,
    EUCALYPTUS = 7,
    BRUSH_DEAD = 8,
    UNDERGROWTH = 9
};

struct VegetationProfile {
    VegetationType type;
    std::string name;
    float fuelLoad;          // kg/m2
    float surfaceAreaVol;    // ratio
    float combustionSpeed;   // m/min base
    float moistureExtinction; // %
    float heatContent;       // kJ/kg
    float flameHeight;       // m
    float emberProduction;   // 0-1
    float fallProbability;   // chance tree falls when burned
    float burnTime;          // seconds
    float colorR, colorG, colorB;
};

inline VegetationProfile getVegetationProfile(VegetationType t) {
    switch(t) {
        case VegetationType::GRASS_DRY: return {t,"Capim Seco",0.3f,12000,3.5f,15,18500,1.2f,0.1f,0.0f,15,0.8f,0.7f,0.2f};
        case VegetationType::GRASS_GREEN: return {t,"Capim Verde",0.4f,8000,1.2f,35,16000,0.8f,0.05f,0.0f,20,0.3f,0.6f,0.2f};
        case VegetationType::SHRUB_LOW: return {t,"Arbusto Baixo",1.2f,6000,2.0f,25,17500,2.0f,0.2f,0.0f,40,0.4f,0.5f,0.2f};
        case VegetationType::SHRUB_DENSE: return {t,"Arbusto Denso",2.5f,5000,2.8f,20,18000,3.5f,0.4f,0.0f,60,0.3f,0.4f,0.15f};
        case VegetationType::PINE_YOUNG: return {t,"Pinheiro Jovem",4.0f,3000,1.5f,30,19500,6.0f,0.5f,0.3f,90,0.1f,0.5f,0.1f};
        case VegetationType::PINE_MATURE: return {t,"Pinheiro Maduro",8.0f,2000,1.8f,28,20000,12.0f,0.8f,0.7f,150,0.05f,0.35f,0.05f};
        case VegetationType::OAK: return {t,"Carvalho",6.0f,1800,0.9f,35,19000,8.0f,0.3f,0.4f,180,0.15f,0.4f,0.15f};
        case VegetationType::EUCALYPTUS: return {t,"Eucalipto",7.0f,3500,4.5f,18,21000,15.0f,0.9f,0.6f,120,0.2f,0.6f,0.25f};
        case VegetationType::BRUSH_DEAD: return {t,"Mato Morto",1.8f,7000,3.0f,12,18500,2.5f,0.6f,0.0f,30,0.5f,0.3f,0.1f};
        case VegetationType::UNDERGROWTH: return {t,"Vegetação Rasteira",0.8f,9000,2.2f,22,17000,1.5f,0.3f,0.0f,25,0.4f,0.5f,0.2f};
    }
    return {t,"Desconhecido",1.0f,5000,1.0f,25,17000,1.0f,0.2f,0.0f,30,0.5f,0.5f,0.5f};
}

enum class AttackType {
    DIRECT = 0,   // Combate direto às chamas
    INDIRECT = 1, // Linha de contenção
    MOPUP = 2     // Rescaldo
};

enum class ToolType {
    HOSE_MAIN = 0,
    HOSE_FOREST = 1,
    EXTINGUISHER_WATER = 2,
    EXTINGUISHER_FOAM = 3,
    PUMP_PORTABLE = 4,
    TANK = 5,
    AXE = 6,
    PULASKI = 7,
    MCLEOD = 8,
    SHOVEL = 9,
    CHAINSAW = 10,
    DRIP_TORCH = 11
};

enum class ThreatLevel {
    LOW = 0,
    MODERATE = 1,
    HIGH = 2,
    VERY_HIGH = 3,
    EXTREME = 4
};

inline std::string threatToString(ThreatLevel t) {
    switch(t){
        case ThreatLevel::LOW: return "BAIXO";
        case ThreatLevel::MODERATE: return "MODERADO";
        case ThreatLevel::HIGH: return "ALTO";
        case ThreatLevel::VERY_HIGH: return "MUITO ALTO";
        case ThreatLevel::EXTREME: return "EXTREMO";
    }
    return "UNKNOWN";
}

} // namespace Fireline
