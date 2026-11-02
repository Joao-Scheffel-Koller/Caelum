# Implementação do Caelum: guia técnico detalhado da conversa

Documento técnico produzido com auxílio de IA (Codex), a pedido dos integrantes. Este arquivo serve para estudar, revisar e manter o código. Não substitui o relatório final de autoria dos alunos em `README.md`, nem altera a especificação `SPEC.md`.

**Recorte documentado:** código consolidado até `e2917d8`, merge do HUD na `main`, em 2 de novembro de 2026. A branch `colisions` foi criada desse ponto. Neste primeiro registro, as colisões completas ainda são a etapa seguinte: o comportamento físico existente limita a posição a `Y=0`. O documento será publicado antes de começar essa implementação, conforme solicitado.

Os trechos C++/GLSL/Python abaixo foram extraídos ou conferidos contra os arquivos desse recorte. Cada trecho mostra uma parte da implementação; assinaturas, includes e detalhes omitidos continuam nos arquivos indicados. Explicações de algoritmos e fórmulas complementam os trechos, sem apresentar pseudocódigo como se fosse código já executado.

## Índice

- [Visão geral: o que mudou no projeto](#visão-geral-o-que-mudou-no-projeto)
- [Arquitetura e organização dos arquivos](#arquitetura-e-organização-dos-arquivos)
- [Inicialização e ciclo de vida da aplicação](#inicialização-e-ciclo-de-vida-da-aplicação)
- [Um quadro completo: entrada, simulação, câmera e desenho](#um-quadro-completo-entrada-simulação-câmera-e-desenho)
- [Estado do jogador e interface com a física existente](#estado-do-jogador-e-interface-com-a-física-existente)
- [Assets do avião: preparação, materiais e créditos](#assets-do-avião-preparação-materiais-e-créditos)
- [Correções das texturas do avião: diagnóstico e reconstrução verificável](#correções-das-texturas-do-avião-diagnóstico-e-reconstrução-verificável)
- [Mapa do Rio e divisão espacial determinística](#mapa-do-rio-e-divisão-espacial-determinística)
- [Base limitada para preencher lacunas do chão](#base-limitada-para-preencher-lacunas-do-chão)
- [Relação entre ModelRenderer, GameScene e a GPU](#relação-entre-modelrenderer-gamescene-e-a-gpu)
- [Posicionamento geográfico e pivô do avião](#posicionamento-geográfico-e-pivô-do-avião)
- [Materiais, iluminação e correções de aparência](#materiais-iluminação-e-correções-de-aparência)
- [Profundidade, transparência e estabilidade em movimento](#profundidade-transparência-e-estabilidade-em-movimento)
- [Texturas do chão: causas distintas e o que cada correção resolve](#texturas-do-chão-causas-distintas-e-o-que-cada-correção-resolve)
- [Culling, inspeção gráfica e limites conhecidos](#culling-inspeção-gráfica-e-limites-conhecidos)
- [Câmeras: separar observação, entrada e estado do avião](#câmeras-separar-observação-entrada-e-estado-do-avião)
- [Animação do segundo avião por curvas de Bézier](#animação-do-segundo-avião-por-curvas-de-bézier)
- [HUD: instrumentos de voo e apresentação das leituras](#hud-instrumentos-de-voo-e-apresentação-das-leituras)
- [Compilação, execução e diagnóstico da versão carregada](#compilação-execução-e-diagnóstico-da-versão-carregada)
- [Estratégia de testes: matemática, arquivos e GPU](#estratégia-de-testes-matemática-arquivos-e-gpu)
- [Controles consolidados no recorte](#controles-consolidados-no-recorte)
- [Git, identificação de IA e organização documental](#git-identificação-de-ia-e-organização-documental)
- [Estado ao fechar este documento e etapa seguinte](#estado-ao-fechar-este-documento-e-etapa-seguinte)

## Visão geral: o que mudou no projeto

O ponto de partida foi o código base de Computação Gráfica, com infraestrutura de janela, contexto OpenGL, shaders, desenho de texto e objetos de demonstração. Ao longo da conversa, a cena inicial passou a carregar um avião comercial A320 com cockpit e um extrato tridimensional do Rio de Janeiro gerado a partir de OpenStreetMap pelo OSM2World. O jogador começa no Santos Dumont; um segundo avião reaproveita a mesma malha GPU e percorre um circuito de curvas de Bézier.

O código novo foi separado em módulos. Câmeras, entrada de mouse, renderização de materiais, animação e telemetria têm responsabilidades diferentes. Essa separação permitiu corrigir o salto de câmera ao apertar o botão direito, ajustar sensibilidade e fontes e acrescentar instrumentos sem modificar as forças usadas pelo modelo de voo.

| Área | Resultado consolidado | Implementação principal |
|---|---|---|
| Modelo do jogador | A320 com materiais, imagens e interior, substituindo o coelho na cena inicial | `data/aircraft/`, `ModelRenderer`, `GameScene` |
| Cidade | 125 partes determinísticas do Rio, nas mesmas coordenadas do mapa original | `data/rio/chunks/manifest.json`, `split_obj.py` |
| Chão adicional | Base limitada ao retângulo do extrato, para preencher lacunas visuais | `build_base_terrain.py`, `base_terrain.obj` |
| Posição inicial | Pista 20L do Santos Dumont, 100 m após a cabeceira norte | `GameScene::Spawn` |
| Materiais | OBJ/MTL, texturas compartilhadas, transparência, pintura e superfície procedural de fallback | `ModelRenderer`, `MaterialPolicy`, shaders 3D |
| Câmeras | Cockpit, observador livre e externa orbital, com estado separado | `CameraController`, `CameraInput` |
| Mouse | Captura pelo botão direito, liberação por foco e estabilização inicial de 100 ms | `CameraInput`, `MouseCapture` |
| Tempo e janela | Abertura em 1280×720; limite de 60 FPS; movimento de câmera com tempo real | `FrameLimiter`, `main.cpp` |
| Tráfego | Segunda instância do A320 em quatro Béziers cúbicas, com Euler sobre distância | `BezierPath`, `BezierFlight` |
| Instrumentos | Velocidade, altitude, potência, bússola e horizonte artificial | `FlightTelemetry`, `FlightHud` |
| Ponteiros | Ângulos proporcionais, movimento intermediário e suavização independente de FPS | `HudLayout`, `HudNeedleMotion` |
| Fontes auxiliares | Tamanho final 1,5 vez o inicial, margens medidas e versões compactas | Bloco de ajuda em `main.cpp` |
| Validação | Nove testes CTest e modos de captura com os assets reais | `tests/`, `RenderInspection`, `HudInspection` |

Isso ainda é um simulador em desenvolvimento. A presença do modelo visual, das câmeras e do painel não implica que a missão decolar/voar/pousar entre aeroportos já esteja concluída. A física simplificada antecede parte deste trabalho, e os controles completos da atitude do jogador, as colisões com a cidade, o menu de escolha de avião e os critérios de conclusão da missão permanecem pendentes no recorte acima.

## Arquitetura e organização dos arquivos

Uma reorganização moveu os arquivos antes dispersos na raiz de `src` para pastas por responsabilidade. Foram atualizados os includes, fontes do CMake/Makefile e caminhos dos shaders. A reorganização preservou as implementações; seu objetivo foi tornar os pontos de integração localizáveis.

```text
src/
  main.cpp                    janela, contexto, eventos e composição dos módulos
  aircraft/
    Aircraft.*                estado mutável do jogador
    FlightModel.*             forças e integração do jogador
    BezierFlight.*            estado e pose do avião autônomo
    FlightTelemetry.*         medições somente de leitura
  animation/
    BezierPath.*               geometria e consultas do caminho cúbico
  camera/
    CameraController.*        matemática e estados das três câmeras
    CameraInput.*             eventos GLFW e intenção de movimento
    MouseCapture.*            confinamento nativo do cursor
  render/
    ModelRenderer.*           leitura de modelos, materiais e desenho GPU
    GameScene.*               integração de mapa, jogador e tráfego
    MaterialPolicy.*          interpretação dos materiais do mapa
    RenderInspection.*        vistas repetíveis para inspeção visual
    shaders/                  shaders do cenário e dos aviões
  ui/
    FlightHud.*               construção e desenho dos instrumentos
    HudLayout.*               posições, dimensões e conversão valor/ângulo
    HudNeedleMotion.*         resposta visual dos ponteiros
    HudInspection.*           fixtures e capturas opcionais do HUD
    textrendering.cpp          fonte e desenho de letras do código base
    shaders/                  shaders exclusivos do HUD
  platform/
    FrameLimiter.*            espera e temporização entre quadros
  input/                      esqueleto de CommandInterface ainda sem lógica
  course/                     código auxiliar das aulas e correção
  third_party/                unidades de compilação de bibliotecas existentes
tests/                        regressões CPU; não dependem de abrir o jogo
docs/                         explicações técnicas de subsistemas
data/aircraft/                OBJ, MTL e imagens do A320
data/rio/chunks/              partes do mapa e recursos compartilhados
data/rio/tools/               divisão determinística e geração da base visual
```

`CameraController` e `FlightTelemetry` são bons exemplos da separação: ambos fazem cálculos sem chamar OpenGL. O renderer recebe matrizes e geometrias já definidas; o HUD recebe medições; a física altera o estado de `Aircraft`. O controle da câmera livre modifica o observador, sem usar o movimento da câmera como força no avião.

`CommandInterface::process_input()` continua vazio. A entrada efetiva usa os callbacks de `main.cpp` e o adaptador `CameraInput`. Da mesma forma, ainda existem funções do código base para objetos de demonstração e depuração; uma função permanecer no arquivo não significa que faça parte do caminho de desenho atual. A cena ativa é composta por `GameScene`.

## Inicialização e ciclo de vida da aplicação

`main()` inicializa GLFW, pede um contexto OpenGL 3.3 core, cria a janela e registra callbacks. O tamanho inicial é explícito:

```cpp
window = glfwCreateWindow(1280, 720, "Caelum - Rio de Janeiro", NULL, NULL);
```

Após carregar as funções OpenGL com GLAD, o programa imprime fabricante, renderer e versões da GPU. Isso ajuda a distinguir um problema de contexto de um erro de leitura de arquivo. O programa 3D é compilado pelos helpers do código base antes de criar e carregar a cena.

Trecho de `src/main.cpp`:

```cpp
std::unique_ptr<GameScene> scene(new GameScene);
try {
    scene->Load(window);
    scene->Spawn(MainAircraft);
} catch(const std::exception& error) {
    fprintf(stderr,"Scene loading failed: %s\n",error.what());
    scene.reset();
    glfwTerminate();
    return EXIT_FAILURE;
}
```

`Load` pode lançar uma exceção por OBJ, manifesto ou textura ausente. O programa informa o erro e libera os recursos enquanto o contexto ainda existe. `Spawn` vem depois da carga, antes de iniciar a simulação, e define constantes de posição e orientação. Os limites geométricos do modelo são usados por `AircraftTransform`, na preparação da câmera e no desenho, para calcular o pivô que mantém as rodas na altura da posição do avião.

Depois disso, `GameScene::Cameras` prepara o quadro de referência do avião, `CameraInput` recebe uma referência para esse quadro, a fonte é inicializada e `FlightHud` cria seus buffers e programa. A instância da cena é proprietária dos modelos; as câmeras consultam sua transformação, sem duplicar a leitura do OBJ para descobrir a posição do jogador.

O mapa inteiro é lido na inicialização. O título da janela indica quantas partes já foram carregadas, e eventos são processados durante essa etapa para permitir cancelar pelo fechamento da janela. Foi discutido carregar somente chunks próximos, mas streaming de disco ou descarregamento de chunks ainda não foi implementado. O descarte por campo de visão acontece no desenho e reduz trabalho de GPU; ele não reduz o conjunto carregado em memória.

## Um quadro completo: entrada, simulação, câmera e desenho

O ciclo principal tem uma ordem definida. Primeiro espera pelo orçamento de quadro e processa os eventos mais recentes. Em seguida, verifica se o framebuffer está disponível. Uma janela minimizada pode ter largura ou altura zero: nesse caso, o programa espera eventos e reinicia a referência temporal, evitando divisão por zero e um salto grande de tempo ao restaurar a janela.

O intervalo real é obtido pelo relógio do GLFW:

```cpp
double current_time = glfwGetTime();
const double elapsed = std::max(current_time - previous_time,0.0);
float dt = static_cast<float>(elapsed);
// Keep the existing physics step limit; camera travel uses real elapsed time.
MainModel.update(&MainAircraft, std::min(dt,0.05f));
previous_time = current_time;
scene->Update(elapsed); // Euler advances traffic once, outside all draw passes.
g_CameraFrame=scene->Cameras(MainAircraft);
g_CameraInput->Update(dt);
```

Há três usos distintos de tempo nesse trecho. A física do jogador mantém o limite de 0,05 s por atualização que já era usado na integração. A câmera recebe o tempo real do quadro. O tráfego da cena também recebe o intervalo real e é atualizado uma única vez. Se o computador travar por um intervalo longo, o clamp da física do jogador descarta parte desse tempo: isso é uma limitação existente, não um sistema de passos fixos com acumulador.

O cálculo da câmera vem depois da física para usar a posição mais nova do avião. Os eventos vêm antes desse cálculo para evitar desenhar a câmera com o mouse do quadro anterior. Então são montadas as matrizes view e projection, enviadas ao programa GPU e executados os passes da cena. No cockpit, o passe do mundo e o passe do interior têm intervalos de profundidade diferentes, explicados adiante.

Após o desenho 3D, o estado do jogador é convertido em telemetria:

```cpp
FlightTelemetry readings=CalculateFlightTelemetry(MainAircraft.get_position(),MainAircraft.get_velocity(),
    MainAircraft.get_forward(),MainAircraft.get_up(),MainAircraft.get_throttle());
if(hud_check)readings=hudInspection.Telemetry(readings);
hud->Update(readings,elapsed);
```

No uso normal, os dados vêm do jogador. A substituição por valores sintéticos só acontece com o argumento explícito de inspeção do HUD. A atualização visual dos ponteiros ocorre mesmo quando `H` oculta as informações. Assim, reabrir o painel não faz uma animação a partir de leituras antigas.

O HUD e os textos auxiliares são desenhados sobre a cena; os modos de inspeção podem ler o framebuffer; finalmente `glfwSwapBuffers` apresenta o quadro. O desenho usa double buffering: o programa prepara um buffer enquanto o anterior está disponível para apresentação. A captura é feita antes da troca para obter exatamente o quadro que acabou de ser desenhado.

A liberação segue a ordem inversa das dependências:

```cpp
g_CameraInput.reset();
hud.reset();
scene.reset();
glfwTerminate();
```

Essa ordem permite liberar o confinamento do cursor e excluir os VAOs, VBOs, programas, texturas e samplers gerenciados pelos módulos novos antes de destruir o contexto. O renderer de texto do código base não recebeu um destrutor geral nesta etapa; seus recursos restantes são liberados com o contexto. A câmera não fica com o cursor preso ao fechar o aplicativo.

## Estado do jogador e interface com a física existente

`Aircraft` guarda posição, velocidade, aceleração e os vetores `forward`, `up` e `right`, além de throttle e parâmetros do modelo de forças. Essa classe e o modelo básico de voo não foram inteiramente escritos nesta etapa da conversa: eles são a interface existente com o trabalho da dupla. O que se integrou aqui foi seu uso na cena, nas câmeras e nos sensores; a correção de contato com o solo também foi acrescentada.

Os valores atuais do construtor são massa 10, gravidade -1 no modelo, sustentação 0,5, arrasto 0,9 e empuxo máximo 80. Eles são parâmetros de uma simulação simplificada em desenvolvimento. A presença de um mesh de A320 não transforma essas constantes em uma calibração física de um A320 real.

O modelo calcula empuxo na direção `forward`, gravidade em -Y, sustentação em `up` e arrasto contrário à velocidade. Em termos de equações, a resultante é `F = F_empuxo + F_gravidade + F_sustentacao + F_arrasto`, seguida de `a = F/m`, atualização de velocidade e atualização de posição. A posição usa a velocidade já atualizada nesse passo.

O problema corrigido no contato com a pista era acumular velocidade de queda mesmo depois de a posição ter sido limitada ao chão. Essa velocidade negativa entrava no módulo da velocidade usado na sustentação e no arrasto. O trecho consolidado de `FlightModel.cpp` é:

```cpp
if (position.y < 0.0f){
    position.y = 0.0f;
    // AI-assisted fix: the runway cancels downward velocity on contact.
    // Otherwise speed/lift would include accumulated falling through the floor.
    if (velocity.y < 0.0f) velocity.y = 0.0f;
    aircraft->set_velocity(velocity);
}
```

O teste resolve o ponto inicial em uma pista de altura zero; não consulta o solo sob o avião nem os prédios. Também apaga a velocidade de impacto, motivo pelo qual o futuro sistema de colisões precisa medir o contato antes de aplicar a resposta. A correção não deve ser confundida com reconhecimento de pouso seguro.

W/S aumentam ou reduzem o throttle em passos de 0,05 quando os eventos correspondentes chegam ao callback e a câmera livre não consome essas teclas. O HUD suaviza a apresentação desses passos, mas não muda o valor armazenado. Os campos legados `pitch`, `yaw` e `roll` não são a fonte escolhida para o horizonte artificial; ele usa os vetores que realmente orientam a malha.

O mouse sem botão direito ainda não altera a atitude do jogador no recorte documentado. Os controles das câmeras foram concluídos sem apresentar essa parte da física como pronta. Isso é importante para testar: mover a câmera externa ao redor do avião é diferente de mudar o rumo do avião.

## Assets do avião: preparação, materiais e créditos

O avião utilizado é **“A320-200 with cockpit (ver VIII)”**, de TonyWony. A escolha substituiu o objeto demonstrativo do código base por uma aeronave comercial com exterior, cabine e bancos. A cadeia de preparação passou pelo Blender e terminou em um conjunto independente do programa de modelagem: `data/aircraft/aircraft.obj`, `aircraft.mtl` e a subpasta `textures/`. O OBJ contém posições, faces, normais, UVs e referências aos materiais; o MTL descreve os materiais e referencia as imagens. Portanto, distribuir apenas o OBJ preservaria a geometria, mas não garantiria a aparência corrigida.

O crédito original foi preservado em `AI_README.md` e `data/aircraft/LEIA-ME.txt`:

> "A320-200 with cockpit (ver VIII)" (https://skfb.ly/6R8YA) by TonyWony is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

O problema observado no Blender envolvia tanto caminhos ou nomes divergentes quanto imagens efetivamente ausentes do download. `data/aircraft/repair_report.json` diferencia as imagens religadas das substituídas. Por exemplo, foram conciliados nomes com espaços e sublinhados, e `panel bumb.png.001` foi relacionado à imagem disponível `panel_bumb.png`. Encontrar uma imagem com nome semelhante não prova que seu conteúdo combine com as UVs: por isso, também houve correção da pintura externa e do mapeamento do símbolo da cauda. A traseira e as portas receberam continuidade da faixa azul; partes fixas e móveis da cauda passaram a compartilhar a projeção do símbolo.

A restauração não foi apresentada como reprodução exata de todos os materiais originais. Onde a imagem não existia, foram usados materiais compatíveis. Os painéis disponíveis do cockpit foram preservados; o tecido `Fabric_Demin` dos bancos dos passageiros foi recuperado, incluindo configurações de cor, rugosidade e normal no arquivo editável. Entretanto, os mapas de rugosidade e normal configurados no Blender **não são interpretados pelo renderer atual do jogo**, que usa a imagem difusa, a cor difusa e a opacidade.

`aircraft.blend` é a versão editável; previews e relatórios documentam a conferência. `validation.json` registra a reimportação do OBJ com 522 malhas, 27.569 vértices e 51.022 faces triangulares, sem imagens ausentes. Esses números descrevem a validação do asset; não são a quantidade de chamadas de desenho do jogo. O carregador agrupa as faces por material, portanto a organização de desenho difere da organização em objetos do Blender.

O OBJ atual usa **Y para cima e nariz em +X**. Seus limites são aproximadamente `(-27.425108, -3.342473, -17.178679)` e `(10.548508, 8.196299, 17.178555)`, correspondendo a 37,97 unidades de comprimento, 11,54 de altura e 34,36 de largura. O jogo mantém essa escala, compatível com o mapa em metros; não normaliza o avião para uma caixa unitária.

## Correções das texturas do avião: diagnóstico e reconstrução verificável

### Por que o avião parecia normal em cinza e ficava rosa ao mostrar materiais

Na etapa de preparação, a visualização sólida mostrava a malha, enquanto a visualização de materiais revelava referências quebradas. Os avisos enviados pelo Blender listavam arquivos que não eram encontrados na pasta `textures`, incluindo `tail_base.png`, `Rudder_base.png`, `Foam_Tubes_Roughness.jpg` e outros. Assim, a troca de visualização expunha um problema já existente nas referências; ela não recriava a textura nem recuperava imagens ausentes. A separação do download em `source/` e `textures/` não era, por si só, um defeito: o problema era a correspondência entre cada imagem referenciada e o arquivo realmente disponível.

Os scripts usados nessa preparação continuam preservados **localmente e ignorados pelo Git**, em `build/fix_aircraft.py`, `build/refine_aircraft.py` e `build/validate_aircraft.py`. Não existe uma pasta versionada `data/aircraft/tools/` com esses scripts. Os resultados utilizados pelo jogo, os relatórios e as imagens de conferência estão em `data/aircraft/`. Essa distinção é relevante para reprodução: os scripts locais contêm caminhos da máquina utilizada e não constituem uma ferramenta portátil distribuída com o projeto.

`fix_aircraft.py` indexou os arquivos disponíveis e fez correspondência explícita para três imagens de estofamento. Também normalizou os nomes, retirando pontuação, espaços e diferenças de caixa. O sufixo interno `.001` de imagens duplicadas do Blender era removido antes da pesquisa:

```python
def key(s): return re.sub('[^a-z0-9]', '', s.lower())
```

```python
name = re.sub(r'\.\d{3}$', '', image.name)
match = aliases.get(name, name)
path = files.get(match) or next((p for n,p in files.items() if key(n)==key(match)), None)
```

Cada imagem encontrada era copiada para a pasta de saída, recebia novo `filepath` e era recarregada com `image.reload()`. O relatório preserva os pares antes/depois. O caso `FabricUpholsteryBrightAnglePattern001_COL_VAR1_3K.jpg` passou a usar `FabricUpholsteryBrightAnglePattern001_COL_.jpeg`; a referência `panel bumb.png.001` passou a usar o arquivo disponível `panel_bumb.png`. Correspondências especiais foram explícitas: não houve tentativa de interpretar automaticamente todo arquivo com nome parecido como se tivesse o mesmo conteúdo.

Quando não existia imagem, o script removeu o nó de imagem inválido e suas ligações. Para entradas de cor, aplicou cores por família de material: exterior claro, couro azul-escuro, painéis escuros e hélices/partes mecânicas acinzentadas. Para entradas de metallic e roughness sem imagem, configurou valores escalares. Isso evita exportar referências conhecidamente inexistentes, mas é uma substituição simplificada, não a recuperação do conteúdo perdido. O arquivo `repair_report.json` separa `relinked`, `missing_images_replaced` e `fallback_materials`, permitindo conferir qual procedimento ocorreu em cada caso.

### Fuselagem, faixa azul e portas: corrigir UVs além de encontrar imagens

Encontrar `I_base.png` resolveu uma referência, mas não garantiu pintura correta. A malha cilíndrica estava amostrando regiões inadequadas do atlas. Na primeira correção, o material `fuselarge` foi copiado para `Exterior_corrected`, e as faces relevantes do objeto `Cylinder` receberam UVs direcionadas à faixa limpa da imagem. A coordenada longitudinal utilizava `progress = (hi − x)/(hi − lo)`, seguida por `u = 0.05 + 0.65 progress` e uma faixa estreita de V. As posições dos vértices foram preservadas; a mudança estava na associação de material e nas coordenadas de textura.

Depois da comparação visual solicitada pelo usuário, uma segunda etapa substituiu a pintura externa problemática por uma imagem simples e contínua. O script refinador obteve a cor azul do primeiro pixel da imagem do símbolo, definiu a cor clara e gerou uma faixa de 1024×4 pixels, com 282 colunas azuis:

```python
navy=tuple(logo.pixels[:3])
white=(.90,.925,.96)
paint=bpy.data.images.new('Exterior_white_navy',width=1024,height=4,alpha=False)
paint.pixels=[c for y in range(4) for x in range(1024) for c in ((*navy,1) if x<282 else (*white,1))]
```

O material `Exterior_continuous_blue` utiliza essa imagem. Sua divisão fica em `u = 282/1024`; portanto, as partes que compartilham a mesma posição longitudinal recebem a mesma cor. O ponto fundamental da continuidade entre fuselagem e portas é projetar com a **posição global do vértice no Blender**, não com coordenadas locais independentes de cada objeto:

```python
p=obj.matrix_world@obj.data.vertices[obj.data.loops[li].vertex_index].co
uv[li].uv=((p.x+27.425108)/37.973616,.15+.7*uv[li].uv.y)
```

Esse U mapeia o comprimento completo para a faixa. Assim, uma porta separada deixa de começar uma nova faixa a partir de sua própria origem. O antes/depois é observável em `tail_preview.png`: a região traseira deixa de apresentar recortes claros e escuros incompatíveis entre partes que deveriam compartilhar a pintura.

### Símbolo da cauda e estofamento dos passageiros

Na cauda, o refinamento aplicou uma projeção comum à parte fixa `Cylinder.007` e ao leme separado `Cube.004`. O material `Tail_logo_corrected` referencia `logo.png`; as coordenadas usam o mesmo retângulo para os dois objetos:

```python
u=(p.x+26.15)/4.3;v=(p.z-2.55)/4.3
normal=obj.matrix_world.to_3x3()@poly.normal
if normal.y<0:u=1-u
uv[li].uv=(u,v)
```

O espelhamento por lado mantém a orientação do símbolo nas duas faces. As fórmulas de preparação acima usam o sistema do Blender **antes** da conversão de eixos na exportação; nesse estágio, a altura é tratada por Z. Isso não altera a convenção Y para cima do OBJ utilizado pelo jogo. Copiar diretamente `p.z` desse script como altura de um ponto do mapa seria uma troca indevida de sistemas de coordenadas.

Os bancos apresentavam outro caso: o material `Fabric Demin` não tinha os nós de imagem necessários. O script conectou a imagem difusa em `Base Color`, roughness como `Non-Color`, e a normal por um nó `ShaderNodeNormalMap` com força 0,35. A UV das faces desse material no objeto `Cube.021` foi projetada no plano dos dois eixos restantes após escolher a componente dominante da normal; o divisor 0,5 definiu repetição espacial do tecido. A seleção de faces por material evitou reaplicar tecido em peças com outro acabamento. O MTL final conserva os três mapas:

```text
map_Kd textures/Fabric_Demin_diffuse.png
map_Ns textures/Fabric_Demin_roughness.png
map_Bump -bm 0.350000 textures/Fabric_Demin_normal.png
```

Isso documenta o asset exportado, mas somente `map_Kd` é usado no desenho atual em OpenGL. A aparência dos bancos no Blender pode, portanto, ter mais detalhes de iluminação superficial que sua versão dentro do jogo. A conferência específica foi preservada em `seats_preview.png`.

### Exportação OBJ e validação após reimportar

A exportação selecionou as malhas do avião, incluindo o interior, e excluiu as câmeras e luzes temporárias de conferência. Os parâmetros explícitos do refinador foram:

```python
bpy.ops.wm.obj_export(filepath=str(root/'aircraft.obj'),export_selected_objects=True,export_triangulated_mesh=True,export_uv=True,export_normals=True,export_materials=True,path_mode='RELATIVE',forward_axis='NEGATIVE_Z',up_axis='Y')
```

`path_mode='RELATIVE'` permite manter OBJ, MTL e `textures/` juntos sem caminhos absolutos do computador de origem. As UVs ligam a malha às imagens; as normais orientam a iluminação; a triangulação combina o asset com o desenho `GL_TRIANGLES`. O `.blend` preserva materiais editáveis e imagens com caminhos relativos `//textures/...`.

O validador abriu uma cena vazia, reimportou o OBJ e recarregou imagens referenciadas pelos materiais. Verificou faces triangulares e ausência de imagens sem dimensões válidas. As 522 malhas exportadas diferem das 525 registradas durante a preparação porque três objetos da origem não tinham faces exportáveis. Essa reimportação testa o conjunto exportado, em vez de depender de imagens ainda carregadas na memória da sessão original do Blender. Para conferir novamente, é necessário examinar exterior, emenda da faixa nas portas, cauda em ambos os lados, bancos e cockpit; uma única vista externa não cobre todo o asset.

## Mapa do Rio e divisão espacial determinística

O cenário veio de um extrato OpenStreetMap convertido pelo OSM2World 0.5.0-SNAPSHOT. O resultado contém geometrias de ruas, construções, árvores, superfícies e aeroportos, com materiais e texturas exportados. Não se trata de fotografia aérea aplicada a um plano nem de um modelo de relevo obtido de um DEM. A origem local documentada é latitude **−22.84895**, longitude **−43.2142**, com **+Y para cima, norte em −Z e leste em +X**.

O arquivo original era grande demais para uma organização prática do repositório. `data/rio/tools/split_obj.py` divide-o por uma regra reproduzível, usando apenas a biblioteca padrão do Python. O manifesto registra **3.997.691 faces de origem**, células iniciais de **2.000 metros** e tamanho máximo de **48.000.000 bytes por OBJ**. São 125 arquivos originais de mapa, sem contar a base complementar acrescentada depois.

A decisão de qual célula recebe uma face usa a média das coordenadas XZ de seus vértices. No código real do divisor:

```python
x,z = center(tokens)
cell = (math.floor(x/args.cell_size), math.floor(z/args.cell_size))
record = str(current_material)+' '+' '.join(tokens)+'\n'
```

Formalmente, para uma face de `n` vértices, calcula-se `cx = Σxi/n`, `cz = Σzi/n` e a célula `(floor(cx/L), floor(cz/L))`, com `L = 2000`. Uma face que atravessa uma fronteira permanece inteira na célula de seu centro; ela não é cortada ou duplicada. Isso mantém a geometria original, mas significa que o conteúdo de um arquivo pode ultrapassar o retângulo nominal da grade. Os limites reais registrados no manifesto consideram os vértices efetivamente presentes.

Se uma célula ultrapassa o limite de bytes, o divisor a subdivide recursivamente em quadrantes na ordem noroeste, nordeste, sudoeste e sudeste. A ordem das faces dentro de cada região segue a origem. Índices positivos e negativos de OBJ são resolvidos antes da redistribuição; posições, UVs, normais, materiais e repetições são preservados. O script relê os resultados e compara o multiconjunto das faces, incluindo a ordem dos vértices de cada face, evitando aceitar uma divisão que simplesmente mantenha a contagem enquanto modifica o conteúdo.

Os nomes `o` e `g` originais não são preservados como entidades individuais: cada arquivo gerado recebe um objeto com nome único. Isso é suficiente para renderizar, mas não fornece automaticamente IDs físicos de prédios ou árvores. Uma implementação futura de colisões precisará considerar essa perda de separação semântica.

As texturas referenciadas pelo MTL são copiadas para `chunks/textures/`, recebem prefixo derivado do SHA-256 e passam a usar caminhos relativos. O prefixo evita colisões entre imagens de mesmo nome e conteúdo diferente. `manifest.json` registra arquivos, tamanhos, bounds e hashes; o OBJ original foi preservado localmente em `build/rio_original/`, fora do Git. Para reproduzir os mesmos arquivos, é necessário o mesmo original e as mesmas imagens, não apenas outro extrato da mesma cidade.

```powershell
python data/rio/tools/split_obj.py build/rio_original/rio.obj data/rio/chunks_novos --cell-size 2000 --max-bytes 48000000
python data/rio/tools/test_split_obj.py
```

O teste pequeno executa a divisão duas vezes, força subdivisão com um limite reduzido, inclui índices negativos e compara hashes das saídas. A pasta de saída deve ser nova; a ferramenta rejeita uma pasta já existente. Os créditos cartográficos permanecem em `data/rio/README.md`: © OpenStreetMap contributors, sob ODbL; conversão e materiais do OSM2World, com referência ao projeto `default-style`.

## Base limitada para preencher lacunas do chão

Algumas regiões azuis vistas entre ruas eram ausência de faces de terreno. Acrescentar texturas ao shader não preencheria esses espaços: era necessário haver geometria recebendo os fragmentos. `data/rio/tools/build_base_terrain.py` gera uma base complementar limitada ao retângulo do extrato original, com quatro vértices e dois triângulos, em **Y = −0,02 m**.

A projeção reproduz a convenção Mercator local usada na exportação. Com latitude central `φ0`, longitude central `λ0`, `S = 40075016.686 cos(φ0)` e `m(φ) = ln((1 + sin φ)/(1 − sin φ))/(4π)`, os pontos são projetados por `x = (λ − λ0) S/360` e `z = −S(m(φ) − m(φ0))`. A latitude e longitude centrais são extraídas dos bounds do próprio OSM. Isso alinha a base às coordenadas já existentes em vez de ajustá-la visualmente por tentativa.

O trecho gerador fixa o material compartilhado e a altura:

```python
lines=['# Bounded background from the original OSM extract; not new land classification.',
    '# OpenStreetMap contributors, ODbL: https://www.openstreetmap.org/copyright',
    '# AI-assisted offline preparation; existing water/streets cover this low surface.',
    'mtllib rio.obj.mtl','o base_terrain','usemtl GRASS_0']
for x,z in points:lines.append(f'v {x:.8f} -0.02 {z:.8f}')
```

O preenchimento reaproveita `GRASS_0` e a imagem do mapa. **Não é classificação geográfica de solo**: um espaço vazio não foi identificado semanticamente como gramado. A base fica sob ruas, água e demais superfícies existentes; `GameScene` aplica também `glPolygonOffset(1.0f,4.0f)` ao desenhá-la, afastando-a no buffer de profundidade. `base_terrain.json` registra bounds, origem, altura, material e hashes da origem e do OBJ. O teste verifica a projeção de um ponto conhecido da pista, os quatro vértices, os dois triângulos, a orientação das faces e a reprodução byte a byte.

## Relação entre ModelRenderer, GameScene e a GPU

`ModelRenderer` concentra leitura e desenho; `GameScene` decide quais modelos carregar, onde posicionar instâncias e em que passes desenhá-las. A chamada central de carregamento é:

```cpp
aircraftModel=renderer.Load("../../data/aircraft/aircraft.obj");
chunks=renderer.LoadMap("../../data/rio/chunks/manifest.json",[window](size_t loaded){
```

`LoadMap` percorre os arquivos do esquema fixo do manifesto. Cada arquivo é carregado por `Load(..., true)`, para ativar as regras próprias dos materiais do mapa. O callback atualiza o título com o progresso, processa eventos e permite cancelar o carregamento. A base complementar é carregada separadamente, com `foundation=true`. Assim, a cena pronta contém o A320 e **126 chunks**, incluindo a base.

Todo o mapa continua sendo carregado na inicialização. A divisão em arquivos não foi transformada em streaming sob demanda. Durante o desenho existe descarte de partes fora do campo de visão, mas os recursos dessas partes permanecem residentes. São duas etapas diferentes: carregamento controla quais dados existem em memória; culling controla quais desses dados são enviados para desenhar naquele quadro.

`ModelRenderer::Load` usa `tinyobjloader`, pede triangulação e rejeita faces que ainda não tenham três vértices. Para cada canto, monta `Vertex { position, normal, uv }`; normais ausentes são calculadas pelo produto vetorial das arestas, com proteção contra triângulos degenerados. As faces são agrupadas pelo ID do material em `std::map<int,std::vector<Vertex>>`. Cada grupo vira um `RenderBatch`, com VAO, VBO, quantidade de vértices, material e centro espacial.

```cpp
glBufferData(GL_ARRAY_BUFFER,group.second.size()*sizeof(Vertex),group.second.data(),GL_STATIC_DRAW);
```

A posição, a normal e a UV ocupam os atributos 0, 1 e 2. O desenho usa `glDrawArrays(GL_TRIANGLES,...)`, e não uma indexação compartilhada por elemento nessa camada. `RenderModel` guarda os batches e bounds gerais; após o carregamento, os vetores temporários da leitura não ficam disponíveis como uma malha física persistente em CPU.

As imagens são compartilhadas por caminho no cache `textures`. `stb_image` lê quatro canais, o carregador configura alinhamento de unpack igual a um, envia a imagem como `GL_SRGB8_ALPHA8` e gera mipmaps. Os materiais importam `Kd`, opacidade, caminho difuso e opção de clamp. Dois samplers distinguem repetição e clamp; filtros usam `GL_LINEAR_MIPMAP_LINEAR` para minificação e `GL_LINEAR` para magnificação. Os recursos são liberados por `Release` e pelos destruidores antes da destruição do contexto OpenGL.

## Posicionamento geográfico e pivô do avião

Os chunks usam matriz identidade e mantêm as coordenadas originais em comum. Centralizar ou normalizar cada parte isoladamente deslocaria ruas, prédios e aeroportos entre si. O jogador começa na pista 20L do Santos Dumont, 100 m após o extremo norte do eixo identificado no OSM:

```cpp
aircraft.set_position(glm::vec3(5237.1933f,0.0f,6295.7127f));
aircraft.set_forward(glm::normalize(glm::vec3(.05899537f,0,.99825826f)));
aircraft.set_up(glm::vec3(0,1,0));
```

O `Y=0` inicial foi conferido nos triângulos de asfalto da pista. A posição do estado do avião representa o **ponto de contato das rodas**, e não o centro da caixa do asset. `AircraftTransform` calcula o centro dos bounds, substitui sua altura pela menor altura do modelo e usa esse ponto como pivô:

```cpp
glm::vec3 pivot=(aircraftModel.min+aircraftModel.max)*.5f;
pivot.y=aircraftModel.min.y;
const glm::vec3 position=aircraft.get_position();
return Matrix_Translate(position.x,position.y,position.z)*orientation*
       Matrix_Translate(-pivot.x,-pivot.y,-pivot.z);
```

Em notação matricial, `M = T(position) R T(−pivot)`. A multiplicação mais à direita remove primeiro o pivô do modelo; `R` aplica orientação; a translação final leva o conjunto ao ponto do mundo. `R` é formado por `forward` normalizado, `right = normalize(cross(forward, up))` e um `up` corrigido por `cross(right, forward)`. Isso compatibiliza o nariz +X do OBJ com a direção de voo. As translações usam `Matrix_Translate` do código da aula, sem construtores proibidos de transformação do GLM.

O outro avião reutiliza o mesmo `aircraftModel`, com uma matriz própria obtida da pose Bézier. Não há segunda leitura do OBJ ou duplicação dos buffers para essa instância. Os detalhes da trajetória e de sua evolução no tempo pertencem ao módulo de animação, separado do carregamento.

## Materiais, iluminação e correções de aparência

`MaterialPolicy` é uma classificação pura em CPU. Ela distingue superfícies de base e camadas de pintura, sem depender do contexto OpenGL. Famílias como faixas de pista, marcações viárias e camadas de janelas são desenhadas como decal. Árvores, billboards, cercas, redes e torres recortadas por alpha são exceções: a transparência da imagem define sua silhueta, e esses objetos continuam no passe sólido com recorte.

A ausência de imagem não elimina a superfície. Nesses casos, a política escolhe detalhe procedural de grama, metal, pintura ou superfície genérica, multiplicado pela cor importada. No fragment shader:

```glsl
vec4 texel = has_diffuse_texture ? texture(diffuse_texture,texcoords) : vec4(1.0);
if(!has_diffuse_texture && procedural_surface != 0)
    texel = vec4(vec3(surfaceDetail(position_model.xyz)),1.0);
float alpha = texel.a * material_opacity;
if(alpha < alpha_cutoff) discard;
```

O detalhe usa posição local, para acompanhar o objeto, e `fwidth` para atenuar frequências menores que um pixel. A iluminação combina ambiente, difusão e um brilho especular calculado com o vetor intermediário entre sol e observador; tecnicamente, esse termo usa a forma Blinn–Phong. A expressão real é:

```glsl
vec3 lit = material_diffuse * texel.rgb * (0.35+0.65*diffuse) + 0.04*specular;
```

Há correção de exibição com expoente `1/2.2` e névoa entre 8 km e 28 km, misturando a cor à cor do céu. Não foram acrescentados mapas de normal, pipeline PBR ou sombras.

Uma correção adicional foi necessária no parser de MTL: o indicador `has_kd` sobrevivia de um material ao seguinte. Um material com imagem e sem `Kd` explícito podia receber uma cor difusa inadequada, escurecendo a textura. A alteração local em `include/tiny_obj_loader.h` reinicializa `has_kd = false` em cada `newmtl`, identifica a fonte com comentário `FONTE` e tem regressão em `tests/MaterialLoadingTest.cpp`.

## Profundidade, transparência e estabilidade em movimento

`GameScene::Draw` coordena três passes. Primeiro desenha sólidos com escrita de profundidade e `GL_SAMPLE_ALPHA_TO_COVERAGE`; a janela pede quatro amostras MSAA. Depois desenha pintura com blending e bias negativo, sem alterar o buffer de profundidade. Finalmente desenha materiais transparentes com blending e ordenação aproximada do mais distante para o mais próximo.

```cpp
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
glDepthMask(GL_FALSE);
```

A pintura do OSM estava aproximadamente 1 mm acima da superfície de base. `glPolygonOffset(-1.0f,-2.0f)` ajuda a estabilizar a seleção de profundidade dessas camadas; ao final, o estado de escrita de profundidade é restaurado. A ordenação transparente usa os centros de instâncias e batches, portanto não resolve perfeitamente superfícies transparentes que se intersectam.

Para reduzir cintilação das texturas em ângulos rasantes, o renderer aplica anisotropia de até 8× somente quando a extensão está disponível. Outra medida reduz a magnitude das UVs repetidas: subtrai-se dos três vértices de cada triângulo o mesmo deslocamento inteiro `floor(uv0)`. Sob repetição, `texture(u − k)` preserva o padrão para `k` inteiro; as diferenças entre UVs e suas derivadas também permanecem iguais. Texturas com clamp não recebem esse tratamento.

A precisão das coordenadas grandes do mapa foi tratada no vertex shader:

```glsl
vec4 world = model * model_coefficients;
// Subtract the camera before rotating. Avoid cancellation between large
// map coordinates and view translation when surfaces are millimeters apart.
vec4 camera_relative = vec4(mat3(view)*(world.xyz-camera_position),1.0);
gl_Position = projection * camera_relative;
```

Subtrair a posição da câmera antes de aplicar a rotação evita combinar, na multiplicação de view, termos grandes de translação que quase se cancelam. A matriz de projeção continua sendo a da aula. As normais são transformadas pela inversa da transposta de `model`, enquanto UVs seguem para interpolação no rasterizador. No cockpit, mundo distante e interior próximo ainda usam faixas de profundidade separadas; o detalhe desse segundo passe está documentado com as câmeras.

## Texturas do chão: causas distintas e o que cada correção resolve

### Separar falha de imagem, cor difusa, lacuna e disputa de profundidade

Os problemas relatados durante movimento não tinham uma causa única. Uma superfície podia existir com imagem, mas ficar escura por multiplicação de `Kd`; podia faltar um triângulo, revelando o céu através da lacuna; uma faixa de pista podia disputar profundidade com o asfalto; ou o padrão repetido podia cintilar por redução de escala. Esses casos exigiram intervenções em camadas diferentes do sistema. A base complementar resolve lacunas geométricas; a política de materiais organiza pintura; o parser resolve o estado de cor; filtros e coordenadas ajudam a estabilidade da amostragem. Nenhum desses mecanismos equivale a acrescentar um normal map.

Antes de investigar um material, também é necessário garantir que o executável corresponde às fontes atuais. A experiência de rodar versões antigas mostrou que recompilar e executar a partir de `bin/Debug`, mantendo os caminhos relativos esperados, é parte da conferência. A presença de um modelo antigo ou de uma escala de fonte antiga não demonstra, por si só, que o carregador ignorou o novo asset.

### Por que uma textura carregada podia aparecer preta

O fragment shader multiplica a imagem por `material_diffuse`. Logo, uma imagem correta não basta se a cor difusa importada for zero. A falha em `tiny_obj_loader.h` era o vazamento do indicador de presença de `Kd` entre blocos `newmtl`: o material novo era reinicializado, mas o booleano ainda refletia o anterior. A rotina que fornece um valor difuso padrão para materiais com `map_Kd` depende desse indicador:

```cpp
if (!has_kd) {
  material.diffuse[0] = static_cast<real_t>(0.6);
  material.diffuse[1] = static_cast<real_t>(0.6);
  material.diffuse[2] = static_cast<real_t>(0.6);
}
```

A correção local consiste em reinicializar o indicador na abertura de cada material, junto dos demais estados temporários. O teste sintético encadeia um material colorido com `Kd`, outro com textura e sem `Kd`, um preto explícito e mais um somente com textura. O resultado esperado para os materiais sem `Kd` é `(0.6,0.6,0.6)`, enquanto o preto explícito precisa continuar preto. Isso evita uma “correção” global que clarearia materiais legitimamente escuros e destruiria as cores do MTL. A regressão independe de Blender, janela ou leitura das imagens: verifica precisamente a interpretação dos materiais.

### Faixas de pista, ruas, campos e janelas como camadas de pintura

O OSM2World exporta várias superfícies em camadas. Por exemplo, o asfalto pode estar em `MAT_13_0` e as faixas sobre ele em `MAT_13_1`. A imagem de uma faixa possui alpha, mas ela não deve ser tratada da mesma forma que um objeto de vidro nem bloquear as superfícies abaixo como se toda a área do seu polígono fosse opaca. `MaterialPolicy` reconhece tanto nomes gerados quanto famílias de imagens e guarda essa decisão separadamente do código de OpenGL.

`MaterialPolicyTest.cpp` exercita exemplos reais: marcação tracejada, faixa central da pista, heliporto, campos de futebol, tênis e vôlei. Também testa as exceções de árvores, cercas e redes, cujos quads recortados por alpha representam a forma visível de um objeto físico, em vez de tinta aplicada sobre outra superfície. Uma regra baseada apenas no sufixo `_1` classificaria incorretamente esses casos.

O resultado é um fluxo de profundidade previsível: base sólida preenche o depth buffer; pintura mistura sua cor sem reescrever profundidade; transparências são desenhadas posteriormente. O bias negativo da pintura não modifica os vértices do arquivo nem sua altura física: ele ajusta a comparação de profundidade durante a rasterização. A base complementar recebe bias positivo, em sentido oposto, para ceder prioridade às superfícies exportadas. Isso é especialmente relevante quando as faces têm distância de milímetros, como nas marcações da pista.

### Por que o problema aparecia ao mover a câmera

Em coordenadas geográficas locais de milhares de metros, as operações em ponto flutuante e a distribuição de profundidade podem tornar diferenças muito pequenas instáveis. O cálculo relativo à câmera reduz o cancelamento numérico no posicionamento da cena, enquanto o polygon offset trata a concorrência entre camadas. São mecanismos complementares: a primeira mudança não transforma o depth buffer em precisão infinita, e a segunda não corrige caminhos de imagens ou UVs erradas.

A textura repetida tinha também UVs de grande magnitude. Subtrair o mesmo inteiro de todos os cantos de um triângulo mantém a repetição e os gradientes. Para vértices `u0,u1,u2` e deslocamento inteiro comum `k`, os valores `ui' = ui − k` satisfazem `u1' − u0' = u1 − u0`. Isso preserva a densidade da textura e a escolha de mipmap; tomar a parte fracionária de cada vértice separadamente não teria a mesma propriedade e poderia criar uma interpolação diferente. O código usa a condição `!materials[id].diffuse_texopt.clamp`, pois uma textura com borda limitada não pode ser transladada livremente dessa maneira.

Por fim, a redução de escala da imagem é tratada por mipmaps e anisotropia. O sampler é configurado para minificação trilinear, e `MaxAnisotropy()` procura as extensões EXT ou ARB correspondentes antes de limitar o valor ao menor entre o suportado e 8. Isso melhora padrões vistos quase paralelos ao solo, sem depender de uma extensão existir em todas as máquinas. Árvores e cercas recebem alpha-to-coverage no passe sólido, aproveitando o MSAA solicitado; vidros continuam sujeitos à ordenação aproximada por centros.

### Como verificar avião e chão sem confundir os subsistemas

O roteiro de conferência deve combinar verificações de arquivos, testes CPU e imagens do programa. Nos arquivos do avião, confirmar `mtllib`, caminhos `map_Kd`, UVs exportadas e a pasta `textures/`; no parser, executar `material_loading`; na classificação, executar `material_policy`; na base, executar `base_terrain`. No jogo, comparar a faixa da pista em três posições próximas, vistos rasantes e de cima; observar áreas antes vazias nos parques; verificar árvores contra o chão e o céu; conferir a cauda e o interior do A320. Os arquivos PPM de `--texture-check` congelam essas vistas para comparação e registram erros da API, mas o aspecto visual continua exigindo inspeção.

A preservação dos mapas de normal e rugosidade do asset não deve ser usada como evidência de que o chão ou o avião já possuem esse efeito no jogo. O shader atual lê uma imagem difusa, `Kd` e opacidade, acrescenta iluminação e o detalhe procedural documentado. Se uma correção futura introduzir normal mapping, ela precisará de uma implementação própria de dados de tangentes, interpretação do mapa e cálculo da iluminação; nada disso foi incluído nas correções aqui descritas.

## Culling, inspeção gráfica e limites conhecidos

`ModelRenderer::Visible` transforma os oito cantos da AABB do chunk para clip space. Se todos ficarem fora de um mesmo plano, a parte é descartada naquele passe. O teste é conservador e baseado na caixa, não em visibilidade exata de cada prédio. O mundo pode ser desenhado no passe `WORLD`, o jogador no passe `AIRCRAFT`, ou ambos em `ALL`; isso permite o tratamento próprio do cockpit, mantendo o tráfego no mundo.

A conferência dos problemas iniciais de materiais usou **13 vistas**. A versão atual de `RenderInspection` acrescenta duas posições do tráfego Bézier e totaliza **15 vistas**: pista baixa e alta, aeroporto de cima, três parques, cidade, Galeão, avião, três deslocamentos na pista, cockpit e as duas vistas do NPC. O programa executa o modo abaixo a partir de `bin/Debug`:

```powershell
.\main.exe --texture-check ../../build/textures-check
```

Cada vista aguarda 30 quadros, captura o framebuffer por `glReadPixels`, grava PPM com as linhas invertidas verticalmente e consulta `glGetError`. Os deslocamentos `runway-move-a/b/c` ajudam a comparar o efeito das correções durante mudança de posição; vistas altas expõem lacunas e transparências; a vista interna verifica a integração dos materiais do cockpit. Um retorno sem erro OpenGL comprova a ausência de erros detectados pela API nesse roteiro, mas não prova que todas as UVs ou todos os materiais da cidade estejam visualmente corretos.

Os testes automatizados também verificam classificação de materiais, leitura de MTL, divisão e base determinísticas. Permanecem limites concretos: carregamento integral do mapa na inicialização; preenchimento genérico sem relevo; transparência com ordenação aproximada; ausência de sombras e PBR; simplificações documentadas da pintura do A320; e nenhuma malha física persistente de obstáculos nessa etapa. Esses limites devem orientar a evolução do projeto sem confundir funcionalidades planejadas com as já presentes.

## Câmeras: separar observação, entrada e estado do avião

O projeto passou a ter três perspectivas selecionáveis: `L` para a câmera externa, `C` para o cockpit e `F` para a câmera livre. A separação importante está nas responsabilidades. `CameraController` calcula a pose da câmera, `CameraInput` traduz os eventos do GLFW e `MouseCapture` contém o confinamento específico de Windows. Nenhuma dessas classes calcula sustentação, arrasto ou aceleração do avião. A ligação com o objeto controlado ocorre por um retrato de sua transformação em cada quadro.

Em `src/camera/CameraController.hpp`, o contrato começa assim:

```cpp
struct CameraFrame {
    glm::mat4 aircraftModel{1.0f};
    glm::vec3 orbitTarget{0.0f};
    glm::vec3 cockpitEyeLocal{7.90f,0.95f,-0.65f};
};
```

`aircraftModel` é a mesma matriz que posiciona a malha do A320. `orbitTarget` é o ponto que a visão externa acompanha. `cockpitEyeLocal` é um ponto medido dentro do modelo, ainda antes da transformação para o mundo. O resultado do controlador, `CameraPose`, contém `position`, `forward`, `up`, `nearPlane`, `farPlane` e `fieldOfView`. A estrutura não cria uma janela nem executa chamadas OpenGL. Já `CameraMovement` representa a intenção de deslocamento nos eixos para a frente, lateral e vertical, junto da opção de velocidade ampliada.

Essa organização permite verificar a matemática em testes de CPU, sem carregar o Rio ou abrir uma janela. Também mantém o movimento da cabeça separado do movimento do avião: olhar para o lado dentro do cockpit não altera o vetor `forward` de `Aircraft`.

### A base de vetores e a formação da matriz view

A orientação usa operações elementares com vetores. A função `Direction`, em `CameraController.cpp`, converte os dois ângulos de olhar em um vetor:

```cpp
glm::vec3 Direction(float yaw,float pitch) {
    return glm::vec3(std::cos(pitch)*std::cos(yaw),std::sin(pitch),
                     std::cos(pitch)*std::sin(yaw));
}
```

Com ângulos nulos, a direção é `+X`, que é a direção do nariz no OBJ. O seno de `pitch` controla a componente vertical. O cosseno de `pitch` reduz as componentes horizontais à medida que o olhar se aproxima da vertical. `UprightRight` calcula o vetor lateral horizontal associado ao yaw; produtos vetoriais formam os demais eixos.

Há uma etapa explícita de ortonormalização. Primeiro, `forward` é normalizado. A componente de `up` paralela a `forward` é removida por projeção: `up - forward * dot(up, forward)`. O resultado é normalizado novamente. Se os vetores estiverem quase alinhados, o código escolhe uma referência alternativa para evitar uma base degenerada. A função auxiliar `Unit` também fornece um vetor de reserva quando o comprimento é nulo ou não finito. Isso protege as matrizes de valores inválidos em limites de inclinação e mudanças de modo.

O controlador entrega os vetores; `main.cpp` usa as matrizes fornecidas pela disciplina:

```cpp
glm::mat4 view=Matrix_Camera_View(glm::vec4(camera.position,1),
    glm::vec4(camera.forward,0),glm::vec4(camera.up,0));
```

Assim, a implementação não usa `glm::lookAt`, `glm::perspective` ou outros construtores de câmera proibidos pelo trabalho. GLM continua sendo usado para armazenar vetores e matrizes e realizar operações como produto escalar, produto vetorial e multiplicação de matrizes. A projeção é formada por `Matrix_Perspective` ou `Matrix_Orthographic`, também do código da aula.

### Câmera externa: órbita sobre um alvo móvel

A câmera externa não gira o avião. Seu estado particular é `orbitYaw`, `orbitElevation` e `orbitDistance`; inicialmente são zero, `0.25` radiano e `70` metros. O rumo horizontal é extraído da primeira coluna de `aircraftModel`. A componente Y é zerada para que a referência da órbita acompanhe a direção horizontal do avião mantendo o horizonte nivelado.

O trecho central de `OrbitPose` é:

```cpp
const glm::vec3 right=glm::cross(forward,WorldUp);
const glm::vec3 offset=(-forward*std::cos(yaw)+right*std::sin(yaw))*std::cos(elevation)
                      +WorldUp*std::sin(elevation);
CameraPose pose;
pose.position=frame.orbitTarget+distance*offset;
pose.forward=Unit(frame.orbitTarget-pose.position,forward);
```

O vetor `offset` combina uma posição atrás do avião, um deslocamento lateral e elevação. Multiplicá-lo pela distância coloca a câmera sobre uma órbita em torno do alvo. O olhar é recalculado do observador para esse alvo. `GameScene::Cameras()` usa a posição do avião mais quatro metros de altura como alvo, evitando mirar somente no ponto de contato das rodas.

O movimento horizontal do mouse modifica o azimute. O vertical modifica a elevação com sinal invertido, pois os eventos de tela têm Y positivo para baixo. Portanto, arrastar para cima eleva a câmera externa. A elevação fica entre `0.02` radiano e `π/2 - 0.01`, evitando chegar ao polo em que a base view perderia uma direção lateral bem definida. A rodinha modifica a distância de forma exponencial, entre `5` e `15000` metros. `Home` restaura a órbita inicial.

### Cockpit: uma câmera em primeira pessoa ligada à aeronave

A câmera do cockpit é uma câmera em primeira pessoa cujo referencial acompanha o A320. O ponto dos olhos foi calibrado em `(7.90, 0.95, -0.65)` nas coordenadas originais do OBJ: próximo do assento esquerdo, acima do encosto, atrás do painel e abaixo do teto. Não é a posição global do avião, nem o centro de sua caixa delimitadora.

O cálculo em `CameraController::Pose` é:

```cpp
pose.position=glm::vec3(frame.aircraftModel*glm::vec4(frame.cockpitEyeLocal+headOffset,1));
const glm::vec3 direction=Direction(headYaw,headPitch);
const glm::vec3 localUp=glm::cross(UprightRight(headYaw),direction);
pose.forward=glm::vec3(frame.aircraftModel*glm::vec4(direction,0));
pose.up=glm::vec3(frame.aircraftModel*glm::vec4(localUp,0));
pose.nearPlane=.05f;
```

O `w=1` da posição inclui a translação da matriz model. Os vetores de direção usam `w=0`, recebendo a orientação sem uma translação indevida. Com isso, inclinação e rolagem aplicadas ao avião também inclinam a câmera. `headYaw`, `headPitch` e `headOffset` representam somente o movimento da cabeça em relação à cabine.

O olhar lateral fica limitado a ±120°, o vertical a 70° para baixo e 80° para cima. As setas e `Page Up/Page Down` deslocam a cabeça a `0.35 m/s`, com limites locais de ±20 cm na direção longitudinal, ±12 cm na altura e ±18 cm na lateral. O vetor de intenção é normalizado, de modo que pressionar várias direções não acelera artificialmente a cabeça. `Home` zera os ângulos e esse pequeno deslocamento. O limite é uma região geométrica simples; ainda não é uma colisão da cabeça com cada painel ou janela da cabine.

O cockpit também exigiu uma solução para a precisão do depth buffer. Uma projeção com near de cinco centímetros e far de trinta quilômetros reduziria muito a precisão das superfícies distantes. Por isso, `main.cpp` desenha duas camadas com a mesma view e FOV:

```cpp
scene->Draw(g_GpuProgramID,MainAircraft,projection*view,camera.position,SceneLayer::WORLD);
glClear(GL_DEPTH_BUFFER_BIT);
glm::mat4 interior=Matrix_Perspective(camera.fieldOfView,g_ScreenRatio,-camera.nearPlane,-100.0f);
glUniformMatrix4fv(g_projection_uniform,1,GL_FALSE,glm::value_ptr(interior));
scene->Draw(g_GpuProgramID,MainAircraft,interior*view,camera.position,SceneLayer::AIRCRAFT);
```

O mundo usa near de `0.5 m` e far de `30000 m`; o A320 do jogador usa near de `0.05 m` e far de `100 m`. A limpeza intermediária afeta somente profundidade, preservando as cores já desenhadas. O avião de tráfego pertence à camada WORLD, inclusive no cockpit. As perspectivas externa e livre continuam com desenho normal em um único conjunto de passes. Esse tratamento do primeiro plano não substitui as colisões ainda pendentes no momento deste documento: penetrar em um prédio continua sendo uma situação que o futuro sistema físico precisará impedir.

### Câmera livre: desacoplar sem mudar de lugar

Ao pressionar `F`, a visão livre começa na posição e orientação da câmera que estava ativa. Ela preserva inclusive a rolagem herdada de um cockpit inclinado. `CaptureFree` copia a pose, ortonormaliza os vetores e extrai yaw, pitch e roll. Depois dessa captura inicial, a câmera deixa de seguir o avião.

`SetMode` garante que repetir a mesma tecla não refaça a captura:

```cpp
if(next==mode)return;
if(next==CameraType::FREE)CaptureFree(Pose(frame));
mode=next;
```

W/S deslocam para a frente e para trás na direção do olhar; A/D deslocam lateralmente; Q/E descem e sobem na vertical do mundo. Shift multiplica a velocidade por cinco. O movimento é normalizado antes de multiplicar velocidade e `dt`, preservando a mesma velocidade total nas diagonais. A velocidade normal inicial é `50 m/s`; a rodinha a modifica exponencialmente entre `1` e `2000 m/s`. `Home` volta à visão geral atrás do avião e restaura os 50 m/s.

`CameraInput::Key` consome W/S/A/D/Q/E enquanto o modo livre está ativo. Isso impede que W e S, usados para explorar o mapa, também mudem o throttle do avião. Ao voltar a cockpit ou externa, W/S continuam disponíveis ao controle da aeronave. Cada câmera mantém seu próprio estado de órbita ou de cabeça durante as trocas.

### Captura do mouse: corrigir o salto no clique em movimento

O controle de olhar depende do botão direito. Apenas receber um evento de cursor não deve girar a câmera. Na ativação, o GLFW entra em `GLFW_CURSOR_DISABLED`, permitindo deslocamento relativo sem parar na borda. Quando suportado, `GLFW_RAW_MOUSE_MOTION` usa contagens diretas do dispositivo em vez da aceleração aplicada pelo sistema operacional. O código mantém um fallback para o cursor virtual normal do GLFW.

A sensibilidade final compartilhada pelas três câmeras é `0.00060` radiano por unidade do mouse. Um deslocamento de cem unidades produz `0.06` radiano, aproximadamente 3,44°. O deslocamento do mouse já representa movimento acumulado; não é novamente multiplicado por `dt`. O tempo é aplicado ao deslocamento por teclas, que representa velocidade sustentada.

A dificuldade identificada durante os testes foi o clique enquanto o mouse já estava em movimento. Recentralização e confinamento podem gerar posições transitórias e compensações em ciclos posteriores de eventos. Aplicar esses deslocamentos diretamente parecia um aumento súbito de sensibilidade. A correção adotou três estados: livre, captura pendente e olhar ativo.

Ao pressionar o botão, o adaptador arma a transição:

```cpp
capturePending=true;
captureReadyAt=glfwGetTime()+CaptureSettleSeconds;
captureBatches=0;
glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
```

`CaptureSettleSeconds` vale `0.100`: cem milissegundos. Durante a transição, callbacks de cursor não chamam `camera.Look`. Ao final dos ciclos de eventos, `Update` atualiza continuamente a posição de referência:

```cpp
glfwGetCursorPos(window,&lastX,&lastY);
++captureBatches;
if(captureBatches>=2 && glfwGetTime()>=captureReadyAt) {
    capturePending=false;
    looking=true;
}
```

Há duas condições, não apenas um descarte do primeiro callback: transcorrer o prazo e processar pelo menos dois ciclos. Isso também evita que um quadro lento pule a estabilização. Depois de armada, cada evento aplica `x-lastX` e `y-lastY` com a sensibilidade normal, sem limitar movimentos rápidos legítimos. Não existe suavização que continue girando a câmera quando o mouse para.

No Windows, `ConfineMouseToWindow` obtém a área cliente da janela, converte seus cantos para coordenadas da tela e chama `ClipCursor`. O retângulo é atualizado durante o arrasto para acompanhar movimentação ou redimensionamento da janela. Soltar o botão, perder foco, trocar de câmera ou destruir o adaptador libera tanto a captura do GLFW quanto o confinamento nativo. Uma janela sem foco não pode ativar uma captura nova. Isso evita deixar o mouse preso ao fechar o jogo ou ao alternar para outro aplicativo.

### Tempo real, resolução inicial e limite de quadros

A janela passou a abrir em `1280 × 720`, redimensionável, e o início dos quadros é limitado a 60 FPS por `FrameLimiter`. Os eventos são processados após a espera e antes de calcular a nova câmera. Assim, o desenho utiliza os eventos recém-recebidos, reduzindo a sensação de responder ao quadro anterior.

O laço mede o tempo com `glfwGetTime`:

```cpp
const double elapsed = std::max(current_time - previous_time,0.0);
float dt = static_cast<float>(elapsed);
// Keep the existing physics step limit; camera travel uses real elapsed time.
MainModel.update(&MainAircraft, std::min(dt,0.05f));
```

A câmera usa o intervalo real; o passo da física mantém o limite preexistente de 50 ms. Portanto, num travamento longo a física do jogador pode avançar menos que o relógio, enquanto o movimento de exploração e o tráfego usam seus próprios intervalos. Essa diferença é uma limitação explícita da integração atual, e não uma promessa de equivalência completa da física em qualquer taxa de quadros. Se o framebuffer estiver zerado durante minimização, o programa espera eventos e atualiza a referência de tempo antes de continuar, evitando divisão por zero nas proporções da projeção.

`FrameLimiter` usa `std::chrono::steady_clock`, um relógio monotônico. No Windows tenta criar um waitable timer de alta resolução, acorda ligeiramente antes do prazo e usa `yield` na fração final. Isso evita que uma espera nominal de aproximadamente 16,67 ms seja arredondada para um intervalo muito maior pelo escalonador. Se o temporizador nativo não estiver disponível, usa `sleep_until`.

O agendamento termina assim:

```cpp
// Do not accumulate missed frames or burst after a stall/minimize.
next=Clock::now()+interval;
```

O limite é um teto por software, não uma garantia de 60 quadros reais se o carregamento ou desenho exigir mais tempo. Também não tenta compensar atraso com uma rajada de quadros imediatos depois de uma pausa. O construtor rejeita FPS inválido, infinito, não positivo ou que produza intervalos irrepresentáveis.

### Testes das câmeras e alcance da verificação

`tests/CameraControllerTest.cpp` exercita a matemática sem GLFW: transformação dos olhos com avião transladado e rotacionado, herança da orientação, preservação da pose ao entrar em F, independência em relação ao avião, movimento após um segundo com passos diferentes, normalização das diagonais, multiplicador de Shift, limites de olhar, zoom e velocidade, restauração por Home e bases/matrizes finitas. As verificações usam as matrizes da aula.

`tests/CameraInputTest.cpp` fornece implementações simuladas das funções GLFW, do foco, das teclas, do cursor e do relógio. Verifica roteamento entre câmera e throttle, captura e liberação, ausência de deslocamento sem botão direito, perda de foco e encerramento. Os testes de eixos projetam um ponto usando as matrizes view/projection reais e a posição do Santos Dumont, verificando separadamente a resposta horizontal e vertical na tela.

A regressão do clique em movimento simula posições de recentralização em ciclos distintos, com e sem raw motion, nos três modos. Antes de 100 ms, a pose deve permanecer igual; depois da transição, um deslocamento conhecido deve produzir somente o ângulo correspondente àquele deslocamento. Também verifica que soltar o botão ou perder o foco cancela uma captura pendente e que um quadro lento não dispensa o segundo ciclo de eventos.

Esses testes automatizados complementam as capturas `--smoke-test` com o A320 e o cenário reais. Os testes de CPU demonstram propriedades matemáticas e de roteamento; as capturas demonstram que a integração desenha na GPU usada na execução. Sensibilidade e conforto continuam sendo escolhas de interação avaliadas pelo usuário, motivo pelo qual os valores foram ajustados repetidamente nesta conversa.

## Animação do segundo avião por curvas de Bézier

### Responsabilidade e separação do avião do jogador

Foi acrescentado um avião autônomo ao cenário para representar tráfego aéreo e
atender ao requisito de movimentação por uma curva cúbica. Ele utiliza o mesmo
modelo A320 carregado para o jogador, mas sua posição e sua orientação são
calculadas independentemente. Essa escolha separa duas tarefas diferentes:
`FlightModel` atualiza a física do jogador, enquanto `BezierFlight` calcula a
animação cinemática do avião secundário. Alterar o throttle do jogador não muda a
rota do NPC, e os controles das câmeras não avançam essa rota.

Os arquivos se dividem em três responsabilidades:

| Arquivo | Responsabilidade |
|---|---|
| `src/animation/BezierPath.hpp/.cpp` | Avaliar cúbicas, calcular derivadas e converter distância percorrida em parâmetro da curva. |
| `src/aircraft/BezierFlight.hpp/.cpp` | Guardar velocidade/distância, avançar pelo tempo e construir a pose do avião secundário. |
| `src/render/GameScene.hpp/.cpp` | Instanciar visualmente a malha compartilhada usando a pose do NPC. |

`BezierPath` e `BezierFlight` dependem de tipos vetoriais e matemática, mas não de
OpenGL, janela ou arquivos OBJ. Por isso, a geometria e a animação podem ser
testadas sem carregar os assets do projeto. `FlightPose` contém posição e os
vetores `forward`, `up` e `right`; esses vetores já constituem a base necessária
para montar a orientação da malha.

### Avaliação da cúbica e derivadas

Cada trecho contém quatro pontos de controle: `p0` e `p3` são os extremos, e
`p1` e `p2` controlam a curvatura. O parâmetro `u` pertence ao intervalo `[0,1]`.
A implementação usa diretamente a base de Bernstein, sem chamar funções de
curva de uma biblioteca externa. Em `BezierPath.cpp`:

```cpp
glm::dvec3 CubicBezier::Position(double u) const {
    CheckParameter(u);
    const double v=1-u;
    return v*v*v*p0+3*v*v*u*p1+3*v*u*u*p2+u*u*u*p3;
}
glm::dvec3 CubicBezier::Tangent(double u) const {
    CheckParameter(u);
    const double v=1-u;
    return 3.0*(v*v*(p1-p0)+2*v*u*(p2-p1)+u*u*(p3-p2));
}
glm::dvec3 CubicBezier::SecondDerivative(double u) const {
    CheckParameter(u);
    return 6.0*((1-u)*(p2-2.0*p1+p0)+u*(p3-2.0*p2+p1));
}
```

A tangente é a primeira derivada `B'(u)`: ela informa a direção instantânea do
deslocamento. A segunda derivada fornece informação sobre a mudança dessa
direção, usada posteriormente para inclinar o avião durante a curva. O uso de
`glm::dvec3` mantém os cálculos do caminho em precisão dupla; a conversão para
`glm::vec3` acontece na pose usada pelo desenho.

O construtor de `BezierPath` verifica pontos finitos, segmentos não degenerados,
união das posições e concordância das direções das tangentes. A validação geral
exige continuidade de posição e direção; a rota circular específica construída
pelo programa também tem derivadas iguais nas junções. As quatro junções são
verificadas, inclusive a ligação do último trecho com o primeiro.

### Circuito fechado de quatro trechos

`HorizontalCircle` constrói quatro arcos cúbicos que aproximam uma circunferência
horizontal. Não se trata de uma circunferência analítica exata. O fator das
alças é `k = 4*tan(pi/8)/3`, aproximadamente `0,55228475`, multiplicado pelo raio.
No código atual, a configuração padrão é:

```cpp
BezierFlight::BezierFlight()
    :BezierFlight(BezierPath::HorizontalCircle(glm::dvec3(4100,180,7200),1200),60) {}
```

Assim, o centro da rota é `(4100,180,7200)`, o raio é 1200 m, e a velocidade é
60 m/s. A altitude permanece em 180 m. Como a curva começa em `center + x`, a
posição inicial é `(5300,180,7200)`. O circuito foi colocado nas proximidades do
Santos Dumont para permitir observar o avião secundário a partir da cena
inicial. A volta leva aproximadamente 126 s, considerando o comprimento da
aproximação cúbica.

### Distância de arco em vez de avanço uniforme de `u`

Avançar `u` sempre pela mesma quantidade não garante velocidade espacial
constante: trechos do parâmetro podem corresponder a distâncias diferentes.
Para evitar aceleração e desaceleração visuais causadas somente pela
parametrização, o estado da animação é a distância percorrida `s`, em metros.

O construtor amostra cada segmento 128 vezes, calcula o comprimento das cordas
entre amostras consecutivas e acumula uma tabela de pares
`{distance, parameter}`. O parâmetro armazenado é global: parte inteira indica
o segmento e parte fracionária indica `u`. Com quatro segmentos, são 512 passos
de amostragem, além do nó inicial. Não se substitui a curva por uma polilinha no
desenho do movimento: a tabela serve para encontrar o parâmetro, e a posição
final continua sendo avaliada na própria cúbica.

`AtDistance` reduz a distância ao comprimento total com `fmod`, corrige distâncias
negativas e realiza busca binária na tabela. O trecho central da inversão é:

```cpp
const auto end=std::lower_bound(knots.begin(),knots.end(),distance,
    [](const Knot& knot,double value){return knot.distance<value;});
double parameter=0;
if(end!=knots.begin()) {
    const auto& before=*(end-1);
    parameter=before.parameter+(end->parameter-before.parameter)*
        ((distance-before.distance)/(end->distance-before.distance));
}
```

A interpolação entre os nós aproxima a inversão do comprimento de arco. A busca
é logarítmica em relação ao tamanho da tabela. A velocidade espacial resultante
é aproximadamente constante: o avanço da distância é matematicamente exato para
velocidade constante, mas a conversão de distância para `u` possui o erro da
amostragem e da interpolação.

### Integração numérica e independência do FPS

Foi utilizada a integração de Euler explícito apresentada no material da aula
de animação fornecido pelo usuário. O comentário `FONTE` identifica a referência
ao professor e aos slides 10 e 18 do PDF. Em `BezierFlight::Update`:

```cpp
void BezierFlight::Update(double dt) {
    if(!std::isfinite(dt)||dt<=0)return;
    // FONTE: Prof. Eduardo Gastal, Aula_24_Animacao.pdf, slides 10 e 18.
    // Explicit Euler applied to arc distance: s(n+1)=s(n)+speed*dt.
    // Constant speed makes this step exact; arc-length inversion is approximate.
    // Reduce dt by the lap period first to support long gaps without overflow.
    if(speed==0)return;
    const double stepTime=std::fmod(dt,path.Length()/speed);
    distance+=speed*stepTime; // Euler: distance += velocity * time step.
    distance=std::fmod(distance,path.Length());
}
```

A equação é `s(n+1) = s(n) + v*dt`. Usar o tempo decorrido evita que um computador
com mais FPS faça o avião completar a rota mais depressa. O módulo mantém a
distância dentro de uma volta. Reduzir o tempo pelo período antes da
multiplicação também permite descartar voltas completas de intervalos muito
longos, sem produzir uma distância intermediária desnecessariamente grande.
Tempos negativos, zero ou não finitos não avançam a animação.

`main.cpp` chama `scene->Update(elapsed)` uma vez por quadro, antes dos passes de
desenho. O NPC usa o tempo real em precisão dupla, sem o limite de 0,05 s mantido
na física do jogador. Quando a janela está minimizada, o loop reinicializa o
relógio; o período minimizado não se transforma em um salto ao restaurar a
janela. `Draw` consulta a pose, mas não faz outra atualização temporal.

### Orientação, inclinação e instanciamento

O nariz acompanha a tangente normalizada. Produtos vetoriais com uma referência
vertical constroem vetores ortogonais para direita e cima. Se a direção ficar
paralela à referência vertical, o código usa outra referência para evitar um
produto vetorial nulo. A curvatura lateral estima a inclinação visual:

```cpp
const double lateralCurvature=glm::dot(sample.secondDerivative,right)/glm::dot(sample.tangent,sample.tangent);
const double limit=25.0*std::acos(-1.0)/180.0;
const double bank=std::max(-limit,std::min(limit,std::atan(speed*speed*lateralCurvature/9.81)));
const auto bankedUp=glm::normalize(up*std::cos(bank)+right*std::sin(bank));
```

O limite é de 25°. Essa inclinação é uma regra para a pose do objeto, e não uma
simulação de forças do segundo avião. `FlightPose::Orientation` escreve os
vetores nas colunas de uma matriz construída pelo próprio código: nariz local
`+X`, cima `+Y` e direita `+Z`. Não são usados construtores de transformação do
GLM.

`GameScene` calcula a transformação do NPC com essa orientação e com
`Matrix_Translate` do material da aula. O pivô do NPC é o centro da malha. O
jogador continua usando o pivô associado ao contato das rodas. Os dois desenhos
referenciam o mesmo `aircraftModel`: não há outro carregamento do OBJ nem uma
cópia independente dos buffers GPU. São duas instâncias do mesmo conjunto de
vértices com matrizes de modelo diferentes.

No cockpit, o NPC pertence ao passe `WORLD`, junto da cidade. O passe próximo
`AIRCRAFT` contém apenas o modelo do jogador. Isso permite ver o avião secundário
através das janelas sem desenhá-lo artificialmente sobre o painel depois da
limpeza de profundidade reservada ao cockpit.

### Verificações da animação

`tests/BezierFlightTest.cpp` verifica extremos e ponto médio da fórmula,
derivadas analíticas comparadas a diferenças finitas, fechamento, junções,
comprimento aproximado, distância negativa, múltiplas voltas e velocidade
espacial aproximadamente constante. Também compara poses após o mesmo tempo a
30, 60 e 144 FPS, compara quadros curtos com um intervalo longo e testa posição,
direção e inclinação dos dois lados de cada junção. Há verificações da base
ortonormal, do alinhamento do nariz e do limite de inclinação.

O modo `--texture-check` recebeu as vistas `bezier-start` e `bezier-later`. Por
isso, o conjunto de inspeção da cena passou das 13 vistas anteriores para 15.
Essas capturas complementam os testes CPU: permitem conferir o uso da malha e
dos passes com os assets reais, enquanto o log informa a posição do NPC e erros
OpenGL. Esse modo de desenvolvimento não altera a definição normal da rota.

## HUD: instrumentos de voo e apresentação das leituras

### Arquitetura e fluxo por quadro

O painel acrescenta cinco instrumentos sobre a cena: velocidade, altitude,
horizonte artificial, bússola e potência/throttle. Foram separados quatro
problemas que poderiam ter ficado misturados no loop principal:

| Módulo | Responsabilidade |
|---|---|
| `FlightTelemetry.*` | Converter estado do jogador em leituras e flags de validade. |
| `HudLayout.*` | Calcular centros/raios e a conversão de leituras em ângulos. |
| `HudNeedleMotion.*` | Suavizar visualmente os ponteiros com tempo real. |
| `FlightHud.*` e shaders | Gerar e desenhar a geometria 2D dos instrumentos. |
| `HudInspection.*` | Produzir dados controlados e capturas apenas no modo de desenvolvimento. |

A coleta ocorre depois da atualização do avião. O trecho de `main.cpp` mostra
que a atualização dos ponteiros acontece mesmo quando o painel está oculto:

```cpp
FlightTelemetry readings=CalculateFlightTelemetry(MainAircraft.get_position(),MainAircraft.get_velocity(),
    MainAircraft.get_forward(),MainAircraft.get_up(),MainAircraft.get_throttle());
if(hud_check)readings=hudInspection.Telemetry(readings);
hud->Update(readings,elapsed);
if(g_ShowInfoText) {
    hud->Draw(window,readings);
```

Os sensores sempre leem `MainAircraft`. Trocar para a câmera livre não faz a
bússola representar a direção da câmera nem transforma sua velocidade em
velocidade do avião. A velocidade de deslocamento da câmera possui texto de
ajuda próprio, identificado como `Camera livre`, para distinguir as duas
informações. `H` utiliza a opção existente `g_ShowInfoText` para alternar a
visibilidade do painel e dos textos auxiliares.

### Unidades, significado e validade dos dados

`FlightTelemetry` armazena valores `double` e flags independentes para altitude,
velocidade, throttle, rumo e atitude. Uma leitura inválida não deve contaminar as
demais. O valor armazenado permanece finito e a flag informa se o instrumento
pode apresentá-lo. O desenho mostra `--` e omite a agulha correspondente quando
não há leitura válida.

| Leitura | Cálculo e significado |
|---|---|
| Altitude | `position.y`, em metros relativos ao nível zero do mapa. |
| Velocidade | Módulo da velocidade tridimensional multiplicado por 3,6, em km/h. |
| Potência | `throttle` limitado a `[0,1]` e multiplicado por 100. |
| Rumo | Direção horizontal do nariz, com norte `-Z` e leste `+X`. |
| Atitude | Pitch e roll extraídos da base vetorial que orienta o modelo. |

A velocidade inclui a componente vertical e não corresponde a um sensor
aerodinâmico de velocidade indicada: o projeto não implementa esse sensor nem
vento nessa versão. A altitude não é uma consulta da distância entre avião e
terreno, nem uma elevação geográfica medida a partir de um modelo de relevo. A
potência é o comando de throttle usado pelo modelo atual, e não uma medição
independente de potência mecânica do motor. Essas distinções evitam atribuir ao
HUD uma precisão que a simulação ainda não possui.

### Rumo, pitch e roll a partir dos vetores do modelo

Os campos de ângulos do avião não foram utilizados como fonte principal da
atitude porque ainda existem campos de ângulos desconectados da orientação
efetivamente desenhada. A telemetria deriva a leitura de `forward` e `up`,
normalizando e ortogonalizando a base da mesma forma que `GameScene`.

O pitch é `atan2(forward.y, hypot(forward.x,forward.z))`. O rumo usa
`atan2(forward.x,-forward.z)`, convertido em graus e normalizado para `[0,360)`:

```cpp
const double horizontal=std::hypot(front.x,front.z);
result.pitchDegrees=std::atan2(front.y,horizontal)*Degrees;
if(horizontal<=AngularEpsilon)return result;

double heading=std::atan2(front.x,-front.z)*Degrees;
if(heading<0)heading+=360.0;
result.headingDegrees=heading>=360.0||heading==0?0.0:heading;
result.headingValid=true;
```

Assim, norte é 0°, leste 90°, sul 180° e oeste 270°. Quando o avião aponta
exatamente na vertical, não há projeção horizontal suficiente para definir um
rumo estável. O módulo conserva as leituras independentes que forem válidas,
mas invalida rumo e atitude completa nesse caso.

Para o roll, primeiro se obtém `right = normalize(cross(front,providedUp))` e
depois `correctedUp = cross(right,front)`. A referência nivelada também é
construída por produtos vetoriais. O `atan2` compara as projeções do vetor cima
do avião sobre a direita e o cima da base nivelada:

```cpp
const glm::dvec3 correctedUp=glm::cross(right,front);
const glm::dvec3 levelRight=glm::normalize(glm::cross(front,glm::dvec3(0,1,0)));
const glm::dvec3 levelUp=glm::cross(levelRight,front);
result.rollDegrees=std::atan2(glm::dot(correctedUp,levelRight),
                            glm::dot(correctedUp,levelUp))*Degrees;
result.attitudeValid=true;
```

Pitch positivo representa nariz para cima; roll positivo representa asa direita
para baixo. A utilização de `atan2`, em vez de uma razão seguida de `atan`,
preserva a distinção de quadrantes, inclusive para a condição invertida.

### Escalas analógicas contínuas

Os riscos dos mostradores são referências de leitura; eles não restringem os
ângulos possíveis. A função `HudNeedleDegrees` recebe um valor decimal e retorna
um ângulo decimal no sentido horário a partir de doze horas. O deslocamento não
é arredondado para o risco mais próximo. Somente o texto numérico é formatado
sem casas decimais.

```cpp
case FlightInstrument::Speed:
    return 210+300*std::max(0.0,std::min(1000.0,reading))/1000;
case FlightInstrument::Throttle:
    return 210+300*std::max(0.0,std::min(100.0,reading))/100;
```

Velocidade cobre 0–1000 km/h e potência cobre 0–100%. Ambas usam 300°, com um
intervalo de 60° entre os extremos. Isso deixa zero e máximo em posições
distinguíveis. O ponteiro satura nos extremos dessas escalas; o número ainda
exibe a velocidade real acima de 1000 km/h. No modelo físico atual, velocidades
baixas naturalmente percorrem uma pequena parte dessa escala.

Bússola e altitude usam toda a volta. O rumo tem período de 360°; o altímetro tem
período de 1000 m, correspondendo a uma volta do ponteiro por mil metros. O
número do altímetro conserva a altitude completa. O módulo também normaliza
altitudes negativas, de modo que passar de 0 para -1 m gire para trás de forma
contínua. Um ajuste realizado durante a validação substituiu o arco incompleto
do altímetro por essa volta completa, evitando o salto ao passar de 999 para
1000 m. As marcas e os ponteiros consultam a mesma função de escala.

### Suavização visual independente da física

O throttle do modelo existente muda 5% por evento de teclado. Embora o cálculo
do ângulo já aceitasse valores fracionários, trocar diretamente entre esses
comandos fazia a agulha parecer dar pequenos saltos. A solução foi acrescentar
`HudNeedleMotion`, mantendo intacto o comando de throttle e as leituras reais.

A resposta é exponencial de primeira ordem, com constante de tempo `tau=0,12 s`:
`alpha = 1-exp(-dt/tau)`. O código utiliza `-expm1(-dt/tau)`, que também evita a
subtração de números muito próximos quando `dt` é muito pequeno:

```cpp
const double alpha=std::isfinite(elapsedSeconds)&&elapsedSeconds>0?
    -std::expm1(-elapsedSeconds/ResponseSeconds):0;
```

O valor visual aproxima-se continuamente do alvo, sem ultrapassá-lo. Para uma
leitura constante, a evolução ao longo do mesmo intervalo de tempo é equivalente
a 30, 60 ou 144 FPS. O primeiro valor válido aparece imediatamente. Ao perder a
validade, o canal remove a agulha; ao recuperar a leitura, inicializa novamente
na posição correta. Tempos inválidos ou não positivos congelam canais válidos,
mas continuam permitindo invalidar ou reinicializar um canal.

Velocidade, potência e altitude são interpoladas como valores escalares. A
altitude é suavizada em metros antes do módulo de mil metros. O rumo precisa de
uma regra circular própria para passar de 359° para 1° pelo arco de 2°:

```cpp
if(circular)current=Heading(current+alpha*std::remainder(target-current,360.0));
```

Aplicar o menor arco também às escalas limitadas seria inadequado: ao ir de zero
para potência máxima, a agulha deve atravessar a escala de 300°, não cruzar o
intervalo vazio de 60°. Os números continuam mostrando as leituras reais, e o
horizonte artificial continua usando a atitude real sem esse atraso visual.

### Layout responsivo e coordenadas da tela

`CalculateHudLayout` usa pixels lógicos com origem no canto superior esquerdo.
O raio preferido dos mostradores comuns é 64 pixels, o horizonte tem raio 20%
maior e a separação entre círculos é proporcional ao raio. Em 1280×720 os cinco
instrumentos ficam em uma linha inferior, com o horizonte no centro. Abaixo de
650 pixels de largura, velocidade e altitude passam para uma linha superior,
enquanto horizonte, bússola e potência ocupam a inferior.

O raio é limitado simultaneamente pela largura e pela altura disponíveis, sem
ampliar acima do tamanho preferido. Dimensões não positivas retornam um layout
vazio. A função conserva a ordem lógica dos instrumentos nos dois arranjos.
Graduações numéricas secundárias são omitidas quando os mostradores ficam muito
pequenos; as leituras principais permanecem. Os textos internos têm escala
mínima de 0,82 para evitar encolhimento excessivo.

O vertex shader converte os pixels para NDC diretamente:

```glsl
gl_Position=vec4(2.0*position.x/resolution.x-1.0,1.0-2.0*position.y/resolution.y,0,1);
```

A largura e a altura lógicas da janela são as mesmas usadas na construção do
layout; o viewport continua usando a resolução real do framebuffer. Isso
preserva as proporções dos círculos com redimensionamento e escalonamento do
monitor, sem utilizar construtores de projeção de bibliotecas externas.

### Geometria, batch GPU e preservação de estado

`FlightHud` produz triângulos coloridos. Linhas são retângulos formados por dois
triângulos; círculos e anéis são aproximados por 96 segmentos. As agulhas usam
um triângulo alongado, uma pequena extensão traseira e um disco central. Isso
permite controlar a espessura sem depender de suporte a linhas largas do driver.

Cada vértice possui posição 2D, RGBA, coordenada local e modo de instrumento.
Após montar todos os mostradores, o código envia o vetor de vértices para um VBO
com `GL_STREAM_DRAW` e realiza um `glDrawArrays` para a geometria. Os textos são
desenhados em seguida pelo renderizador de fonte já existente. O fundo dos
mostradores recebe uma variação procedural discreta no fragment shader; não
foram adicionadas imagens de textura para o HUD.

O painel desabilita testes/escritas de profundidade, culling, scissor e
alpha-to-coverage durante seu passe. Ativa blending para desenhar a geometria
por cima da cena. Um objeto local `State` salva e restaura programa, VAO, buffer,
função/máscara de profundidade, modo de polígono, fatores/equações de blending e
os estados habilitados. Essa preservação inclui alterações feitas pelo
renderizador da fonte. O destrutor de `FlightHud` libera programa, VAO e VBO;
`main.cpp` destrói o objeto antes de encerrar o contexto OpenGL.

### Horizonte artificial

A referência amarela do avião permanece fixa no centro do instrumento. O céu,
o solo e a linha de horizonte se movem de acordo com pitch e roll. No fragment
shader, o modo de horizonte recorta um círculo na coordenada local e calcula:

```glsl
float level=-sin(attitude.y)*local.x+cos(attitude.y)*local.y+attitude.x/0.7853981634;
float aa=max(fwidth(level),0.002);
vec3 ground=vec3(0.29,0.15,0.07),sky=vec3(0.07,0.35,0.59);
vec3 surface=mix(ground,sky,smoothstep(-aa,aa,level));
```

O divisor é aproximadamente `pi/4`: uma mudança de pitch de 45° desloca a linha
pela unidade local da escala escolhida. O roll rotaciona o horizonte, e a
translação de pitch faz o solo descer na imagem quando o nariz sobe. `fwidth`
ajusta a suavização da borda pela variação do fragmento. Marcas de pitch de 10°
são geradas na CPU, com traços maiores a cada 20°, aplicando uma transformação
coerente com a do shader. No voo invertido o instrumento mostra solo em cima e
céu embaixo; dados inválidos substituem o fundo por cinza e exibem `--`.

### Texto por atlas e ajustes de legibilidade

O desenho de texto reutiliza `src/ui/textrendering.cpp` e a fonte empacotada no
código base. A textura da fonte contém um atlas de glifos. Para cada caractere,
o programa localiza suas métricas e coordenadas UV, monta um retângulo de seis
vértices e desenha os dois triângulos correspondentes. O fragment shader
combina o interior branco da letra com uma borda escura, aumentando o contraste
sobre o céu, terreno e cockpit. Essa fonte não é um texto nativo do Windows:
também é geometria com textura desenhada pelo OpenGL.

`TextRendering_PrintString(window,text,x,y,scale)` recebe posição em NDC e uma
escala por chamada, multiplicada pelo `textscale=1.5f` interno do renderizador.
As métricas são divididas pela dimensão da janela. Consequentemente, aumentar
somente o tamanho da janela não multiplica automaticamente o tamanho lógico da
fonte. `FlightHud` converte suas posições de texto em pixels para NDC e utiliza
a largura dos caracteres para centralizar cada legenda.

Os textos de ajuda no canto foram ajustados em testes com o usuário. O estado
final, depois de dobrar, reduzir à metade e aumentar novamente em 1,5 vez, é:

```cpp
const float headerScale=1.2f,controlsScale=1.05f,speedScale=1.125f;
```

Esses valores pertencem somente à ajuda das câmeras. As escalas dos números e
legendas dos instrumentos e do contador de FPS não foram multiplicadas junto.
O bloco de ajuda mede a largura real da string, reserva 96 pixels para o FPS e
seleciona versões compactas quando necessário. O texto começa com margem de
12 pixels; a primeira baseline e o espaçamento são calculados em pixels, em vez
de dependerem apenas de frações fixas da altura. Na câmera livre, o texto extra
de velocidade é omitido se alcançaria o topo dos instrumentos. Isso permite
aumentar a legibilidade sem provocar sobreposição nos tamanhos pequenos.

### Inspeção GPU e testes do HUD

`--hud-check` ativa exclusivamente o modo de desenvolvimento. São onze vistas:
leitura real inicial; cockpit apontando para norte; câmera livre com subida e
curva; descida com curva para oeste; voo invertido; canais inválidos; altitude de
12345 m; velocidade acima da escala e potência máxima; altitudes de 999 e
1001 m; e janela de 320×240. As vistas cobrem as três câmeras e larguras de 320
a 1920 pixels. A altura solicitada de 1080 pode ser limitada pelo ambiente da
janela; o log registra a dimensão efetivamente capturada.

O modo monta vetores sintéticos a partir dos rumos e atitudes de teste e passa
esses vetores pela função real de telemetria. As capturas aguardam 90 quadros
por vista, permitindo acomodar a resposta exponencial depois de mudanças
artificiais grandes, como passar instantaneamente de 12345 para 800 m. A
captura salva PPM com `glReadPixels`, restaura o alinhamento de leitura e testa
`glGetError`. O log registra dimensões, valores e flags. A execução normal não
substitui dados do jogador por esses fixtures.

As regressões de CPU se dividem em três suítes. `flight_telemetry` cobre unidades,
cardinais, combinação de ângulos, voo invertido, ortogonalização, entradas
inválidas e preservação dos dados originais. `hud_layout` cobre limites,
ausência de sobreposição, tamanhos compactos/ultrawide, transição entre linhas,
ângulos fracionários, saturação e continuidade do altímetro. `hud_needle_motion`
cobre passos de throttle, movimento intermediário, equivalência entre FPS,
passagem pelo norte nos dois sentidos, transição de mil metros, falha e
recuperação de canais e intervalos de tempo inválidos. Essas verificações não
dependem de uma janela, driver gráfico ou carregamento da cidade.

## Compilação, execução e diagnóstico da versão carregada

A lista `SOURCES` em `CMakeLists.txt` contém os arquivos efetivamente compilados. Acrescentar um `.cpp` em `src` não basta nesse fluxo: ele precisa estar nessa lista. O Makefile do macOS também recebeu os arquivos novos. O padrão C++ exigido no CMake é C++11. Os headers de módulos próprios são procurados em `src` e os de bibliotecas e matrizes em `include`.

No Windows, a configuração identifica o compilador e escolhe a biblioteca GLFW compatível. Com MinGW, distingue UCRT64 de outras variantes pela saída do compilador; com MSVC, usa a biblioteca correspondente. Não se deve misturar arbitrariamente uma biblioteca estática de uma ABI com um compilador de outra. A validação desta conversa foi realizada no Windows, com GCC/UCRT64, CMake e Ninja já configurados no ambiente.

Com o diretório de build já configurado, a sequência de uso é:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
Push-Location bin/Debug
.\main.exe
Pop-Location
```

Em uma configuração nova, começa-se por `cmake -S . -B build`, usando um gerador e compilador disponíveis no ambiente. Os passos de instalação e as diferenças de plataforma permanecem em `COMPILACAO.md`. A presença de suporte no build não significa que todas as plataformas foram executadas e verificadas nesta conversa.

O diretório de trabalho ao executar importa porque o código usa caminhos como `../../data/aircraft/aircraft.obj` e `../../src/ui/shaders/hud_vertex.glsl`. A execução em `bin/Debug`, `bin/Release`, `bin/Linux` ou no diretório equivalente do Makefile mantém a relação esperada com a raiz. Um executável antigo aberto pelo editor ou uma configuração diferente de Release/Debug pode continuar mostrando o coelho ou a fonte de uma versão anterior, mesmo depois de editar os arquivos corretos. Por isso os testes interativos passaram a abrir explicitamente o executável recém-compilado e a conferir a captura do framebuffer.

O carregamento ainda pode levar dezenas de segundos no computador de desenvolvimento, pois percorre o mapa completo. Isso é diferente de congelar o loop de desenho durante o voo. O título com o contador dos chunks indica progresso; erros de caminho ou de textura são reportados pela rotina de carga. O programa permite cancelar o carregamento fechando a janela.

## Estratégia de testes: matemática, arquivos e GPU

O teste de uma fórmula e a inspeção de uma imagem respondem a perguntas diferentes. A suíte CPU consegue simular centenas de orientações e eventos sem abrir uma janela. A inspeção GPU confirma que os shaders compilam, os materiais reais são encontrados e o resultado se mantém visível no ambiente gráfico usado pelo jogador. Um teste não substitui o outro.

No recorte documentado, o CTest executa nove alvos:

| Alvo | Problema que ele ajuda a detectar |
|---|---|
| `camera_controller` | Base do cockpit, continuidade da livre, independência do avião, limites e eixos da externa |
| `camera_input` | Encaminhamento de teclas, captura, estabilização do clique, relógio simulado, liberação por foco |
| `material_policy` | Classificação entre pintura sobreposta e geometria real com recorte alpha |
| `material_loading` | Independência do estado de cor/material ao ler materiais consecutivos |
| `base_terrain` | Geração reproduzível da base limitada e uso das coordenadas do mapa |
| `bezier_flight` | Avaliação da curva, comprimento aproximado, evolução temporal, junções e orientação |
| `flight_telemetry` | Unidades, sinais, cardinais, normalização de base e leituras inválidas |
| `hud_layout` | Limites, círculos sem sobreposição, troca entre linhas e ângulos entre graduações |
| `hud_needle_motion` | Resposta contínua, equivalência a 30/60/144 FPS, recuperação e passagem pelo norte |

O alvo Python depende de encontrar Python durante a configuração do CMake. No ambiente usado, ele foi encontrado e os nove alvos passaram. O teste determinístico do splitter continua disponível separadamente:

```powershell
python data/rio/tools/test_split_obj.py
python data/rio/tools/test_base_terrain.py
```

O primeiro usa exemplos pequenos para provocar subdivisão e comparar resultados reproduzíveis. O manifesto do mapa real, por sua vez, registra hashes e limites da conversão completa. Os testes pequenos não afirmam que o mapa passou a ter relevo real ou que todos os dados OSM do mundo estão completos.

Para a GPU, há três entradas distintas, executadas a partir do diretório do binário:

```powershell
.\main.exe --smoke-test ../../build/scene.ppm orbit
.\main.exe --smoke-test ../../build/cockpit.ppm cockpit
.\main.exe --smoke-test ../../build/free.ppm free
.\main.exe --smoke-test ../../build/interactive.ppm orbit --keep-open
.\main.exe --texture-check ../../build/material-check
.\main.exe --hud-check ../../build/hud-check
```

O smoke test carrega os assets reais, verifica erros OpenGL e pode salvar uma imagem PPM depois de 60 quadros. `--keep-open` conserva a janela para teste manual. O modo de texturas usa atualmente 15 vistas, incluindo as duas vistas do tráfego acrescentadas depois das 13 usadas na correção inicial dos materiais. O modo de HUD gera 11 vistas e aguarda 90 quadros por vista para os ponteiros acomodarem os saltos intencionais dos valores de teste.

As imagens PPM são leituras dos pixels produzidos pela GPU. A conversão posterior para PNG usada na conferência facilita visualizar o arquivo; não cria texturas para o jogo nem modifica a imagem renderizada. Os arquivos de teste e logs ficam em `build`, ignorado pelo Git.

As conferências incluíram, entre outros casos, a pintura sobre o asfalto em posições de câmera diferentes, parques antes com lacunas de chão, transparência nas árvores, o cockpit com painel próximo, a aeronave autônoma em posições distintas do circuito, curvas à direita e à esquerda no horizonte, voo invertido, dados inválidos, leitura acima da escala e a passagem do altímetro de 999 para 1001 metros.

## Controles consolidados no recorte

| Entrada | Comportamento |
|---|---|
| `C`, `F`, `L` | Cockpit, câmera livre e câmera externa |
| Botão direito + mouse | Olhar ou orbitar com captura do cursor |
| `Home` | Restaurar o ajuste da câmera atual |
| Rodinha na externa | Ajustar a distância de observação |
| Rodinha na livre | Ajustar velocidade da câmera |
| `WASD` na livre | Avançar, recuar e deslocar lateralmente |
| `Q/E` na livre | Descer/subir no eixo vertical do mundo |
| `Shift` na livre | Multiplicar temporariamente a velocidade por cinco |
| Setas no cockpit | Deslocar a cabeça dentro dos limites locais |
| `Page Up/Down` no cockpit | Ajustar a altura dos olhos |
| `W/S` fora da livre | Alterar throttle do jogador |
| `P/O` | Perspectiva/ortográfica fora do cockpit |
| `R` | Recarregar os shaders 3D do código base |
| `H` | Alternar instrumentos e textos auxiliares |
| `Esc` | Encerrar a aplicação |

O recarregamento `R` usa a infraestrutura existente dos shaders 3D; ele não é apresentado como um sistema geral de hot reload de todos os programas GPU. Teclas legadas de demonstração ainda existentes no callback não substituem a futura implementação dos controles de atitude.

## Git, identificação de IA e organização documental

O trabalho foi desenvolvido em etapas: preparação dos assets e integração gráfica, reorganização e câmeras, tráfego em Bézier e HUD. As branches permitiram revisar os blocos antes de integrá-los à `main`. Durante a conversa também foi discutida a exclusão de branches e a relação entre commits locais e remotos; uma branch é uma referência para um commit, e sua exclusão não é um mecanismo de arquivamento dos arquivos do projeto.

O histórico antigo recebeu identificação de auxílio de IA conforme o pedido do usuário e as regras do trabalho. Os commits novos passaram a usar o prefixo `IA:` e os prompts foram preservados em `prompts.md`. Há referências antigas reescritas em outras branches locais; para estudar a versão consolidada, deve-se partir do histórico alcançável pela `main` ou pelo commit do recorte, em vez de interpretar a saída de `git log --all` como uma única sequência linear de mudanças distintas.

Alguns marcos alcançáveis pelo recorte ajudam a localizar as alterações:

| Commit | Conteúdo |
|---|---|
| `a63dca9` | Transferência de notas geradas por IA para fora do relatório final |
| `098236c` | Reorganização dos fontes por responsabilidade |
| `42e07f2` | Estabilização dos materiais e preenchimento visual do chão |
| `bcf8ed6` | Câmeras modulares e integração dos controles |
| `efc30df` | Caminho Bézier e animação por Euler |
| `e6eb55e` | Instância de tráfego integrada aos passes da cena |
| `c56d70f` | Telemetria, layout e cálculos dos ponteiros com testes |
| `7d803ae` | Desenho e integração dos cinco instrumentos |
| `e2155d8` | Escalas finais das fontes auxiliares |

O `README.md` foi deixado vazio a pedido do usuário, para o relatório de autoria da dupla. As notas anteriores foram preservadas em `AI_README.md`; por isso esse arquivo é uma cópia histórica e pode descrever uma etapa anterior do jogo. `SPEC.md` não foi alterado pelo desenvolvimento documentado aqui. As explicações específicas continuam em `docs/rendering.md`, `docs/cameras.md`, `docs/bezier.md`, `docs/hud.md` e `data/rio/README.md`.

Os OBJ/MTL gerados têm hashes no manifesto. Para que a troca de plataforma não altere seus bytes apenas pela conversão de fim de linha, `.gitattributes` preserva LF nos chunks:

```gitattributes
# Preserve generated chunk hashes across Windows/Linux checkouts.
data/rio/chunks/*.obj text eol=lf
data/rio/chunks/*.mtl text eol=lf
data/rio/chunks/manifest.json text eol=lf
```

O aviso de Git sobre LF/CRLF observado durante a conversa não indicava uma textura quebrada ou uma falha no OBJ. Ainda assim, a conversão automática precisava ser controlada para manter reproduzíveis os hashes da saída determinística. `bin` e `build` estão ignorados: capturas, logs, compilados e os scripts locais de preparação não são incluídos automaticamente no push.

O uso de GLM neste projeto serve a vetores, produtos e armazenamento de matrizes. As matrizes de transformação, câmera e projeção utilizadas pela cena vêm do material da disciplina em `include/matrices.h`. As partes novas não usam construtores proibidos como `glm::lookAt`, `glm::perspective`, `glm::translate`, `glm::rotate` ou `glm::scale`. Trechos reaproveitados foram identificados com comentários `FONTE`; as bibliotecas mantêm seus avisos de licença.

## Estado ao fechar este documento e etapa seguinte

A implementação consolidada permite carregar a cena real, observar o jogador pelas três câmeras, explorar a cidade com a câmera livre, acompanhar um segundo avião animado e consultar os cinco instrumentos do jogador. Os ajustes de textura e de profundidade fazem parte desse resultado, tanto no A320 quanto nas superfícies do cenário. Eles são detalhados nas seções anteriores porque o resultado não depende apenas de carregar um OBJ: materiais, UVs, precisão numérica e ordem de desenho também interferem na imagem final.

Ainda faltam, nesse recorte: colisões com solo real e obstáculos, classificação de pouso/acidente, objetivo de viagem A→B, menu com escolha de avião, controle completo da atitude do jogador, animação de árvores pelo vento e luzes piscantes previstas na especificação. As posições do cenário continuam sendo de uma conversão OSM; a base adicional não fornece um modelo de elevação do terreno.

A etapa autorizada depois da publicação deste documento é o sistema de colisões. O plano é manter testes geométricos em `collisions.cpp`, como exige o enunciado, criar um índice espacial independente da câmera, usar volumes próprios da aeronave e contatos separados do trem de pouso, e classificar o impacto antes de zerar a velocidade vertical. Pintura, água e imagens de árvores exigem políticas físicas diferentes: a imagem de uma árvore pode conter pixels transparentes, e asfalto também existe fora de aeroportos.

Esses itens são planejamento neste primeiro commit do documento. A implementação de colisões e suas evidências de teste devem ser consultadas nos commits e notas técnicas acrescentados depois dele.
