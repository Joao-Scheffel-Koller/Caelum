## File used to store ai prompts

37. Claude Code (2026-11-11): "I took a printscreen of how the text appears on
    the screen and you can check it under /docs/ as image.png. Please check if
    option 1 would really solve this problem. If so. let's do it (and dont
    forget to add the last prompt to prompts.md)". Confirmar pela captura que
    o texto era desfocado por reducao do atlas de 18 px, deslocamento de meio
    texel e posicoes fracionarias. Gerados com IA:
    tools/generate_font_atlas.py (DejaVu Sans Mono rasterizada pelo FreeType
    via Pillow, com hinting, em todos os tamanhos inteiros de 8 a 32 px e
    36-64 px), data/fonts/dejavu_mono_atlas.png/.txt e o caminho 1:1 em
    src/ui/textrendering.cpp (tamanho mais proximo, pena e linha de base em
    pixels inteiros, sem meio texel, quad com 1 texel de borda para o
    contorno, uma chamada de desenho por string e retorno ao atlas antigo se
    os arquivos faltarem). O pedido sobre o z-fighting (36) foi registrado
    nesta mudanca.

36. Claude Code (2026-11-10): "Nice job so far! I merged the changes into main
    (I discarded the changes regarding the correction of the text on the
    screen because I did not like them... we can get back to it later). Now I
    am observing that there is some z-fighting between some roads and some
    lanes near the runaway and the green ground. Can you spot where this is
    happening in the code and show me? Then suggest me a solution for that
    that you find best (e.g. maybe insert a margin bewteen the lanes and the
    ground (so that they do not share the same points) (but I'm not sure
    whether this is the best soltion, so you can give suggestions). Then apply
    the solution that you suggested. The working directory is now set to anew
    branch dedicated to this fix". Medir no OBJ do mapa as sobreposicoes
    coplanares (Y=0) entre materiais opacos (asfalto x grama/agua, concreto x
    grama, pista/ciclofaixa x asfalto) e explicar por que uma margem vertical
    falha com a precisao do z-buffer (near 0,5 m, far 30 km) e alteraria a
    fisica. Gerados com IA: prioridade de profundidade por classe de material
    em MaterialPolicy.* aplicada com glPolygonOffset no passe solido de
    ModelRenderer.*, terreno base integrado ao mesmo mecanismo em
    GameScene.cpp, testes em MaterialPolicyTest.cpp e vistas
    rasantes/deslocadas em RenderInspection.cpp para comparar antes e depois.

35. Claude Code (2026-11-09): "I merged the last changes into main than
    created a new branch called physics from main. Now we are going to refine
    the flight physics. I want the parameters envolved (mass, darg-force, drag
    coefficient, lift factor, lift force, speed, etc (i.e. any parameters
    related to physics) to be realistic for an aribus a320. Please make this
    first". Substituir os parametros de brinquedo (massa 10, gravidade -1,
    lift_factor, drag_factor, empuxo 80) por valores reais do A320-200 (60 t,
    S 122,6 m2, envergadura 34,1 m, 2x120 kN, CD0 0,020, e 0,80, flaps 0..FULL
    calibrados pelas velocidades de estol publicadas) e do monomotor (classe
    Cessna 172S). Gerados com IA: FlightProfile.*, FlightModel.* reescrito
    (atmosfera ISA, CL(alfa) com estol, polar de arrasto, arrasto transonico
    de Lock, empuxo turbofan de Mattingly/helice, atrito de rolamento e freio,
    lei de fator de carga com protecao de alfa e manutencao da trajetoria),
    tests/FlightModelTest.cpp e docs/physics.md. Ajustes em Aircraft.*,
    FlightControls.* (rolagem por perfil, direcao por modelo de bicicleta,
    rotacao na decolagem), volume da fuselagem/cauda do A320 em
    AircraftContacts.cpp medido na malha (tail strike ~9 graus em vez de ~3),
    GameScene.cpp, main.cpp (flaps F6/F7, freio '.', HUD) e nota em
    docs/steering.md. Verificado com CTest, smoke test e --collision-check no
    mapa real; sem teste pilotado.

34. Claude Code (2026-11-09): "Nice job! but I see there is a maximum angle
    for the pitch and the left or right turn. I want it to be ilimited. Can
    you fix that?". Remover os limites de arfagem (~50 graus) e rolagem (~60
    graus) em FlightControls.*, permitindo looping, giro de 360 graus e voo de
    dorso. Restringir a estabilidade lateral a |rolagem| < 90 graus e atenuar
    a curva coordenada por cos(arfagem) perto da vertical. Substituir os
    testes de limite e atualizar docs/steering.md.

