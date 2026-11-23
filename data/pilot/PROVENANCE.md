# Piloto: origem, preparação e reprodução do asset

Registro técnico escrito com auxílio de IA (Claude Code). Este arquivo documenta
os assets desta pasta; não substitui o relatório final da dupla.

## Fonte e licença

O modelo foi escolhido e baixado manualmente pelo usuário no Sketchfab, no
formato **GLB**:

> "Low-poly PSX Pilot flying a plane (animated)"
> (https://sketchfab.com/3d-models/low-poly-psx-pilot-flying-a-plane-animated-96ae6f2dc4864a2bacd7a39b7d4ae5d9)
> by Jungle Jim (https://sketchfab.com/jungle_jim) is licensed under Creative
> Commons Attribution 4.0 (http://creativecommons.org/licenses/by/4.0/).

Autor, título, link e licença estão gravados nos metadados do próprio GLB
(`asset.extras`). O arquivo baixado,
`low-poly_psx_pilot_flying_a_plane_animated.glb`, tem SHA-256
`d0d141b38aaac9e0e8fdf121a2f381c170df866bf6ed1e01c8fe38d75137bb47` e fica
fora do repositório. Os arquivos desta pasta são uma **versão modificada**
(ver abaixo); o crédito ao autor original deve acompanhar sua redistribuição.

## O que está nesta pasta

| Arquivo | Função |
| --- | --- |
| `pilot.obj` | Geometria (800 triângulos), UVs e normais, na pose sentada. |
| `pilot.mtl` | Material único `pilot`, com `map_Kd pilot_diffuse.jpg`. |
| `pilot_diffuse.jpg` | Textura de cor (512 × 512) extraída do GLB, sem alteração. |
| `pilot_preview.png` | Conferência das UVs: vista de frente e de lado. |
| `extract_pilot_textures.py` | Script que extraiu a textura e corrigiu o MTL. |

## Preparação

1. **Blender (feito pelo usuário):** importação do GLB e exportação como OBJ.
   O modelo é animado (esqueleto e animação "Scene"); o OBJ guarda apenas a
   pose do quadro exportado, que já é a pose sentada com as mãos à frente.
2. **Textura perdida:** o material do GLB usa a extensão
   `KHR_materials_pbrSpecularGlossiness`, que o importador do Blender não liga
   ao material. Por isso o MTL exportado não tinha `map_Kd` (o piloto ficaria
   todo cinza).
3. **`extract_pilot_textures.py` (gerado com IA):** lê o GLB diretamente (só
   biblioteca padrão do Python), grava as imagens embutidas e reescreve o MTL:
   `Kd 1 1 1` (para não escurecer a textura), `Ks 0` (o GLB tem
   `specularFactor 0`) e `map_Kd pilot_diffuse.jpg`.
   ```
   python3 -I extract_pilot_textures.py low-poly_psx_pilot_flying_a_plane_animated.glb Untitled.mtl
   ```
4. **Cópia para o projeto:** `Untitled.*` renomeados para `pilot.*`, com
   `mtllib`, objeto e material renomeados para `pilot`.

O GLB também traz um **mapa de normais** (PNG 512 × 512). Ele não foi incluído:
o shader do jogo não usa mapas de normais e, num modelo low-poly, a diferença
seria pequena.

## Medidas para posicionar no avião

Unidades em metros, eixos do OBJ:

| Medida | Valor |
| --- | --- |
| Frente do piloto | **+Z** (joelhos e mãos para +Z); para cima: **+Y** |
| Altura sentado | 1,43 m (Y de −0,03 a 1,41) |
| Pés | Y ≈ 0 |
| Apoio no assento (parte de baixo do quadril) | Y ≈ 0,40, Z ≈ 0,11 |
| Costas (ponto mais atrás) | Z ≈ −0,17, Y ≈ 0,99 |
| Centro da cabeça | (0,02; 1,29; 0,05) |

Os aviões do jogo têm o nariz em **+X**. Para sentar o piloto, a matriz Model
precisa girar +Z para +X (rotação de 90° em Y) e transladar o apoio do quadril
até o assento de cada avião.

## Verificação

`tests/PilotAssetTest.cpp` (teste `pilot_asset`) carrega o OBJ com o mesmo
carregador do jogo (tinyobj) e confere: material com `map_Kd` e arquivo
presente, UVs e normais em todos os vértices, UVs dentro da textura, ~800
triângulos, altura ~1,43 m em metros, pés em Y ≈ 0 e frente para +Z.
