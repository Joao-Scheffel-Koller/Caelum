# Hélice animada do monomotor (branch `propeller`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

**Ideia em uma frase:** a hélice é separada do resto do avião no carregamento
do OBJ e desenhada com a própria matriz Model, que a mantém no nariz e a gira
em torno do eixo longitudinal por um ângulo θ. θ avança com o tempo do quadro
numa velocidade dada pela potência (throttle), e, quando a hélice gira rápido
demais para ser mostrada sem erro, as pás se transformam num disco
translúcido.

## 1. A hélice dentro do OBJ

O OBJ do monomotor já tinha a hélice em objetos próprios (linhas `o`), com
nomes herdados do modelo original:

| Objeto | O que é | Material |
|---|---|---|
| `wing`, `wing.001`, `wing.002` | as 3 pás | `Propeller_Red` (faces) e `Propeller_Cyan` (bordas) |
| `wingRoot` | o cone central (*spinner*) | `Red`, `Cyan` (os mesmos da fuselagem) |

**O eixo.** O cone é um sólido de revolução: em cada fatia de X, todos os seus
vértices estão à mesma distância da reta **Y = 1,9327, Z = 0**. Essa reta,
paralela a X (o eixo longitudinal do avião), é o eixo da hélice. Girando em
torno de qualquer outra reta, o cone "bambolearia".

**As pás não são simétricas.** A de cima está a 0°, mede 0,92 m e fica um
pouco mais para trás; as de baixo estão a ±115° (não 120°) e medem 1,10 m. É
uma imprecisão do modelo low-poly; como giram juntas com o cone, isso não
atrapalha.

## 2. Separar no carregamento

Antes, `ModelRenderer::Load` juntava os triângulos **por material** e
descartava os nomes dos objetos. Como o cone usa os mesmos materiais da
fuselagem, ele ia parar no mesmo lote de desenho que o corpo: não havia como
girá-lo sozinho.

`ModelRenderer::LoadSplit` lê o OBJ **uma vez** e entrega vários modelos: os
objetos de cada lista vão para um modelo próprio, e o resto vai para o corpo.
O monomotor vira três peças:

| Peça | Objetos | Comportamento |
|---|---|---|
| corpo | todo o resto | como antes |
| pás | `wing`, `wing.001`, `wing.002` | giram e viram disco |
| cone | `wingRoot` | gira, sempre sólido |

O arquivo do modelo não muda. Se um nome não existir no OBJ, o carregamento
falha com uma mensagem, em vez de desenhar o avião sem hélice.

## 3. O pivô do avião

O avião é posicionado pelo centro da sua **caixa envolvente**
(`AircraftTransform`): esse centro vai para a posição calculada pela física.
Sem a hélice, a caixa do corpo fica mais curta na frente, e o centro dela
**anda 12,9 cm para trás**: o avião inteiro andaria 12,9 cm para a frente.

Por isso, a cena guarda a caixa do **avião inteiro**, a união das caixas do
corpo e das peças da hélice (`airframeMin`/`airframeMax`), e usa essa caixa
em todo lugar que antes usava a do modelo:

- o pivô do avião do jogador e do avião do tráfego (Bézier);
- o volume de colisão (`AircraftContacts`), nos 6 lugares em que é
  configurado.

O resultado é idêntico ao de antes da separação.

## 4. A matriz Model da hélice

```
Model das pás = Model do avião · T(cubo) · R_x(θ) · T(−cubo)
```

Lida da direita para a esquerda, aplicada a cada vértice das pás:

1. **`T(−cubo)`**: leva o eixo da hélice para a origem. Os vértices continuam
   nas coordenadas do OBJ do avião; só o cubo vai para a origem.
2. **`R_x(θ)`**: gira em torno do eixo X, que agora passa pelo cubo.
3. **`T(cubo)`**: devolve a hélice ao nariz.
4. **`Model do avião`**: leva tudo para o mundo. Por isso a hélice **segue
   todos os movimentos do avião** (subir, inclinar, capotar).

```
R_x(θ) = | 1    0      0   |     θ > 0:  (0, 1, 0) → (0, cos θ, sin θ)
         | 0  cos θ −sin θ |     cima → direita (+Z), sentido horário
         | 0  sin θ  cos θ |     visto da cabine
```

O sentido horário visto da cabine é o dos motores Lycoming/Continental usados
nesse tipo de avião. O cone usa a mesma matriz das pás. As matrizes vêm de
`include/matrices.h` (`Matrix_Translate`, `Matrix_Rotate_X`).

## 5. θ avança com o tempo, na velocidade da potência

A cada quadro, `GameScene::Update(Δt, throttle)` atualiza a hélice
(`propeller::Spin`):

```
rpm_alvo = throttle × 2700                  (180 hp, Lycoming O-360)
rpm      = rpm_alvo + (rpm − rpm_alvo) · e^(−Δt / 1 s)
θ       += rpm_mostrado · 2π / 60 · Δt
```

- **Δt, não quadros.** Tudo é multiplicado pelo tempo do quadro: a hélice gira
  igual num computador rápido ou lento, e para no menu de pausa (Δt = 0).
- **O motor não muda de rotação na hora.** A rotação segue o throttle com uma
  constante de tempo de 1 s, como um motor real acelerando. A fórmula com
  `e^(−Δt/τ)` é exata para qualquer Δt: depois do mesmo tempo, a rotação é a
  mesma a 30 ou a 144 FPS.
