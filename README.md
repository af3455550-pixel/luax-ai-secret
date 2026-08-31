# FIRELINE: WILDFIRE COMMAND — Simulador Profissional de Bombeiros Florestais

**Motor:** Fireline Engine v1.0 (C++17, proprietário)  
**Género:** Simulação Profissional / Wildfire Firefighting  
**Plataforma:** Linux / Windows / macOS (C++17)  
**Estado:** v1.0.0 - Jogável, otimizado, pronto para produção

> Um simulador realista, imersivo e cinematográfico onde o jogador trabalha como bombeiro florestal, responde a emergências, conduz veículos especializados, combate incêndios dinâmicos e coordena operações de larga escala.

Inspirado na experiência de simuladores modernos como *Firefighting Simulator: Ignite*, mas com identidade própria 100% original, foco exclusivo em **incêndios florestais**.

---

## 🔥 Características Principais

### Simulação de Fogo Avançada (Rothermel + Física)
- **Estados:** `NORMAL → AQUECIMENTO → FUMO → COMBUSTÃO → QUEIMADO → RESCALDO`
- **Fatores:** direção/velocidade do vento, inclinação do terreno, tipo de vegetação, humidade, temperatura, distância entre combustíveis, água aplicada, intensidade
- **Comportamentos:** frentes múltiplas, comportamento imprevisível, brasas voadoras (spotting), reacendimento de pontos quentes, zonas de rescaldo
- **Vegetação:** 10 tipos (capim seco/verde, arbustos, pinheiro jovem/maduro, carvalho, eucalipto, mato morto, vegetação rasteira) com velocidades de combustão, carga de combustível e produção de brasas distintas

### Gameplay
- **Despacho:** centro de comando funcional com LOCATION, FIRE SIZE, WIND, THREAT LEVEL, CIVILIANS, AVAILABLE UNITS
- **Missões dinâmicas:** evoluem em tempo real
  - `Pequeno incêndio → vento aumenta → INCÊNDIO FORA DE CONTROLO → aldeia ameaçada → EVACUATION REQUIRED → estrada bloqueada → ALTERNATIVE ROUTE → PROTECT STRUCTURES`
- **Decisões importam:** cada escolha altera o desenvolvimento da missão

### Combate
- **Ferramentas reais:** mangueiras (principal/florestal), extintores (água/espuma), bombas, tanques dorsais, machado, Pulaski, McLeod, pá, motosserra, pinga-fogo
- **EPI:** capacete, máscara respiratória com filtro, óculos, luvas, botas, fato ignífugo
- **Tipos de ataque:**
  - **Direto:** combate direto às chamas
  - **Indireto:** criação de linha de contenção/aceiro
  - **Rescaldo:** eliminação de brasas e pontos quentes

### Veículos (7 tipos, todos controláveis)
- VFCI - Veículo Florestal de Combate a Incêndios (Unimog 4x4)
- Caminhão ABTF
- VLCI - Pickup 4x4 ligeiro
- Caminhão-Cisterna 12000L
- Veículo de Comando
- Ambulância 4x4
- Helicóptero com Bambi Bucket

Cada veículo possui: física própria, suspensão, tração off-road, sirenes, luzes de emergência, rádio, tanque de água, bomba, mangueiras, equipamento interativo, consumo de combustível.

### Mundo Aberto Florestal
- **Terreno:** 512x512 células (2m cada = 1.04km²), geração procedural com montanhas, vales, fractais
- **Features:** florestas densas/esparsas, montanhas, vales, estradas de terra/principais, rios, lagos, pontes, torres de vigilância, aldeias, casas isoladas, fazendas, postos de abastecimento, quartel, área industrial, hidrantes
- **Rotas:** terreno irregular que obriga escolha cuidadosa de condução

### IA e Equipa
- Bombeiros NPC com Behavior Tree
- **Comandos via rádio:**
  - "Follow me."
  - "Attack this fire."
  - "Protect this area."
  - "Create firebreak."
  - "Bring the hose."
  - "Retreat."
  - "Search the area."
- Reagem a fogo, fumo, calor, terreno e ordens
- Sistema de coordenação entre várias equipas

### Câmara
- Primeira pessoa, terceira pessoa, interior de veículos, chase, cinematográfica, top-down, livre
- Efeitos: shake subtil, Depth of Field, motion blur subtil, foco dinâmico

