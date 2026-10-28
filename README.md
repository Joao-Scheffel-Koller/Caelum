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

- `W` / `S`: aumentar / reduzir a potência do motor, a cada pressionamento.
- Arrastar com o botão esquerdo: girar a câmera ao redor do avião.
- Rodinha: aproximar / afastar a câmera.
- `P` / `O`: projeção perspectiva / ortográfica.
- `R`: recarregar os shaders; `H`: mostrar/ocultar informações; `Esc`: sair.

Esta etapa conecta os modelos ao motor existente. A física e os controles de voo
ainda são simplificados; as outras câmeras continuam pendentes. O contato com o
solo usa o plano Y=0, adequado ao ponto inicial: ainda falta colisão com prédios
e amostragem da altura do restante do cenário. A iluminação usa cor, textura
difusa e transparência; mapas de normais e materiais PBR não são aplicados.

Para verificar o carregamento e a renderização, a partir de `bin/Debug`:

```powershell
.\main.exe --smoke-test ../../build/scene-smoke.ppm
```

Esse modo verifica erros de OpenGL, salva uma captura PPM opcional e fecha após
60 quadros. O teste da divisão determinística está descrito em `data/rio/README.md`.

## Créditos

"A320-200 with cockpit (ver VIII)" (https://skfb.ly/6R8YA) by TonyWony is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

Dados do Rio: © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright),
sob ODbL. Conversão e materiais do [OSM2World](https://osm2world.org/).
Detalhes em `data/rio/README.md`.
