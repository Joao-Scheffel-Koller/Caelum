# Câmeras do Caelum

Implementação com auxílio de IA (Codex), registrada em `prompts.md`.

## Estrutura

- `src/camera/CameraController.hpp/.cpp`: matemática, estado e limites das três câmeras.
  Usa somente GLM e biblioteca padrão, sem GLFW, OpenGL ou física do avião.
- `src/camera/CameraInput.hpp/.cpp`: adapta eventos e teclas sustentadas do GLFW para
  comandos do controlador. Também captura/libera o cursor e trata perda de foco.
- `src/camera/MouseCapture.hpp/.cpp`: confina o cursor à área interna da janela
  no Windows, inclusive ao mover ou redimensionar a janela durante um arrasto.
- `src/platform/FrameLimiter.hpp`: limita os inícios dos quadros a 60 por segundo com
  relógio monotônico, sem depender da frequência do monitor e sem acumular atrasos.
  No Windows usa temporizador de alta precisão para evitar a granularidade do Sleep.
- `GameScene::Cameras()`: fornece a transformação exata usada para desenhar o
  avião, o alvo da câmera externa e a posição dos olhos no modelo.
- `main.cpp`: atualiza a física, obtém a câmera, monta view/projection e desenha.
  Os callbacks apenas encaminham entrada; a física não recebe movimentos da
  câmera livre ou da cabeça.

`CameraFrame` descreve o avião naquele quadro. `CameraMovement` descreve a
intenção de movimento. `CameraPose` devolve posição, direção, vetor para cima,
campo de visão e planos de corte, sem executar desenho.

## Cockpit: ponto de vista do piloto

O OBJ usa +X para o nariz, +Y para cima e +Z para o lado direito. O ponto dos
olhos é `(7.90, 0.95, -0.65)`, nas coordenadas originais do modelo: acima e à
frente do encosto do assento esquerdo, atrás do painel e abaixo do teto.
Essa calibração fica em `GameScene::Cameras()`.

A posição no mundo é `M * vec4(olhos + deslocamento, 1)`, com a mesma matriz M
que posiciona o avião. A direção e o vetor para cima usam `w=0`, portanto recebem
a rotação sem receber translação. Isso faz a câmera acompanhar deslocamento,
inclinação e rolagem do avião quando a física fornecer esses vetores.

O mouse gira a cabeça em relação à aeronave. O olhar fica limitado a 120 graus
para cada lado, 70 para baixo e 80 para cima. As setas deslocam a cabeça nos
eixos locais do avião; `Page Up/Down` ajusta a altura. Os limites de deslocamento
são 20 cm para frente/trás, 18 cm para os lados e 12 cm para cima/baixo. É uma
região limitada para inspeção do cockpit, sem detecção de colisão da cabeça.
`Home` restaura o ponto inicial e o olhar para a frente.

O cockpit usa perspectiva de 60 graus. Para enxergar o painel próximo sem
prejudicar a profundidade do mapa, há dois passes com a mesma view/FOV:

1. Mundo: near 0,5 m e far 30 km, sem desenhar o avião.
2. Limpeza somente do depth; avião/interior: near 5 cm e far 100 m.

As camadas `WORLD` e `AIRCRAFT` evitam desenhar o cenário duas vezes. Esse
tratamento de primeiro plano vale somente no cockpit; oclusão normal em passe
único é mantida nas outras câmeras. Os contatos do avião com o cenário são
resolvidos antes da câmera por `AircraftContacts`, usando volumes aproximados
(ver `docs/collisions.md`). A câmera livre e a cabeça não têm colisão própria.

## Livre: observador independente

Ao entrar com `F`, são copiados posição, direção e orientação da câmera atual,
inclusive a rolagem. Não há retorno à origem do mapa. Depois dessa cópia, mover
o avião não move a câmera. Pressionar `F` novamente não reposiciona a visão.

`W/S` avançam/recuam na direção do olhar, `A/D` movem lateralmente e `Q/E`
descem/sobem no eixo vertical do mundo. O movimento combinado é normalizado
para a diagonal não ser mais rápida. A distância percorrida é velocidade vezes
`dt`, portanto independe do FPS.

A velocidade inicial é 50 m/s, ajustável pela rodinha entre 1 e 2000 m/s.
`Shift` multiplica por cinco enquanto estiver pressionado. `Home` retorna à
visão de referência atrás da posição atual do avião e restaura 50 m/s.
A câmera livre atravessa geometria, como um modo espectador.