- **Regra do trapézio.** Enquanto a rotação muda, θ avança pela média da
  velocidade no início e no fim do quadro. Usando só a do fim, quadros longos
  adiantavam a hélice (o teste mediu 3° de diferença entre 30 e 60 FPS); com a
  média, a diferença some.
- **Throttle 0 = hélice parada.** Na física do jogo, throttle 0 dá empuxo zero
  (não há marcha lenta), então a hélice parada é coerente. Cada toque em `W`
  (+5%) acrescenta 135 rpm.
- **Acidente:** com o avião acidentado, a hélice recebe throttle 0 e para aos
  poucos.
- **Tráfego:** o avião da curva de Bézier, que usa o mesmo modelo, tem a
  própria hélice, a 75% de potência constante.

## 6. Aliasing (efeito "roda de carroça")

A 2700 rpm a hélice dá 45 voltas por segundo. A 60 FPS, isso é 270° por
quadro. Como 3 pás se repetem a cada 120°, o olho só percebe o resto da
divisão: 270° − 240° = **30°**, e a hélice parece girar devagar. Em outras
rotações, parece parada ou girando **para trás**. É o mesmo efeito das rodas
de carro nos filmes.

Para o movimento ser visto corretamente, cada quadro precisa girar as pás
**menos que metade do padrão** (60°), o limite de Nyquist. Com folga para
30 FPS:

```
60° por quadro × 30 quadros/s = 1800°/s = 5 voltas/s = 300 rpm
```

A solução tem duas partes:

1. **Rotação mostrada limitada a 300 rpm** (`DisplayedRpm`). Até 300 rpm as
   pás giram na velocidade real; acima disso, continuam girando a 300 rpm.
2. **Pás viram disco.** Entre 300 e 900 rpm, as pás ficam transparentes (até
   12% de opacidade) e um **disco translúcido** aparece no lugar (até 30%),
   imitando o borrão de uma hélice real. Acima de 900 rpm, só se vê o disco e
   um contorno fraco das pás.

```
 rpm:     0 ──────── 300 ──────────── 900 ─────────── 2700
 pás:     sólidas     │  ficam transparentes  │   12%
 disco:   ─           │  aparece              │   30%
 giro:    real        │  limitado a 300 rpm (sem aliasing)
```

O disco (`ModelRenderer::CreateDisc`) é um círculo de 48 fatias com o raio da
maior pá (1,10 m), na cor das faces das pás (`Propeller_Red`). Tem duas faces,
1 mm uma da outra, para ser iluminado dos dois lados. Fica no centro da
hélice: `Model do disco = Model do avião · T(cubo)` (um disco não precisa
girar).

**Transparência.** O material das pás tinha opacidade 0,24 no MTL: era um
"borrão" fixo, para a hélice parecer em movimento sem animação. Agora o
movimento é animado, então essa opacidade é trocada por 1 no carregamento, e a
transparência passa a depender da rotação. `ModelRenderer::Draw` ganhou um
multiplicador de opacidade: com ele abaixo de 1, o modelo vai para o passo
transparente, desenhado depois dos sólidos e ordenado do mais longe para o
mais perto, junto com os vidros. O disco é desenhado antes das pás fracas.

## 7. Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/aircraft/Propeller.hpp/.cpp` | **Novo.** `Spec` (objetos, cubo, raio, rpm máximo), `Spin` (rotação do motor e θ, por Δt), limites de aliasing, opacidade das pás e do disco, `BladeMatrix()` e `DiscMatrix()`. Sem OpenGL. |
| `src/aircraft/AircraftCatalog.hpp/.cpp` | A hélice do monomotor (objetos, eixo medido, 1,10 m, 2700 rpm); o A320 sem hélice. |
| `src/render/ModelRenderer.hpp/.cpp` | `LoadSplit()` (um OBJ, várias peças, cada uma com sua caixa), `CreateDisc()` e o multiplicador de opacidade em `Draw()`. |
| `src/render/GameScene.hpp/.cpp` | Carrega as peças, guarda a caixa do avião inteiro (pivô e colisões), atualiza as hélices do jogador e do tráfego em `Update(Δt, throttle)` e as desenha. |
| `src/main.cpp` | Passa a hélice do avião escolhido e o throttle (0 se acidentado) para a cena. |
| `tests/PropellerTest.cpp` | **Novo.** Rotação segue o throttle, gradualmente e igual a 30/60/144 FPS; pausa não move; nunca mais de 60° por quadro a 30 FPS; pás e disco mudam de opacidade no sentido certo; a matriz gira em torno do eixo, no sentido horário visto da cabine; os objetos existem no OBJ; o cone é centrado no eixo; o raio do disco é o da maior pá; a caixa do corpo + hélice é a do modelo inteiro (pivô inalterado). |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam `Propeller.cpp` e o teste `propeller`. |

## 8. Limitações

- A rotação é só visual: o empuxo continua vindo da física
  (`FlightModel::EngineThrust`), não da rotação da hélice.
- A física não tem marcha lenta: com throttle 0 o motor não produz empuxo e a
  hélice fica parada.
- Abaixo de 30 FPS, a rotação mostrada (300 rpm) pode voltar a ter aliasing.
- O A320 não tem peças giratórias; os fans dos motores poderiam usar o mesmo
  mecanismo se fossem objetos separados no OBJ.
