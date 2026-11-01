# Computação Gráfica e Visualização I (INF01047) - INF/UFRGS

Este repositório contém o código base para o trabalho final. O enunciado completo do trabalho final está no Moodle:

https://moodle.ufrgs.br/mod/assign/view.php?id=6302370

## Cena integrada

O jogo carrega o A320 de `data/aircraft/aircraft.obj` e as 125 partes do Rio
listadas em `data/rio/chunks/manifest.json`, com materiais MTL e texturas
compartilhadas. O coelho, a esfera e o plano de exemplo deixaram de fazer parte
da cena inicial. Implementação realizada com auxílio de IA (ver `prompts.md`).

O avião começa parado na pista **20L do Santos Dumont**, aproximadamente 100 m
depois da cabeceira norte, apontando para o sul. A posição no mapa é
`(5237.1933, 0, 6295.7127)`, em metros. A transformação do modelo mantém as rodas
sobre o asfalto e a câmera começa atrás do avião.

A janela inicia em **1280 x 720** e o desenho tem limite de **60 FPS**.

Com o ambiente CMake/compilador configurado, compile e execute a versão nova:

```powershell
cmake -S . -B build
cmake --build build
Push-Location bin/Debug
.\main.exe
Pop-Location
```

No Linux, execute dentro de `bin/Linux`. Os caminhos dos assets e shaders são
relativos ao diretório do executável; no Windows Release, use `bin/Release`.
Feche uma versão antiga aberta antes de recompilar. O mapa é carregado inteiro
na inicialização, com progresso no título da janela; isso pode levar algum tempo.
Durante o desenho, partes fora do campo de visão da câmera são descartadas.

Controles disponíveis nesta etapa:

- `C` / `F` / `L`: cockpit / câmera livre / câmera externa.
- Botão direito + mouse: olhar ao redor ou orbitar; na externa, mouse para cima
  eleva a câmera. Solte para liberar o cursor.
- `W` / `S`: potência do motor no cockpit/externa; movimento na câmera livre.
- Na livre: `WASD` para mover, `Q/E` para descer/subir, `Shift` para acelerar.
- No cockpit: setas para mover a cabeça, `Page Up/Down` para subir/baixar os olhos.
- Rodinha: distância na externa, velocidade de deslocamento na livre.
- `Home`: centralizar cockpit, restaurar externa ou voltar à visão livre atrás do avião.
- `P` / `O`: projeção perspectiva / ortográfica fora do cockpit (cockpit usa perspectiva).
- `R`: recarregar os shaders; `H`: mostrar/ocultar informações; `Esc`: sair.

As três câmeras têm estado separado; cockpit acompanha a transformação do avião
e a livre se desprende da posição/direção atual. Detalhes da implementação e
roteiro de teste em [docs/cameras.md](docs/cameras.md).

A física e os controles de voo ainda são simplificados. O mouse sem botão direito
ainda não controla a atitude da aeronave. O contato com o
solo usa o plano Y=0, adequado ao ponto inicial: ainda falta colisão com prédios
e amostragem da altura do restante do cenário. A iluminação usa cor, textura
difusa e transparência; mapas de normais e materiais PBR não são aplicados.

Para verificar o carregamento e a renderização, a partir de `bin/Debug`:

```powershell
.\main.exe --smoke-test ../../build/scene-smoke.ppm
.\main.exe --smoke-test ../../build/cockpit.ppm cockpit
.\main.exe --smoke-test ../../build/free-camera.ppm free
```

Esse modo verifica erros de OpenGL, salva uma captura PPM opcional e fecha após
60 quadros. O teste da divisão determinística está descrito em `data/rio/README.md`.
As verificações matemáticas e de entrada das câmeras rodam sem abrir janela:

```powershell
ctest --test-dir build --output-on-failure
```

## Créditos

"A320-200 with cockpit (ver VIII)" (https://skfb.ly/6R8YA) by TonyWony is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

Dados do Rio: © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright),
sob ODbL. Conversão e materiais do [OSM2World](https://osm2world.org/).
Detalhes em `data/rio/README.md`.