33. Claude Code (2026-11-09): "This is the repository of the final project of
    a class of Computer Graphics. In it we have to create a 3d game that
    utilizes some key concepts of coompuyter graphics (e.g. colision
    detection, bezier curves, etc). We decided to make a simples flight
    simulator inspired by GEOfs browser flight simulator (i.e. we want it to
    be a light game). Almost everything is done, but it has almost no flight
    physics still. It only makes the airplane go up according to its forward
    speed. My working directory is currently showing the branch "steering". In
    this branch my goal is to implement the mouse control over the plane. I
    want it to bem like the geofs (and other simulators): If the mouse goes
    down, nose up (and vice-versa). And if it goes to the right, the aircraft
    turns right (and the same for the left side). Help me implement this.
    Bascially what we have to do (please correct me if I'm mistaken) is to
    change the pitch for mouse up or down and change both yaw and roll for
    mouse going to left or right. Please mak an markdown file in /docs/ in
    brazilian portuguese explaining the changes you've made.". Implementar na
    branch steering o controle por mouse no estilo GeoFS, corrigindo o modelo
    proposto: ailerons comandam a rolagem e a curva resulta da inclinacao
    (curva coordenada turnRate*sin(rolagem)) mais uma pequena guinada direta.
    Gerados com IA: src/aircraft/FlightControls.* (manche virtual pela posicao
    absoluta do cursor com zona morta e curva de resposta; rotacoes por
    Rodrigues nos eixos do aviao; comandos de taxa, estabilidade lateral,
    Gram-Schmidt; velocidade alinhada ao nariz; no solo, direcao da bequilha e
    retorno a atitude de solo), tests/FlightControlsTest.cpp e
    docs/steering.md. Integracao em main.cpp (leitura do cursor, aplicacao
    antes do FlightModel, linha do manche no HUD), CMakeLists.txt e
    Makefile.macOS. Verificacao com CTest e smoke test com os assets reais.

32. Codex (2026-11-03): "vamos fazer a implementar o phong tb ent".
    Substituir Blinn-Phong pelo modelo de reflexao de Phong nos fragment
    shaders do cenario/aeronaves e das lentes das luzes. Usar reflect(-sun,n)
    comparado a direcao da camera, manter a condicao de superficie iluminada
    e ajustar os expoentes de 32/36 para 8/9 como aproximacao da largura
    anterior dos brilhos. Registrar a referencia da formula, atualizar a
    documentacao tecnica e conferir compilacao GLSL/capturas dos dois modelos.
    Preservar README.md e SPEC.md; manter as luzes na branch light.

31. Codex (2026-11-03): "vamos começar pelas luzes piscantes nas aeronaves,
    vamos criar um branch light a partir da main e vamos pensar sobre isso";
    "pode começar a implementar". Criar light da main atualizada em 5594f68.
    Implementar luzes de navegacao continuas, beacons vermelhos e strobes
    brancos nas duas aeronaves, com ancoras medidas nos OBJ. Separar perfis e
    padroes temporais CPU de renderer OpenGL com nucleos emissivos e halos
    procedurais. Integrar jogador e trafego Bezier com suas mesmas matrizes;
    atualizar o relogio uma vez por quadro, respeitando pausa e os dois passes
    do cockpit. Preservar profundidade e estado GL; testar periodos, FPS,
    oclusao, instancias e capturas dos assets reais. Documentar em docs/lights.md
    e implementation.md sem alterar o relatorio README.md nem SPEC.md.

30. Codex (2026-11-02): "o implementation.md está atualizado, atualize caso
    ele n esteja com as novas adições do projeto". Antes dos commits/push ja
    autorizados, revisar o guia originalmente limitado ao HUD e2917d8 e
    atualiza-lo para a branch menu baseada em 3ab3bea. Preservar o detalhamento
    de texturas, cameras, Rio, Bezier e HUD; corrigir bootstrap/loop/estado
    fisico/controles/testes/limites obsoletos. Acrescentar explicacoes e trechos
    reais de colisoes CCD/BVH/perfis, missao A-B e marcador M, menu/pausa,
    progresso e cancelamento assincronos, troca de aeronave e monomotor CC BY.
    Atualizar indice, arquitetura e referencias a data/ASSETS e proveniencia.
    Distinguir recursos implementados de controles de atitude e voo manual
    completo ainda pendentes. Nao escrever README final nem alterar SPEC.md.

29. Codex (2026-11-02): "deixa como esta no modelo da internet, documente como
    os modelos foram achados e produzidos dentro da pasta data/, incluindo rio
    e as duas aeronaves". Preservar pintura vermelha/azul original do monomotor.
    Produzir data/ASSETS.md e guias de proveniencia dos tres assets, com fontes,
    autoria, licencas, aquisicao, conversao, correcoes, arquivos ativos e de
    origem, hashes e comandos reproduziveis. Distinguir dados OSM/estilo/texturas,
    adaptacoes do A320, monomotor estilizado e suas alteracoes de cabine/vidro.
    Nao inventar historico nao registrado nem chamar o monomotor de Cessna exato.
    Corrigir link do repositorio do estilo OSM2World. Preservar README.md/SPEC.md.

28. Codex (2026-11-02): "vamos fazer o menu e uma tela de carregamento em uma
    branch menu vinda da main"; "Quero acrescentar outra aeronave";
    "Monomotor, como um Cessna"; "eu n baixei o modelo ainda, se vc conseguir
    pode baixar, se não me de as instruções"; "pode continuar o trabalho anterior".
    Criar menu da main atualizada (3ab3bea). Implementar menu inicial com
    selecao real entre A320 e monomotor, controles, iniciar e sair; pausa com
    Escape, retomar e voltar ao menu. Separar modelo CPU/layout de renderer
    OpenGL e entradas; preservar capturas e controles existentes. Mostrar menu
    antes de ler assets, carregamento com progresso por trabalho concluido,
    preparacao CPU em worker e eventos/desenho/GL na thread principal.
    Permitir cancelar, tratar falhas sem entrar em cena incompleta e reutilizar
    o mapa ao reiniciar ou trocar de aviao. Obter monomotor gratuito com licenca
    de redistribuicao, registrar fonte/licenca/alteracoes e calibrar escala,
    orientacao, contato e camera. Documentar em docs/menu.md; testes CPU e
    capturas reais de menu/carregamento/jogo. Nao alterar README.md e SPEC.md.

27. Codex (2026-11-02): "tem como colocara um ponto de exclamação gigante
    encima do aeroporto para o jogador saber onde ir? deixe ele oculto com
    uma tecla para ser desocultado". Implementar MissionMarker modular,
    exclamacao procedural amarela com contorno acima do Galeao, ancorada no
    mundo e voltada para a camera, legivel a distancia. Comecar oculto;
    M alterna visibilidade por pressionamento, independente de H. Desenhar
    apos cenario/cockpit usando projecao de mundo e coordenadas relativas a
    camera; preservar estado OpenGL. Atualizar build, ajuda e documentacao
    tecnica; conferir capturas proxima/distante, testes e registro do prompt.
    Preservar README.md e SPEC.md.

26. Codex (2026-11-02): "vamos criar a branch mission pela main e trabalhar na
    parte da missão de ir para o outro aeroporto"; "termine oq restou".
    Criar mission da main atualizada (2da34fd), preservar README.md/SPEC.md.
    Implementar geometria de pistas Santos Dumont/Galeao a partir do mesmo
    extrato OSM, metadados auditaveis em data/rio/airports.json e modulo CPU
    FlightMission. Confirmar decolagem real da origem, aproximacao, toque
    fisico dentro da pista do destino, velocidade/alinhamento/apoio e 2 s
    estaveis; impedir conclusao por proximidade, camera livre, taxiamento
    desde fora da pista ou ausencia de partida. Ligar apos resposta de colisao,
    reiniciar com Enter e mostrar orientacoes responsivas via MissionOverlay.
    Preservar indicacao correta de alinhar/reduzir/manter em janelas pequenas.
    Validar altura pelos fundos das rodas, aceitando primeiro toque inclinado
    sem aceitar telhados. Testes CPU, replay contra geometria real e inspecao
    OpenGL explicita de estados de interface. Documentar docs/mission.md e
    limitacao atual dos controles de atitude, sem reescrever a fisica do colega.

25. Codex (2026-11-02): "Resolva oq esta pendente". Concluir a implementacao
    de colisoes autorizada no pedido 24, depois da publicacao de implementation.md
    em d5f0d57. Gerados: src/collision/collisions.*, CollisionWorld.*,
    AircraftContacts.*, CollisionDebug.*, shaders e tres suites CPU.
    Extrair dados fisicos na mesma leitura OBJ do renderer, indexar BVH,
    excluir decals, gerar proxies de arvores e diferenciar chao/agua/solidos.
    Integrar resposta apos predicao da fisica, preservar velocidade de impacto,
    permitir apoio/pouso/acidente/reinicio e inspecao com B/Enter/--collision-check.
    Corrigir regressao de rodas em obstaculos baixos, movimento apos toque e
    decolagem com contato inicial. Documentar docs/collisions.md e verificar
    testes/GPU com mapa real. Preservar README.md e SPEC.md.

24. Codex (2026-11-02): "pode ser, começe a implementar ent. mas antes crie um
    arquivo chamado implementation.md explicando tudo q implementamos nesse
    projeto nesta conversa, eu quero um resumo ultra detalhado, começe com uma
    visão em alto nivel do q foi feito e depois va para o codigo mostrando ele
    e como funciona. ent faça implementation.md -> commit e push -> e
    implementação das colisões"; "lembre-se q implementamos tb a parte de
    texturas, eu gostaria q o resumo sobre isso tb"; "sobre o avião e o chão".
    Produzir primeiro um guia tecnico detalhado do snapshot e2917d8, cobrindo
    arquitetura, assets, texturas/UVs/correcoes do A320 e chao, mapa/split,
    renderizacao, cameras, animacao Bezier, HUD, testes, build e historico.
    Incluir trechos reais do codigo e limitacoes, sem alterar README/SPEC.
    Publicar implementation.md e este prompt na branch colisions antes de
    iniciar a implementacao modular das colisoes ja discutida.

23. Codex (2026-11-02): "1.5 vezes e roda". Aumentar as escalas atuais dos
    textos de ajuda em 1.5 vezes: 0.8->1.2, 0.7->1.05 e 0.75->1.125.
    Atualizar docs/hud.md, recompilar e abrir o executavel atualizado de
    bin/Debug. Preservar SPEC.md e README.md.

22. Codex (2026-11-02): "acho que eu tinha rodado uma versão antiga, diminui na
    metade e roda de novo". Reduzir pela metade as escalas atuais dos textos
    de ajuda: 1.6->0.8, 1.4->0.7 e 1.5->0.75. Manter ajuste responsivo,
    recompilar e abrir o binario atualizado de bin/Debug para teste. Atualizar
    docs/hud.md; preservar SPEC.md e README.md.

21. Codex (2026-11-02): "da pra aumentar mais um pouco a fonte, dobrar acho q
    ficaria bom". Dobrar as escalas dos textos de ajuda no canto em main.cpp:
    cabecalho 0.8->1.6, controles 0.7->1.4 e velocidade da camera 0.75->1.5.
    Ajustar margens e espacamento em pixels, medir textos para selecionar
    versoes compactas sem colidir com FPS e evitar sobrepor os instrumentos
    em janelas pequenas. Preservar tamanho das fontes dos instrumentos/FPS,
    SPEC.md e README.md. Atualizar docs/hud.md e conferir compilacao/capturas.

20. Codex (2026-11-02): "n teria como fazer as setas serem mais dinamicas? usar
    o angulo de 360 graus e fazer uma escala para que os ponteiros fiquem sendo
    movidos junto do negocio de forma mais real, n só em intervalos fixos".
    Suavizar os ponteiros com resposta exponencial baseada em dt sem alterar
    a fisica ou as leituras numericas. Modulo CPU HudNeedleMotion.* com testes
    de FPS, menor arco da bussola, altitude antes do modulo e validade dos dados.
    Usar 300 graus para velocidade/potencia, distinguindo zero de maximo, e
    manter 360 graus em bussola/altimetro. Unificar marcas/ponteiros pela mesma
    funcao de escala e testar movimento entre valores fracionarios. Integrar
    ao FlightHud/main/build e documentar em docs/hud.md. Preservar SPEC/README.

19. Codex (2026-11-02): "vamos no hud ent, como poderiamos fazer?";
    "pode implementar todos da hud"; "pode terminar oq falta".
    Criar branch hud a partir da main atualizada.
    Implementar altimetro, velocimetro, potencia/throttle, bussola e horizonte
    artificial com ponteiros/indicadores dinamicos, usando dados reais do jogador.
    Separar telemetria pura, layout responsivo e desenho OpenGL 2D, usando fonte
    existente e shaders proprios sem construtores de matrizes de bibliotecas.
    Gerados com IA: FlightTelemetry.*, FlightHud.*, HudLayout.*, HudInspection.*,
    shaders em src/ui/shaders/, testes de telemetria/layout e docs/hud.md.
    Integrar ao main/CMake/Makefile, preservar estado OpenGL e recursos, testar
    as tres cameras, resolucoes e atitudes por --hud-check com fixtures somente
    no modo de desenvolvimento. Verificar continuidade do altimetro em 1000 m
    e legibilidade na janela 320x240. Manter SPEC.md e README.md intactos.

18. Codex (2026-11-02): "pode continuar a resolver as curvas de bezier".
    Retomar e concluir a validacao da implementacao na branch bezier. Revisar
    matematica, reutilizacao da malha e passes do cockpit; explicitar a operacao
    de Euler em BezierFlight.cpp. Acrescentar regressoes de continuidade da pose
    nas quatro juncoes e de evolucao ao longo de varias voltas. Manter SPEC.md
    e README.md intactos; compilar, executar os testes e abrir a versao normal.

17. Codex (2026-11-01): "vamos para a curva de bezier, crie uma branch chamada
    bezier e começar a pensar na implementação"; "acho q é bom usar um desses
    metodos do slide, integração numerica pelo menos, ta nos planos ja?";
    "pode implementar ent". Usar Aula_24_Animacao.pdf do professor como referencia
    para Euler explicito com dt real. Implementar quatro cubicas fechadas,
    consulta por comprimento aproximado e seguidor autonomo com orientacao pela
    tangente/inclinacao moderada. Reutilizar a malha GPU do A320 como segunda
    instancia, com NPC no passe WORLD e atualizacao unica por quadro.
    Gerados com IA: src/animation/BezierPath.*, src/aircraft/BezierFlight.*,
    tests/BezierFlightTest.cpp e docs/bezier.md. Integracao em GameScene,
    main.cpp, RenderInspection, CMake, Makefile.macOS e docs/rendering.md.
    Branch criada a partir da main apos merge da camera; preservar SPEC/README.

16. Codex (2026-11-01): "justo, faça um AI_README.md para guardar o seu readme
    e apague oq esta no readme atual"; "perfeito, pode dar commit e push na
    branch". Preservar as notas geradas por IA e creditos em AI_README.md,
    deixando README.md vazio para o relatorio de autoria dos alunos. Revisar e
    separar as alteracoes pendentes em commits com prefixo IA:, incluindo
    organizacao dos fontes, renderizacao/terreno e cameras; publicar em camera.

15. Codex (2026-11-01): "da pra aumentar mais um pouco o delay: ta ocorrendo
    ainda". Aumentar CaptureSettleSeconds de 50 para 100 ms em
    src/camera/CameraInput.cpp, mantendo a sensibilidade e o minimo de dois
    lotes de eventos. Atualizar tempos do teste existente e docs/cameras.md,
    recompilar e reabrir o jogo para teste do usuario.

14. Codex (2026-11-01): "agr eu entendi melhor o problema, quando eu clico
    parado e depois movo funciona normal, mas se eue clico mexendo o mouse a
    sense é mt alta e ele move mt a camera mt rapido, dai da a impressão de um
    salto". Investigar o clique com mouse em movimento. Bug upstream GLFW #2523
    tem o mesmo comportamento, inclusive em 3.3.8; tratar offsets atrasados por
    uma transicao inicial de 50 ms e pelo menos dois lotes de eventos, medidos
    com glfwGetTime desde o clique. Reancorar durante a transicao sem alterar
    sensibilidade ou limitar deltas durante o arrasto. Alteracoes em
    src/camera/CameraInput.*, regressao com clock simulado e offsets em ciclos
    separados em tests/CameraInputTest.cpp, documentacao em docs/cameras.md.

13. Codex (2026-11-01): "quando eu clico o botão direito ele ta colcoando a
    camera em um lugar meio aleatorio, tirando isso ta perfeito". Corrigir a
    transicao de captura em src/camera/CameraInput.*: ignorar callbacks de
    recentralizacao sincronicos e enfileirados ate o fim do ciclo de eventos,
    estabelecer a referencia em Update e cancelar a transicao ao soltar/perder
    foco/trocar modo. Manter sensibilidade e matematica das cameras. Regressao
    em tests/CameraInputTest.cpp para as tres cameras com/sem raw motion e
    cancelamento da captura pendente; documentar em docs/cameras.md.

12. Codex (2026-11-01): "e tem mts arquivos soltos na pasta src, separe-os mais
    em pastas por favor". Reorganizar src em aircraft, camera, render/shaders,
    platform, ui, input, course e third_party, mantendo apenas main.cpp na raiz.
    Atualizar includes, CMake, Makefiles, caminhos de shaders e documentacao tecnica.

11. Codex (2026-11-01): "quando eu estou me movendo da problema nas texturas,
    como na foto, alem disso tem lugares sem textura alguma, tem como arrumar?";
    "resolva oq esta pendente, duplique a sense mais uma vez. E tem como deixar
    o mouse preso dentro da janela enquanto eu estiver com o botão direito
    apertado para mover a camera?"; "tem bem mais texturas erradas, eu n sei se
    tu consegue captura-las sozinho ou eu devo te ajudar. se der pra tu corrigir
    sozinho faça"; "lembre-se de fazer nada ilegal de acordo com o trabalho.md".
    Auxilio de IA: passes de opacos/pintura/transparencia, politica de materiais,
    filtragem anisotropica opcional, UVs locais e posicoes relativas a camera
    em src/render e shaders. Corrigir estado has_kd entre materiais em
    include/tiny_obj_loader.h; comentario FONTE identifica a alteracao local.
    Gerar base limitada ao extrato OSM com tools/build_base_terrain.py em data/rio,
    reutilizando a textura de grama do mapa; metadados e teste de determinismo.
    Sensibilidade de 0,00030 para 0,00060 rad/unidade, confinamento nativo do
    cursor em src/camera/MouseCapture.* e integracao em CameraInput.*.
    Testes em tests/, inspecao de 13 vistas com RenderInspection.* e notas em
    docs/rendering.md, docs/cameras.md e data/rio/README.md. Manter README.md
    vazio, AI_README.md como copia anterior e SPEC.md sem alteracoes.

10. Codex (2026-11-01): "da pra duplicar a sense ainda, rode depois pra eu testar".
    Duplicar a sensibilidade de 0,00015 para 0,00030 rad/unidade em
    CameraController.cpp, atualizar a documentacao tecnica, recompilar e abrir.

9. Codex (2026-11-01): "da pra triplicar a sense agr" e conferir conformidade
   com trabalho.md. Sensibilidade de 0,00005 para 0,00015 em CameraController.cpp.
   Corrigir usos proibidos de funcoes de transformacao/camera do GLM em
   GameScene.cpp e testes, reutilizando matrizes do codigo base da aula.
   Auditoria identificou ainda prefixo IA: ausente nos commits antigos e trechos
   de README gerados por IA, contrariando o enunciado. Historico/README nao foram
   reescritos nesta correcao; relatorio final precisa ser de autoria dos alunos.

8. Codex (2026-11-01): limitar em 60 FPS, iniciar em 720p, reduzir sensibilidade
   para 99% abaixo da original e investigar demora/eixos na camera externa (L).
   Auxilio de IA: FrameLimiter.hpp/.cpp, integracao da janela/pacing/eventos em main.cpp,
   captura inicial e raw motion em CameraInput.*, sentido vertical da orbita e
   sensibilidade 0,00005 em CameraController.cpp, testes dos eixos com as matrizes
   reais do jogo, README e docs/cameras.md. Nao foi reproduzida troca matematica
   entre eixos; corrigido descarte do primeiro movimento real e entrada antes do desenho.

7. Codex (2026-11-01): "reduz pra 95% da sensibilidade original e rode pra mim".
   Pelo contexto das reducoes anteriores, aplicar reducao de 95% sobre o valor
   original: 0,00025 rad/pixel em CameraController.cpp; documentar e abrir o jogo.

6. Codex (2026-11-01): "diminua bem mais a sensibilidade". Reduzir o valor
   compartilhado em CameraController.cpp de 0,0015 para 0,0005 rad/pixel,
   um terco do ajuste anterior; atualizar documentacao e recompilar.

5. Codex (2026-11-01): "a sensibilidade de mexer na camera no botão direito
   esta mt grande, diminua". Reduzir a sensibilidade compartilhada das cameras
   de 0,005 para 0,0015 rad/pixel (70%) em CameraController.cpp e documentar.
   Avaliar carregamento dos chunks sob demanda sem alterar esse carregamento.

4. Codex (2026-11-01): "implemente as duas cameras ent, tente deixar bem modular
   como sempre e explique cuidadosamente depois oq foi feito". Implementar
   cockpit e camera livre na branch camera, manter orbita externa e separar
   controles da camera/fisica. Gerados com IA: CameraController.*, CameraInput.*,
   testes correspondentes e docs/cameras.md. Integracao em GameScene.*, main.cpp,
   CMakeLists.txt e README.md. Ajustes de legibilidade em textrendering.cpp e
   verificacao de janela minimizada. Posicao dos olhos calibrada pela geometria
   do OBJ; verificacao com CTest e capturas do programa com os assets reais.

3. Codex (2026-10-28): "pode implementar, depois separe cada parte commits e
   de push na branch". Integrar os assets do Rio e do A320 ao OpenGL, carregar
   materiais e texturas do MTL, e iniciar sobre a pista do Santos Dumont.
   Implementacao com IA: ModelRenderer.*, GameScene.*, shader_fragment.glsl,
   integracao em main.cpp/CMakeLists.txt e documentacao correspondente.
   Ajuste adicional em FlightModel.cpp: cancelar velocidade vertical negativa
   no contato com o solo. Verificacao da cena com --smoke-test e captura PPM.

2. Codex (2026-10-28): dividir o OBJ do Rio em partes deterministicas por uma
   grade espacial de 2 x 2 km, subdividindo as regioes acima de 50 MB e
   preservando coordenadas, faces, materiais e texturas. Pedido: "faça isso ent,
   perfeito". Gerados com IA: data/rio/tools/split_obj.py,
   data/rio/tools/test_split_obj.py e a
   documentacao data/rio/README.md. Os OBJ menores sao derivados automaticamente
   do mapa OSM2World; identificar este auxilio no commit correspondente.

1. formate o @file:trabalho.md  para ficar bonito em markdown
