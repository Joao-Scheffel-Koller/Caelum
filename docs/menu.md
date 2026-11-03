# Menu e carregamento

Documentação técnica escrita com auxílio de IA; não é o relatório final da dupla.
Branch `menu`, criada da `main` em `3ab3bea`, após o merge da missão.

## Fluxo

A janela abre no menu antes de ler os modelos e o cenário. O jogador escolhe
a aeronave, consulta os controles ou inicia a missão Santos Dumont → Galeão.
O catálogo de `src/aircraft/AircraftCatalog.*` associa cada opção a um modelo
real e à posição da câmera do piloto. Opções cujo arquivo não está disponível
não podem iniciar um voo.

Durante o voo, `Escape` abre a pausa e libera o mouse. A física e a animação do
tráfego param enquanto o menu está aberto. Retomar restaura o voo; voltar ao
menu inicial permite começar outra tentativa. O tempo gasto no menu não é
aplicado à física ao retomar.

O estado dos menus e as ações ficam em `src/ui/FrontendModel.*`, sem OpenGL ou
GLFW. `FrontendScreen.*` desenha os painéis, botões e carregamento com geometria
e shaders próprios, reutilizando a fonte do código base. Os callbacks de
`main.cpp` encaminham mouse/teclado ao menu ou ao jogo conforme o estado ativo.

## Carregamento

`LoadCallbacks`, em `src/platform/LoadProgress.hpp`, separa dois sinais:
`progress`, com etapa e unidades de trabalho concluídas/totais, e `pump`, que
permite atender eventos e desenhar a tela enquanto a preparação CPU acontece.
A barra representa trabalho concluído; não é uma previsão de segundos restantes.

A leitura e preparação de OBJ e a construção da geometria de colisão usam um
worker CPU. O contexto OpenGL, a criação de texturas/buffers e o desenho da
interface permanecem na thread principal. O carregamento não consulta a cena
parcial como se ela já estivesse pronta para jogar.

Cancelar ou fechar a janela aguarda a tarefa CPU em andamento terminar com
segurança antes de liberar seus dados. Falhas voltam ao menu, com detalhes no
terminal. Depois que o Rio foi carregado, iniciar outra missão ou trocar a
aeronave reutiliza o cenário e suas colisões; só o modelo necessário é trocado.

Os modos de inspeção existentes podem iniciar o carregamento diretamente para
continuar verificando os assets reais sem interação com o menu.

## Aeronaves e controles

O catálogo contém o A320 de TonyWony e um monomotor estilizado de MagicGames.
Fontes, licenças e preparação estão em [data/ASSETS.md](../data/ASSETS.md).
O monomotor mantém as cores vermelha/azul do modelo original. Seu perfil de
contato usa duas rodas principais e uma roda de cauda, e o enquadramento
externo começa mais próximo para acomodar a malha menor. O interior adicionado
é simples; a hélice permanece estática. A física de voo continua compartilhada,
sem pretensão de calibrar as características reais de cada aeronave.

Use o mouse para clicar ou setas/`Tab` para mudar o foco e `Enter` para ativar.
`Escape` volta nas páginas, retoma o voo na pausa e cancela o carregamento.
No menu inicial, `Escape` sai do programa. Escolher e iniciar outra aeronave
começa uma tentativa nova no Santos Dumont; apenas voltar/continuar conserva
o voo atual.

## Verificação

Na raiz:

```powershell
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

São 16 suítes, incluindo as duas novas: `frontend_model` (372 verificações de
seleção, teclado/mouse, indisponibilidade, pausa, retorno, erro e áreas clicáveis)
e `responsive_loading` (12 verificações de threads, exceções e drenagem após
cancelamento). Os testes de contatos e câmeras também cobrem o perfil pequeno.

Inspeção visual de páginas, sem carregar o mapa, a partir de `bin/Debug`:

```powershell
.\main.exe --menu-check ../../build/menu-check
```

Esse modo gera 11 capturas de interface, incluindo as aeronaves, controles,
pausa e carregamento em resoluções diferentes. Os percentuais dessas telas são
fixtures explícitas para conferir layout; não representam conversão de mapa.

Capturas de progresso de uma carga **real** e quadro do monomotor:

```powershell
.\main.exe --smoke-test ../../build/light-flight.ppm orbit --aircraft=light --capture-loading=../../build/light-loading
```

O prefixo de carregamento recebe o número da tentativa e três estágios
(`0`, `1`, `2`, para início, metade e término). Os números vêm do trabalho real
concluído. A inspeção desta implementação aprovou as 11 telas sem erros OpenGL
e registrou uma carga completa de 126 partes/11.213.227 triângulos físicos,
com o monomotor apoiado por três rodas e a partida da missão armada no Santos
Dumont. As imagens de inspeção ficam em `build/`, fora do Git.

Os controles de atitude do avião continuam pendentes conforme `mission.md`;
o menu não introduz direção por mouse no modelo físico.
