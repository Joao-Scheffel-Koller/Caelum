# Missão Santos Dumont → Galeão

Documentação técnica escrita com auxílio de IA. Não é o relatório final da dupla.
Implementação iniciada na branch `mission`, a partir da `main` em `2da34fd`.

## Comportamento

O jogador começa na pista 20L/02R do Santos Dumont, como antes. A missão lê a
posição, a velocidade, a orientação e o apoio das rodas **depois** da resposta de
colisão. Movimentar a câmera livre ou observar o avião autônomo não avança o
objetivo. Nenhum desses dados é substituído pela interface de instrumentos.

1. **Partida (`Departure`):** as três rodas devem estar dentro do mesmo retângulo
   de pista do Santos Dumont, com pelo menos duas apoiadas. A região tem margem
   interna de 2 m. A altura dos fundos das esferas das rodas é comparada com a
   pista exportada; o pivot do avião não precisa estar a menos de 1 m do chão.
2. **Em rota (`EnRoute`):** a transição de apoio para voo deve ocorrer a até 50 m
   da pista de origem. A decolagem só é confirmada após 0,5 s de voo e altura de
   pelo menos 5 m sobre a referência da pista. O primeiro quadro aéreo não recebe
   o tempo do intervalo anterior, que pode ter sido passado no chão.
3. **Aproximação (`Approach`):** começa a até 2 km de uma pista do Galeão. Uma
   margem de saída de 2,5 km evita alternar de estado ao oscilar nessa fronteira.
4. **Pouso (`Landing`):** é necessário observar um toque `Flying → Grounded`
   dentro de uma pista do Galeão. Um primeiro toque com uma roda pode registrar
   a chegada, aguardando o restante do trem. As três rodas precisam continuar
   dentro do mesmo retângulo; o apoio físico vem do sistema de colisões.
5. **Concluída (`Completed`):** exige pelo menos duas rodas apoiadas, velocidade
   horizontal de até 5 m/s (18 km/h), alinhamento de até 25° com o eixo da pista
   em qualquer sentido e 2 s consecutivos nessas condições. O primeiro quadro
   qualificado não recebe o tempo do trecho aéreo ou do taxiamento anterior.
6. **Falha (`Failed`):** qualquer acidente reconhecido pelas colisões antes da
   conclusão encerra a tentativa, inclusive no quadro que completaria os 2 s.

Chegar perto, pousar no chão fora da pista e entrar taxiando ou começar no Galeão
não conclui a missão. Acelerar ou desalinhar interrompe a contagem. Sair da
pista, mudar para outra ou voltar a voar elimina o registro de chegada; um novo
toque dentro de uma pista válida será necessário. Os estados finais permanecem
até reiniciar; a conclusão não congela a simulação.

`Enter` restaura o jogador, as colisões, a câmera e a missão no Santos Dumont.
`H` oculta/mostra a interface durante a tentativa. Conclusão e falha continuam
visíveis mesmo com o HUD oculto. As teclas de câmera e o `B` de volumes de colisão
mantêm suas funções; a missão não ativa os volumes por padrão.

## Marcador opcional do destino

**M** mostra ou esconde um ponto de exclamação amarelo grande sobre o **Galeão**.
Ele começa **oculto**. A tecla alterna uma vez por pressionamento, sem repetir
enquanto estiver segurada. Sua visibilidade é independente de `H`; reiniciar
com `Enter` preserva a preferência de exibição.

O marcador fica 450 m acima da média dos centros das duas pistas. Permanece
fixo sobre o aeroporto, sem seguir o alvo de aproximação nem apontar de volta
ao Santos Dumont no início. Ele está ancorado no mundo, sempre de frente para
a câmera, com até 144 pixels lógicos de altura para continuar legível de longe.
Quando o aeroporto estiver atrás da câmera ou fora da tela, é preciso virar a
câmera para vê-lo. Ele não é uma seta nas bordas da janela.

`src/ui/MissionMarker.*` e os shaders `mission_marker_*.glsl` desenham uma
superfície procedural com contorno escuro, sem novos assets de imagem. A
projeção usa as matrizes do código da disciplina, descontando a câmera uma
única vez. O desenho ocorre após os passes do cenário/cockpit e antes do HUD,
sem teste/escrita de profundidade para servir como auxílio de navegação mesmo
atrás de prédios. Os estados OpenGL modificados são restaurados em seguida.

