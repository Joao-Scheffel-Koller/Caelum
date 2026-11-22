# Z-fighting no chão do mapa

Implementação realizada com auxílio de IA (ver `prompts.md`).

## O problema

O mapa do Rio (exportado pelo OSM2World) tem várias superfícies **exatamente
na mesma altura** (Y = 0): a grama dos parques, o asfalto das ruas, as
calçadas, os caminhos de terra, os círculos de areia etc. Uma fica por cima da
outra, no mesmo plano.

O depth buffer guarda, em cada pixel, o que está mais perto da câmera. Quando
duas superfícies têm a mesma profundidade, quem fica "na frente" é decidido
por erros de arredondamento, que mudam de pixel para pixel e de quadro para
quadro. O resultado são listras e cintilação: o **z-fighting**.

```
  câmera
     \        areia  ─────────────   Y = 0
      \       grama  ─────────────   Y = 0   ← mesma profundidade: empate
```

## A ideia: prioridade por material

Subir a areia alguns centímetros não resolve: longe da câmera, a precisão do
depth buffer é pior que isso (near = 0,5 m, far = 30 km), e a geometria do
mapa também é usada nas colisões.

Usamos então **`glPolygonOffset`**. Ele **não move nenhum vértice**: apenas
empurra um pouco para trás o valor de profundidade **gravado no depth buffer**.
O deslocamento é medido em "degraus" do próprio depth buffer, e não em metros,
por isso funciona a qualquer distância.

Cada material do chão recebe uma **prioridade** (`DepthPriority`). Quanto
maior a prioridade, mais degraus para trás ele é empurrado e mais ele "perde"
para o que está em cima:

| Degraus | Nível | Materiais |
|---|---|---|
| 0 | `SURFACE` | faixas, bases de pista e de quadra, trilhos, telhados |
| 1 | `ASPHALT` | asfalto (ruas atravessam praças e áreas pavimentadas) |
| 2 | `SETT` | paralelepípedo, cascalho, piso-grama |
| 3 | `CONCRETE` | concreto, piso intertravado |
| 4 | `COBBLESTONE` | pedra irregular, seixos |
| 5 | `GROUND_PATCH` | areia, cascalho solto, água, brejo |
| 6 | `BARE_GROUND` | terra, arbustos |
| 7 | `GRASS` | grama (o fundo de todos os parques) |

O terreno de preenchimento (Y = −0,02) fica mais 3 degraus atrás da grama.

## O que foi corrigido agora

A primeira versão tinha só três níveis: superfícies, pavimentos e chão natural.
Materiais diferentes que caíam **no mesmo nível** continuavam empatados. Perto
do Santos Dumont, no Aterro do Flamengo, isso aparecia como:

- o **círculo de areia** (≈ 27 m, x ≈ 4240, z ≈ 8815): areia e grama eram ambos
  "chão natural", e a grama aparecia em listras sobre a areia;
- os **caminhos de terra** que contornam o retângulo e os ovais do parque:
  terra e grama empatavam, e o caminho aparecia quebrado.

Para não adivinhar, um script percorreu o mapa inteiro procurando pares de
materiais diferentes sobrepostos no mesmo plano. A nova escala dá **um nível
diferente para cada par encontrado**: areia sobre grama e terra, terra sobre
grama, paralelepípedo sobre concreto e pedra, asfalto sobre todos eles.

## Cuidado com o `factor`: prédios "flutuando"

A assinatura é `glPolygonOffset(factor, units)`:

- **`units`**: um deslocamento fixo e muito pequeno. É ele que define a
  **ordem** entre os níveis (2 unidades por nível).
- **`factor`**: é multiplicado pela **inclinação** da profundidade do
  triângulo na tela. Olhando o chão de forma rasante (avião no chão, olhando
  para o horizonte), essa inclinação é enorme.

Na primeira versão da nova escala, o `factor` também crescia com o nível
(grama = 7). Na margem do outro lado da baía, a vários km, a profundidade do
chão já é quase 1,0 (o fim do depth buffer). O empurrão extra levava esse chão
**para além de 1,0**: ele era descartado no teste de profundidade e o céu
aparecia numa faixa fina sob os prédios, que pareciam flutuar. Ao subir, o
ângulo deixa de ser rasante e o efeito sumia.

```
  antes:   ▆▆▆▆ prédios           depois:  ▆▆▆▆ prédios
           ~~~~ céu (chão sumiu)           ████ chão
           ████ chão perto                 ████ chão perto
```

A correção mantém a ordem em `units` e usa um `factor` pequeno:
`glPolygonOffset(0,25 × degraus, 2 × degraus)`. A grama fica com `factor`
1,75, menor que os 2 da escala original de três níveis, quando o problema não
existia. A vista `horizon-south` (olho a 2 m da pista, olhando para o sul)
confere que o chão continua desenhado sob os prédios, e as vistas
`coplanar-step-*` e `flamengo-*` conferem que o z-fighting não voltou.

## Onde está no código

| Arquivo | O que faz |
|---|---|
| `src/render/MaterialPolicy.hpp` | A escala `DepthPriority` (0 a 7). |
| `src/render/MaterialPolicy.cpp` | `GroundPriority()`: escolhe o nível pelo nome do material. A ordem dos testes importa (`grass_paver` antes de `grass`, `sandstone` antes de `sand`). |
| `src/render/ModelRenderer.cpp` | Antes de cada grupo de triângulos, liga `glPolygonOffset(0,25 × degraus, 2 × degraus)` (ver a seção abaixo). |
| `src/render/GameScene.cpp` | Terreno de preenchimento: 3 degraus além da grama. |
| `tests/MaterialPolicyTest.cpp` | Nível de cada material e, para cada par medido sobreposto no mapa, que os dois têm níveis diferentes. |
| `src/render/RenderInspection.cpp` | Vistas `flamengo-*` e `horizon-south` para conferir no jogo (`--texture-check`). |

## Limitações

- A ordem é por **material**, não por objeto: dois polígonos do *mesmo*
  material sobrepostos não são separados (como têm a mesma cor, não aparece).
- Algumas faixas pintadas (por exemplo, a ciclovia vermelha com a faixa
  tracejada) e bases `MAT_n` continuam no mesmo nível. São poucas e pequenas
  (menos de 200 m² no mapa todo).
- Um material novo no mapa precisa ser colocado na escala.
