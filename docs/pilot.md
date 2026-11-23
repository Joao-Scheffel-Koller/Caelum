# Pilotos sentados na cabine (branch `pilot`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

Este documento explica como o modelo do piloto (`data/pilot/pilot.obj`) é
desenhado sentado na cabine do **monomotor** e do **A320**. As seções 1 a 7
usam o monomotor como exemplo; a seção 8 mostra o que muda no A320.

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
(Y = 1,695) e **frente do encosto** (X = 2,00), lidos dos objetos `SeatBase` e
`SeatBack` do OBJ. Na lateral, os pilotos ficam em **Z = −0,24** (esquerda) e
**Z = +0,26** (direita), um pouco para dentro do centro dos bancos (±0,30);
ver a seção 4.

## 3. A matriz de um assento

Para levar o piloto até um assento, montamos (de direita para esquerda, como
são aplicadas no ponto):

```
SeatMatrix = T(assento) · R_y(90°) · S(0,72) · T(−apoio)
```

1. **`T(−apoio)`**: move o piloto para que o ponto de apoio fique na origem.
   A partir daqui, rotação e escala acontecem "em torno do quadril".
2. **`S(0,72)`**: escala uniforme (ver seção 4).
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

## 4. Escala e posição lateral: medidas nas malhas

O monomotor é **estilizado**: a cabine é menor que a de um avião real. A
primeira versão usava escala **0,8** (a câmera do cockpit tem os olhos 0,70 m
acima do assento; o piloto, 0,89 m acima do quadril) e os pilotos no centro
dos bancos (Z = ±0,30). Mas o braço do lado da janela **atravessava o vidro**:
na altura dos ombros, as paredes da cabine se inclinam para dentro.

Para escolher novos valores sem adivinhar, medimos nas malhas reais: para cada
vértice do piloto já posicionado, um raio sai da linha central da cabine
(Z = 0) em direção ao vértice. Se o raio cruza a casca do avião (janelas,
paredes) antes de chegar ao vértice, aquele ponto está **para fora**.

```
        janela          linha central         janela
          │                   ┆                   │
          │   ●───────────────┤ raio              │
   ●──────┼───────────────────┤  ← cruza a janela: vértice para fora
          │                   ┆                   │
```

| Escala | Z esq. / dir. | Saindo pela janela (esq. / dir.) | Folga entre os pilotos |
|---|---|---|---|
| 0,80 | −0,30 / +0,30 | 9,4 cm / 6,7 cm | 10,4 cm |
| 0,80 | −0,25 / +0,25 | 4,4 cm / 1,7 cm | 2,7 cm |
| 0,76 | −0,25 / +0,25 | 2,6 cm / 0 | 3,8 cm |
| **0,72** | **−0,24 / +0,26** | **0 / 0** | **5,8 cm** |

Só aproximar os pilotos do centro não basta: antes de os braços saírem da
janela, eles já se encostariam. A escolha foi **escala 0,72** com **Z = −0,24
/ +0,26**: nada atravessa a casca e sobra a maior folga entre os dois. O lado
esquerdo vai mais para dentro (6 cm contra 4 cm) porque o modelo do piloto é
assimétrico: o braço do lado de fora alcança mais longe. Com 0,72, os pés
ficam apoiados no piso e a cabeça fica 6 cm abaixo dos olhos da câmera do
cockpit (que não desenha esse piloto).

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

Cada avião tem 2 pilotos no avião do jogador e 2 no do tráfego: **4 desenhos
dos mesmos vértices com 4 matrizes Model diferentes**, o requisito de
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

## 8. O A320

A técnica é a mesma; mudam só os números, lidos dos objetos `a_chair*` (os
bancos) do OBJ do A320.

| | Comandante (esquerda) | Copiloto (direita) |
|---|---|---|
| Centro do banco (Z) | −0,69 | +0,69 (piloto em +0,70) |
| Frente do encosto na altura dos ombros (X) | 7,73 | 7,73 |
| Topo da almofada (Y) | 0,147 (apoio em 0,16) | 0,147 (apoio em 0,16) |

