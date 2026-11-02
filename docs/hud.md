# Instrumentos de voo

Notas técnicas produzidas com auxílio de IA (Codex), registradas em `prompts.md`.
Este documento não é o relatório final dos alunos.

## Estrutura

- `src/aircraft/FlightTelemetry.*`: leitura matemática do estado, sem alterar
  o avião ou executar chamadas de OpenGL. Funciona independentemente da câmera.
- `src/ui/HudLayout.*`: posições e raios em pixels lógicos e ângulos dos ponteiros.
- `src/ui/HudNeedleMotion.*`: resposta visual dos ponteiros baseada no tempo,
  independente da física e do desenho OpenGL.
- `src/ui/FlightHud.*` e `src/ui/shaders/`: geometria 2D, superfícies procedurais,
  ponteiros e horizonte artificial. Reutiliza a fonte do código base.
- `src/ui/HudInspection.*`: fixtures e capturas para inspeção gráfica opcional.
- `src/main.cpp`: coleta os dados do jogador depois da atualização da física e
  desenha o HUD depois dos passes 3D, antes dos textos auxiliares.

## Leituras

| Instrumento | Leitura e escala |
|---|---|
| Velocidade | Módulo da velocidade no mundo × 3,6, em km/h. Escala analógica de 0 a 1000; valor numérico permanece disponível acima da escala. |
| Altitude | Coordenada Y em metros, relativa ao nível zero do mapa. O ponteiro faz uma volta a cada 1000 m; o número exibe a altitude completa. |
| Potência | Comando de throttle em 0–100%, proporcional à propulsão no modelo atual. Não é uma medição independente de potência do motor. |
| Bússola | Rumo do nariz: norte=-Z=0°, leste=+X=90°, sul=180°, oeste=270°. Usa N/L/S/O e ponteiro. |
| Horizonte | Pitch e roll extraídos dos vetores forward/up que orientam a malha, não dos campos de ângulos ainda desconectados da física. |

A velocidade inclui as componentes horizontal e vertical; o projeto não possui
modelo de vento ou sensor de velocidade indicada. A altitude não é distância
ao solo sob o avião nem elevação geográfica levantada por um DEM.

A referência do avião no horizonte permanece fixa. O fundo céu/solo e as marcas
de pitch se movem de acordo com a atitude. Pitch positivo indica nariz para cima;
roll positivo indica asa direita para baixo. A base é ortonormalizada como no
desenho 3D. Na vertical, rumo/roll ficam indefinidos e os respectivos instrumentos
indicam ausência de leitura; entradas não finitas também aparecem como `--`.

## Apresentação

As graduações são referências fixas; a posição dos ponteiros usa valores decimais
sem arredondamento. Velocidade e potência percorrem 300°, deixando um espaço
entre zero e máximo. Bússola e altímetro percorrem a volta completa de 360°.
As marcas da escala e os ponteiros usam a mesma conversão de valor para ângulo.

Os ponteiros acompanham a leitura com resposta exponencial de constante de tempo
0,12 s (`alpha = -expm1(-dt/0.12)`), de forma independente do FPS. Isso suaviza,
por exemplo, o throttle atual, que muda em passos de 5% por evento de teclado.
Os números continuam mostrando a leitura real, sem atraso; o horizonte também
mantém a atitude real. A suavização atua sobre velocidade, potência e altitude
antes da conversão para ângulo. O rumo usa o menor arco na passagem 359° → 0°.
O primeiro valor válido aparece imediatamente; leituras inválidas removem o
ponteiro, e a recuperação reinicia sua posição. Atualizar continua com o HUD oculto.

A escala de velocidade continua sendo 0–1000 km/h. Na física atual, a velocidade
pode ser baixa diante dessa escala; a suavização não aumenta os valores do avião.

Em 1280×720, os mostradores ficam em uma linha inferior, com o horizonte 20%
maior no centro. Em janelas menores que 650 pixels lógicos de largura, o layout
usa duas linhas. Graduações numéricas secundárias são reduzidas nos menores
mostradores; as leituras principais e unidades permanecem disponíveis.

O cálculo usa o tamanho lógico da janela; a transformação para NDC é escrita
no vertex shader e o viewport usa a resolução real do framebuffer. Isso mantém
os círculos proporcionais e acompanha o escalonamento do monitor.

`H` alterna HUD e textos auxiliares, como já fazia para as informações na tela.
As câmeras C/F/L continuam independentes dos sensores: na câmera livre, o painel
continua exibindo o avião do jogador. A velocidade adicional da câmera fica
identificada nos textos de ajuda. Os controles foram movidos para a parte superior.
Os textos de ajuda usam as escalas 0,8 no cabeçalho, 0,7 nos controles e 0,75
na velocidade da câmera, com margens e espaçamento em pixels.
Sua largura é medida para escolher versões compactas em janelas menores e
reservar espaço ao FPS; informações adicionais cedem espaço aos instrumentos.
As escalas das fontes dos mostradores e do FPS permanecem independentes.

O passe preserva os estados de profundidade, blending, programa, VAO, buffer,
polígonos, culling, scissor e alpha-to-coverage, incluindo alterações feitas pelo
desenho da fonte. Os recursos do HUD são liberados antes de destruir o contexto.

## Verificação

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

`flight_telemetry` verifica unidades, cardinais, combinações de pitch/rumo/roll,
voo invertido, vetores não ortogonais, dados inválidos e preservação das entradas.
`hud_layout` verifica limites e ausência de sobreposição em dimensões compactas,
720p, 1080p, ultrawide, transição entre linhas e dimensões inválidas. Também
verifica saturação das escalas e continuidade do altímetro ao cruzar 1000 m.
`hud_needle_motion` verifica resposta entre valores, independência de FPS,
transições circulares, validade de canais e intervalos de tempo inválidos.

A partir de `bin/Debug`, o modo de desenvolvimento abaixo gera onze capturas e
encerra, sem modificar o estado do avião para produzir os valores dos instrumentos:

```powershell
.\main.exe --hud-check ../../build/hud-check
```

Inclui as três câmeras, subida com curva à direita, descida com curva à esquerda,
voo invertido, ausência de leituras, altitude com vários milhares, velocidade
acima da escala, throttle máximo, passagem de 999 para 1001 m e janela mínima
de 320×240, entre 320 e 1920 pixels de largura.
Somente esse argumento ativa leituras sintéticas identificadas no log; em uso
normal, todos os valores são obtidos do avião do jogador. Cada captura verifica
erros OpenGL e registra dimensões, valores e flags de validade. Cada vista aguarda
90 quadros para acomodar a resposta dos ponteiros após os saltos dos fixtures.
