# Vento nas árvores (branch `wind-animation`)

Implementação realizada com auxílio de IA (ver `prompts.md`).

Este documento explica como as árvores do mapa balançam com o vento, item
prometido no `SPEC.md` em "Animações baseadas no tempo (Δt)".

**Ideia em uma frase:** a cada quadro, antes de desenhar uma árvore, empurramos
os cantos de cima do seu retângulo um pouco para o lado e de volta, enquanto
os cantos de baixo ficam parados. A árvore parece balançar.

## 1. O que é uma árvore neste jogo

A árvore não é um modelo 3D. É um **retângulo vertical plano** com a imagem de
uma árvore (partes transparentes da imagem dão o formato). São 2 triângulos e 4
cantos (vértices). Exemplo real do OBJ (`rio_x+002_z+003.obj`):

```
 cima-esquerda (y=10 m)     cima-direita (y=10 m)
        ┌──────────────────┐
        │      🌳          │   ← imagem da árvore
        │    (textura)     │
        └──────────────────┘
 baixo-esquerda (y=0)       baixo-direita (y=0)
```

Há milhares desses retângulos espalhados pelo mapa (cerca de 12 000 triângulos
de árvores só no trecho do Santos Dumont), já embutidos nos arquivos do mapa
(materiais `TREE_BILLBOARD_BROAD_LEAVED_0` e `TREE_BILLBOARD_CONIFEROUS_0`).

## 2. Como a GPU desenha: o vertex shader

A cada quadro, a placa de vídeo recebe os vértices de todos os triângulos e,
para **cada vértice**, executa um pequeno programa, o **vertex shader**
(`src/render/shaders/shader_vertex.glsl`), cuja função é dizer onde esse
vértice vai parar na tela (aplicando as matrizes Model, View e Projection).

O ponto-chave: **podemos mudar a posição do vértice dentro desse programa**, a
cada quadro, sem mexer nos arquivos do mapa. Se a posição muda um pouco de um
quadro para o outro, o objeto parece se mover.

## 3. Fazendo a árvore balançar

Se movermos **só os cantos de cima** para o lado e deixarmos os de baixo
parados, o retângulo inclina:

```
  sem vento        vento p/ direita       vento p/ esquerda
 ┌────────┐            ┌────────┐       ┌────────┐
 │        │           ╱        ╱         ╲        ╲
 │   🌳   │          ╱   🌳   ╱           ╲   🌳   ╲
 │        │         ╱        ╱             ╲        ╲
 └────────┘        └────────┘               └────────┘
 (base fixa)       (base fixa)              (base fixa)
```

A imagem estica junto com o retângulo: a copa se move e o tronco continua
enraizado. Repetindo "direita → centro → esquerda → centro" continuamente,
temos o balanço.

## 4. Como o shader sabe qual vértice é o de cima

O shader vê um vértice por vez e não sabe "isto é o topo da árvore". Mas cada
vértice traz a **coordenada de textura** (u, v), que diz qual ponto da imagem
vai naquele canto. Nessas árvores, **v = 0 na base e v = 1 no topo**
(conferido no OBJ). Então `v` funciona como uma "altura de 0 a 1":

> **deslocamento = v² × (quanto o vento empurra agora)**

Na base (v = 0) nada se move; no topo (v = 1), move o máximo. Usamos `v²` em
vez de `v` porque uma árvore real verga mais perto do topo: com `v²`, o meio
move só ¼ do topo. Com 4 cantos por retângulo a diferença é sutil, mas é o
formato correto caso as árvores virem malhas 3D no futuro.

## 5. "Quanto o vento empurra agora": seno no tempo

Para ir e voltar suavemente, usamos um seno do tempo:

```
empurrão(t) = A · sin(ω · t)
```

- `t`: tempo decorrido em segundos;
- `A`: amplitude, o deslocamento máximo (0,30 m no topo);
- `ω`: velocidade (2,1 rad/s, um balanço completo a cada ~3 s).

**De onde vem `t` (requisito de Δt):** `GameScene::Update(elapsed)` já é
chamada uma vez por quadro com o tempo decorrido (move o avião Bézier e pisca
as luzes). Ali fazemos `windClock.Update(elapsed)` e enviamos o tempo ao
shader. Como é tempo real, a árvore balança na mesma velocidade em qualquer
computador (requisito "animação baseada no tempo") e **para com o menu de
pausa**.

## 6. Problema: todas as árvores juntas

Só com `sin(ω·t)`, **todas as árvores da cidade balançariam perfeitamente
sincronizadas**, como uma coreografia. Damos a cada árvore um **atraso
(fase)** que depende de onde ela está, ao longo da direção do vento:

