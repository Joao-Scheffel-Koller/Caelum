# A320: origem, preparação e reprodução do asset

Registro técnico escrito com auxílio de IA (Codex). Este arquivo documenta os
assets desta pasta; não substitui o relatório final da dupla.

## Fonte e aquisição

O modelo foi escolhido e baixado manualmente pelo usuário no Sketchfab:

> "A320-200 with cockpit (ver VIII)" (https://skfb.ly/6R8YA) by TonyWony is
> licensed under Creative Commons Attribution
> (http://creativecommons.org/licenses/by/4.0/).

Esse é o crédito fornecido pelo usuário e preservado em `LEIA-ME.txt`. O modelo
inclui exterior, cockpit e interior de passageiros. O resultado nesta pasta é
uma **versão modificada**, com correções de materiais e UVs, pintura simplificada
e reexportação feitas com auxílio de IA. O crédito do autor original deve
acompanhar a redistribuição dessa versão, identificando também as modificações.

O histórico da conversa registra o download em uma pasta separada do projeto,
organizada em `source/` e `textures/`, e a importação no Blender. `LEIA-ME.txt`
menciona referências de imagens do FBX. Isso documenta o contexto da reparação,
mas **não identifica com segurança qual opção de download do site foi escolhida**
(FBX original, GLB convertido ou outra). O arquivo compactado baixado, seu hash
e uma cópia integral da página de download não estão versionados nesta pasta.
Portanto, não se atribui aqui um formato ou hash ao download original.

A licença indicada acima é a informação registrada para o modelo fornecido.
Os nomes das texturas foram preservados quando possível; o repositório não
contém um inventário independente de licenças de cada imagem do pacote.

## O que está efetivamente versionado

| Arquivo ou pasta | Função |
| --- | --- |
| `aircraft.obj` | Geometria final triangulada, UVs, normais e referência ao MTL. |
| `aircraft.mtl` | Materiais e caminhos relativos para as imagens. |
| `textures/` | Imagens recuperadas do pacote e a faixa de pintura produzida na preparação. |
| `aircraft.blend` | Cena editável com as correções; ponto de partida para novas exportações. |
| `aircraft.blend1` | Backup do Blender, sem participação no carregamento do jogo. |
| `repair_report.json` | Referências religadas, imagens ausentes substituídas e materiais simplificados. |
| `validation.json` | Resultado da reimportação do OBJ exportado. |
| `preview.png` | Conferência geral do avião. |
| `tail_preview.png` | Conferência da pintura traseira e do símbolo na cauda. |
| `seats_preview.png` | Conferência do tecido dos assentos. |
| `LEIA-ME.txt` | Notas originais da preparação e crédito do autor. |

O conjunto foi adicionado ao Git no commit
`cd07d6f10fdab57efd88b6e11744f07ee9c7506a`, com a mensagem
`IA: assets: add A320 aircraft with AI-assisted material and UV fixes`.
O `.obj`, o `.mtl` e `textures/` precisam permanecer juntos: o jogo não abre o
arquivo `.blend` para desenhar a aeronave.

## Diagnóstico dos materiais

O modelo parecia íntegro na visualização sólida do Blender, mas partes ficavam
rosas ao ativar a visualização de materiais. Os avisos indicavam referências a
imagens que não eram encontradas. A separação em `source/` e `textures/` não era,
por si só, um problema: havia tanto nomes/caminhos diferentes quanto arquivos
que não tinham vindo no conjunto disponível.

`repair_report.json` separa esses casos:

- **`relinked`:** referências que passaram a apontar para imagens existentes.
  Por exemplo, `leather07 normal 4k.jpg` foi ligado a
  `leather07_normal_4k.jpg`, `panel bumb.png.001` a `panel_bumb.png`, e os nomes
  antigos do estofamento `FabricUpholsteryBrightAnglePattern001_*_3K.jpg`
  receberam correspondências explícitas para os arquivos `.jpeg` disponíveis.
- **`missing_images_replaced`:** imagens realmente ausentes, incluindo
  `tail_base.png`, `Rudder_base.png`, `Foam_Tubes_Roughness.jpg`,
  `Foam_Tubes_Metallic.jpg`, `leather07 diffuse 4k.jpg`, `prop texture.png` e
  outras listadas integralmente no relatório.
- **`fallback_materials`:** materiais cujos nós de imagem inválidos foram
  removidos e substituídos por cores ou valores escalares compatíveis, como
  couro azul-escuro, exterior claro, painéis escuros e peças mecânicas cinzas.

As imagens ausentes **não foram recuperadas do conteúdo perdido**. A substituição
é uma reconstrução simplificada. Do mesmo modo, encontrar uma imagem de nome
semelhante não garantia compatibilidade com as UVs da peça; por isso houve uma
segunda etapa de correção visual.

## Pintura, cauda e assentos

### Fuselagem e continuidade da traseira

A primeira correção remapeou as faces externas relevantes para a faixa limpa
de `I_base.png`, usando o material `Exterior_corrected`. Depois da comparação
visual solicitada pelo usuário, a pintura problemática foi substituída por uma
faixa simples branca e azul, no material `Exterior_continuous_blue`.

A imagem produzida nessa etapa é `textures/Exterior_white_navy.png`, com
1024 × 4 pixels. As primeiras 282 colunas receberam o azul extraído da imagem
`logo.png`; as demais receberam a cor clara. Ela não é uma nova pintura
baixada do site nem uma restauração exata da pintura original.

Para impedir que cada objeto separado reiniciasse a faixa em um ponto
diferente, a coordenada U foi calculada pela posição global do vértice no
Blender, ao longo do comprimento do avião:

```python
p = obj.matrix_world @ obj.data.vertices[obj.data.loops[li].vertex_index].co
uv[li].uv = ((p.x + 27.425108) / 37.973616, 0.15 + 0.7 * uv[li].uv.y)
```

Assim, fuselagem, traseira e portas recebem a mesma cor na mesma posição
longitudinal. A malha não precisou ser deformada para corrigir essa emenda.

### Símbolo da cauda

O material `Tail_logo_corrected` usa a imagem fornecida `logo.png`. A parte fixa
`Cylinder.007` e o leme separado `Cube.004` receberam uma projeção compartilhada
do símbolo, com espelhamento do U em um dos lados:

```python
u = (p.x + 26.15) / 4.3
v = (p.z - 2.55) / 4.3
if normal.y < 0:
    u = 1 - u
```

Essas fórmulas pertencem à cena de preparação no Blender, antes da conversão
dos eixos ao exportar. Nesse estágio, a altura está em Z; o OBJ do jogo usa Y
para cima. As fórmulas não devem ser copiadas como coordenadas do mapa.

### Bancos dos passageiros

O material `Fabric Demin` não tinha os nós de imagem necessários. Foram
conectadas as imagens disponíveis `Fabric_Demin_diffuse.png`,
`Fabric_Demin_roughness.png` e `Fabric_Demin_normal.png`. Roughness e normal
foram tratadas como `Non-Color`, e a normal passou por um nó `Normal Map` com
força 0,35. As faces desse material no objeto `Cube.021` receberam projeção
planar conforme a componente dominante da normal, com repetição espacial do
tecido a cada 0,5 unidade.

O MTL preserva as referências:

```text
map_Kd textures/Fabric_Demin_diffuse.png
map_Ns textures/Fabric_Demin_roughness.png
map_Bump -bm 0.350000 textures/Fabric_Demin_normal.png
```

O renderer atual do jogo lê a imagem difusa, a cor e a opacidade. Os mapas de
normal e rugosidade configurados no Blender não são interpretados pelo desenho
atual em OpenGL. Portanto, a aparência do tecido dentro do jogo pode ser menos
detalhada que no preview do Blender. Os painéis disponíveis do cockpit foram
preservados durante a preparação.

## Exportação utilizada e validação registrada

O cabeçalho dos arquivos finais identifica o Blender **5.2.3 LTS Release
Candidate**. Foram selecionadas as malhas, incluindo o interior, e excluídas da
exportação as câmeras e luzes temporárias de conferência. A chamada registrada
na preparação foi:

```python
bpy.ops.wm.obj_export(
    filepath=str(root / 'aircraft.obj'),
    export_selected_objects=True,
    export_triangulated_mesh=True,
    export_uv=True,
    export_normals=True,
    export_materials=True,
    path_mode='RELATIVE',
    forward_axis='NEGATIVE_Z',
    up_axis='Y',
)
```

O OBJ final tem **Y para cima e nariz em +X**, sem normalização para uma caixa
unitária. Os limites no sistema exportado são aproximadamente:

```text
min = (-27.425108, -3.342473, -17.178679)
max = ( 10.548508,  8.196299,  17.178555)
```

Isso corresponde a cerca de 37,97 unidades de comprimento, 11,54 de altura e
34,36 de envergadura. O jogo usa essa escala em metros. Os limites de
`repair_report.json` foram registrados na orientação anterior à exportação;
não devem ser confundidos com os eixos do OBJ.

O OBJ foi reimportado numa cena vazia e suas imagens foram recarregadas.
`validation.json` registra **522 malhas, 27.569 vértices, 51.022 faces, somente
triângulos e nenhuma imagem ausente**. As 525 malhas registradas durante a
preparação incluem três objetos sem faces exportáveis, explicando a diferença.
Essas contagens são do asset reimportado; não são contagens de chamadas de
desenho, pois o renderer do jogo agrupa faces por material.

## Reexportar a partir do `.blend` versionado

Não é necessário baixar novamente o modelo para ajustar ou reexportar a versão
que o jogo usa. Abra `aircraft.blend` mantendo `textures/` ao lado e confira os
materiais antes de exportar. Para uma execução por linha de comando, salve o
exemplo abaixo como `build/reexport_a320.py` e execute a partir da raiz do
repositório. **A pasta de saída escolhida precisa ainda não existir**; o exemplo
não sobrescreve o `.blend`, o OBJ ou as texturas originais desta pasta.

```python
import bpy
import shutil
import sys
from pathlib import Path

source = Path(bpy.data.filepath).resolve().parent
output = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
output.mkdir(parents=True, exist_ok=False)
shutil.copytree(source / 'textures', output / 'textures')

for image in bpy.data.images:
    if image.source == 'FILE':
        name = Path(bpy.path.abspath(image.filepath)).name
        target = output / 'textures' / name
        if not target.is_file():
            raise RuntimeError('Imagem ausente: ' + name)
        image.filepath = str(target)
        image.reload()

bpy.ops.object.select_all(action='DESELECT')
meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
if not meshes:
    raise RuntimeError('Nenhuma malha para exportar')
for obj in meshes:
    obj.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.wm.obj_export(
    filepath=str(output / 'aircraft.obj'),
    export_selected_objects=True,
    export_triangulated_mesh=True,
    export_uv=True,
    export_normals=True,
    export_materials=True,
    path_mode='RELATIVE',
    forward_axis='NEGATIVE_Z',
    up_axis='Y',
)
```

```powershell
# Substitua apenas o caminho do executável do Blender.
& 'C:\caminho\blender.exe' --background '.\data\aircraft\aircraft.blend' --python '.\build\reexport_a320.py' -- '.\build\a320_reexport'
```

O exemplo religa as imagens à **cópia** de `textures/` antes de exportar, de modo
que o MTL novo continue independente da pasta original. Ele não salva as
mudanças de caminho no `.blend` de entrada. As opções reproduzem a configuração
documentada; versões diferentes do Blender podem mudar organização, ordenação
ou escrita numérica, portanto não se promete igualdade byte a byte.

Para validar uma nova exportação, importe o novo `aircraft.obj` numa cena vazia,
usando os mesmos eixos (`NEGATIVE_Z`, `Y`). Recarregue as imagens dos materiais
e confira se todas têm dimensões válidas. Verifique faces triangulares e
compare as contagens com `validation.json`. Examine também a pintura externa,
a emenda nas portas, a cauda dos dois lados, os bancos e o cockpit; uma única
vista externa não cobre essas regiões. O OBJ, o MTL e a cópia de `textures/`
devem funcionar sem depender da cena original ainda aberta no Blender.

## Limites da reprodução desde o download bruto

As etapas iniciais de reparação usaram `build/fix_aircraft.py`,
`build/refine_aircraft.py` e `build/validate_aircraft.py`. Esses arquivos estão
presentes na máquina de preparação e são descritos em `implementation.md`, mas
**não estão versionados**: `build/` é ignorada pelo Git. Eles também contêm
caminhos locais e não constituem um pipeline portátil distribuído com o projeto.

Assim, o repositório permite reexportar a versão corrigida a partir do `.blend`
e inspecionar seus resultados, mas não contém todos os insumos e scripts
necessários para refazer automaticamente a reparação completa a partir de um
novo download bruto. As fórmulas e os relatórios acima registram o procedimento
conhecido sem apresentar essa etapa histórica como integralmente reproduzível.
O `LEIA-ME.txt` informa que os arquivos originais do download não foram alterados;
eles ficaram separados do conjunto corrigido incluído no projeto.

Para identificar exatamente os principais arquivos corrigidos documentados
aqui, os SHA-256 dos **conteúdos versionados no Git** nesta versão são:

| Arquivo | SHA-256 |
| --- | --- |
| `aircraft.obj` | `86ef1494bcd5a2fbcf5419b047bfd198f0abfb219fd123c534c708f353a90eba` |
| `aircraft.mtl` | `2fc4ea78dfc9da303b8425acc65149e73ff71e0601a04d68ef9e814630b99878` |
| `aircraft.blend` | `2bf2ded61f629f70a201e978650edce6bfbb9aa7e6b4fd40224de3c721451441` |

Esses hashes foram calculados a partir de `git show HEAD:caminho/do/arquivo`,
sem conversão de quebras de linha do checkout. Configurações como `core.autocrlf`
podem mudar os bytes do OBJ/MTL na cópia de trabalho sem mudar sua geometria ou
materiais. Os hashes identificam os resultados versionados, **não** o arquivo
original baixado do Sketchfab.
