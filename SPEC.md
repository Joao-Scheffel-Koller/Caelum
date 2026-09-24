# Especificação da Implementação

> [!CAUTION]
> - Você <ins>**não pode utilizar ferramentas de IA para escrever esta
>   especificação**</ins>

> [!WARNING]
> - Após a entrega da primeira versão completa, esta especificação não
>   poderá ser alterada. A implementação final deverá corresponder ao que
>   estiver descrito neste arquivo.

## Integrantes da dupla

- **Aluno 1 - Nome**: João Luis Scheffel Koller
- **Aluno 1 - Cartão UFRGS**: 00589701

- **Aluno 2 - Nome**: Vítor Santana Feijó
- **Aluno 2 - Cartão UFRGS**: 00588403

## Detalhes do que será implementado

- **Título do trabalho**: Caelum
- **Parágrafo curto descrevendo o que será implementado**: 
O nosso trabalho  será a implementação de um simulador de voo em avião de física simples (não há a pretensão de ela ser muito realista). Inicialmente, o avião do jogador é posicionado em um aeroporto (aqui chamado de "A") e seu objetivo é decolar, voar e aterrisar em outro aeroporto (aqui chamado "B"). Haverá três perspectivas de gameplay: a do piloto em primeira pessoa na qual será possivel mover-se e ver toda a cabine(cockpit). Adicionalmente, haverá a perspectiva em terceira pessoa do avião, a qual será uma câmera look-at focada na aeronave, acompanhando seu movimento. Por fim será implementado uma visão "God mode", em que será possivel mover-se livremente pelo cenário (independentemente da posição do avião). No mundo, haverá mais de uma pista de decolagem e pouso para que o avião se desloque de um ponto do mapa para outro.
A movimentação do avião será controlada pelo teclado: "W" e "S" controlarão o Throttle da aeronave (respectivamente, mais e menos aceleração), e o mouse controlará para onde o avião estará mirando. Por fim, caso o botão direito do mouse estiver pressionado, a movimenação do dipositivo passará a controlar a câmera e não mais o sentido e a direção do avião.

## Especificação visual

### Vídeo - Link

> [!IMPORTANT]
> - Coloque aqui um link para um vídeo que mostre a aplicação gráfica
>   de referência que você vai implementar. **Sua implementação deverá
>   ser o mais parecido possível com o que é mostrado no vídeo (mais
>   detalhes abaixo).**
> - **Você não pode escolher como referência: (1) algum trabalho realizado
>   por outros alunos desta disciplina, em semestres anteriores. (2) Minecraft.**
> - Por exemplo, você pode colocar um vídeo de um jogo que você gosta,
>   e seu trabalho final será uma re-implementação do jogo.
> - O vídeo pode ser um link para YouTube, Google Drive, ou arquivo mp4 dentro
>   do próprio repositório. Mas, garanta que qualquer um tenha
>   permissão de acesso ao vídeo através deste link.

https://www.youtube.com/live/HXZ2hlLK9Qg?si=3gEkvlRpy14oUR3d

### Vídeo - Timestamp

> [!IMPORTANT]
> - Coloque aqui um **intervalo de ~30 segundos** do vídeo acima, que
>   será a base de comparação para avaliar se o seu trabalho final
>   conseguiu ou não reproduzir a referência.

- **Timestamp inicial**: 1:38
- **Timestamp final**: 2:08

### Imagens

> [!IMPORTANT]
> - Coloque aqui **três imagens** capturadas do vídeo acima, que você
>   irá usar como ilustração para as explicações que vêm abaixo.
> - As imagens devem estar armazenadas neste repositório, no diretório
>   `images/spec/`, com os nomes `image1`, `image2` e `image3`.
> - Cada imagem deve usar o formato `.jpg` ou `.png`. Ajuste a extensão
>   nos vínculos abaixo para que corresponda ao arquivo armazenado.
> - Escolha imagens que correspondam a momentos do intervalo indicado
>   acima ou que sejam relevantes para a comparação com a implementação.

#### Imagem 1

- **Descrição**: Decolagem do avião e câmera look-at

![Imagem 1](images/spec/image1.png)

#### Imagem 2

- **Descrição**: Visão de dentro do cockpit

![Imagem 2](images/spec/image2.png)

#### Imagem 3

- **Descrição**: Câmera livre no video

![Imagem 3](images/spec/image3.png)

## Especificação textual

