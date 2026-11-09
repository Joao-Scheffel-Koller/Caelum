# Física de voo realista (branch `physics`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

Esta etapa troca os parâmetros "de brinquedo" (`mass = 10`, `gravity = -1`,
`lift_factor = 0.5`, `drag_factor = 0.9`, `max_thrust = 80`) por valores reais
do **Airbus A320-200**, e o modelo de forças por um modelo aerodinâmico de
verdade. O monomotor recebeu valores de um **Cessna 172S**, a classe do modelo
3D, para que ele não voe como um avião de 60 toneladas.

## 1. Por que não bastava trocar os números

O modelo antigo calculava `sustentação = k·v²` sempre na direção `up` do avião,
independente do ângulo de ataque. Com números reais isso não funciona: o avião
sustentaria o peso em qualquer atitude, inclusive com o nariz para baixo. Na
física real, a sustentação depende do **ângulo de ataque** (α, o ângulo entre o
nariz e o vento relativo) e é **perpendicular ao vento relativo**, não ao avião.
Por isso o `FlightModel` foi reescrito com coeficientes aerodinâmicos.

## 2. Parâmetros (`src/aircraft/FlightProfile.cpp`)

### Airbus A320-200

| Parâmetro | Valor | Fonte / justificativa |
|---|---|---|
| Massa | 60 000 kg | MTOW 78 t, OEW ~42,6 t (Airbus *Aircraft Characteristics*). 60 t é uma massa típica de decolagem em voos curtos; operações no Santos Dumont (pista de 1 323 m) são limitadas por peso. |
| Área de asa S | 122,6 m² | Airbus AC |
| Envergadura b | 34,10 m | Sem *sharklets*, como a malha (34,36 m) |
| Alongamento AR = b²/S | 9,48 | calculado |
| Eficiência de Oswald e | 0,80 | valor típico de asa enflechada |
| Fator de arrasto induzido k = 1/(π·e·AR) | 0,042 | calculado |
| CD0 (arrasto parasita, limpo) | 0,020 | dá L/D máximo = 1/(2√(CD0·k)) ≈ 17, valor publicado do A320 |
| CL0 (α = 0, limpo) | 0,25 | arqueamento + incidência da asa |
| CLα (inclinação da curva de sustentação) | 4,8 /rad | Helmbold: 2πAR/(2+√(AR²+4)) ≈ 5,1, corrigido pelo enflechamento de 25° |
| CL mínimo | −0,8 | estol negativo |
| Mach crítico | 0,70 | Lock: divergência de arrasto em Mcrit + 0,108 ≈ 0,80 |
| Empuxo estático | 2 × 120,1 kN | CFM56-5B4 (27 000 lbf cada) |
| Fator de carga (limpo) | +2,5 g / −1,0 g | FCOM, lei normal |
| Fator de carga (flaps) | +2,0 g / 0 g | FCOM, lei normal |
| Taxa máxima de rolagem | 15°/s | lei normal: *sidestick* todo para o lado |
| Distância entre eixos | 12,64 m | Airbus AC |
| Ângulo da bequilha | 70° (*tiller*) → 6° (pedais) | *tiller* até 75°, pedais ±6° |
| Rotação | 3°/s a partir de ~135 kt (VR) | procedimento padrão |
| Atrito de rolamento / freio | 0,015 / 0,40 | pneu em asfalto seco / frenagem forte |

Configurações de flaps (CLmax calibrado pelas velocidades de estol 1 g
publicadas):

| Flaps | ΔCL0 | CLmax | ΔCD0 | Estol a 60 t |
|---|---|---|---|---|
| 0 | 0 | 1,50 | 0 | 140 kt |
| 1+F (decolagem) | 0,40 | 2,10 | 0,010 | 119 kt |
| 2 | 0,55 | 2,35 | 0,020 | — |
| 3 | 0,70 | 2,55 | 0,030 | — |
| FULL (pouso) | 0,90 | 2,85 | 0,050 | 102 kt (Vref ≈ 1,23·Vs ≈ 126 kt) |

### Monomotor (classe Cessna 172S)

Massa 1 000 kg, S = 16,2 m², b = 11,0 m, CD0 = 0,031 (trem fixo), e = 0,75,
CL0 = 0,30, CLα = 4,6 /rad, 180 hp com hélice de eficiência 0,80 (empuxo
estático ~2,9 kN), flaps 0/10/20/30, +3,8/−1,52 g (categoria normal), 45°/s de
rolagem e distância entre eixos de 6,1 m (medida na malha). Resultado: estol
limpo 48 kt e com flaps 42 kt (o 172S publica 48 e 40 kt).