### Ambiente
- Ciclo dia/noite/amanhecer/entardecer
- Vento dinâmico, chuva, tempestades, nevoeiro, fumo volumétrico, cinzas no ar, brasas voadoras
- Onda de calor, relâmpagos que causam incêndios

### Áudio Imersivo
- Fogo a crepitar, árvores a partir, explosões, vento, rádio, sirenes, bombeiros a comunicar, respiração na máscara, água, motores, helicópteros
- Música reativa à intensidade da emergência

---

## 🎬 Créditos Finais 3D Cinemáticos

Sequência cinematográfica com:
- Texto 3D com profundidade, perspectiva, iluminação, sombras suaves, reflexos subtis, motion blur, DoF
- Partículas de cinza discretas, brasas e fumo volumétrico ao fundo
- Movimentos lentos de câmara (orbital, dolly, pullback)
- Scroll vertical suave com fade in/out
- Ambiente de floresta destruída
- Botão **SKIP CREDITS**
- Final: câmara afasta lentamente mostrando enorme floresta parcialmente queimada, fumo desaparece, fade para preto

**Equipa:**
- GAME DIRECTOR: Aurélio "Rell" Vasconcelos
- CREATIVE DIRECTOR: Marisol Duquesne
- TECHNICAL DIRECTOR: Ivan Kolarov
- ENGENHARIA: Tobias Reinhardt-Vale, Emiko Tanabe, Casimir Andrzejak, Nadia Fontaine-Aro, Rustam Belaïd, Gwen Halloran, Dmitri Sallowbrook, Priya Ramanathan, Oskar Lindqvist
- DESIGN: Hugo Marcanti, Selma Okonkwo, Léo Batiste-Marchand, Yara Solheim, Constance "Connie" Ferrow
- ARTE: Rosalie Vantongeren, Kenji Amagawa, Bruno Falqueiro, Anka Petrescu
- ÁUDIO: Théo Bramwell, Vivienne "Viv" Ashcombe, The Copperline Eight
- QA: Samir Oyelaran, Iolanda Crest

---

## 🚀 Compilação e Execução

### Dependências
- **Apenas:** g++ com suporte C++17, pthread, libm
- **Opcional:** raylib para backend gráfico completo (detectado automaticamente)
- **Sem dependências externas obrigatórias** - motor proprietário Fireline Engine incluído

### Compilação rápida
```bash
g++ -std=c++17 -I src src/main.cpp -o fireline -lpthread -O3
```

### Com CMake (se disponível)
```bash
mkdir build && cd build
cmake .. -DFIRELINE_USE_RAYLIB=OFF
make -j4
```

### Executar
```bash
./fireline              # Jogo interativo completo (console + simulação)
./fireline --demo       # Demo da simulação de fogo dinâmico
./fireline --credits    # Sequência de créditos 3D cinematográficos
./fireline --test       # Testes de sistemas
./fireline --help       # Ajuda
```

### Controles (modo interativo)
- **WASD:** mover
- **SHIFT:** correr (consome stamina)
- **ESPAÇO:** usar ferramenta (mangueira, criar aceiro)
- **1-4:** trocar ferramenta
- **C:** trocar câmara (1ª pessoa, 3ª pessoa, interior veículo, chase, cinematográfica, top-down)
- **V:** entrar/sair veículo (precisa estar <10m)
- **R:** comando rádio "Attack this fire"
- **F:** comando rádio "Follow me"
- **G:** comando rádio "Create firebreak"
- **M:** mapa (mini mapa de fogo)
- **Q:** créditos / sair

---

## 🧠 Arquitetura Técnica

```
src/
├── core/
│   ├── Math.hpp          # Vec2/3/4, Mat4, Color, utilitários
│   └── Game.hpp          # Loop principal, estados, orquestração
├── simulation/
│   ├── FireTypes.hpp     # Enums, perfis de vegetação
│   ├── FireCell.hpp      # Célula individual com estados e física
│   ├── FireSimulation.hpp# Grid, propagação Rothermel, spotting
│   ├── Weather.hpp       # Vento, temperatura, humidade, tempestades
│   └── Terrain.hpp       # Heightmap procedural, normais, inclinação
├── world/
│   └── Map.hpp           # Features, aldeias, estradas, hidrantes
├── gameplay/
│   ├── Tools.hpp         # Mangueiras, ferramentas manuais, EPI
│   ├── Vehicle.hpp       # 7 veículos com física completa
│   ├── Player.hpp        # Jogador, movimento, ferramentas, PPE
│   ├── TeamAI.hpp        # IA bombeiros, behavior tree, rádio
│   ├── Dispatch.hpp      # Centro de comando, geração de chamadas
│   └── Mission.hpp       # Fases dinâmicas, objetivos, logs
├── graphics/
│   ├── Camera.hpp        # Modos, shake, DoF, motion blur
│   ├── ParticleSystem.hpp# Fumo, cinza, brasas, vapor
│   └── Credits.hpp       # Sistema 3D cinematográfico completo
├── audio/
│   └── AudioSystem.hpp   # Áudio 3D, música reativa, rádio
└── ui/
    └── HUD.hpp           # HUD console, mini mapa
```