Para capturas de inspeção com o marcador explicitamente ativado:

```powershell
# Executar a partir de bin/Debug.
.\main.exe --texture-check ../../build/mission-marker --show-mission-marker
```

As vistas `galeao` e `destination-far` conferem o marcador junto do aeroporto e
a partir da região do Santos Dumont. O sinalizador de inspeção é opcional;
a execução normal continua começando com o marcador oculto.

Verificação do marcador: compilação e 14 suítes CTest aprovadas; 16 capturas do
inspetor sem erros OpenGL, incluindo conferência visual do Galeão de perto e
à distância, além do cockpit e dos instrumentos.

## Arquivos e fluxo

| Arquivo | Responsabilidade |
| --- | --- |
| `src/mission/Airports.*` | Geometria das quatro pistas, teste de retângulo, distância e rumo. |
| `src/mission/FlightMission.*` | Máquina de estados CPU, histórico da decolagem, toque e confirmação. |
| `src/ui/MissionOverlay.*` | Texto, alvo, distância, rumo e contagem, usando o renderizador de fonte existente. |
| `src/ui/MissionMarker.*` | Marcador procedural do Galeão, projetado na tela a partir de sua âncora no mundo. |
| `src/render/GameScene.*` | Produz `MissionSample` com o resultado físico e os volumes das rodas. |
| `src/main.cpp` | Executa missão após colisões, desenha interface e trata reinício. |
| `src/mission/MissionInspection.*` | Inspeção visual opcional com estados de interface explicitamente simulados. |
| `data/rio/airports.json` | Proveniência cartográfica e valores projetados auditáveis. |

O fluxo normal é `FlightModel::update → GameScene::ResolvePlayer →
GameScene::PlayerMissionSample → FlightMission::Update → MissionOverlay::Draw`.
O núcleo não chama OpenGL, GLFW, câmera ou funções que alteram a física.
`MissionSnapshot` transporta o estado, a instrução, o alvo e os valores usados
pela interface. As constantes de jogabilidade estão em `FlightMission.cpp`.

`MissionSample` carrega as três posições e raios das rodas em coordenadas do
mundo. A validação de altura usa `center.y - radius`, aceitando o primeiro toque
inclinado sem confundir o teto de um prédio com a pista. Leituras não finitas,
raios inválidos ou históricos interrompidos não podem produzir uma vitória.

O alvo em rota fica 1 km antes do extremo de aproximação da pista 15/33 do
Galeão. Perto do aeroporto é escolhida a pista mais próxima; o alvo passa ao
centro dela. Distância horizontal e rumo referem-se ao **mesmo alvo mostrado**.
O rumo é cartesiano: norte = 000°, leste = 090°, sul = 180°, oeste = 270°.
Não representa declinação magnética nem uma rota aeronáutica homologada.

Os temporizadores usam o passo da física (`min(dt, 0.05)`), sem contagem de
quadros. A contagem de voo começa na decolagem confirmada. Em pausas grandes,
o limite de passo já existente também limita o avanço desses temporizadores.

## Coordenadas e origem dos dados

As pistas foram obtidas do **mesmo `rio.osm` usado na conversão OSM2World**,
com SHA-256 `0dd75b824eb1eb11345ac4c5fc6834b090ea1123204db2eb21706aa0519730e4`.
O arquivo original permanece fora do repositório; não é necessário em execução.
A origem Mercator local é latitude −22,84895, longitude −43,2142. As unidades
são metros, `+Y` para cima e norte em `−Z`. A projeção reproduz a escala local
`40075016.686 * cos(latitude da origem)` usada na geração do cenário.

| Aeroporto | Pista | Way OSM | Largura | Extremo inicial XZ | Extremo final XZ |
| --- | --- | --- | --- | --- | --- |
| Santos Dumont | 20L/02R | 510811286 | 42 m | (5231,294; 6195,887) | (5309,545; 7519,976) |
| Santos Dumont | 20R/02L | 12292143 | 30 m | (5154,355; 6199,929) | (5229,499; 7463,977) |
| Galeão | 15/33 | 12291116 | 47 m | (−2591,027; −2278,171) | (−4971,293; −3990,601) |
| Galeão | 10/28 | 12291112 | 45 m | (−4204,834; −5243,320) | (−353,434; −6318,690) |