### Outras constantes

- **Gravidade**: 9,80665 m/s² (o mundo está em metros).
- **Atmosfera padrão ISA** (ICAO Doc 7488), com altitude = Y do mapa
  (nível do mar em Y = 0):
  `T = 288,15 − 0,0065·h`, `ρ = 1,225·(T/288,15)^4,2559`, `a = √(1,4·287,05·T)`.

## 3. Forças (`FlightModel::update`)

A cada passo de física:

1. **Ângulo de ataque**: `α = atan2(−v̂·up, v̂·forward)`.
2. **Coeficientes**:
   - região linear: `CL = CL0 + ΔCL0(flaps) + CLα·α`;
   - além do estol, CL cai `stallDrop` por radiano, sem ficar abaixo do valor de
     placa plana `sin(2α)`;
   - polar de arrasto: `CD = CD0 + ΔCD0 + k·CL² + 20·(M − Mcrit)⁴`
     (aumento transônico de Lock), com mínimo de placa plana `1,28·sin²α`.
3. **Forças**, com pressão dinâmica `q = ½·ρ·V²`:
   - sustentação `L = q·S·CL`, perpendicular ao vento relativo, no plano de
     simetria do avião;
   - arrasto `D = q·S·CD`, oposto à velocidade;
   - empuxo ao longo do nariz:
     - turbofan: `T = manete·T0·σ·(1 − 0,49·√M)` (Mattingly), sendo σ = ρ/ρ0;
     - hélice: `T = manete·min(T0, η·P/V)·σ` (potência constante);
   - peso `m·g`;
   - no solo, atrito `μ·N` contra o movimento horizontal, com `N = m·g − L`
     (nunca negativo). Com o freio, μ inclui o atrito de frenagem. O atrito
     pode parar o avião, mas nunca empurrá-lo para trás.
4. **Integração** (Euler semi-implícito): `a = F/m`, `v += a·dt`, `p += v·dt`.

A manete agora é uma fração de 0 a 1 do empuxo máximo, e `W`/`S` aceitam
segurar a tecla.

## 4. Lei de controle: mouse vertical = fator de carga (como o A320)

O A320 é *fly-by-wire*: o *sidestick* não move o profundor diretamente, mas
pede um **fator de carga** (g). O jogo faz o mesmo:

- **Mouse centralizado** pede `n = cos γ / cos φ`: mantém o ângulo de trajetória
  γ e compensa a inclinação φ até 33°, exatamente como a lei normal. Além disso,
  o γ do momento em que o mouse volta ao centro é guardado, e pequenos desvios
  (aceleração, atraso do α) são corrigidos (*flight path stability*).
- **Mouse para baixo** aumenta `n` até +2,5 g (sobe/cabra); **para cima** reduz
  até −1 g (desce/pica). Com flaps, os limites são +2 g e 0 g.
- O α necessário é calculado de `n·m·g = q·S·CL`. Ele é limitado ao
  **α de proteção** (2° antes do estol), então a lei nunca estola o avião, e α
  responde com um atraso de primeira ordem (τ = 0,6 s, o "período curto").
- O corpo do avião é então posicionado a partir da nova velocidade, girado de α
  e **sem derrapagem** (voo coordenado).

Consequência importante: **a curva agora vem da física**. Ao inclinar, a
sustentação inclinada tem componente horizontal que curva a trajetória, e o
nariz acompanha. A taxa de curva resulta em `g·tan φ / V`, como no avião real.
Nos testes, com 30° de inclinação a 130 m/s: 24° de curva em 10 s, contra 25°
pela teoria. A guinada artificial (`turnRate`, `directYawRate`) e o alinhamento
artificial da velocidade em voo, da branch `steering`, foram removidos.

Continua não havendo limite de arfagem nem de rolagem. Um looping depende de
energia: com 2,5 g a 140 kt, o A320 perde velocidade antes do topo, como um
A320 de verdade.

## 5. Solo: rotação, direção e freio

- **Rotação**: o profundor só vence o peso sobre a bequilha com pressão
  dinâmica suficiente; a autoridade cresce com `(V/VR)²`. Mouse para baixo gira
  o nariz a até 3°/s. Perto de VR, o mouse no centro **mantém** a atitude de
  rotação; abaixo disso, ou com o mouse para cima, o nariz volta para a pista.
- **Direção**: modelo de bicicleta, `taxa_guinada = V·tan(δ)/distância_entre_eixos`.
  O ângulo máximo δ vai de 70° (*tiller*, em táxi) até 6° (pedais, acima de
  30 m/s).
