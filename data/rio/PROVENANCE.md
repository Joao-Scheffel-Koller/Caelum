# Origem e preparação do cenário do Rio de Janeiro

Registro técnico escrito com auxílio de IA. Este arquivo documenta a aquisição
e a preparação dos assets; não substitui o relatório final da dupla.

## O que foi obtido

O cenário parte de dados vetoriais do **OpenStreetMap** referentes à região do
Rio que inclui o **Santos Dumont**, o **Galeão** e o trajeto entre eles. Não é um
modelo fotogramétrico pronto nem um mapa de imagens de satélite: prédios,
ruas, vegetação e superfícies foram convertidos em geometria pelo OSM2World.

No diálogo de preparação, a área foi escolhida na página de exportação do OSM.
O serviço informou que o retângulo era grande e apresentou outras fontes,
incluindo o Planet OSM. Isso **não significa que o arquivo do planeta inteiro
tenha sido baixado**. O arquivo efetivamente preservado é um extrato `rio.osm`.
Seu cabeçalho identifica `Overpass API 0.7.62.11 87bfad18` como gerador e informa
o estado da base em `2026-10-18T14:52:06Z`. A consulta e o endpoint exatos da
extração final não foram preservados, portanto não se atribui o download a um
espelho específico nem se reconstrói uma consulta histórica inexistente.

Os limites registrados no próprio extrato são:

| Limite | Graus decimais |
| --- | --- |
| Latitude mínima | −22,9306000 |
| Longitude mínima | −43,2803000 |
| Latitude máxima | −22,7673000 |
| Longitude máxima | −43,1481000 |

