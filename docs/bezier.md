# Animação do avião secundário

Notas técnicas com auxílio de IA (Codex), registradas em `prompts.md`.
Este arquivo não é o relatório final dos alunos.

## Organização

- `src/animation/BezierPath.*`: avaliação cúbica pela base de Bernstein,
  derivadas e consulta por distância. Não depende de OpenGL ou aviões.
- `src/aircraft/BezierFlight.*`: estado da animação, Euler e orientação do NPC.
  Não usa o `FlightModel` nem controla o avião do jogador.
- `src/render/GameScene.*`: mantém uma única malha A320 na GPU e a desenha
  novamente com a matriz do segundo avião. Libera os buffers uma única vez.
- `src/main.cpp`: chama `scene->Update(elapsed)` uma vez por quadro, antes
  dos passes de desenho, usando o tempo real em precisão dupla.

## Curva e velocidade

Cada trecho usa quatro pontos. P0/P3 são extremos; P1/P2 são alças de controle.
A posição é B(u)=(1-u)^3 P0+3(1-u)^2 u P1+3(1-u)u^2 P2+u^3 P3.
A tangente e a segunda derivada são calculadas analiticamente.

O circuito inicial tem quatro trechos cúbicos que aproximam um círculo.
O centro é `(4100,180,7200)`, raio 1200 m e altitude constante de 180 m.
O avião começa em `(5300,180,7200)`, aproximadamente 1 km à frente da câmera
inicial do jogador, e voa a 60 m/s. A volta dura aproximadamente 126 s.
As posições e derivadas coincidem nas quatro junções, inclusive no fechamento.
O círculo é aproximado por cúbicas; não é uma circunferência analítica exata.

A integração usa Euler explícito na distância: `s(n+1)=s(n)+speed*dt`.
FONTE: Prof. Eduardo Gastal, `Aula_24_Animacao.pdf`, slides 10 e 18, fornecido
pelo usuário a partir do material da disciplina. O PDF é referência conceitual;
não é necessário para compilar ou executar a aplicação.

Como a velocidade escalar é constante, o avanço da distância é exato
matematicamente. A inversão de comprimento é aproximada: uma tabela com 128
amostras por trecho acumula distâncias de cordas; busca binária e interpolação
encontram o parâmetro u correspondente. A posição final é avaliada na cúbica,
evitando acumular deriva lateral por integração de posições XYZ.

A distância volta ao início por módulo do comprimento total. Para grandes
intervalos, voltas completas são removidas do dt antes da multiplicação,
preservando o avanço com velocidade constante. Não é usado o limite de dt da
física do jogador. Intervalos negativos, zero ou não finitos não avançam o NPC.
Ao minimizar a janela, o relógio é reinicializado como nas câmeras; a animação
pausa durante esse período.

## Orientação e desenho

O nariz local +X segue a tangente normalizada. Produtos vetoriais geram uma
base ortonormal com +Y para cima e +Z à direita. A curvatura lateral fornece
uma inclinação proporcional a `atan(speed²*curvatura/9.81)`, limitada a 25 graus;
isso é uma orientação visual, sem simulação de forças no NPC.

A matriz usa a base construída pelo programa e `Matrix_Translate` da aula.
O pivô do NPC é o centro da malha; o pivô das rodas do jogador permanece o mesmo.
Não são utilizados construtores de câmera ou transformação do GLM.

O NPC participa dos passes sólido, pintura e transparência como o restante do
mundo. No cockpit, pertence a `WORLD`; o passe próximo `AIRCRAFT` contém somente
o jogador. Dessa forma não é desenhado sobre o painel nem atualizado duas vezes.

## Verificação

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

`bezier_flight` verifica fórmula e derivadas, junções, fechamento, inversão de
comprimento, velocidade aproximada, poses equivalentes a 30/60/144 FPS, quadros
longos, base ortonormal, alinhamento do nariz, limite de inclinação e entradas
inválidas. Também compara posição, direção e inclinação imediatamente antes e
depois das quatro junções e a evolução por vários quadros ao longo de várias
voltas. Os testes usam apenas CPU e os módulos da animação.

A partir de `bin/Debug`, o modo de inspeção existente agora inclui o NPC:

```powershell
.\main.exe --texture-check ../../build/bezier-check
```

As 15 vistas incluem `bezier-start` e `bezier-later`, que acompanham o avião
secundário em instantes diferentes. O log registra suas posições e verifica
erros OpenGL em cada captura. O modo normal continua usando C/F/L para câmeras.