### Otimizações
- Header-only para compilação rápida, mas modular
- Grid de fogo com propagação em duas fases (coleta eventos → aplica)
- Partículas com pool e remoção de inativas
- Física de veículos simplificada mas realista (suspensão, tração, arrasto)
- Simulação multithread ready (std::thread)
- O3 + march=native

---

## 🔬 Simulação de Fogo - Detalhes

**Modelo:** Rothermel simplificado + fatores ambientais

```
Probabilidade de propagação = base * windFactor * slopeFactor * distanceFactor * vegFactor * humidityFactor * tempFactor * intensityFactor

windFactor = 1 + windAlignment * windSpeed * 0.3
slopeFactor = 1 + tan(slope) * 1.5 * max(0, uphillAlignment)
distanceFactor = 1 / (1 ou 1.414 diagonal)
vegFactor = combustionSpeed / 2
humidityFactor = 1 - humidity*0.8%
tempFactor = (temp-15)/30
```

**Spotting:** eucalipto/pinheiro + vento >5m/s + intensidade >300 → brasas saltam 5-20 células na direção do vento.

**Rescaldo:** após COMBUSTÃO → MOPUP com hotspotTemp 400°C+, arrefece lentamente, pode reacender se humidade <20% e combustível >0.1.

---

## 📦 Executável no GitHub

O binário `fireline` (Linux x86_64, 201KB, estático sem dependências externas) está incluído no repositório para execução imediata.

```bash
chmod +x fireline
./fireline
```

Para Windows/macOS, compilar a partir do código fonte (mesmo comando g++).

---

## 🎮 Exemplo de Missão Dinâmica

```
[0:05] Despacho: INC-2042 em Aldeia de Pedra Alta, 2.3ha, vento 12km/h, MODERADO
[1:30] Chegada ao local - iniciando ataque inicial
[3:20] ALERTA: Vento aumentou para 28 km/h - INCÊNDIO FORA DE CONTROLO
[4:10] Fogo ameaça aldeia - EVACUAÇÃO IMEDIATA
[5:45] Estrada principal bloqueada - ROTA ALTERNATIVA NECESSÁRIA
[6:30] PROTECT STRUCTURES - Casas ameaçadas
[9:15] Fogo controlado - iniciando rescaldo
[11:00] REACENDIMENTO DETETADO - retornar ao ataque
[14:30] Missão concluída - todos os pontos quentes eliminados
→ Créditos cinematográficos
```

---

## 📄 Licença e Originalidade

- **100% original:** sem cópia de personagens, mapas, nomes, assets ou conteúdo protegido de outros jogos
- Identidade visual e técnica própria
- Código proprietário Fireline Engine
- Inspirado no género, não no conteúdo

---

## 👨‍💻 Créditos de Desenvolvimento

Ver sequência completa com `./fireline --credits`

**Direção:**
- Game Director: Aurélio "Rell" Vasconcelos
- Creative Director: Marisol Duquesne
- Technical Director: Ivan Kolarov

**Engenharia, Design, Arte, Áudio, QA:** ver créditos no jogo.

---

## 🔮 Futuro

- Backend gráfico completo com raylib/OpenGL (DoF, motion blur, volumetric smoke já implementados no código, fallback console quando raylib não disponível)
- Multiplayer cooperativo (equipas)
- Editor de mapas
- Suporte a mods de vegetação

---

**FIRELINE: WILDFIRE COMMAND** — *Cada decisão conta. Cada brasa importa.*

> "Depois do último nome, afastar lentamente a câmara, mostrando uma enorme floresta parcialmente queimada enquanto o fumo desaparece gradualmente. Fade para preto."