Para cada um dos requisitos abaixo (detalhados no [Enunciado do Trabalho final - Moodle](https://moodle.ufrgs.br/mod/assign/view.php?id=6302370)), escreva um parágrafo **curto** explicando como este requisito será atendido, apontando itens específicos do vídeo/imagens que você incluiu acima que atendem estes requisitos.

### Malhas poligonais complexas

- Serão formados por malhas poligonais complexas os seguintes objetos da cena: aviões, pilotos, a cabine de comando(cockpit) e as árvores do cenário.

### Transformações geométricas controladas pelo usuário

Um exemplo de transformação geométrica controlada pelo usuário será controlar o avião por meio do WS no teclado e mouse. Inclinações de "pitch", "yaw" e "roll" da aeronave também serão modeladas e controladas pelo jogador por intermédio de transformações geométricas.

### Diferentes tipos de câmeras

Haverá 3 câmeras:
- Câmera em primeira pessoa dentro do cockpit do avião (câmera fixa livre).
- Câmera em terceira pessoa na perspectiva do avião (câmera look-at).
- Câmera totalemente livre ("God mode").

### Instâncias de objetos

Haverá aviões instanciados possivelmente nos aeroportos e em voo no céu. Além disso, várias árvores estarão espalhadas pelo mundo.

### Testes de intersecção
Aviões poderão se colirdir com os predios do aeroporto, terreno e arvóres inapropriadamente, o que representaria um acidente. Vale ressaltar que o teste de colisão com o terreno (solo) também servirá para posicionar a aeronave sobre o solo em uma situação regular pré- ou pós-voo.

### Modelos de Iluminação em todos os objetos

- Será utilizado o modelo de reflexão de Phong.
- Comentário: também implementaremos as luzes piscantes da aeronave presente no vídeo.

### Mapeamento de texturas em todos os objetos

Os seguintes objetos terão textura: Aviões, cabine de comando (cockpit), pista de pouso e decolagem, prédios do aeroporto, grama e árvores. 

### Movimentação com curva Bézier cúbica

- Haverá pelo menos um outro avião em voo pelo céu seguindo uma curva de Bézier em sua trajetória com uma direção de movimento que simula a movimentação de um avião real, i.e. movimentos plausíveis para uma aeronave. (para simular um contexto real de vias aéreas sobre uma cidade, já que, geralmente, um número maior que 1 de aeronaves sobrevoam um espaço aéreo concomitantemente).

### Animações baseadas no tempo ($\Delta t$)

- Arvóres movimentar-se-ão para simular o efeito do vento.
- A animação tanto do avião controlado pelo jogador quanto do segundo avião voando pelo céu será igualmente baseada no tempo.

### Funcionalidade extra obrigatória

> [!IMPORTANT]
> - Descreva a funcionalidade extra relacionada à Computação Gráfica
>   que será implementada.
> - Esta funcionalidade também deverá ser documentada no arquivo
>   `README.md` da entrega final.


- Interface Gráfica: Botões em uma espécie de menu servirão para escolher qual avião o usuário deseja pilotar.
- Além disso, Instrumentos/Sensores da aeronave  apresentarão (não diretamente no cockpit, como em um caso real, mas sobre a tela do simulador):
  - altitude
  - velocidade
  - potência do motor
  - bússola (sentido de voo)
  
  OBS: os instrumentos terão design com ponteiros dinâmicos similar a medidores reais, conforme é apresentado no vídeo de referência.


## Limitações esperadas

> [!IMPORTANT]
> - Coloque aqui uma lista de detalhes visuais ou de interação que
>   aparecem no vídeo e/ou imagens acima, mas que você **não pretende
>   implementar** ou que você **irá implementar parcialmente**.
> - Para cada item, **explique por que** não será implementado ou por
>   que será implementado parcialmente.

- Não haverá um minimapa, já que o terreno não disporá de regiões e ambientes significamente distintos como cidades, regiões únicas, etc.

- colisões não deformarão modelos, devido a alta complexidade do cálculo das deformações de objetos baseado no seu formato e em seus materiais.

- Interface gráfica mais simples que a apresentada no vídeo, já que nossa aplicação terá um número significativamente menor de funcionalidades e opções de jogo do que a apresentada no vídeo (e.g. jogo online em servidores globais, voo por regiões reais do planeta terra, física mais realista, etc). 

- Não haverá modelagem de sombras, tendo em vista que escolhemos uma outra opção de funcionalidade extra e buscamos desenvolver uma aplicação mais leve de ser executada.

- A física de voo será simplificada. Entre outros aspectos, os seguintes pontos serão desconsiderados:
Peso do avião, resistência e arrasto do ar, distribuição de forças de sustentação pelo corpo e diferentes regiões da asa da aeronave, entre outros.

  Decidimos isso devido à alta complexidade inerente à modelagem desses fenômenos físicos que foge ao escopo da disciplina e requer conhecimentos mais avançados de física superior aplicada à aviação.

- O cockpit não será interagível como com botões pressionáveis ou com display HUD presentes no vídeo, pois o realismo da simulação foge ao escopo da disciplina e requer conhecimentos mais profundos sobre aviação e aviônica.
