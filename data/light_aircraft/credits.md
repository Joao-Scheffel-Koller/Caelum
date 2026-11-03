# Low Poly Airplane — monomotor leve

Modelo original **Low Poly Airplane**, por **MagicGames**.

- Página original: https://magic-games.itch.io/low-poly-airplane
- Licença: **Creative Commons Attribution 4.0 International (CC BY 4.0)**,
  https://creativecommons.org/licenses/by/4.0/
- Atribuição: “Low Poly Airplane” by MagicGames is licensed under CC BY 4.0.
  Modified for Caelum: orientation, approximate scale, transparent windows and
  propeller blades, and a simple cockpit interior.
- A licença original acompanha `source/License.txt`. O arquivo `source/Airplane.fbx`
  é o download original sem alteração, obtido na opção gratuita do autor.

Este é um monomotor estilizado de asa alta com trem convencional (duas rodas
principais e uma de cauda). **Não é uma reprodução de um Cessna específico.**

## Adaptações para o projeto

Conversão assistida por IA usando Blender, sem executar scripts do arquivo
baixado. A ferramenta reproduzível é `tools/convert_asset.py`; ela lê o FBX
original e gera `aircraft.obj`, `aircraft.mtl` e `metadata.json`.

- Eixos do OBJ: frente `+X`, cima `+Y`, direita `+Z`.
- Dimensões aproximadas de jogo: 9 m de comprimento, 11 m de envergadura e
  3 m de altura. Não são medidas aeronáuticas certificadas.
- Centro da caixa em XZ próximo de zero e base das rodas principais em `Y=0`.
- Vidros originalmente azuis e opacos foram tornados transparentes.
- Foram adicionados um painel simples, piso e dois bancos à cabine. Não há
  instrumentos interativos; os instrumentos do jogo permanecem no HUD.
- As pás da hélice são semitransparentes para não bloquear a câmera. A hélice
  **ainda não possui animação de rotação**; o cubo central permanece opaco.
- Materiais mantêm as cores originais, com as superfícies procedurais e a
  iluminação do renderizador do projeto. Nenhuma nova imagem de textura é exigida.

A câmera sugerida fica em `(2.44, 2.40, -0.26)` no sistema local do OBJ. A
geometria e as posições aproximadas das rodas estão registradas em
`metadata.json` para calibrar câmeras e volumes físicos. O trem de cauda tem
raio diferente das rodas principais; o perfil físico deve usar essas posições,
em vez de reutilizar as proporções do A320.

## Reproduzir a conversão

Com Blender instalado, a partir da raiz do projeto:

```powershell
blender --background --factory-startup --python data/light_aircraft/tools/convert_asset.py
```

O arquivo OBJ é escrito diretamente nas coordenadas esperadas pelo jogo, sem
normalização automática em execução. `metadata.json` inclui os hashes SHA-256
do arquivo ZIP original e do FBX, a licença, as alterações e os limites da malha.
O resultado possui 1.094 triângulos, incluindo as adições da
cabine. O FBX de origem pode conter dados que não são usados pelo jogo; o jogo
carrega somente o OBJ e o MTL convertidos.

## Conferência

Conversão executada com Blender 5.2.3 LTS Release Candidate. As prévias externas
e de dentro da cabine foram renderizadas e inspecionadas localmente. O teste
no OpenGL do jogo continua sendo necessário para conferir materiais e câmeras
no mesmo caminho de renderização utilizado em execução.
