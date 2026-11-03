# Origem e preparação dos modelos do Caelum

Guia técnico de proveniência produzido com auxílio de IA. Registra os arquivos
e o processo de preparação dos assets; não substitui o relatório final da dupla.

| Asset | Como foi encontrado | Origem | Preparação para o jogo |
| --- | --- | --- | --- |
| Rio de Janeiro | Escolha de uma região real com Santos Dumont e Galeão no OpenStreetMap. | Dados dos colaboradores do OpenStreetMap; conversão OSM2World. | OSM → OBJ/MTL/imagens → divisão determinística em 125 partes e base complementar. |
| Airbus A320-200 | Busca de um avião comercial no Sketchfab; modelo escolhido e baixado manualmente pelo usuário. | TonyWony, “A320-200 with cockpit (ver VIII)”. | Importação no Blender, correção de materiais/UVs e exportação OBJ. |
| Monomotor leve | Busca de um segundo avião gratuito de pequeno porte para a seleção no menu. | MagicGames, “Low Poly Airplane”, no itch.io. | Download da opção gratuita, conversão do FBX e adaptações simples da cabine. |

Os modelos originais de terceiros não foram criados pela dupla nem pela IA.
A IA auxiliou a preparação, os scripts de conversão/divisão, a integração e
esta documentação. A autoria original e as alterações estão identificadas abaixo.

## 1. Rio de Janeiro

O cenário foi produzido a partir de **dados vetoriais do OpenStreetMap**,
convertidos pelo **OSM2World 0.5.0-SNAPSHOT**. Não foi baixado um modelo pronto
de uma cidade, nem usado um mapa de satélite como textura de um plano.

A região escolhida contém os aeroportos Santos Dumont e Galeão. O `rio.osm`
usado na conversão tem estes limites registrados em `rio/chunks/base_terrain.json`:

| Limite | Valor |
| --- | --- |
| Latitude mínima / máxima | −22,9306000 / −22,7673000 |
| Longitude mínima / máxima | −43,2803000 / −43,1481000 |
| Origem da projeção local | latitude −22,84895; longitude −43,2142 |

O aviso de tamanho da exportação do site não significa que o projeto tenha
carregado o arquivo do planeta inteiro. A entrada preservada para reprodução
é o extrato local `rio.osm`, cujo cabeçalho identifica o gerador Overpass API.
Não foi registrado um endereço imutável do servidor
que entregou esse download; não se deve inventar uma origem mais específica.

O OSM2World gerou as construções, ruas, pistas, árvores e superfícies com base
nas informações do extrato e no estilo da ferramenta. O OBJ resultante continha
**3.997.691 faces**. O script [`split_obj.py`](rio/tools/split_obj.py) preservou
as faces, materiais, normais e UVs, distribuindo-as por uma grade de 2 km em XZ,
com subdivisão das partes maiores que 48.000.000 bytes. O resultado são os
**125 OBJ listados em [`manifest.json`](rio/chunks/manifest.json)**.

As imagens do MTL foram copiadas para `rio/chunks/textures/`, receberam prefixos
derivados de hash para evitar colisões de nomes e passaram a usar caminhos
relativos. A divisão não acrescentou relevo. A base `base_terrain.obj`, gerada
separadamente, tem apenas dois triângulos a `Y=-0,02 m`, limitados ao retângulo
do extrato, para preencher lacunas de chão. Não é um DEM nem uma reconstrução
topográfica da cidade.

As pistas e suas coordenadas para a missão estão em [`airports.json`](rio/airports.json).
São metadados derivados do mesmo extrato; o jogo usa os valores correspondentes
em `src/mission/Airports.cpp`.

