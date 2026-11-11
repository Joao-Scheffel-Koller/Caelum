# Atlas de fonte do texto na tela

Arquivos gerados com auxílio de IA (ver `prompts.md`, entrada 37) por
`tools/generate_font_atlas.py`:

- `dejavu_mono_atlas.png`: cobertura de 8 bits de cada caractere ASCII
  imprimível (32 a 126), rasterizada pelo FreeType (via Pillow) com o
  *hinting* da fonte, nos tamanhos inteiros de 8 a 32 px e em 36, 40, 44, 48,
  56 e 64 px.
- `dejavu_mono_atlas.txt`: posição de cada caractere no atlas e suas métricas
  (deslocamento, avanço, ascendente/descendente) para cada tamanho.

`src/ui/textrendering.cpp` escolhe o tamanho mais próximo do pedido e desenha
cada caractere 1:1, alinhado aos pixels. Sem esses arquivos, o jogo volta ao
atlas antigo de 18 px (`include/dejavufont.h`).

## Fonte e licença

- Fonte: **DejaVu Sans Mono 2.37** (`DejaVuSansMono.ttf`, pacote Debian/Ubuntu
  `fonts-dejavu-mono`, SHA-256
  `a54dca07c76d6289e717e75e0a58c0128f6d7269ef3faf76417c9d7d3bba37ab`).
- Projeto: https://dejavu-fonts.github.io/ — mesma família usada pelo atlas
  original da disciplina (freetype-gl).
- Licença: DejaVu Fonts License (Bitstream Vera Fonts License + alterações do
  DejaVu em domínio público). Permite uso, cópia, incorporação e
  redistribuição; fontes derivadas não podem usar os nomes "Bitstream" ou
  "Vera". Texto completo: https://dejavu-fonts.github.io/License.html

Para regenerar, a partir da raiz do repositório:

```bash
python3 -I tools/generate_font_atlas.py /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf data/fonts
```