- **Escala 1,0:** o A320 está em tamanho real (38 m de comprimento), então o
  piloto também. Os olhos dele ficam praticamente onde está a câmera do
  cockpit, que, por isso, esconde o comandante.
- **Encosto reclinado:** a frente do encosto muda com a altura. Como o ponto
  mais atrás do piloto são os ombros, o apoio usa a frente do encosto **na
  altura dos ombros** (X = 7,73). Mais para trás, as costas entrariam no
  encosto.
- **Apoio em Y = 0,16 em vez de 0,147:** com o topo da almofada, os sapatos
  afundariam 2 cm no piso da cabine (Y = −0,258). Subindo 1,3 cm, os pés
  ficam no piso.
- **Copiloto 1 cm para fora (Z = +0,70):** o modelo do piloto é assimétrico e
  o joelho direito do copiloto entrava 9 mm na almofada de joelho sob o
  painel.

**Teste de encaixe no A320.** O raio da seção 4 saía da linha central da
cabine. No A320 isso não funciona: entre a linha central e os pilotos fica o
console central (manetes), e todos os raios o cruzariam. Por isso o raio
agora sai do **centro do próprio assento** em direção a cada vértice: o braço
de fora encontra a janela ou a parede se a atravessar, e o braço de dentro
encontra o console central. O mesmo teste continua valendo para o monomotor.

No A320, o teste ignora os bancos e uma **coluna estreita** sob o painel
(`dash-8_single.001` e `.005`, 5 cm de largura), alinhada ao centro de cada
banco: ela passa **entre os joelhos** do piloto, como a coluna dos pedais.

```
   vista de cima (comandante)
        painel
   ┌──────────────┐
   │      ▐▌      │ ← coluna, entre os joelhos
   │   ╲  ▐▌  ╱   │
   │    joelhos   │
   │      ●       │ ← centro do banco: origem dos raios
   └──────────────┘
```

## 9. Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/aircraft/PilotCrew.hpp/.cpp` | **Novo.** Ponto de apoio e caixa do piloto, `Crew` (assentos e escala), `SeatMatrix()` e `CameraInsidePilot()`. Sem OpenGL. |
| `src/aircraft/AircraftCatalog.hpp/.cpp` | Cada avião tem sua tripulação: o monomotor com 2 assentos (Z = −0,24 e +0,26) e escala 0,72; o A320 com 2 assentos (Z = −0,69 e +0,70) e escala 1,0. **É aqui que se ajusta a posição dos pilotos.** |
| `src/render/GameScene.hpp/.cpp` | Carrega `pilot.obj` uma vez, guarda a tripulação (`SetCrew`) e desenha os pilotos do jogador e do tráfego (`DrawCrew`). |
| `src/main.cpp` | Ao escolher o avião no menu, passa a tripulação para a cena. |
| `src/render/RenderInspection.cpp` | Vistas `pilot-front` (monomotor, usar com `--texture-check <prefixo> --aircraft=light`) e `a320-pilots-front`/`a320-pilots-side` (A320, pelo para-brisa e pela janela do comandante). |
| `tests/PilotCrewTest.cpp` | **Novo.** Apoio cai no assento, frente para o nariz, escala, pilotos do monomotor dentro da cabine (teto, piso, painel, encosto, largura); nos dois aviões, a câmera do cockpit esconde só o piloto da esquerda. |
| `tests/PilotFitTest.cpp` | **Novo.** A medição das seções 4 e 8 nas malhas reais, nos dois aviões: nenhum vértice do piloto atravessa janelas, paredes ou o console central, e os dois pilotos ficam a mais de 2 cm um do outro. |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam `PilotCrew.cpp` e os testes `pilot_crew` e `pilot_fit`. |

## 10. Limitações

- O piloto é estático: a animação do modelo original não foi exportada, então
  ele não mexe as mãos com os comandos.
- O monomotor não tem janelas laterais: os pilotos aparecem pelo para-brisa
  (de frente e de cima), não pela lateral.
