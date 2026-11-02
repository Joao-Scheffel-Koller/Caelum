# Colisões do Caelum

Notas técnicas geradas com auxílio de IA, registradas em `prompts.md`. A implementação
começou depois do commit/push `d5f0d57`, que publicou primeiro o guia
`implementation.md`, conforme a sequência solicitada. README e SPEC permanecem intactos.

## Organização

| Arquivos | Responsabilidade |
|---|---|
| `src/collision/collisions.hpp/.cpp` | Geometria pura: AABB, ponto mais próximo e sweep contínuo esfera/triângulo. |
| `CollisionWorld.hpp/.cpp` | Geometria física compacta, política de superfícies, proxies de árvore e BVH por chunk. |
| `AircraftContacts.hpp/.cpp` | Volumes do A320, apoio do trem, resposta ao contato e estados de voo/acidente. |
| `CollisionDebug.hpp/.cpp` e `shaders/` | Esferas físicas em wireframe, selecionadas por B. |
| `ModelRenderer.*` | Encaminha os vértices já lidos do OBJ ao mundo físico durante a mesma leitura usada para a GPU. |
| `GameScene.*` | Configura volumes com os limites reais do modelo, aplica resultados e fornece inspeção com o mapa real. |
| `main.cpp` | Ordena física/contatos/câmera, trata Enter/B e apresenta o estado. |

Os três primeiros módulos dependem de vetores e da biblioteca padrão; não usam
OpenGL, GLFW ou a posição da câmera. Os testes geométricos estão no arquivo
`collisions.cpp`, como exige o enunciado. Transformações e interpolações novas
são calculadas diretamente, sem construtores de câmera/matriz do GLM.

## Detecção contínua

Uma consulta informa o centro inicial e final de uma esfera e seu raio. O
resultado contém existência do contato, fração temporal em `[0,1]`, ponto,
normal, penetração inicial e tipo de superfície.

O teste não compara somente a posição final. Resolve candidatos contra as duas
faces deslocadas pelo raio, os três cilindros das arestas e as três esferas dos
vértices. Escolhe o primeiro instante válido. Assim, atravessar uma parede inteira
em um quadro ainda produz contato antes de passar para o outro lado. Há casos
explícitos para sobreposição inicial, tangência, raio zero e triângulos reduzidos
a segmentos ou pontos. Entradas não finitas não produzem normais NaN no jogo.

Antes desse teste detalhado, usa-se a AABB do deslocamento inteiro, expandida pelo
raio. Cada chunk tem uma BVH binária por mediana dos centros, com até oito
triângulos por folha. Os limites são calculados dos vértices efetivos; uma face
pode atravessar a célula nominal do arquivo porque o splitter a distribui por
centroide. O frustum da câmera não participa das consultas físicas.

As posições retidas no mundo são floats, assim como os dados do mapa; os
candidatos selecionados são promovidos a double para os cálculos detalhados.
Não são duplicadas as UVs, normais ou imagens no índice físico. A BVH acrescenta
custo de memória e de inicialização: o mapa continua sendo carregado inteiro.

## Superfícies e preparação do mapa

- `Ground`: asfalto, terra, grama, pavimentação e famílias de terreno reconhecidas.
- `Water`: superfícies de água; contato representa acidente.
- `Solid`: prédios, telhados, árvores físicas e demais obstáculos.
- Pintura/decal: fica fora da geometria física, evitando contatos repetidos com
  as camadas quase coplanares de faixas, setas e janelas sobrepostas.

O concreto é ambíguo. A extração considera apoio o `CONCRETE` horizontal próximo
de Y=0; concreto elevado ou vertical continua sólido. Essa regra é específica do
mapa atual, cujo nível do aeroporto foi conferido em Y=0. Materiais desconhecidos
ficam sólidos, e `SANDSTONE` não é confundido com areia.

As imagens de árvore têm cantos transparentes. Em vez de colidir com o retângulo
completo, os triângulos dos billboards fornecem centro, altura e largura. As
partes da mesma instância são agrupadas por centro/altura quantizados a 0,1 m.
São gerados um tronco octogonal e uma copa cônica dentro da imagem. Trata-se de
um volume físico aproximado; não reproduz cada ramo ou pixel. O desenho original
das árvores não é alterado.

A base `base_terrain.obj`, limitada ao extrato e em Y=-0,02, também oferece apoio
onde falta geometria original. Ela segue o chão genérico visível, sem fornecer
elevação real ou recuperar a classificação geográfica das lacunas. A água real
do mapa, em Y=0, é encontrada antes dessa base. Nos empates de tempo, a consulta
prioriza água, depois sólido, depois chão. A base não se estende infinitamente.

## Volumes do avião e transformação

Os limites efetivos do A320 configuram cadeias de esferas sobrepostas para a
fuselagem, asas, cauda e motores. Elas aproximam cápsulas, evitando preencher
todo o espaço vazio de uma única caixa ao redor da aeronave. Três esferas
separadas representam o contato do trem de pouso.

O pivô coincide com o renderer: centro dos limites em X/Z e mínimo em Y. O nariz
local é +X, o eixo vertical é +Y e o lado direito é +Z. Os pontos do trem foram
aproximados a partir do A320 atual: roda dianteira perto de X=5,5 e principais
perto de X=-7,5, Z=±3,3 nas coordenadas originais do modelo.

