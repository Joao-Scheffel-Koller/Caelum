# Hélice animada do monomotor (branch `propeller`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

**Ideia em uma frase:** a hélice é separada do resto do avião no carregamento
do OBJ e desenhada com a própria matriz Model, que a mantém no nariz e a gira
em torno do seu eixo por um ângulo θ. θ avança com o tempo do quadro
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

**A escala desigual.** `data/light_aircraft/tools/convert_asset.py` ajustou o
modelo original a 9 m × 3 m × 11 m com um fator diferente por eixo:
`S = (0,99562; 1,13289; 1,375)` (comprimento, altura, envergadura). Por isso,
no OBJ, a hélice aparece deformada: pás de 0,92 m e 1,10 m, a ±115°.

**O eixo inclinado.** Desfazendo essa escala, as 3 pás são iguais e
coincidem ao girar 120° (erro menor que 0,1 mm) em torno de um eixo que **não
é X**: ele aponta **12,9° para cima**. O cone usa o mesmo eixo. No OBJ
(deformado), o plano das pás fica a 11,4° da vertical. O avião está modelado
pousado sobre a roda da cauda, o que pode explicar a inclinação.

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
pás  = Model do avião · T(cubo) · S(1,2481) · R_z(11,4°) · R_x(θ) · R_z(−12,9°) · S⁻¹ · T(−cubo)
cone = Model do avião · T(cubo) ·     S     · R_z(12,9°) · R_x(θ) · R_z(−12,9°) · S⁻¹ · T(−cubo)
disco = Model do avião · T(cubo) · R_z(11,4°)
```

Lida da direita para a esquerda, aplicada a cada vértice:

1. **`T(−cubo)`**: leva o centro das pás, (4,2646; 1,9345; 0), para a origem.
2. **`S⁻¹`**: desfaz a escala desigual; a hélice volta a ser redonda.
3. **`R_z(−12,9°)`**: alinha o eixo da hélice com X.
4. **`R_x(θ)`**: gira. θ positivo leva cima para a direita: sentido horário
   visto da cabine.
5. **`R_z(...)`**: devolve a inclinação. Nas pás são 11,4°, a inclinação do
   plano delas no OBJ.
6. **Escala final**: no **cone**, o mesmo `S`, para ele continuar encaixado na
   fuselagem, que também foi esticada. Nas **pás**, uma escala **uniforme**
   (1,2481, média geométrica da altura e da envergadura): com `S`, cada pá
   esticaria e encolheria cerca de 22% por volta. O raio final é 1,035 m.
7. **`T(cubo)`** e **`Model do avião`**: de volta ao nariz e depois ao mundo.
   Por isso a hélice **segue todos os movimentos do avião**.

As matrizes vêm de `include/matrices.h`. As normais usam a inversa da
transposta da Model (já no vertex shader), correta com escala desigual.

### O erro da primeira versão

A primeira versão girava em torno de X: `T(cubo) · R_x(θ) · T(−cubo)`. Como o
plano das pás é inclinado, a inclinação girava junto, como um prato torto, e a
ponta das pás saía **36 a 43 cm** do plano a cada volta.

```
   errado: gira em X            certo: gira no eixo da hélice
      ╲  →  │  →  ╱                  ╲  →  ╲  →  ╲
   (a inclinação muda)           (o plano não muda)
```

O teste da época não pegou o erro. Ele comparava vértices do cone com o mesmo
X, mas em cada fatia quase só havia pares espelhados, que passariam com quase
qualquer eixo. O teste atual gira a hélice 120° e exige que ela **coincida
consigo mesma**, e verifica que o eixo antigo é reprovado.

## 5. θ avança com o tempo, na velocidade da potência

A cada quadro, `GameScene::Update(Δt, throttle)` atualiza a hélice
(`propeller::Spin`):

```
rpm_alvo = throttle × 2700                  (180 hp, Lycoming O-360)
rpm      = rpm_alvo + (rpm − rpm_alvo) · e^(−Δt / 1 s)
θ       += rpm_mostrado · 2π / 60 · Δt
```

- **Δt, não quadros.** Tudo é multiplicado pelo tempo do quadro: a hélice gira
  igual num computador rápido ou lento, e para no menu de pausa (o laço não chama `Update`).
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

O disco (`ModelRenderer::CreateDisc`) é um círculo de 48 fatias com o raio das
pás (1,035 m), na cor das faces delas (`Propeller_Red`). Tem duas faces, 1 mm
uma da outra, para ser iluminado dos dois lados, e fica no plano das pás (um
disco não precisa girar).

**Transparência.** No MTL as pás têm opacidade 0,24 para **não tapar a vista
do cockpit** (ver `metadata.json`). Por fora elas agora são sólidas quando
lentas. Na vista do cockpit, a opacidade nunca passa de 0,24
(`CockpitBladeOpacity`), e a do disco não passa de 0,08
(`CockpitDiscOpacity`): a 0,30 ele tingia a pista inteira. `ModelRenderer::Draw` tem um multiplicador de
opacidade: abaixo de 1, o modelo vai para o passo transparente, depois dos
sólidos e ordenado do mais longe para o mais perto, junto com os vidros. O
disco é desenhado antes das pás fracas.

## 7. Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/aircraft/Propeller.hpp/.cpp` | **Novo.** `Spec` (objetos, cubo, escala do asset, inclinação do eixo, raio, rpm máximo), `Spin` (rotação do motor e θ, por Δt), limites de aliasing, opacidade das pás (fora e no cockpit) e do disco, `BladeMatrix()`, `SpinnerMatrix()` e `DiscMatrix()`. Sem OpenGL. |
| `src/aircraft/AircraftCatalog.hpp/.cpp` | A hélice do monomotor (objetos, eixo medido, 1,035 m, 2700 rpm); o A320 sem hélice. |
| `src/render/ModelRenderer.hpp/.cpp` | `LoadSplit()` (um OBJ, várias peças, cada uma com sua caixa), `CreateDisc()` e o multiplicador de opacidade em `Draw()`. |
| `src/render/GameScene.hpp/.cpp` | Carrega as peças, guarda a caixa do avião inteiro (pivô e colisões), atualiza as hélices do jogador e do tráfego em `Update(Δt, throttle)` e as desenha. |
| `src/main.cpp` | Passa a hélice do avião escolhido e o throttle (0 se acidentado) para a cena. |
| `tests/PropellerTest.cpp` | **Novo.** Rotação segue o throttle, gradualmente e igual a 30/60/144 FPS; pausa não move; nunca mais de 60° por quadro a 30 FPS; pás e disco mudam de opacidade no sentido certo; os objetos existem no OBJ; pás e cone coincidem consigo mesmos ao girar 120° (e o eixo antigo é reprovado); as pás ficam no plano do disco e mantêm o raio em todos os ângulos; o cone parado fica onde o OBJ o tem; sentido horário visto da cabine; opacidade no cockpit; a caixa do corpo + hélice é a do modelo inteiro (pivô inalterado). |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam `Propeller.cpp` e o teste `propeller`. |

## 8. Limitações

- A rotação é só visual: o empuxo continua vindo da física
  (`FlightModel::EngineThrust`), não da rotação da hélice.
- A física não tem marcha lenta: com throttle 0 o motor não produz empuxo e a
  hélice fica parada.
- As pás são desenhadas redondas, então diferem um pouco das do OBJ.
- Abaixo de 30 FPS, a rotação mostrada (300 rpm) pode voltar a ter aliasing.
- O A320 não tem peças giratórias; os fans dos motores poderiam usar o mesmo
  mecanismo se fossem objetos separados no OBJ.