- **Freio**: segure `.` (tecla padrão do MSFS).
- **Flaps**: `F6` recolhe e `F7` estende um estágio (padrão do MSFS). O avião
  nasce em 1+F (A320) ou 10° (monomotor).
- O HUD mostra o manche, os flaps, o α e o freio.

### Volume de colisão da cauda do A320

O volume antigo da fuselagem era um cilindro de raio constante cuja barriga
ficava a 0,97 m do solo. Com ele, a cauda tocava a pista com apenas ~3° de
rotação, o que tornaria impossível uma decolagem real (o A320 decola com
7–10°). Medindo a parte de baixo da malha: a barriga fica a 1,49 m e o cone de
cauda sobe (2,06 m em X = −20,5; ~3,5 m em X = −25). O volume agora segue esse
perfil com esferas de raio variável, e o *tail strike* passa a acontecer em
~9°, o valor da própria malha (o A320 real tem ~11,7°). Nos testes, a rotação
padrão decola com 6,8°.

## 6. Resultados medidos (`tests/FlightModelTest.cpp`)

| Grandeza | Simulado | Referência real |
|---|---|---|
| Estol limpo / 1+F / FULL (A320, 60 t) | 140 / 119 / 102 kt | ~145 / ~120 / ~105 kt |
| L/D máximo | 16,4 (a 3°) | 16–18 |
| Decolagem A320, 1+F, TOGA | 914 m, 143 kt, 6,8° | ~1 000 m, 140–150 kt, 7–10° |
| Cruzeiro em potência máxima a 10,5 km | Mach 0,81 | MMO 0,82 |
| Frenagem de 117 kt até parar (sem reverso) | 560 m | 500–800 m |
| Estol do monomotor limpo / flaps | 48 / 42 kt | 48 / 40 kt (C172S) |
| Corrida de decolagem do monomotor | 182 m, 57 kt | ~290 m (C172S, MTOW) |

O teste também verifica a atmosfera ISA, o empuxo estático de 240 kN e sua
queda com a altitude, a proteção de α, o voo nivelado com o mouse centralizado
e a curva coordenada sem perda de altitude.

```bash
ctest --test-dir build --output-on-failure -R "flight_model|flight_controls"
```

## 7. Arquivos

| Arquivo | Mudança |
|---|---|
| `src/aircraft/FlightProfile.hpp/.cpp` | **Novo.** Parâmetros físicos do A320 e do monomotor, com fontes. |
| `src/aircraft/FlightModel.hpp/.cpp` | Reescrito: ISA, coeficientes, empuxo, atrito, lei de fator de carga, flaps. |
| `src/aircraft/Aircraft.hpp/.cpp` | Perfil, flaps, α, freio e referência de trajetória substituem `lift_factor`/`drag_factor`/`max_thrust`. |
| `src/aircraft/FlightControls.hpp/.cpp` | Em voo só faz a rolagem; no solo faz direção por bicicleta, rotação e aderência dos pneus. |
| `src/collision/AircraftContacts.cpp` | Volume da fuselagem do A320 segue a malha (cone de cauda). |
| `src/render/GameScene.cpp` | O *spawn* restaura flaps de decolagem, α, freio e referência. |
| `src/main.cpp` | Perfil por aeronave, `F6`/`F7`, freio `.`, manete com repetição, HUD. |
| `tests/FlightModelTest.cpp` | **Novo.** Desempenho comparado com os números reais. |
| `tests/FlightControlsTest.cpp`, `CMakeLists.txt`, `Makefile.macOS` | Atualizados. |

## 8. Limitações e próximos passos

- Não há trem de pouso recolhível, *spoilers*, reverso nem marcha lenta (com a
  manete em 0, o empuxo é 0).
- O vento é zero, a velocidade mostrada é a verdadeira (não há velocidade
  indicada) e não há limitação de VMO/MMO.
- A rotação é cinemática (sem momentos de inércia); a dinâmica de arfagem é o
  atraso de primeira ordem de α.
- O monomotor usa a mesma lei de fator de carga do A320, por simplicidade; um
  avião convencional teria comando direto do profundor.
- O monomotor da malha tem bequilha na cauda (*taildragger*), mas a rotação no
  solo é a mesma do A320 (nariz para cima). Um *taildragger* real levanta a
  cauda primeiro.
- Nada disso foi testado pilotando no jogo; os números vêm dos testes de CPU e
  da inspeção de colisões no mapa real.