A orientação é extraída dos vetores forward/up, ortonormalizada e convertida por
fórmulas próprias para quaternion. A rotação entre poses usa interpolação pelo
arco curto, com subpassos de até 5°. Dentro de cada subpasso, cada esfera tem um
sweep contínuo. O tratamento rotacional é uma aproximação por subpassos; não é
uma colisão exata entre duas malhas arbitrárias em rotação.

O modo B desenha as mesmas esferas retornadas pelo resolver: verdes para o corpo,
amarelas para o trem. O renderer de depuração recebe a câmera somente para
projetar os volumes; a detecção continua independente dela. Os pontos são
subtraídos da câmera antes de converter para float, e o estado OpenGL alterado
pelo overlay é restaurado.

## Apoio, pouso e acidente

A sequência por quadro passou a ser:

```text
guardar pose anterior
→ FlightModel prevê posição/velocidade
→ AircraftContacts encontra o primeiro contato
→ aplicar pose/velocidade corrigidas
→ atualizar câmera e HUD
```

O antigo clamp de Y=0 saiu do `FlightModel`: ele eliminava a velocidade de
impacto antes de distinguir pouso de acidente. As forças existentes de empuxo,
gravidade, sustentação e arrasto continuam com seus parâmetros anteriores.

Os estados são `Flying`, `Grounded` e `Crashed`. No contato do trem com apoio
adequado, a correção levanta o avião até a superfície e elimina somente a
velocidade descendente. Mantém o deslocamento horizontal de taxiamento. Uma
sondagem curta de 8 cm conserva o contato entre quadros, e uma roda que começa
a subir não fica presa à tangência inicial.

As condições configuradas para este simulador simplificado são:

| Condição | Limite atual |
|---|---|
| Velocidade de descida no primeiro contato | até 3 m/s |
| Pitch no contato | até 15° em módulo |
| Roll no contato | até 10° em módulo |
| Normal de apoio | componente Y de pelo menos 0,85 |

Esses números são parâmetros de gameplay, não critérios de certificação ou
instruções de pouso de uma aeronave real. O contato de asas, fuselagem, motores
ou cauda com o chão é acidente, assim como atingir água ou um sólido. Um impacto
forte ou uma atitude inadequada também produz acidente.

No acidente, o jogador para no instante do contato e sua potência é zerada.
O estado permanece congelado até reiniciar. Câmeras e avião autônomo continuam
funcionando; a câmera livre permite inspecionar o local.

Dois detalhes do contato das rodas têm regressões próprias. Um contato de chão
no início da consulta não pode esconder uma barreira baixa ou água encontrados
depois: a busca de perigos ignora apenas candidatos de apoio com normal para
cima e continua procurando. Após um pouso suave, o movimento restante é testado
do ponto de toque até o destino corrigido no chão, com tempo remapeado. Usar uma
diagonal do ponto ainda no ar até o destino final poderia pular um obstáculo
baixo logo após o toque. A mesma busca de perigos vale para uma roda saindo do
chão durante a decolagem.

## Controles e inspeção

- `B`: mostrar/ocultar volumes físicos do jogador.
- `Enter`: reiniciar o jogador no Santos Dumont e restaurar a câmera atual.
- `C/F/L`: câmeras continuam disponíveis inclusive depois de um acidente.
- `R`: conserva a função anterior de recarregar os shaders 3D.

Na raiz:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

A partir de `bin/Debug`:

```powershell
.\main.exe --collision-check ../../build/collision-check.ppm orbit
.\main.exe --collision-check ../../build/collision-check.ppm orbit --keep-open
```

O modo opcional carrega os assets reais e usa um resolver local para conferir
apoio das rodas no Santos Dumont, taxiamento, decolagem, pouso forte, estado de
acidente, reinício e amostras reais de água e sólido. Registra candidatos testados
pela BVH em relação ao total. Gera um relatório `.ppm.txt` e uma captura após
60 quadros, com volumes visíveis e verificação de erros OpenGL. Os testes usam
os dados reais do mapa; não substituem a geometria normal por uma cena fictícia.

Os três novos alvos CPU são `collisions`, `collision_world` e `aircraft_contacts`.
Eles cobrem fórmulas, precisão nas coordenadas do Rio, faces/arestas/vértices,
geometria degenerada, atravessamento de paredes, água coplanar, BVH local, volumes
de árvore, rodas, pouso, asas, mudança de orientação e regressões de obstáculos
baixos antes/depois do toque e durante a decolagem. As nove suítes anteriores
continuam no CTest, totalizando doze no ambiente configurado.

## Limitações e separação do objetivo de jogo

Os volumes são aproximações, sem deformação, suspensão detalhada ou colisão por
triângulo da malha do A320. Árvores usam proxies, materiais exigem classificação
heurística e a base de preenchimento continua genérica. O sistema físico não
identifica automaticamente aeroportos só por encontrar asfalto.

`Grounded` significa apoio geométrico, inclusive em terra firme fora da pista;
não significa que a missão foi concluída. As zonas explícitas das pistas e o
objetivo Santos Dumont→Galeão continuam na etapa de lógica da missão. O avião
autônomo permanece uma instância visual animada, sem colisão entre aeronaves.
O controle completo da atitude do jogador também continua a cargo da evolução
do modelo de voo; as colisões já aceitam essas orientações pela interface vetorial.
