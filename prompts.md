## File used to store ai prompts

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
