# Assets - Fireline Wildfire Command

Este diretório contém assets originais do jogo.

## Estrutura planejada (para backend gráfico completo com raylib)

```
assets/
├── models/
│   ├── vehicles/
│   │   ├── vfci_unimog.obj
│   │   ├── tanker.obj
│   │   ├── helicopter.obj
│   │   └── ...
│   ├── vegetation/
│   │   ├── pine_mature.obj
│   │   ├── oak.obj
│   │   └── eucalyptus.obj
│   └── structures/
│       ├── watchtower.obj
│       ├── house_isolated.obj
│       └── fire_station.obj
├── textures/
│   ├── terrain/
│   │   ├── grass_albedo.png
│   │   ├── dirt_albedo.png
│   │   └── rock_normal.png
│   ├── vegetation/
│   ├── fire/
│   │   ├── flame_01.png (billboard)
│   │   └── smoke_alpha.png
│   └── vehicles/
├── shaders/
│   ├── terrain.vert/frag (triplanar, LOD)
│   ├── fire.vert/frag (emissive, animated UV, light)
│   ├── smoke_volumetric.frag (raymarching)
│   ├── dof.frag (depth of field, bokeh)
│   ├── motion_blur.frag (velocity buffer)
│   ├── water.frag (river/lake)
│   └── credits_3d.vert/frag (depth, perspective, soft shadow, reflection)
├── audio/
│   ├── fire_crackle.ogg
│   ├── tree_fall.ogg
│   ├── wind.ogg
│   ├── radio_chatter/
│   ├── siren.ogg
│   ├── breathing_mask.ogg
│   └── music/
│       ├── low_intensity.ogg
│       ├── high_intensity.ogg
│       └── extreme.ogg
└── fonts/
    └── fireline_font.ttf (para créditos 3D)
```

## Estado atual

O motor atual é **console + simulação** sem dependências externas para garantir compilação em qualquer ambiente. Todos os sistemas de rendering estão implementados como código em `src/graphics/Renderer.hpp` e `Credits.hpp`, prontos para ligar a raylib/OpenGL quando `FIRELINE_HAS_RAYLIB=1`.

Os efeitos visuais (DoF, motion blur, volumetric smoke, soft shadows, reflections) estão codificados como lógica e shaders stub, com fallback ASCII para o modo console que demonstra:

- Texto 3D com profundidade e perspectiva (via Mat4)
- Partículas de cinza e brasas (ParticleSystem)
- Fumo volumétrico (densidade + scattering)
- Câmara cinematográfica com movimentos lentos
- Pullback final revelando floresta queimada

Para ativar o backend gráfico completo:

```bash
make raylib
# ou
cmake .. -DFIRELINE_USE_RAYLIB=ON
```

Requer raylib instalado (`libraylib-dev`).

## Originalidade

Todos os assets listados seriam criados originalmente, sem cópia de outros jogos. O código atual não inclui assets protegidos, apenas simulação procedural e renderização via código.
