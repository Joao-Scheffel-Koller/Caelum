# Rio de Janeiro em partes

Mapa convertido pelo OSM2World 0.5.0-SNAPSHOT, dividido com auxilio de IA
(Codex) pelo script `data/rio/tools/split_obj.py`.

## Arquivos para carregar

Carregue **todos os OBJ listados em `chunks/manifest.json`**, mantendo a mesma
matriz de posicionamento para todos. Cada arquivo tem um objeto com nome unico.
Nao centralize nem normalize a escala de cada parte separadamente.

Os arquivos usam as coordenadas originais do OSM2World: Y para cima, norte em
-Z e aproximadamente um metro por unidade. A origem geografica e latitude
-22.84895, longitude -43.2142. A divisao nao acrescenta relevo ao mapa.

Mantenha `chunks/rio.obj.mtl` e a pasta `chunks/textures/` junto dos OBJ.
As imagens antes referenciadas por caminhos absolutos em Downloads foram
copiadas e passaram a ter caminhos relativos. A divisao preserva as imagens.

## Ponto inicial no jogo

`src/GameScene.cpp` posiciona o aviao na pista 20L do Santos Dumont. Os extremos
do eixo da pista no OSM usado na conversao sao (-22.9045972, -43.163205) e
(-22.9164863, -43.1624422). Projetando com a mesma Mercator local do OSM2World
(origem indicada acima), os pontos XZ sao aproximadamente (5231.294, 6195.887)
e (5309.545, 7519.976).

O ponto inicial fica 100 m a partir do primeiro extremo, na direcao do segundo:
XZ=(5237.1933, 6295.7127), direcao XZ=(0.05899537, 0.99825826). A altura foi
conferida nos triangulos `ASPHALT_0` de `rio_x+002_z+003.obj`: Y=0. O modelo do
aviao e posicionado pela base das rodas, mantendo a escala original em metros.

## Regra de divisao

- Grade XZ de 2000 metros, ancorada em (0, 0).
- Cada face pertence a celula que contem o centro medio de seus vertices.
- Celulas com OBJ maior que 48.000.000 bytes sao subdivididas em quatro,
  na ordem noroeste, nordeste, sudoeste e sudeste (-Z aponta para norte).
- A ordem das faces dentro de cada regiao segue a ordem do arquivo de origem.
- Uma face que cruza a borda permanece inteira; ela nao e cortada ou duplicada.

Para repetir a divisao, a partir da raiz do projeto, escolha uma pasta de saida
que ainda nao exista:

```powershell
python data/rio/tools/split_obj.py build/rio_original/rio.obj data/rio/chunks_novos --cell-size 2000 --max-bytes 48000000
```

O OBJ original grande foi preservado localmente em `build/rio_original/`,
diretorio ignorado pelo Git. Para repetir em outra maquina e necessario obter
o mesmo arquivo de origem e as texturas indicadas no MTL. O SHA-256 da origem
esta no manifesto. O script usa somente a biblioteca padrao do Python 3.

O manifesto registra arquivos, tamanhos, limites espaciais e hashes. A verificacao
rele os OBJ gerados e compara todas as faces com a origem, incluindo ordem dos
vertices, coordenadas, UVs, normais, materiais e repeticoes.

Teste pequeno de reproducibilidade e subdivisao:

```powershell
python data/rio/tools/test_split_obj.py
```

## Fontes e creditos

- Dados cartograficos: © OpenStreetMap contributors,
  https://www.openstreetmap.org/copyright (ODbL).
- Conversao e materiais/texturas do estilo padrao: OSM2World,
  https://osm2world.org/ e https://github.com/OSM2World/default-style .
  Os nomes originais das imagens foram mantidos apos um prefixo de hash.
- Script de divisao e verificacao escrito com auxilio de IA (Codex).
