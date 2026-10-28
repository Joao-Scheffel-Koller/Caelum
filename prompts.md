## File used to store ai prompts

1. formate o @file:trabalho.md  para ficar bonito em markdown

2. Codex (2026-10-28): dividir o OBJ do Rio em partes deterministicas por uma
   grade espacial de 2 x 2 km, subdividindo as regioes acima de 50 MB e
   preservando coordenadas, faces, materiais e texturas. Pedido: "faça isso ent,
   perfeito". Gerados com IA: data/rio/tools/split_obj.py,
   data/rio/tools/test_split_obj.py e a
   documentacao data/rio/README.md. Os OBJ menores sao derivados automaticamente
   do mapa OSM2World; identificar este auxilio no commit correspondente.