Os retângulos acompanham os extremos e a largura dos eixos OSM. A margem de
2 m é conservadora em relação às pequenas variações dos pontos intermediários.
As áreas de cabeceira deslocada mapeadas separadamente não são incluídas na
pista 15/33. `Y=0` corresponde ao piso plano exportado, e não a uma medição
topográfica ou elevação real sobre o nível do mar.

`Airports.cpp` contém os valores utilizados em execução; o JSON guarda IDs,
tags, extremos geográficos, projeção e licença para auditoria. Não há parser
JSON adicional nem acesso a mapas online no jogo. A vista `galeao` do inspetor
de renderização foi corrigida para usar essa mesma geometria.

FONTE dos dados: © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright),
sob ODbL. As funções e a lógica de missão foram escritas com auxílio de IA;
não foi incorporado código externo de navegação ou de transformação.

## Verificação

```powershell
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

Os alvos CPU `airports` e `flight_mission` cobrem limites das pistas (inclusive
extremos sem arredondamento), rumos, dados inválidos, geografia real, histórico
da decolagem, salto direto para o destino, toque fora da pista, taxiamento,
primeiro toque de uma roda, altura de apoio, velocidade, alinhamento, reinício,
prioridade de acidente e confirmação equivalente em 30, 60 e 144 atualizações/s.
As doze suítes anteriores permanecem no CTest.

Inspeção opcional, a partir de `bin/Debug`:

```powershell
.\main.exe --mission-check ../../build/mission-check
.\main.exe --mission-check ../../build/mission-check --keep-open
```

A inspeção carrega a geometria real e grava `mission-check.txt`. Primeiro
confere três pontos de cada pista contra os triângulos exportados (piso `Y=0`,
sem aceitar a base de preenchimento a `Y=-0,02`). Depois usa instâncias locais
do mesmo resolvedor de contato para conferir apoio no Santos Dumont e pousos
seguros/fortes nas duas pistas do Galeão, ligados à máquina de estados.

Essa sequência é um **replay determinístico de amostras**, não um voo manual
completo nem um piloto automático. Não move o jogador e não altera a missão
normal. As capturas `.ppm` seguintes verificam separadamente a apresentação de
partida, rota, aproximação, pouso, conclusão e falha, em janelas de 1280×720 a
320×240. As mensagens de alinhamento e velocidade também têm vistas compactas.
Os logs identificam expressamente esses estados como `fixture UI (not gameplay)`.
Cada captura verifica erros OpenGL. `--keep-open` encerra as prévias e deixa a
janela 1280×720 na missão real inicial; sem essa opção o programa fecha.

Verificação desta implementação em Windows: compilação C++11 concluída, 14/14
suítes CTest aprovadas (44 verificações de aeroportos e 140 da missão), 31
verificações com o mapa carregado aprovadas e 11 capturas de interface sem
erros OpenGL. As imagens foram inspecionadas para legibilidade e sobreposição
com os instrumentos, inclusive as instruções compactas de alinhar e reduzir.
Esses resultados não substituem o teste de voo manual descrito abaixo.

## Limitações atuais

A lógica da missão está integrada às colisões e não modifica os controles de
voo. **O controle da atitude do avião pelo jogador ainda falta no código atual**:
`CommandInterface::process_input()` está vazio; o botão direito move a câmera,
e `W/S` ajusta a potência fora da câmera livre. Portanto, ainda não se deve
considerar validado um trajeto manual completo Santos Dumont → Galeão.
O próximo trabalho na física deve conectar as entradas à orientação do avião
e permitir a manobra e a redução de velocidade de forma controlada.

Os limites são regras simplificadas de jogo, não critérios de operação de um
A320. Não há controle de tráfego, plano de voo, sistema de navegação real,
colisão com a aeronave da curva de Bézier ou detecção exata de cada pneu da malha.
Os volumes físicos continuam sendo os proxies descritos em `docs/collisions.md`.
