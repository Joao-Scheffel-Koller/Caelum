# Controle da aeronave pelo mouse (branch `steering`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

> **Atualização (branch `physics`)**: o manche virtual (seção 3) continua igual,
> mas em voo o mouse vertical passou a pedir **fator de carga** em vez de taxa
> de arfagem, e a curva/alinhamento da velocidade agora vêm das forças
> aerodinâmicas. A guinada artificial (`turnRate`, `directYawRate`) foi
> removida, e taxas de rolagem, direção no solo e rotação vêm do perfil de
> cada aeronave. Veja [physics.md](physics.md).

Este documento explica o controle de atitude da aeronave pelo mouse, no estilo
do GeoFS: o mouse funciona como um **manche virtual**.

- Mouse **abaixo** do centro da janela → **nariz para cima** (cabrar).
- Mouse **acima** do centro → **nariz para baixo** (picar).
- Mouse à **direita** → o avião **inclina para a direita e curva para a direita**.
- Mouse à **esquerda** → inclina e curva para a esquerda.

## 1. Arfagem, rolagem e guinada — o que muda em relação à ideia inicial

A ideia inicial era: mouse vertical altera a **arfagem** (pitch) e mouse
horizontal altera **guinada** (yaw) e **rolagem** (roll). Ela está quase certa,
com uma correção de causa e efeito:

- Num avião real, mover o manche para o lado comanda os **ailerons**, que
  produzem apenas **rolagem**. A curva acontece *porque* o avião está
  inclinado: a sustentação, perpendicular às asas, passa a ter uma componente
  horizontal que puxa a trajetória para o lado. O leme (guinada direta) é
  usado apenas para coordenar a curva.
- Por isso, no código a guinada é calculada principalmente **a partir da
  inclinação** (curva coordenada), e não diretamente do mouse:

  ```
  taxa_guinada = turnRate * sin(rolagem) + directYawRate * comando_lateral
  ```

  O primeiro termo é a curva causada pela inclinação; o segundo é uma pequena
  guinada direta (como um leme automático), que dá resposta imediata ao mouse.

O resultado final é o que se esperava (mouse para o lado → rola e guina), mas
com o comportamento de um avião: com o mouse centralizado após inclinar, o
avião **continua curvando** enquanto estiver inclinado, e só para de curvar
quando as asas voltam ao nível.

## 2. Arquivos alterados e criados

| Arquivo | Mudança |
|---|---|
| `src/aircraft/FlightControls.hpp/.cpp` | **Novo.** Manche virtual e controle de atitude (matemática pura, sem GLFW/OpenGL). |
| `src/main.cpp` | Lê o cursor a cada quadro, aplica os controles antes da física e mostra os comandos no HUD. |
| `tests/FlightControlsTest.cpp` | **Novo.** Testes sem janela do manche e das rotações. |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam o novo módulo e registram o teste `flight_controls`. |

`FlightModel` (forças), `Aircraft` e o tratamento de colisões não foram
modificados.

## 3. Manche virtual: do cursor ao comando (`MouseYoke`)

A posição **absoluta** do cursor (não o deslocamento) é convertida em dois
comandos normalizados em `[-1, 1]`:

```
nx = (x - largura/2) / (largura/2)    → comando de rolagem
ny = (y - altura/2)  / (altura/2)     → comando de arfagem
```

Como no GLFW o eixo Y da janela cresce **para baixo**, `ny > 0` (mouse abaixo
do centro) já corresponde a **nariz para cima**, sem inversão de sinal.

Cada eixo passa ainda por:

1. **Saturação**: valores fora da janela são limitados a ±1.
2. **Zona morta** (`deadZone = 5%`): pequenos desvios perto do centro são
   ignorados, para que seja fácil manter o voo nivelado.
3. **Curva de resposta** (`|v|^1.5`): o comando cresce devagar perto do centro
   e rápido nas bordas, dando precisão para correções pequenas.

O manche **mantém o último valor** quando:

- o botão direito está pressionado (o cursor fica capturado para a câmera, e
  sua posição deixa de ter significado);
- a janela está sem foco;
- o menu/pausa está aberto (a física não roda).

## 4. Rotações da aeronave (`ApplyControls`)

A orientação do avião é guardada como três vetores (`forward`, `up`, `right`),
e não como ângulos de Euler. Por isso as rotações são feitas **em torno dos
eixos do próprio avião**, com a **fórmula de Rodrigues** (a mesma usada em
`Matrix_Rotate` de `matrices.h`, vista em aula):

```
v' = v·cos(θ) + (k × v)·sin(θ) + k·(k·v)·(1 − cos(θ))
```

Os comandos são **de taxa** (como no GeoFS): o mouse define a *velocidade* de
rotação, e não o ângulo. Em cada passo de física (`dt`):

| Movimento | Eixo de rotação | Taxa máxima |
|---|---|---|
| Arfagem (profundor) | `right` do avião | `maxPitchRate = 0,5 rad/s` |
| Rolagem (ailerons) | `forward` do avião | `maxRollRate = 1,0 rad/s` |
| Guinada (curva) | vertical do mundo `(0,1,0)` | `(turnRate·sin(rolagem) + 0,1·comando)·cos(arfagem)` |

Convenções de sinal (sistema destro, `right = forward × up`):

