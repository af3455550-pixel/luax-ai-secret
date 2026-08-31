# ApexEngine 3D — Especificação Arquitetural e Design Técnico de Engine AAA

**Autor:** Arquiteto de Software & Engenheiro de Engine AAA  
**Versão do Documento:** 2.4.0-PROD  
**Padrão da Linguagem:** C++20 / C++23 Moderno  
**Inspiração:** Unreal Engine 5, Frostbite, id Tech, Decima Engine  

---

## 1. Diagrama de Arquitetura em Camadas (Layered Architecture)

A engine adota uma arquitetura em camadas estritamente desacopladas, com isolamento de dependências através do padrão **Data-Driven & Interface-Oriented Design**:

```
+---------------------------------------------------------------------------------------------------+
|                                      APPLICATIONS & TOOLS                                         |
|  +---------------------------+  +-------------------------------+  +---------------------------+  |
|  |       ApexEditor.exe      |  |         AssetCooker.exe       |  |      ApexEngineApp.exe    |  |
|  |  (ImGui/Qt, Gizmos, Undo, |  |  (SPIR-V, glTF/FBX, DDC,      |  |  (Standalone Game Client  |  |
|  |   Sequencer, MaterialNode)|  |   Texture Cruncher, NavMesh)  |  |   & Dedicated Server)     |  |
|  +---------------------------+  +-------------------------------+  +---------------------------+  |
+---------------------------------------------------------------------------------------------------+
|                                      GAMEPLAY & SCRIPTING                                         |
|  +---------------------------+  +-------------------------------+  +---------------------------+  |
|  |      Lua / Python VM      |  |      Gameplay Framework       |  |     World Partition       |  |
|  | (Sol2 / LuaJIT / C-API)   |  | (PlayerController, Pawn, Game)|  | (Quadtree Streaming & HLOD|  |
|  +---------------------------+  +-------------------------------+  +---------------------------+  |
+---------------------------------------------------------------------------------------------------+
|                                     ENGINE CORE SUBSYSTEMS                                        |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
|  |   Scene Graph     |  |   ECS Registry    |  |   Event Bus &     |  |    Network Engine       |  |
|  | (Hierarquia &     |  |  (Sparse Set /    |  |   Message Broker  |  | (Client-Server, State   |  |
|  |  Dirty Transforms)|  |   Archetype Pool) |  |   (Async/Sync)    |  |  Replication, Predict)  |  |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
+---------------------------------------------------------------------------------------------------+
|                                     HIGH-LEVEL MIDDLEWARE                                         |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
|  | Render Graph /    |  |   Physics Engine  |  | 3D Spatial Audio  |  | Asset Pipeline & DDC    |  |
|  | Frame Graph (PBR, |  |  (PhysX / Bullet  |  | (HRTF, Doppler,   |  | (Streaming, LRU Cache,  |  |
|  | Deferred/Forward+)|  |  Colliders, Joints)|  |  DSP, WAV/OGG/MP3)|  |  Hot-Reloading)         |  |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
+---------------------------------------------------------------------------------------------------+
|                                 RHI (RENDER HARDWARE INTERFACE)                                   |
|  +---------------------------------------------------------------------------------------------+  |
|  |                      RHI Device / CommandBuffer / PipelineState Abstraction                 |  |
|  |     +-------------------------+  +-------------------------+  +-------------------------+   |  |
|  |     |     Vulkan 1.3 RHI      |  |     DirectX 12 RHI      |  |     OpenGL 4.6 RHI      |   |  |
|  |     +-------------------------+  +-------------------------+  +-------------------------+   |  |
+---------------------------------------------------------------------------------------------------+
|                                       PLATFORM & FOUNDATION                                       |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
|  | Memory Allocators |  | Task Graph & Jobs |  | SIMD / Math Core  |  | Reflection & Metadata   |  |
|  | (Arena, Pool, TLS)|  | (Work Stealing)   |  | (AVX-512/NEON)    |  | (UPROPERTY/UCLASS Macro)|  |
|  +-------------------+  +-------------------+  +-------------------+  +-------------------------+  |
+---------------------------------------------------------------------------------------------------+
|                                  OPERATING SYSTEM & HARDWARE                                      |
|            [ Windows (Win32) / Linux (POSIX) / macOS (Metal) / Console SDKs / GPU / NVMe ]        |
+---------------------------------------------------------------------------------------------------+
```

---

## 2. Estrutura Completa de Pastas do Projeto

Organização modular no modelo corporativo da Unreal Engine / Chromium:

```
luax-ai-secret/
├── CMakeLists.txt                         # Script mestre CMake cross-platform
├── Makefile                               # Build rápido com suporte a C++20 nativo
├── README.md                              # Documentação de visão geral
├── bin/                                   # Binários compilados (.exe / ELF)
│   ├── ApexEngine                         # Executável Linux / POSIX
│   └── ApexEngine.exe                     # Executável Win64 / Cross-Target
├── Config/                                # Arquivos de configuração e ini
│   ├── DefaultEngine.ini
│   ├── DefaultInput.ini
│   └── DefaultScalability.ini
├── Docs/                                  # Documentação técnica e manuais de arquitetura
│   └── ARCHITECTURE_DESIGN.md
├── Engine/
│   ├── Assets/                            # Assets empacotados e brutos
│   │   ├── Audio/                         # Efeitos sonoros e trilha (WAV, OGG, MP3)
│   │   ├── Materials/                     # Definições de materiais PBR (.mat, .json)
│   │   ├── Meshes/                        # Modelos 3D (FBX, OBJ, glTF)
│   │   ├── Scenes/                        # Níveis e descrições de mundo (.scene)
│   │   ├── Scripts/                       # Scripts de gameplay (Lua, Python)
│   │   └── Textures/                      # Albedo, Normal, Roughness, HDR Skyboxes
│   ├── Shaders/                           # Shaders fonte e módulos SPIR-V
│   │   ├── PBR_Deferred.glsl              # Deferred MRT + Clustered Lighting
│   │   ├── PostProcess_Tonemap.glsl       # ACES Tonemapping + Bloom
│   │   └── Shadow_CSM.glsl                # Cascaded Shadow Maps
│   └── Source/
│       ├── Runtime/
│       │   ├── Core/                      # Módulo Base da Engine
│       │   │   ├── Math/                  # Matrizes, Quaternions, Vetores, SIMD
│       │   │   ├── Memory/                # Arena, Pool, Stack Allocators, Memory Tracking
│       │   │   ├── Threading/             # Task Graph, Work Stealing, Lock-Free Queues
│       │   │   ├── Events/                # EventBus pub/sub assíncrono e síncrono
│       │   │   ├── Log/                   # Logger thread-safe colorido com categorias
│       │   │   ├── Profiling/             # Micro-profiler CPU/GPU e captura de frames
│       │   │   └── Reflection/            # Sistema de Tipos, Metadados e RTTI
│       │   ├── CoreUObject/               # Gerenciador de ciclo de vida de objetos e GC
│       │   ├── ECS/                       # Entity Component System (Sparse Set & Dense Archetypes)
│       │   ├── Engine/                    # Game Framework de Alto Nível
│       │   │   ├── Scene/                 # Scene Graph, Hierarquia de Transformações
│       │   │   ├── World/                 # Gerenciamento de Nível, Streaming e Spawner
│       │   │   └── EngineApp.hpp          # Bootstrap e Master Engine Loop
│       │   ├── RHI/                       # Render Hardware Interface (Abstração de GPU)
│       │   ├── RenderCore/                # Render Graph, PBR Pipeline, Forward+, Deferred
│       │   ├── Physics/                   # PhysX / Bullet Integration, Raycast, Colliders
│       │   ├── Audio/                     # 3D Spatial Audio, HRTF, DSP Mixer
│       │   ├── Scripting/                 # Lua/Python VM Bridge & Native Bindings
│       │   ├── Networking/                # State Replication, Prediction, Interpolation
│       │   └── AssetPipeline/             # Asset Manager, Streaming Assíncrono, LRU Cache
│       ├── Editor/                        # Ferramental do Editor Visual
│       │   ├── EditorCore/                # Undo/Redo, Gizmos 3D, Contexto de Seleção
│       │   ├── SceneEditor/               # Viewport 3D, Hierarquia de Atores, Inspector
│       │   ├── MaterialEditor/            # Node Graph de Shaders e Compilador Visual
│       │   └── Sequencer/                 # Timeline de Animação e Cutscenes
│       └── Programs/
│           ├── ApexEngineApp/             # Ponto de entrada Runtime (main.cpp)
│           └── AssetCooker/               # Pipeline de compilação e baking de assets
```

---

## 3. Matriz de Módulos e Dependências

A compilação e vinculação seguem o grafo acíclico dirigido (DAG) abaixo:

| Módulo | Tipo | Dependências Diretas | Responsabilidade Principal |
| :--- | :--- | :--- | :--- |
| **Apex::Core** | Estático | STL, pthreads, OS APIs | Alocação de memória, Job System, SIMD Math, Logging, Profiling. |
| **Apex::CoreUObject** | Estático | Apex::Core | Registro de tipos, macros `APEX_PROPERTY`, serialização binária. |
| **Apex::ECS** | Estático | Apex::Core | Armazenamento contiguous cache-friendly de componentes e consultas com máscaras de bits. |
| **Apex::RHI** | Dinâmico/Plug | Apex::Core, Vulkan/GL/D3D12 SDK | Camada agnóstica de hardware para buffers, texturas, shaders e comandos de GPU. |
| **Apex::RenderCore** | Estático | Apex::RHI, Apex::ECS, Apex::Core | Frame Graph, G-Buffer, CSM, Clustered PBR Lighting, ACES Tonemapping. |
| **Apex::Physics** | Dinâmico/Plug | Apex::Core, Apex::ECS, PhysX/Bullet | Simulação de corpos rígidos, colisões contínuas (CCD), juntas e raycasting. |
| **Apex::Audio** | Dinâmico/Plug | Apex::Core, Apex::ECS, OpenAL/FMOD | Espacialização 3D, atenuação por distância, efeito Doppler e DSP. |
| **Apex::Scripting** | Dinâmico/Plug | Apex::Core, Apex::ECS, LuaJIT/Python | Execução de scripts em runtime, hot-reload e bindings nativos C++. |
| **Apex::Networking** | Estático | Apex::Core, Apex::ECS, Sockets | Conexões UDP, replicação autoritativa de entidades, predição e interpolação. |
| **Apex::AssetPipeline**| Estático | Apex::Core, Apex::Threading | Leitura assíncrona de assets, descompressão, cache LRU e streaming sob demanda. |
| **Apex::Engine** | Estático | Todos os subsistemas | Loop principal, coordenação de subsistemas, Scene Graph e World. |
| **Apex::Editor** | Estático | Apex::Engine, ImGui/Qt | Interface de desenvolvimento, gizmos de translação/rotação/escala e timeline. |

---

## 4. Fluxo de Inicialização e Master Game Loop

### 4.1. Sequência de Bootstrap da Engine
1. **Config & Environment Load:** Leitura de arquivos `.ini`, detecção de hardware (CPUs, GPUs, Cores).
2. **Core Memory & Threading:** Inicialização dos Alocadores Globais (Arena & Pool) e subida do **TaskGraph** (Worker Threads = `std::thread::hardware_concurrency()`).
3. **Event Bus & Reflection Registry:** Registro dos tipos de dados via RTTI (`UCLASS` / `UPROPERTY`).
4. **Asset Pipeline:** Configuração do orçamento de memória (ex: 2GB) e pool de I/O em background.
5. **RHI Device:** Criação do Contexto Gráfico (Vulkan 1.3 / D3D12), criação de command pools e swapchain.
6. **Render Pipeline:** Alocação dos render targets do G-Buffer, Shadow Maps e compilação do Render Graph.
7. **Physics World:** Inicialização do Broadphase (BVH) e solver de corpos rígidos.
8. **Audio Engine:** Abertura do dispositivo de áudio, inicialização dos barramentos e DSPs.
9. **Scripting Runtime:** Criação do estado da VM Lua/Python e injeção de tabelas nativas (`Apex.*`).
10. **Networking:** Abertura de sockets UDP e inicialização da tabela de replicação de entidades.
11. **World & Level Load:** Leitura da cena padrão, instanciação de entidades no ECS, registro no Scene Graph.

### 4.2. Fluxo do Frame (Tick Loop)

```
[ Início do Frame ] -> [ Begin Frame Profiler ]
         |
         v
[ Leitura de Inputs & Eventos de Janela ]
         |
         v
[ Fixed Update Loop (Física a 60Hz com Timestep Fixo) ]
    ├── Integrar Forças (Gravidade, Velocidade, Atrito)
    ├── Broadphase Collision (AABB Tree)
    ├── Narrowphase Collision & Contact Solving
    └── Disparo de Triggers e Eventos de Contato
         |
         v
[ Multithreaded Logic & Task Graph Dispatch ]
    ├── Task A (Worker 1): Scripting Update (Lua OnUpdate)
    ├── Task B (Worker 2): 3D Audio Spatialization & DSP Update
    ├── Task C (Worker 3): Network Tick (Replicação de Estado & Predição)
    └── Task D (Worker 4): Frustum Culling & Occlusion Query
         |
         v
[ Scene Graph Transform Hierarchy Update ] (Propagação de matrizes com Dirty Flags)
         |
         v
[ Render Graph Execution (GPU) ]
    ├── Pass 1: Cascaded Shadow Map Generation (Depth Only)
    ├── Pass 2: G-Buffer Base Pass (Albedo, Normal, MR, AO, Emissive)
    ├── Pass 3: Clustered Light Assignment (Compute Shader)
    ├── Pass 4: Deferred Lighting & PBR Cook-Torrance BRDF Resolve
    ├── Pass 5: Forward+ Pass (Transparências, Partículas, UI)
    └── Pass 6: Post-Process Pipeline (ACES Tonemapping, Bloom, TAA)
         |
         v
[ RHI SwapChain Present & End Frame Profiler ] -> [ Pacing / V-Sync Wait ]
```

