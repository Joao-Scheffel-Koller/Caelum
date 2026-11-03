# Luzes das aeronaves

Implementação assistida por IA na branch `light`, criada da `main` em `5594f68`
(merge do menu). O prompt está registrado no item 31 de `prompts.md`.

## Resultado e organização

O A320 e o monomotor recebem luzes de navegação contínuas, beacons vermelhos e
strobes brancos. O jogador e o tráfego Bézier usam o perfil da aeronave
selecionada e a mesma malha de lâmpada, com matrizes diferentes.

| Arquivo | Responsabilidade |
|---|---|
| `src/aircraft/AircraftLights.*` | Perfis locais, intensidades temporais e relógio; sem OpenGL |
| `src/render/AircraftLightRenderer.*` | Lentes e halos, recursos GPU e preservação do estado GL |
| `src/render/shaders/aircraft_light_*.glsl` | Textura procedural da lente, emissão e halo suave |
| `src/render/GameScene.*` | Integração das duas instâncias, tempo e passes |
| `src/render/AircraftLightsInspection.*` | Verificações GPU e capturas dos dois assets reais |
| `tests/AircraftLightsTest.cpp` | Regressões CPU dos perfis, pulsos e relógio |

## Posição e referências

Os dois OBJ usam `+X` para o nariz, `+Y` para cima e `+Z` para a direita.
Portanto, a luz vermelha fica no lado `Z < 0` e a verde em `Z > 0`.
Há também uma luz branca na parte traseira. Essa distribuição segue a
[referência visual da FAA sobre operações noturnas](https://www.faa.gov/sites/faa.gov/files/regulations_policies/handbooks_manuals/aviation/airplane_handbook/12_afh_ch11.pdf).
A separação entre beacon vermelho e strobe branco toma como referência a
[descrição dos sistemas anticolisão da FAA](https://www.faa.gov/air_traffic/publications/aim_html/chap4_section_3.html).

As coordenadas são medidas nos modelos versionados, não copiadas de uma planta
do A320 real. No jato, as pontas da asa principal ficam perto de
`(-10,93; 0,72; ±17,18)`. No monomotor, a ponta dianteira da asa fica perto de
`(2,65; 2,87; ±5,50)`. As lâmpadas são deslocadas alguns centímetros para fora
da superfície. Os comentários de `Profile()` identificam os objetos e pontos
usados na medição. Os perfis contêm sete lâmpadas no jato e seis no monomotor.

`Profile("a320")` e `Profile("light")` retornam esses conjuntos; um ID
desconhecido lança `std::invalid_argument` para expor uma configuração incorreta.
`startFlight` escolhe o perfil pelo ID do catálogo após carregar a aeronave.

## Piscadas e pausa

O período visual é de 1,2 s. O beacon fica ativo nos primeiros 0,16 s do ciclo,
com transições suaves de 0,02 s. O strobe produz dois flashes de 0,065 s,
começando em 0 e 0,16 s; cada borda dura 0,008 s. As luzes de navegação ficam
acesas. Esses tempos são parâmetros visuais simplificados, sem pretensão de
reproduzir exatamente o equipamento de uma aeronave específica.

`Intensity(pattern, time, phaseOffset)` avalia o ciclo com `std::fmod`; offsets
são medidos em segundos. O segundo avião usa um deslocamento de 0,43 s para
não piscar sempre junto com o jogador.

`GameScene::Update(elapsed)` avança o relógio uma única vez por quadro, junto
do tráfego. `Draw` apenas consulta esse relógio. O menu de pausa e a janela
minimizada não chamam essa atualização. Reiniciar o jogador reinicia o relógio.
O relógio ignora passos negativos/não finitos e adições que causariam overflow.

## Desenho e profundidade

A lente é uma esfera pequena de 240 vértices. Sua aparência combina textura
procedural, iluminação de Phong e emissão; desligada, permanece uma lente visível.
O termo especular compara `reflect(-sun,n)` com a direção da câmera. O expoente
da lente é 9, e o do cenário/aeronaves é 8. A escolha aproxima a largura do brilho
da versão anterior, que usava Blinn–Phong com expoentes 36 e 32. Essa correção,
registrada no prompt 32, alinha a reflexão ao modelo prometido no `SPEC.md`.
O halo é um quad de seis vértices voltado para a câmera, com queda radial
suave calculada no fragment shader. Seu tamanho é físico, sem ampliação mínima
em pixels que faria o avião distante parecer coberto de grandes manchas.

O ponto local passa pela matriz completa de cada instância. O jogador e o NPC
usam pivôs diferentes, já resolvidos nessas matrizes. A posição da câmera é
subtraída em CPU com precisão dupla antes do envio à GPU, preservando pontos
pequenos nas coordenadas quilométricas do Rio.

As lentes são desenhadas depois dos sólidos, com teste e escrita de
profundidade. Os halos são desenhados depois dos vidros, com teste de
profundidade, escrita desativada e blending aditivo. Asas, fuselagem e prédios
opacos ocultam as luzes. O halo posterior não recebe a atenuação física do vidro;
a ordenação das superfícies transparentes mantém as limitações do renderer.

No cockpit, as luzes do tráfego pertencem ao passe `WORLD`, e as do jogador ao
passe `AIRCRAFT`, cada uma com sua projeção. O relógio não avança nesses passes.
O renderer restaura programa, VAO/VBO, profundidade, blending, culling, modo
poligonal e estados de cobertura de amostras para preservar cenário e HUD.
Os recursos são criados no primeiro desenho e liberados antes do contexto GL.

O brilho representa a aparência da fonte; não acrescenta iluminação dinâmica
no chão, bloom ou sombras. As cores de navegação não possuem setores angulares
regulamentares: a oclusão é determinada pelas malhas. Não há comandos elétricos
separados; as luzes funcionam automaticamente durante a sessão de voo.

## Verificação

```powershell
cmake --build build -j 4
ctest --test-dir build --output-on-failure
Set-Location bin/Debug
.\main.exe --lights-check ../../build/lights
```

`--lights-check` abre um contexto oculto e usa os assets reais das duas aeronaves,
sem carregar o Rio. Confere emissão ligada/desligada, oclusão por profundidade,
restauração de estado e estabilidade ao transladar a cena/câmera para 6 km.
Produz capturas PPM de vistas externas, cockpit e detalhes de asa/beacon.
Essas capturas verificam o subsistema; não substituem um voo manual completo.

Na validação desta etapa, as 17 suítes CTest passaram. A inspeção isolada
concluiu as dez capturas sem erros OpenGL. A inspeção integrada
`--texture-check ../../build/lights-world --hidden` carregou as 126 partes do
Rio e completou 16 vistas, incluindo jogador, tráfego e cockpit, sem erros GL.
Os arquivos de captura e logs ficam em `build/`, que está ignorado pelo Git.

Para testar no jogo, iniciar qualquer aeronave pelo menu e observar as pontas
das asas e a fuselagem em `L` ou `F`. `C` mantém a visão do cockpit. Abrir a
pausa com `Escape` deve congelar as piscadas junto com a simulação; retornar
continua do mesmo instante. Nenhum controle de câmera foi alterado.