**Fontes e licenças:** dados © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright),
sob ODbL. A ferramenta é o [OSM2World](https://osm2world.org/). As imagens e o
estilo têm proveniência própria no [repositório oficial do estilo padrão](https://github.com/tordanik/OSM2World-default-style),
cujo README identifica domínio público/CC0; essa licença do estilo não substitui
a ODbL dos dados cartográficos. Os scripts de preparação do projeto são
identificados como assistidos por IA.

Veja o roteiro completo e os comandos em [`rio/PROVENANCE.md`](rio/PROVENANCE.md)
e o guia dos arquivos ativos em [`rio/README.md`](rio/README.md).

## 2. Airbus A320-200

O usuário procurava um avião comercial semelhante à referência do trabalho e
escolheu no Sketchfab **“A320-200 with cockpit (ver VIII)”**, de **TonyWony**:
[página original](https://skfb.ly/6R8YA). O download e a importação inicial no
Blender foram feitos pelo usuário. O registro local menciona referências de
texturas vindas do FBX e um pacote separado em `source` e `textures`.

Na importação, parte das imagens tinha nomes ou caminhos diferentes, e outras
referências não estavam no download. As imagens disponíveis foram religadas.
As ausentes receberam materiais substitutos; a pintura branca/azul, a cauda,
as portas e os bancos foram corrigidos. Portanto, o A320 do projeto é uma
**versão adaptada**, e não uma restauração idêntica de todas as texturas do site.

O resultado foi exportado pelo Blender como OBJ triangulado com normais, UVs,
MTL e imagens relativas. A reimportação do conjunto exportado registrou
**522 objetos de malha, 51.022 faces e nenhuma imagem ausente**. Esses números
estão em [`validation.json`](aircraft/validation.json). O diagnóstico inicial
e as substituições estão em [`repair_report.json`](aircraft/repair_report.json).

Arquivos ativos: `aircraft/aircraft.obj`, `aircraft/aircraft.mtl` e
`aircraft/textures/`. O `aircraft.blend` é a cópia editável das correções;
`aircraft.blend1` é backup. As imagens `preview.png`, `tail_preview.png` e
`seats_preview.png` são referências de conferência, sem uso em execução.

**Crédito original fornecido com o modelo:**

> "A320-200 with cockpit (ver VIII)" (https://skfb.ly/6R8YA) by TonyWony is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

As alterações de materiais, UVs e exportação tiveram auxílio de IA. Veja os
detalhes em [`aircraft/PROVENANCE.md`](aircraft/PROVENANCE.md) e o aviso existente
em [`aircraft/LEIA-ME.txt`](aircraft/LEIA-ME.txt).

## 3. Monomotor leve

Para acrescentar uma segunda aeronave ao menu, o usuário escolheu um monomotor
“como um Cessna” e autorizou procurar e baixar um modelo gratuito. Foi escolhido
**“Low Poly Airplane”**, de **MagicGames**, na
[página do autor no itch.io](https://magic-games.itch.io/low-poly-airplane).
A opção gratuita contém o FBX e um arquivo de licença CC BY 4.0. Não foi
adquirido o pacote premium. O modelo é estilizado, de asa alta e com roda de
cauda; **não representa um Cessna específico**.

O download original foi preservado em `light_aircraft/source/Airplane.fbx`,
com sua licença em `source/License.txt`. O script
[`convert_asset.py`](light_aircraft/tools/convert_asset.py), executado no Blender,
importa esse FBX e gera OBJ/MTL nas coordenadas do Caelum. Não executa scripts
do arquivo baixado.

As adaptações foram:

1. Frente em `+X`, cima em `+Y` e direita em `+Z`.
2. Dimensões aproximadas de jogo de 9 × 3 × 11 m (comprimento, altura, envergadura).
3. Vidros transparentes e adição de piso, painel e dois bancos simples à cabine.
4. Pás semitransparentes para preservar a visão da câmera. A hélice permanece estática.
5. Exportação com 1.094 triângulos, normais e materiais; o renderizador acrescenta
   a superfície procedural usada pelo projeto nos materiais sem imagem.

**A pintura vermelha e azul original foi mantida**, conforme a preferência do
usuário. As transparências e a cabine são adaptações identificadas; não foi
substituída a pintura por branco ou por outra companhia/modelo.

Os limites da malha, as rodas, a posição da câmera e os hashes da origem estão
em [`metadata.json`](light_aircraft/metadata.json). Esses dados calibram o perfil
de colisões de trem convencional e o enquadramento da câmera, em vez de usar
as proporções do A320 para a aeronave pequena.

**Crédito:** “Low Poly Airplane” by MagicGames, sob
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), modificado para Caelum.
Veja a atribuição completa, as alterações e o comando de reprodução em
[`light_aircraft/credits.md`](light_aircraft/credits.md).

## 4. Reprodução e arquivos necessários

O jogo carrega os **OBJ/MTL/imagens preparados**, sem precisar executar Blender,
OSM2World ou os scripts de preparação. Os `.blend`, o FBX de origem, relatórios,
prévias e metadados existem para edição, auditoria e reprodução.

Para reconstruir exatamente o cenário, é preciso o **mesmo extrato OSM e o mesmo
OBJ original**, além das imagens. O OSM muda ao longo do tempo: baixar hoje a
mesma região pode gerar outro resultado. As identidades preservadas são:

| Entrada | SHA-256 registrado |
| --- | --- |
| Extrato `rio.osm` | `0dd75b824eb1eb11345ac4c5fc6834b090ea1123204db2eb21706aa0519730e4` |
| OBJ grande do Rio | `996ba586c584884feb62793c66a77b6226879db747009353511d3cde43ab4425` |
| FBX do monomotor | `8d55c2e30326e3e1168fc5b07bcf631e9c44a90bfced6d0343ddb66b18bd9863` |

O OSM e o OBJ grande do Rio foram preservados localmente fora dos arquivos
necessários à execução. Os scripts de divisão e da base usam esses originais.
O monomotor tem o FBX preservado no repositório. Para o A320, a edição corrigida
está no `.blend`; os relatórios registram o resultado das correções, mas não
constituem um script completo que recria cada edição histórica do zero.

Mantenha os caminhos relativos de MTL e imagens. Não normalize separadamente
os chunks do Rio. A regra de finais de linha em `.gitattributes` preserva LF
nos OBJ/MTL gerados do cenário para manter os hashes entre Windows e Linux.

As silhuetas vistas no menu são ilustrações procedurais desenhadas em C++/OpenGL;
não são novos modelos baixados nem substituem as duas malhas 3D usadas no voo.
