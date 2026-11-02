## File used to store ai prompts

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

1. formate o @file:trabalho.md  para ficar bonito em markdown

2. Codex (2026-10-28): dividir o OBJ do Rio em partes deterministicas por uma
   grade espacial de 2 x 2 km, subdividindo as regioes acima de 50 MB e
   preservando coordenadas, faces, materiais e texturas. Pedido: "faça isso ent,
   perfeito". Gerados com IA: data/rio/tools/split_obj.py,
   data/rio/tools/test_split_obj.py e a
   documentacao data/rio/README.md. Os OBJ menores sao derivados automaticamente
   do mapa OSM2World; identificar este auxilio no commit correspondente.