Enquanto esse modo está ativo, `W/S` são consumidos antes do tratamento da
potência. O avião continua sendo simulado com a potência definida anteriormente.

## Entrada e câmera externa

Nas três câmeras, o botão direito captura o mouse para permitir olhar sem ficar
preso à borda da janela. O início de cada captura tem uma estabilização de
100 ms e pelo menos dois lotes de eventos. Durante esse intervalo, callbacks de
reposicionamento são ignorados e `Update` renova a referência do cursor. Isso
contorna o bug [GLFW #2523](https://github.com/glfw/glfw/issues/2523): apertar o
botão com o mouse já em movimento pode gerar offsets incorretos em quadros
posteriores ao clique, inclusive na versão 3.3.8 usada pelo projeto.
O prazo usa `glfwGetTime()` a partir do clique, sem depender do FPS ou do tempo
anterior ao clique. Movimento durante a transição não prolonga esse prazo.
Depois de estabilizar, cada deslocamento é aplicado com a sensibilidade normal,
sem limitar movimentos rápidos, descartar o primeiro evento ou suavizar o olhar.
Quando disponível,
GLFW raw mouse motion evita a aceleração do cursor pelo sistema operacional.
Soltar o botão, trocar de modo ou perder foco libera o cursor.
Perder foco também impede movimentos por teclas sustentadas na câmera.
No Windows, `ClipCursor` complementa a captura do GLFW: enquanto o botão direito
está pressionado, o cursor não pode escapar para outra tela. A restrição é
removida junto da captura, inclusive ao fechar a aplicação.

`L` retorna à câmera externa. Ela acompanha o rumo do avião mantendo o horizonte
nivelado, com órbita e distância próprias. A rodinha controla a distância e
`Home` restaura 70 m atrás do avião. Os limites de inclinação evitam alinhamento
exato entre direção de visão e vetor vertical, que invalidaria a matriz view.
Mover o mouse para cima eleva a câmera externa. A órbita continua limitada antes
do polo superior e perto do chão; `Home` restaura a posição inicial se atingir um limite.

A janela abre em 1280 x 720, com limite de 60 FPS por software. Os eventos são
processados antes de calcular a câmera de cada quadro, evitando desenhar com a
entrada do quadro anterior. Não há interpolação ou suavização que faça a câmera
continuar girando depois de parar o mouse.

## Verificação e roteiro para ajuste

Os testes `CameraControllerTest.cpp` e `CameraInputTest.cpp` rodam pelo CTest:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Verificam transformação do cockpit com avião rotacionado, continuidade ao entrar
na livre, independência do avião, velocidade com diferentes passos de tempo,
limites, matrizes finitas, separação dos controles, captura e perda de foco.
Também simulam eventos de recentralização em ciclos distintos após o clique
direito em movimento, com e sem raw motion nas três câmeras, seguidos de um
deslocamento conhecido. Um relógio simulado verifica o intervalo inicial e o
caso de um quadro lento; soltura/perda de foco cancelam a transição.
O teste de entrada usa funções GLFW simuladas; os testes de eixos usam as
matrizes view/projection reais do jogo na posição do Santos Dumont.
As capturas com `--smoke-test` verificam o desenho com a GPU e os assets reais.
Para capturar, medir o FPS e continuar testando na mesma janela:

```powershell
.\main.exe --smoke-test ../../build/camera-check.ppm orbit --keep-open
```

Para experimentar no jogo:

1. `C`: conferir painel e para-brisa; olhar com botão direito e mover a cabeça.
2. `Home`: verificar retorno ao assento/olhar inicial.
3. `F`: verificar que posição e direção começam de onde você estava olhando.
4. `WASD`, `Q/E`, `Shift` e rodinha: explorar a cidade e variar a velocidade.
5. `L`, depois `C`: confirmar que os ajustes de órbita/cabeça foram preservados.
6. Trocar de janela durante um arrasto e voltar: o cursor deve estar liberado.

Sensibilidade, velocidades e limites ficam em `CameraController.cpp`. A
sensibilidade atual é 0,00060 radianos por unidade do mouse (contagem direta
quando suportada, pixel no fallback), duas vezes o ajuste anterior e 88% menor
que a constante da versão inicial.
Esse valor é compartilhado pelas três câmeras. A calibração do assento fica em
`GameScene.cpp`. Esses são os pontos principais
para ajustar depois da avaliação visual e de conforto dos controles.
