# Pilotos sentados na cabine (branch `pilot`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

Este documento explica como o modelo do piloto (`data/pilot/pilot.obj`) é
desenhado sentado na cabine do **monomotor**. O A320 virá numa próxima etapa,
com a mesma técnica.

**Ideia em uma frase:** existe **uma única malha** de piloto na memória da
placa de vídeo, desenhada **uma vez por assento**, cada vez com uma matriz
Model diferente que leva o piloto das coordenadas dele até o assento dentro do
avião, e daí até o mundo, junto com o avião.

## 1. Dois sistemas de coordenadas

O piloto e o avião foram modelados por pessoas diferentes, cada um com seus
eixos:

| | Piloto (`pilot.obj`) | Monomotor (`aircraft.obj`) |
|---|---|---|
| Frente | **+Z** | **+X** (nariz) |
| Para cima | +Y | +Y |
| Origem | entre os pés (Y = 0) | centro do avião, rodas em Y = 0 |
| Unidade | metros | metros (modelo estilizado) |

Se desenhássemos o piloto com a mesma matriz Model do avião, ele apareceria na
origem do avião (entre as rodas), olhando para a ponta da asa.

## 2. Onde ele encosta no assento: o ponto de apoio

No piloto, medimos o ponto que deve tocar o banco (**ponto de apoio**):

- **Y = 0,40**: a parte de baixo do quadril;
- **Z = −0,17**: o plano das costas (o ponto mais atrás do corpo).

```
     piloto (vista lateral)         assento do monomotor
          O                              │ encosto (frente em X = 2,00)
         /|\___ mãos                     │
        / |                              │
   ●───┘  └──\      ● = apoio            └──────── almofada (topo em Y = 1,695)
   (Z = −0,17, Y = 0,40)                 ● = assento
```

No monomotor, cada assento é o ponto equivalente: **topo da almofada**
(Y = 1,695), **frente do encosto** (X = 2,00) e **centro do banco** (Z = −0,30
à esquerda, +0,30 à direita). Esses valores foram lidos dos objetos `SeatBase`
e `SeatBack` do OBJ.

## 3. A matriz de um assento

Para levar o piloto até um assento, montamos (de direita para esquerda, como
são aplicadas no ponto):

```
SeatMatrix = T(assento) · R_y(90°) · S(0,8) · T(−apoio)
```

1. **`T(−apoio)`**: move o piloto para que o ponto de apoio fique na origem.
   A partir daqui, rotação e escala acontecem "em torno do quadril".
2. **`S(0,8)`**: escala uniforme (ver seção 4).
3. **`R_y(90°)`**: gira 90° em torno do eixo Y. A matriz leva +Z em +X: a
   frente do piloto passa a apontar para o nariz.
   ```
   R_y(θ) = |  cos θ  0  sin θ |      com θ = 90°:  (0, 0, 1) → (1, 0, 0)
            |    0    1    0   |                    (1, 0, 0) → (0, 0, −1)
            | −sin θ  0  cos θ |
   ```
4. **`T(assento)`**: leva o ponto de apoio (que está na origem) até o assento.

Todas as matrizes vêm de `include/matrices.h` (`Matrix_Translate`,
`Matrix_Rotate_Y`, `Matrix_Scale`), sem as funções proibidas da GLM.

## 4. Por que a escala 0,8

O monomotor é **estilizado**: a cabine é menor que a de um avião real. A
câmera do cockpit, posicionada pela geometria do avião, fica com os olhos
**0,70 m** acima do assento. No modelo do piloto, os olhos ficam **0,89 m**
acima do quadril. Com 0,70 / 0,89 ≈ **0,8**, a cabeça do piloto fica na altura
dos olhos da câmera, abaixo do teto, e os pés no piso (afundam poucos
centímetros dentro da espessura do piso, invisível de fora).

## 5. Do assento até o mundo

O avião já tem sua matriz Model (`AircraftTransform`), que leva o OBJ do avião
para o mundo (posição e orientação calculadas pela física). A matriz do piloto
é o **produto**:

```
Model do piloto = Model do avião · SeatMatrix(assento)
```

Por isso, quando o avião sobe, gira ou capota, os pilotos vão junto, sem
nenhum cálculo extra. O mesmo vale para o avião do tráfego (curva de Bézier),
que usa o mesmo modelo: ele também ganha os dois pilotos, com a matriz Model
dele.

## 6. Instâncias: uma malha, vários desenhos

O piloto é carregado **uma vez** (`GameScene::Load`) e desenhado em
`GameScene::DrawCrew`:

```cpp
for(const auto& seat:crew.seats) {
    if(pilot::CameraInsidePilot(aircraftTransform,seat,crew.scale,camera))continue;
    renderer.Draw(pilotModel,aircraftTransform*pilot::SeatMatrix(seat,crew.scale),...);
}
```

No monomotor isso dá 2 pilotos no avião do jogador e 2 no do tráfego: **4
desenhos dos mesmos vértices com 4 matrizes Model diferentes**, o requisito de
"instâncias de objetos" do trabalho.

## 7. A câmera do cockpit

A câmera do cockpit fica no assento **esquerdo**, exatamente onde está um dos
pilotos. Se ele fosse desenhado, a câmera veria o interior da cabeça e dos
braços dele.

A regra é geométrica: **não desenhar o piloto que contém a câmera**. Para
testar, levamos a câmera para as coordenadas do próprio piloto com a
**inversa** da matriz Model dele. Lá, o corpo dele é uma caixa alinhada aos
eixos (medida no OBJ, braços e joelhos incluídos), e basta comparar X, Y e Z
com os limites da caixa:

```
p = (Model do avião · SeatMatrix)⁻¹ · câmera
escondido = p dentro da caixa do piloto
```

Assim, no cockpit o piloto da esquerda some e o da direita continua visível
(olhando para o lado). Na câmera externa os dois aparecem. A mesma regra
esconde um piloto se a câmera livre entrar nele.

## 8. Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/aircraft/PilotCrew.hpp/.cpp` | **Novo.** Ponto de apoio e caixa do piloto, `Crew` (assentos e escala), `SeatMatrix()` e `CameraInsidePilot()`. Sem OpenGL. |
| `src/aircraft/AircraftCatalog.hpp/.cpp` | Cada avião tem sua tripulação: o monomotor com 2 assentos e escala 0,8; o A320 ainda sem pilotos. |
| `src/render/GameScene.hpp/.cpp` | Carrega `pilot.obj` uma vez, guarda a tripulação (`SetCrew`) e desenha os pilotos do jogador e do tráfego (`DrawCrew`). |
| `src/main.cpp` | Ao escolher o avião no menu, passa a tripulação para a cena. |
| `src/render/RenderInspection.cpp` | Vista `pilot-front` (pelo para-brisa); usar com `--texture-check <prefixo> --aircraft=light`. |
| `tests/PilotCrewTest.cpp` | **Novo.** Apoio cai no assento, frente para o nariz, escala, pilotos dentro da cabine (teto, piso, painel, encosto, largura), sem sobreposição entre os dois, câmera do cockpit esconde só o piloto da esquerda. |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam `PilotCrew.cpp` e o teste `pilot_crew`. |

## 9. Limitações

- O piloto é estático: a animação do modelo original não foi exportada, então
  ele não mexe as mãos com os comandos.
- O monomotor não tem janelas laterais: os pilotos aparecem pelo para-brisa
  (de frente e de cima), não pela lateral.
- Os pés afundam poucos centímetros dentro do piso da cabine.
