# Simulação de Fogo - Detalhes Técnicos

## Estados
```
NORMAL: saudável, pode incendiar
  ↓ heat > 150 * (1.5 - flammability)
HEATING: aquecimento pré-ignição, heat acumulando
  ↓ heat > 300 ou timeInState >5s
SMOKE: libertação de voláteis, smokeDensity aumentando
  ↓ timeInState > 2 + (1-flammability)*3
COMBUSTION: fogo ativo, burnProgress, fuelRemaining diminuindo, intensity = heatContent*0.015*fuel*(1+heat*0.001), temperature 600+intensity*0.5
  ↓ fuelRemaining <=0.05 ou burnProgress >=1
MOPUP: rescaldo, hotspotTemp 400°C+, arrefece lentamente, pode reacender se moisture<20% e fuel>0.1
  ↓ hotspotTemp<80 e waterContent>1 ou time>120 e hotspotTemp<100
BURNED: queimado, sem intensidade
```

## Flammability
```
moistureFactor = 1 - moisture/100
tempFactor = clamp((temperature-10)/40, 0.1, 2.0)
fuelFactor = fuelRemaining
vegFactor = combustionSpeed/2
flammability = clamp(moistureFactor*tempFactor*fuelFactor*vegFactor, 0,1)
* 0.05 se firebreak
* 0.2 se protegido por retardante
* clamp(1 - waterContent*0.1, 0.05,1)
```

## Propagação (Rothermel simplificado)
Para cada célula em COMBUSTION, verifica 8 vizinhos:

```
windAlignment = windDir.dot(spreadDir)
windFactor = 1 + windAlignment * windSpeedMS * 0.3, min 0.1
slopeFactor = 1 + tan(slopeRad)*1.5*max(0, uphillAlignment), clamp 0.2-5.0
distanceFactor = 1 / (1 ou 1.414 diagonal)
vegFactor = neighbor.combustionSpeed/2
humidityFactor = 1 - neighbor.moisture/100*0.8
tempFactor = clamp((ambientTemp-15)/30, 0.2,2.0)
baseProb = 0.02 * dt *10
spreadProb = baseProb * windFactor * slopeFactor * distanceFactor * vegFactor * humidityFactor * tempFactor * (intensity/100)
```

Se rand < spreadProb → heatTransfer = intensity*0.5*distanceFactor → neighbor.applyHeat

## Ember Spotting
```
Se veg.emberProduction>0.5 e windSpeed>5 e intensity>300:
  chance 0.001 * emberProduction por frame
  jump = windDir * rand(5,20) células
  se target.canIgnite() e rand<0.3 → ignição
```

Eucalipto tem emberProduction 0.9, pinheiro maduro 0.8 → principais causadores de spotting.

## Água
```
applyWater(liters):
  waterContent += liters
  temperature -= liters*8
  heat -= liters*30
  smokeDensity += liters*0.05 (vapor)
  se COMBUSTION: intensity -= liters*15, se <20 → MOPUP com hotspotTemp
  se MOPUP: hotspotTemp -= liters*10, se <60 → BURNED
  evaporação: se temp>100 waterContent -=0.1 por frame
```

## Firebreak
```
hasFirebreak=true, fuelRemaining=0, flammability=0
Criado por ferramentas manuais (Pulaski, McLeod, pá) ou veículos
Largura 2-4 células, bloqueia propagação (flammability*0.05)
```

## Vegetação
| Tipo | fuelLoad | combSpeed | moistureExt | heatContent | flameH | ember | fallProb | burnTime |
|------|----------|-----------|-------------|-------------|--------|-------|----------|----------|
| Capim Seco |0.3|3.5|15|18500|1.2|0.1|0|15s|
| Capim Verde|0.4|1.2|35|16000|0.8|0.05|0|20s|
| Arbusto Baixo|1.2|2.0|25|17500|2.0|0.2|0|40s|
| Arbusto Denso|2.5|2.8|20|18000|3.5|0.4|0|60s|
| Pinheiro Jovem|4.0|1.5|30|19500|6.0|0.5|0.3|90s|
| Pinheiro Maduro|8.0|1.8|28|20000|12.0|0.8|0.7|150s|
| Carvalho|6.0|0.9|35|19000|8.0|0.3|0.4|180s|
| Eucalipto|7.0|4.5|18|21000|15.0|0.9|0.6|120s|
| Mato Morto|1.8|3.0|12|18500|2.5|0.6|0|30s|
| Rasteira|0.8|2.2|22|17000|1.5|0.3|0|25s|

Eucalipto: mais perigoso, rápida combustão, muitas brasas, chama alta.

## Clima
- Vento varia a cada 10s ±15°, velocidade ±2 km/h, clamp 0-80
- Onda calor: temp lerp para 42°C, humidade para 12%
- Tempestade: 5% chance a cada 120s, duração 60-180s, vento 20-50, precipitação 5-20, humidade 70-95, relâmpagos a cada 5-15s com 30% chance de causar fogo
- Dia/noite: dayTime 0-24, daySpeed 0.02, temp base 25 + sin(day)*8

## Terreno
- Geração fractal: sin* cos *50 + sin*15 + cos*sin*80 + valley exp + ridge sin + flat central para aldeia
- Altura afeta vegetação: <30 capim, <60 arbusto, >30° inclinação mato morto, senão pinheiro/carvalho/eucalipto
- SlopeFactor: uphill dot spreadDir, tan(slope)*1.5*alignment

## Reacendimento
Em MOPUP, se hotspotTemp>200 e moisture<20 e fuel>0.1, chance 0.001*dt de voltar a COMBUSTION com burnProgress 0.7 e fuel 0.3.
Simula necessidade de rescaldo completo.

## Performance
- 256x256 grid = 65k células
- Update cada célula O(n)
- Propagação apenas para células em combustão (tipicamente <1k)
- 8 vizinhos por célula em combustão → ~8k checks por frame
- A 20Hz, ~160k checks/s, trivial
- Para 512x512 = 262k células, ~32k checks/s, ainda OK
- Multithread: dividir grid em chunks, paralelizar com job system

## Validação
Testes em `--test` verificam estados, flammability, água, veículos, despacho, créditos.
Demo em `--demo` mostra propagação, vento, spotting.
Jogo interativo mostra sistema completo com equipa, missões, clima, partículas.