```
empurrão(t) = A · sin(ω · t + fase),   fase = −k · (posição · direção_do_vento)
```

Como a fase muda devagar com a posição (k = 0,05 rad/m, rajadas de ~125 m),
**árvores vizinhas se movem quase juntas e árvores distantes ficam
defasadas**: parece uma rajada atravessando o parque, e ela anda **no sentido
do vento**.

```
instante 1:   🌳/ 🌳/ 🌳| 🌳\ 🌳\     ← a onda de vento
instante 2:   🌳| 🌳/ 🌳/ 🌳| 🌳\     ← andou uma árvore para a direita
```

## 7. Problema: regular demais

Um seno só tem um ritmo óbvio. Somamos um segundo seno, menor e mais rápido:

```
empurrão = 0,7·sin(ω·t + fase) + 0,3·sin(2,3·ω·t + 1,7·fase)
```

Como 2,3 não é inteiro, as duas ondas raramente se alinham e o movimento não
parece repetir.

**Folhas:** além do balanço grande, a copa (v > 0,4) tem uma vibração pequena
(4 cm) e rápida (9 rad/s), com fase própria por vértice, como folhas
farfalhando.

## 8. Só as árvores devem se mover

O mesmo vertex shader desenha tudo (prédios, pista, aviões). Por isso o C++
envia um "interruptor" (uniform) `wind_strength`: **1 ao desenhar materiais de
árvore, 0 para o resto**. Com 0, o deslocamento é zero.

## 9. Por que na GPU e não no C++

Mover as árvores no C++ exigiria recalcular e reenviar milhares de vértices à
placa de vídeo a cada quadro (lento). No vertex shader, a GPU faz isso em
paralelo para todos os vértices, quase sem custo, sem alterar os dados do
mapa. É assim que jogos animam grama e árvores (T. Sousa, *Vegetation
Procedural Animation and Shading in Crysis*, GPU Gems 3, cap. 16, 2007).

## 10. Resumo

| Peça | Função |
|---|---|
| `v` da textura | diz a altura do vértice na árvore (0 = base, 1 = topo) |
| `v²` | o tronco verga mais perto do topo |
| `sin(ω·t)` | vai e volta no tempo |
| `windClock` avançado por `elapsed` | animação baseada no tempo; para na pausa |
| fase pela posição ao longo do vento | árvores defasadas, rajada atravessando a cidade |
| segundo seno | movimento irregular, natural |
| vibração da copa | folhas farfalhando |
| `wind_strength` | só árvores se movem |

## 11. Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/render/WindSway.hpp/.cpp` | **Novo.** `WindParameters` (direção, amplitude, velocidade, comprimento da rajada, folhas), `WindClock` (tempo acumulado por Δt) e `WindOffset()`, a mesma fórmula do shader em C++ para testes. |
| `src/render/shaders/shader_vertex.glsl` | Uniforms `wind_*` e o deslocamento aplicado à posição no mundo antes da câmera. |
| `src/render/MaterialPolicy.hpp/.cpp` | Novo campo `sway`: verdadeiro só para materiais de árvore do mapa (nunca nos aviões). |
| `src/render/ModelRenderer.hpp/.cpp` | `SetWind()`, localização dos uniforms (também ao recarregar shaders com `R`), envio dos parâmetros e de `wind_strength` por material. |
| `src/render/GameScene.hpp/.cpp` | `windClock` avançado em `Update(dt)` e repassado ao renderer em `Draw`. |
| `src/render/RenderInspection.cpp` | Vistas `wind-a`/`wind-b` (mesma câmera, ~0,5 s de diferença) para conferir que só as copas se movem. |
| `tests/WindSwayTest.cpp` | **Novo.** Base nunca se move, amplitude limitada, meio move menos que o topo, movimento suave a 60 FPS, vizinhas juntas e distantes defasadas, rajada anda com o vento, relógio por Δt. |
| `tests/MaterialPolicyTest.cpp` | Só árvores têm `sway`. |
| `CMakeLists.txt`, `Makefile.macOS` | Compilam `WindSway.cpp` e o teste `wind_sway`. |

Para ajustar o vento (mais forte, mais lento, outra direção), basta mudar os
valores padrão de `WindParameters` em `src/render/WindSway.hpp`.

## 12. Limitações

- O volume de colisão das árvores continua parado (30 cm no topo não afetam o
  jogo).
- As normais não são recalculadas; com esse deslocamento a iluminação quase não
  muda.
- As caixas usadas para descartar trechos fora da câmera ignoram os 30 cm.
- O vento tem direção e força fixas.
