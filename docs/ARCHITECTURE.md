# Arquitetura Fireline Engine

## Visão Geral
Fireline Engine é um motor proprietário C++17 otimizado para simulação de incêndios florestais em larga escala.

### Princípios
- **Sem dependências externas obrigatórias:** apenas STL, pthread, math
- **Header-only onde possível:** compilação rápida, fácil integração
- **Cache-friendly:** grid de fogo contíguo, SoA para partículas
- **Determinístico:** seed fixo para replays, testes
- **Multithread ready:** job system preparado para paralelizar update de células

## Módulos

### Core
- `Math.hpp`: Vec2/3/4, Mat4, Color, lerp, clamp, smoothstep
- `Game.hpp`: máquina de estados (MAIN_MENU, DISPATCH, IN_MISSION, CREDITS), loop, orquestração
- `Engine.hpp`: abstração de janela, renderer, input, frame pacing

### Simulation
- `FireTypes.hpp`: enums FireState, VegetationType, ThreatLevel, ToolType, perfis com fuelLoad, combustionSpeed, emberProduction, etc.
- `FireCell.hpp`: célula individual, 6 estados, heat, moisture, fuelRemaining, waterContent, hotspotTemp, firebreak
- `FireSimulation.hpp`: grid 2D, Rothermel simplificado, coleta de SpreadEvent para evitar propagação imediata, spotting de brasas, stats
- `Weather.hpp`: vento (direção 2D + velocidade), temperatura, humidade, precipitação, tempestade, onda de calor, relâmpagos, ciclo dia/noite
- `Terrain.hpp`: heightmap procedural fractal, normais, slope, worldToGrid, gridToWorld

### World
- `Map.hpp`: features (aldeias, casas, torres, estradas, rios, lagos, pontes, fazendas, hidrantes, florestas), queries por raio

### Gameplay
- `Tools.hpp`: mangueiras com pressão e tipos de bico (jato, neblina, largo), ferramentas manuais com progresso de aceiro, EPI com proteção térmica e de fumo
- `Vehicle.hpp`: 7 tipos, spec com massa, potência, capacidade água, tração, suspensão, rodas com compressão, física básica (aceleração, arrasto, gravidade, terreno), bomba, sirene, mangueira, rádio, combustível
- `Player.hpp`: saúde, stamina, hidratação, movimento com yaw/pitch, ferramentas, PPE, interação veículo, dano por calor/fumo
- `TeamAI.hpp`: FirefighterAI com estados (IDLE, FOLLOW, MOVE_TO, ATTACK_FIRE, PROTECT_AREA, CREATE_FIREBREAK, BRING_HOSE, RETREAT, SEARCH, RESCUE, REFILL, MOPUP), blackboard, percepção de fogo, comandos via rádio, TeamManager com broadcast
- `Civilian.hpp`: civis e animais, pânico, fuga, evacuação, saúde
- `Dispatch.hpp`: geração procedural de chamadas com localização, tamanho, vento, ameaça, civis, causa, unidades recomendadas
- `Mission.hpp`: fases (DISPATCH, ENROUTE, INITIAL_ATTACK, ESCALATION, EVACUATION, STRUCTURE_PROTECTION, MOPUP, COMPLETED, FAILED), objetivos, logs, transição dinâmica baseada em vento, intensidade, civis

### Graphics
- `Camera.hpp`: modos (1ª pessoa, 3ª pessoa, interior veículo, chase, cinematográfica, top-down, livre), shake, DoF (focusDistance, blur), motion blur, matrizes view/proj, movimentos lentos cinematográficos
- `ParticleSystem.hpp`: partículas (SMOKE, ASH, EMBER, SPARK, STEAM, DUST) com posição, velocidade, aceleração, cor, tamanho, vida, rotação, emissão específica (fumo com intensidade, brasas com gravidade, cinza com vento)
- `Renderer.hpp`: abstração, suporte raylib opcional, métodos para terreno, vegetação, fogo, veículos, features, partículas, pós-processamento (DoF, motion blur, volumetric)
- `Credits.hpp`: sistema 3D cinematográfico completo com scroll vertical, fade, profundidade, perspectiva, iluminação, sombras suaves, reflexos, motion blur, DoF, partículas, pullback final, SKIP

### Audio
- `AudioSystem.hpp`: fontes 3D com atenuação por distância, tipos (fogo crepitar, árvore cair, explosão, vento, rádio, sirene, voz bombeiro, respiração máscara, água, motor, helicóptero), música reativa à intensidade

### UI
- `HUD.hpp`: render console com stats, missão, objetivos, logs, mini mapa ASCII de fogo

## Fluxo de Update
```
Game::update(dt)
  Weather::update
    -> variação vento, onda calor, tempestade, relâmpago
  FireSimulation::update
    -> FireCell::update para cada célula
    -> coleta SpreadEvent (8 vizinhos, windFactor, slopeFactor, vegFactor, humidityFactor, tempFactor, ember spotting)
    -> aplica spreads
    -> updateStats
  Player::update
    -> movimento, stamina, heat/smoke exposure com PPE, ferramentas
  Vehicle::update (para cada)
    -> física, terreno, combustível, bomba, rodas
  TeamAI::update
    -> percepção, decisão, movimento, ataque fogo, aceiro, resgate
  Mission::update
    -> transição fases, objetivos, logs
  Camera::update
    -> modo, shake, DoF, motion blur
  ParticleSystem::update
    -> emissão baseada em fogo, update com vento
  AudioSystem::update
    -> música reativa, atenuação 3D
```

## Otimizações
- Grid contíguo `vector<FireCell>` 256x256 = 65k células, cada com ~100 bytes = 6.5MB
- Propagação em duas fases evita data race e permite paralelizar
- Partículas com erase-remove e pool
- Veículos com update simples mas realista
- O3 + march=native
- Sem alocações no hot path (exceto spreads vector com reserve)

## Extensibilidade
- Adicionar novo VegetationType: editar getVegetationProfile
- Novo veículo: adicionar em VehicleType e getVehicleSpec
- Novo comando rádio: RadioCommand + lógica em giveCommand
- Novo modo câmara: CameraMode + update
- Backend gráfico: implementar métodos em Renderer com raylib/OpenGL quando FIRELINE_HAS_RAYLIB=1