---

## 5. Implementação Técnica Detalhada dos 10 Pilares

### Pilar 1: Arquitetura e Núcleo (Core & ECS)
- **Sparse Set ECS:** Mapeamento esparso-denso garantindo inserção, remoção e consultas em $O(1)$ sem fragmentação de memória cache L1/L2.
- **Scene Graph:** Árvore espacial onde nós filhos herdam transformações relativas aos pais, utilizando sinalizadores (*dirty flags*) para evitar multiplicações de matriz desnecessárias ($4 \times 4$).
- **Alocadores Customizados:** Eliminação de chamadas ao `malloc`/`free` global em loops quentes com Alocadores de Arena (Linear) e Pool (Blocos fixos).

### Pilar 2: Renderização PBR e RHI Moderna
- **RHI Abstraction Layer:** Abstração uniforme de `RHIDevice`, `RHICommandBuffer`, `RHITexture`, `RHIBuffer` e `RHIPipelineState`.
- **Pipeline Híbrido:**
  - **Deferred Shading:** Para geometria opaca pesada com centenas de luzes dinâmicas.
  - **Forward+ / Clustered Shading:** Para materiais translúcidos, vidros e emissores de partículas.
- **PBR BRDF (Cook-Torrance):**
  $$f_r = k_d \frac{c}{\pi} + \frac{D(h) F(v, h) G(l, v, h)}{4 (n \cdot l)(n \cdot v)}$$
  Onde $D$ é a distribuição GGX, $F$ é a aproximação de Fresnel-Schlick e $G$ é o sombreamento geométrico de Smith-GGX.

### Pilar 3: Física e Simulação
- Wrapper com arquitetura para integração nativa com PhysX 5 e Bullet Physics.
- Detecção contínua de colisão (CCD) para evitar tunelamento de projéteis em alta velocidade.
- Suporte a raycasting espacial com retorno de normal de impacto, ponto no espaço-mundo e identificador de entidade.

### Pilar 4: Áudio Espacial 3D
- Atenuação volumétrica por lei do inverso do quadrado da distância com clamp (*min/max roll-off*).
- Simulação de atenuação binaural (HRTF) e efeito Doppler relativo:
  $$f = f_0 \left( \frac{c + v_{\text{listener}}}{c + v_{\text{source}}} \right)$$

### Pilar 5: Scripting e Bindings
- Integração de máquina virtual Lua/Python.
- Exposição de APIs nativas C++ para lógica de gameplay, manipulação de componentes e forças físicas.
- Hot-reload dinâmico em tempo de execução sem desligar a engine.

### Pilar 6: Editor Visual & Ferramentas
- Visualização de viewport 3D com Gizmos de transformação (Translate, Rotate, Scale).
- Sistema de Prefabs com instanciamento e sobrescrita de propriedades.
- Editor de Shaders em nós (Node Graph) gerando código SPIR-V otimizado.
- Timeline / Sequencer para interpolação de curvas Bézier e animação de câmeras.

### Pilar 7: Rede e Multiplayer AAA
- Arquitetura Cliente-Servidor Autoritativa sobre protocolo UDP sem bloqueio.
- Replicação de propriedades com compactação de delta e máscaras de bits (*dirty bitmasks*).
- Interpolação de snapshot (Buffer de histórico) e reconciliação com predição no cliente.

### Pilar 8: Pipeline de Assets & Cooker
- Importação automatizada de FBX, glTF 2.0 e OBJ para formatos binários nativos `.apexmesh` e `.apextex`.
- Derived Data Cache (DDC) para evitar reprocessamento de recursos idênticos.
- Compilação de shaders cross-target via SPIR-V Cross.

### Pilar 9: Performance e Concorrência
- **Task Graph / Job System:** Divisão do frame em grafos de tarefas com resolução de dependências por roubo de trabalho (*work-stealing queue*).
- **Culling Inteligente:** Hierarchical-Z Occlusion Culling e Frustum Culling computados em paralelo.

### Pilar 10: Extensibilidade e Plugins
- Sistema de módulos em bibliotecas dinâmicas (`.so` / `.dll`) com C-ABI estável para expansão de ferramentas e modding seguro em sandbox.

---

## 6. Verificação do Executável (.exe) e Execução dos Testes

O binário foi compilado com suporte nativo C++20 com flags de otimização máxima (`-O3 -pthread`).  
O executável encontra-se em:
- `/home/user/luax-ai-secret/bin/ApexEngine.exe` (Binário executável)
- `/home/user/luax-ai-secret/bin/ApexEngine` (Binário ELF)

Para compilar e executar o projeto a qualquer momento:
```bash
cd /home/user/luax-ai-secret
make clean
make
./bin/ApexEngine.exe 120
```
