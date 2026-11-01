# Notas técnicas de renderização

Documento técnico produzido com auxílio de IA (Codex), registrado em
`prompts.md`. Não é o relatório final dos alunos.

## Organização dos fontes

```text
src/
  main.cpp           entrada, janela, atualização e integração
  aircraft/          estado do avião e modelo de voo
  camera/            controle das câmeras, entrada e captura do cursor
  render/            cena, carregamento, materiais e inspeção visual
    shaders/         programas GLSL de vértices e fragmentos
  platform/          limite de quadros e temporização
  ui/                desenho de texto
  input/             CommandInterface preexistente
  course/            exemplos e suporte do código da disciplina
  third_party/       implementações GLAD, stb_image e tinyobjloader
```

Os headers próprios ficam junto de sua implementação. Os includes usam o
caminho relativo a `src`, por exemplo `camera/CameraController.hpp`. O CMake
declara explicitamente os fontes compilados. `CommandInterface.cpp` e
`buildtriangles.cpp` continuam preservados fora desse alvo, como antes.
Os headers de dependências e matrizes da disciplina permanecem em `include/`.

## Materiais e superfícies sobrepostas

`ModelRenderer` carrega OBJ/MTL e compartilha imagens/samplers. `MaterialPolicy`
classifica materiais, sem chamadas OpenGL; `GameScene` coordena os passes:

1. Geometria sólida, com escrita de profundidade e alpha-to-coverage.
2. Pintura de ruas, pistas, quadras e janelas sobre suas superfícies de base,
   com blending, pequeno polygon offset e sem escrita de profundidade.
3. Materiais transparentes, ordenados do mais distante para o mais próximo,
   com blending e sem escrita de profundidade.

Árvores, cercas e redes mantêm recorte por alpha no passe sólido. A janela
solicita quatro amostras MSAA; a disponibilidade depende do driver. Mipmaps e
filtragem anisotrópica de até 8x, quando a extensão existe, reduzem cintilação
de texturas vistas em ângulos rasantes. Coordenadas UV repetidas perdem um
deslocamento inteiro comum por triângulo, preservando repetição e derivadas;
texturas com clamp não recebem esse tratamento.

Materiais sem imagem recebem detalhe procedural discreto multiplicado pelo
Kd importado. Imagens existentes continuam sendo usadas. O parser antigo de
tinyobjloader conservava `has_kd` entre materiais: depois de um Kd explícito,
um material seguinte com imagem e sem Kd podia ficar preto. A correção local
reinicia esse estado em cada `newmtl`, com comentário FONTE e teste específico.

## Precisão e lacunas do mapa

O vertex shader calcula a posição relativa à câmera antes de aplicar a rotação
da view. Isso reduz cancelamento numérico em coordenadas distantes da origem.
As matrizes view/projection continuam sendo calculadas pelas funções da aula;
nenhum construtor de transformação ou câmera do GLM foi introduzido.

No cockpit, o mundo e o interior usam faixas de profundidade distintas para
permitir enxergar o painel próximo e o mapa distante. Os valores e controles
estão em `docs/cameras.md`.

Parte dos espaços azuis entre ruas eram lacunas na geometria exportada.
`data/rio/chunks/base_terrain.obj` fornece um chão texturizado limitado ao
extrato original e abaixo das superfícies existentes. É um preenchimento
genérico, sem DEM ou novos dados de classificação do solo. Detalhes da geração
determinística, origem e licença estão em `data/rio/README.md`.

## Verificação

Na raiz do projeto, com o build já configurado:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Os cinco grupos verificam câmeras, entrada/foco/captura, classificação de
materiais, carregamento do MTL e geração determinística da base de terreno.

A partir de `bin/Debug`, para inspecionar com os assets reais:

```powershell
.\main.exe --texture-check ../../build/textures-check
```

O modo captura 13 vistas em arquivos PPM e encerra: pista próxima/distante,
vista superior do aeroporto, três regiões de parques, cidade, Galeão, avião,
três posições sobre a pista e cockpit. Cada captura consulta erros OpenGL.
As vistas são uma amostra de inspeção; não cobrem cada material ou cada ponto
da cidade. Sem o argumento, a aplicação abre normalmente para interação.