- ângulo positivo em torno de `right` levanta o nariz;
- ângulo positivo em torno de `forward` inclina a asa direita para baixo;
- curva para a direita é um ângulo **negativo** em torno de +Y.

Depois das rotações, a base é reortonormalizada (Gram-Schmidt), para que erros
de ponto flutuante não deformem o modelo ao longo do tempo:

```
forward = normalize(forward)
right   = normalize(forward × up)
up      = right × forward
```

### Sem limites de atitude e estabilidade

- **Não há ângulo máximo** de arfagem nem de rolagem: é possível fazer
  loopings, tonneaux (giros de 360°) e voar de dorso. Como a orientação é
  guardada em vetores e as rotações são feitas nos eixos do avião, não existe
  *gimbal lock* ao passar pela vertical.
- A curva coordenada gira em torno da vertical do mundo; por isso ela é
  multiplicada por `cos(arfagem)`. Com o nariz na vertical, girar em torno de
  +Y só faria a fuselagem rodar sobre si mesma, então o efeito some ali.
- Com o comando lateral na zona morta, uma **estabilidade lateral simples**
  (`rollStability`) devolve lentamente as asas ao nível, **apenas com o avião
  em pé** (|rolagem| < 90°). De dorso ou no topo de um looping ela não age, para
  não desfazer a manobra. Para um comportamento mais "puro", basta zerar essa
  constante.

### Velocidade acompanha o nariz

O `FlightModel` atual empurra o avião na direção `forward`, mas nada fazia a
velocidade mudar de direção quando o nariz gira. Sem isso, cabrar apenas
inclinaria o avião sem mudar a trajetória. Por isso a **direção** da
velocidade é aproximada do nariz a cada passo, sem alterar seu módulo:

```
blend   = 1 − e^(−taxa·dt)
direção = normalize(direção + (forward − direção)·blend)
```

A taxa é `1,5/s` em voo (simplificação da estabilidade direcional) e `6/s` no
solo (atrito dos pneus). O uso de `e^(−taxa·dt)` deixa o resultado
independente da taxa de quadros.

## 5. Comportamento no solo

O estado `Grounded`, vindo da resolução de contatos do passo anterior
(`AircraftContacts`), muda o comportamento:

- **Arfagem e rolagem não são comandadas.** O pivô do modelo fica no centro da
  fuselagem; girar em torno dele com as rodas no chão bateria a cauda ou a asa
  no asfalto (acidente). Em vez disso, o avião volta suavemente à atitude de
  solo — isso também abaixa o nariz depois do pouso.
- **O comando lateral vira direção da bequilha**: o avião gira em torno da
  vertical, com taxa proporcional à velocidade (parado, não gira; a partir de
  5 m/s, gira a 0,5 rad/s).
- A decolagem continua acontecendo pelo `FlightModel` existente (a sustentação
  supera o peso ao ganhar velocidade). Assim que o avião sai do solo, arfagem e
  rolagem passam a responder ao mouse.

## 6. Integração no laço principal

Em `main.cpp`, a cada quadro:

1. Se a janela tem foco e o cursor está livre (`GLFW_CURSOR_NORMAL`), lê-se a
   posição do cursor e o tamanho da janela e calcula-se `g_MouseYoke`.
2. Antes de `MainModel.update`, chama-se
   `flightcontrols::ApplyControls(MainAircraft, g_MouseYoke, grounded, physicsStep)`.
   A mudança de atitude acontece **depois** de `previousPose` ser registrada,
   então a detecção contínua de colisões varre a rotação entre a pose antiga e a
   nova, como já fazia com a translação.
3. Com o HUD ligado (`H`), uma linha mostra os comandos atuais:
   `Manche (mouse): arfagem +32% | rolagem -10%`.

## 7. Como testar

Testes sem janela:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R flight_controls
```

Eles verificam: centro neutro, zona morta, sinais de cada borda, saturação fora
da janela, rotação de Rodrigues, cabrar/picar, inclinação e curva para cada
lado, looping completo, giro de 360°, voo de dorso mantido, retorno das asas ao
nível, base ortonormal, e o comportamento no
solo (parado não gira, rolando faz curva, nariz baixa após o pouso).

No jogo:

1. Aumente a potência com `W` e deixe o mouse perto do centro; use o mouse para
   os lados para manter o avião na pista.
2. Depois de decolar, mova o mouse para baixo para subir e para cima para descer.
3. Mova o mouse para a direita: o avião inclina e começa a curvar. Volte o mouse
   ao centro: a curva continua enquanto houver inclinação e as asas voltam
   devagar ao nível. O instrumento de horizonte e a bússola mostram o resultado.

## 8. Limitações conhecidas

- As constantes de `ControlTuning` são de jogabilidade e valem para as duas
  aeronaves; o A320 e o monomotor respondem igualmente.
- A sustentação do `FlightModel` ainda não depende do ângulo de ataque, então o
  avião tende a subir com velocidade alta mesmo com o nariz nivelado. Ajustar
  isso é trabalho de física, fora do escopo desta branch.
- Não há rotação de decolagem (levantar o nariz no chão), pois exigiria girar em
  torno do trem principal em vez do pivô central do modelo.
- Como o manche usa a posição absoluta do cursor, ao voltar do menu o comando
  corresponde a onde o mouse ficou; centralize o mouse ao retomar.