Para obter dados equivalentes, abra [OpenStreetMap — Exportar](https://www.openstreetmap.org/export),
selecione esse retângulo e obtenha os dados OSM da região. Se o limite da
exportação direta for excedido, utilize uma extração regional, como o Overpass,
em vez de importar o planeta inteiro. Uma extração feita em outra data pode
conter outros objetos e não terá necessariamente o mesmo hash. Ways e relações
incluídos no extrato podem carregar geometria que ultrapassa o retângulo; os
chunks preservam essa geometria, sem recortar as faces na borda geográfica.

FONTE dos dados e crédito: **© OpenStreetMap contributors**, sob
[Open Database License — ODbL](https://www.openstreetmap.org/copyright).

## Conversão para OBJ

Foi utilizado **OSM2World 0.5.0-SNAPSHOT**, com o estilo padrão
`standard.properties`. A tentativa inicial abriu o visualizador gráfico; o log
preservado mostra a inicialização sem subcomando, advertências sobre geometrias
OSM e falhas `OutOfMemoryError: Java heap space`. A tela preta do visualizador
não constitui uma verificação de que uma exportação terminou corretamente.

O fluxo de preparação passou à conversão direta para OBJ, sem depender de
conseguir desenhar todo o mapa no visualizador. O comando final dessa conversão
não foi registrado junto dos arquivos do projeto. Para repetir a etapa, use a
operação de conversão de entrada OSM para saída Wavefront OBJ indicada pela
ajuda da versão instalada do OSM2World, com `rio.osm` como entrada e `rio.obj`
como saída. Preserve o arquivo de configuração e as pastas do estilo. Como a
versão utilizada é um snapshot, não se presume que os argumentos de outra
versão sejam idênticos.

A saída necessária tem três partes: **`rio.obj`**, **`rio.obj.mtl`** e as
**imagens referenciadas pelo MTL**. O OBJ guarda a malha e as coordenadas UV; o
MTL associa materiais e caminhos; a pasta de imagens fornece as texturas.
Importar apenas o OBJ não transporta essas imagens automaticamente.

O resultado foi aberto no Blender para inspeção. O enquadramento e o alcance
de visualização precisaram ser ajustados para mostrar a cidade inteira; a
prévia de materiais serviu para conferir as cores/texturas. O `.blend` é um
arquivo de trabalho local, e não uma dependência do executável. Os assets de
execução continuam sendo OBJ, MTL e imagens. Não foi adicionado um DEM nem
relevo topográfico real nessa preparação.

FONTE da ferramenta: [OSM2World](https://osm2world.org/). A distribuição usada
inclui licença MIT para o software, atribuída aos contribuidores do OSM2World;
essa licença de ferramenta não substitui a licença dos dados cartográficos.

## Divisão determinística e organização das texturas

O OBJ original grande foi mantido fora do conjunto de execução, em uma pasta
local ignorada pelo Git. O script [tools/split_obj.py](tools/split_obj.py), escrito
com auxílio de IA, gerou os arquivos de `chunks/` de forma determinística:

- Grade XZ de **2.000 m**, ancorada em `(0, 0)`; a face pertence à célula que
  contém o centro médio de seus vértices.
- Se o OBJ de uma célula exceder **48.000.000 bytes**, a célula é subdividida
  recursivamente em quatro quadrantes, na ordem NW, NE, SW, SE; norte é `−Z`.
- Cada face continua inteira, mesmo quando cruza uma borda. Não há corte,
  simplificação nem duplicação de faces.
- Posições, ordem dos vértices, normais, UVs e material de cada face são
  preservados; a ordem original das faces permanece dentro de cada região.
- Os índices OBJ são remapeados localmente e cada arquivo recebe um nome de
  objeto próprio, sem deslocar, centralizar ou normalizar o chunk.

O resultado registrado contém **125 chunks**, **3.997.691 faces** e **49 imagens**
de textura. Esses números correspondem ao extrato e à conversão preservados;
outro download ou outra versão do conversor pode gerar números diferentes.

O mesmo script copia as imagens efetivamente referenciadas pelo MTL para
`chunks/textures/`, troca os caminhos absolutos por relativos e acrescenta ao
nome um prefixo derivado do SHA-256 do conteúdo. Esse prefixo evita colisões de
nomes; ele não é uma alteração visual da imagem. Opções do MTL, como repetição
ou `-clamp`, são mantidas.

Para refazer a divisão a partir do mesmo OBJ original, execute na raiz do
projeto, escolhendo uma pasta de saída **que ainda não exista**:

```powershell
python data/rio/tools/split_obj.py build/rio_original/rio.obj data/rio/chunks_novos --cell-size 2000 --max-bytes 48000000
python data/rio/tools/test_split_obj.py
```

O script relê a saída e compara o conjunto de faces com a origem, incluindo
multiplicidade, material, vértices ordenados, UVs e normais. O manifesto registra
o total, os limites reais de cada chunk, os tamanhos e os hashes de OBJ/imagens.
Isso permite conferir a preparação preservada; não prova que o OpenStreetMap
continha todos os prédios reais nem que toda geometria problemática foi
convertida pelo OSM2World.

## Base para lacunas de chão

[tools/build_base_terrain.py](tools/build_base_terrain.py) gera um preenchimento
limitado ao retângulo do extrato. São **quatro vértices e dois triângulos**, a
**Y = −0,02 m**, usando `GRASS_0` e a textura já compartilhada do cenário.
`chunks/base_terrain.obj` é carregado separadamente dos 125 chunks do manifesto.

Essa base cobre lacunas visuais da malha exportada. Não é terreno infinito,
não reconstrói altitude e não classifica automaticamente o solo real. Ruas,
água e outras superfícies exportadas ficam por cima; a renderização também usa
um pequeno afastamento de profundidade. A base é física e visualmente finita.

```powershell
# Substitua build/rio_original/rio.osm pelo extrato original disponível.
python data/rio/tools/build_base_terrain.py build/rio_original/rio.osm build/rio_original/base_terrain.obj
python data/rio/tools/test_base_terrain.py
```

Para execução normal, não é preciso regenerar nada: o OBJ, o MTL e as imagens
de `chunks/` já fornecem os recursos utilizados pelo jogo.

## Coordenadas, pistas e hashes

A origem local é latitude **−22,84895**, longitude **−43,2142**, o centro dos
limites do extrato. As unidades são aproximadamente metros, com `+Y` para cima
e norte em `−Z`. A projeção Mercator local usa a escala
`40075016.686 * cos(latitude da origem)`; a convenção pode ser conferida em
`tools/build_base_terrain.py`. Todos os chunks compartilham esse sistema.

[airports.json](airports.json) registra os eixos e as larguras das duas pistas
do Santos Dumont e das duas do Galeão, extraídos do mesmo OSM. Inclui IDs de
ways/nós, tags, coordenadas geográficas e projetadas, limites e referência de
licença. É metadado de auditoria: os valores usados em execução estão em
`src/mission/Airports.cpp`. A altura zero representa a referência plana do mapa
exportado, não uma elevação topográfica medida sobre o nível do mar.

| Arquivo de referência | SHA-256 |
| --- | --- |
| Extrato original `rio.osm` | `0dd75b824eb1eb11345ac4c5fc6834b090ea1123204db2eb21706aa0519730e4` |
| OBJ original antes da divisão | `996ba586c584884feb62793c66a77b6226879db747009353511d3cde43ab4425` |
| Base de preenchimento `base_terrain.obj` | `d9556a572322416212de6ea2c757326d274d7b8002f2e4c6bf7c47bd5195effa` |

Os registros locais correspondentes são `chunks/manifest.json`,
`chunks/base_terrain.json` e `airports.json`. O arquivo OSM e o OBJ grande
originais não são necessários para jogar; eles são necessários para reproduzir
essas preparações exatamente. Atualizar os dados pode exigir recalcular a
origem e os metadados de pistas para manter o alinhamento.

## Materiais, imagens e licenças distintas

Os dados da cidade têm origem no OpenStreetMap, sob ODbL. Os materiais e as
imagens vêm do **estilo padrão do OSM2World**, não de fotografias extraídas de
um mapa comercial. O repositório oficial do estilo informa que usa texturas
de fontes como ambientCG e a biblioteca de texturas da comunidade OSM, em
domínio público, e disponibiliza a configuração e as imagens adicionais com
dedicação CC0. Essa origem e licença de imagens é distinta da licença da base
cartográfica. [FONTE: OSM2World default style](https://github.com/tordanik/OSM2World-default-style),
[COPYING.txt](https://github.com/tordanik/OSM2World-default-style/blob/master/COPYING.txt).

Os nomes originais das imagens foram mantidos depois do prefixo de hash, e o
manifesto permite identificá-las por conteúdo. A reorganização, a divisão e a
base adicional não tornam todos os componentes CC0 nem apagam os créditos do
OpenStreetMap. Preserve os avisos de origem ao redistribuir o cenário.
