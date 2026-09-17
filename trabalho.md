# Trabalho Final de Computação Gráfica e Visualização I

## Objetivo

O objetivo deste trabalho é consolidar seu conhecimento sobre a representação e visualização de objetos bidimensionais (2D) e tridimensionais (3D) através do desenvolvimento de uma **Aplicação Gráfica** (programa de computador). Você irá exercitar os conceitos básicos de Computação Gráfica, como visualização em ambientes tridimensionais, interação, detecção de colisão, e utilização de texturas, entre outros. Opcionalmente, você pode integrar conceitos de Visualização no seu trabalho.

## Resumo dos Prazos

| Prazo | O quê |
|-------|-------|
| **21 de setembro de 2026** | Preenchimento do Google Forms em https://forms.gle/oWV6qgL7yTykq8xN8 |
| **21 de setembro de 2026** | Preenchimento do arquivo `SPEC.md` no repositório GitHub |
| **4 e 11 de novembro de 2026** | Avaliação parcial dos trabalhos finais, em laboratório |
| **7 e 9 de dezembro de 2026** | Apresentação dos trabalhos finais, para toda a turma |
| **11 de dezembro de 2026 (sexta-feira)** | Entrega final do trabalho completo |

## Especificação

Você deve desenvolver uma **Aplicação Gráfica** de sua escolha que preencha todos os requisitos descritos na lista abaixo. A nota final do trabalho será calculada com base nestes requisitos, portanto é importante que você se esforce para atingi-los.

### Exemplos de Aplicações

Exemplos de Aplicações Gráficas desenvolvidas por alunos em semestres anteriores podem ser vistos na seguinte playlist do YouTube:

📺 [Playlist - Trabalhos de Computação Gráfica - INF/UFRGS](https://www.youtube.com/playlist/CGVIS)

## Requisitos
## Requisitos

- O trabalho deve ser desenvolvido em **duplas**.

- Parte da entrega do trabalho final de CGVis será através do **Git** (hosting no GitHub). Assim, o professor poderá colaborar através de revisão de código (dado um commit específico), pull requests, etc., caso a dupla precise de alguma ajuda durante o desenvolvimento do trabalho.

- Até o dia **21 de setembro de 2026**, preencha o formulário do link abaixo informando os integrantes de sua dupla e uma breve descrição de qual será a aplicação gráfica desenvolvida, além de outras informações sobre a entrega do trabalho final. O formulário deve ser preenchido por somente um dos integrantes da dupla.
  - **Link para o formulário**: https://forms.gle/oWV6qgL7yTykq8xN8
  - ⚠️ Duplas que não seguirem à risca a instrução acima poderão sofrer desconto de nota.

- A primeira parte obrigatória do trabalho será criar um arquivo de especificação, chamado `SPEC.md`, no seu repositório Git.
  - Você deve seguir o template existente no seguinte repositório: https://github.com/cgvis-inf-ufrgs/cgvis-base-trabalho-final
  - O repositório no GitHub deve conter imagens de referência visual do que será implementado, e essas imagens devem estar vinculadas no arquivo `SPEC.md`.
  - A primeira versão completa deste arquivo deve estar preenchida **até o dia 21 de setembro de 2026**, para revisão do professor.

- **Importante**: Após a revisão do professor e a realização das correções solicitadas, o arquivo `SPEC.md` não poderá mais ser alterado. Isto é, você precisará implementar exatamente o que foi especificado.

### Regras MUITO Importantes

1. **Linguagem de Programação**
   - Você pode utilizar a linguagem de programação de sua escolha.

2. **APIs Gráficas**
   - O código deve utilizar **obrigatoriamente** somente APIs gráficas de **baixo nível** (OpenGL, Direct3D, Metal, WebGPU ou Vulkan).
   - ❌ **NÃO** podem ser utilizados motores gráficos (graphic engines) de mais alto nível, incluindo mas não limitado a: Unity, Unreal, Godot, raylib, pygame, Ogre, Three.js, Bevy, entre outros.
   - ✅ **Recomenda-se** o uso de **C++ com OpenGL**. Isto possibilitará a reutilização de código desenvolvido em nossas aulas práticas (laboratórios).

3. **Uso de Ferramentas de IA**
   - Você pode utilizar livremente ferramentas de IA para geração de código neste trabalho.
   - **PORÉM**, você obrigatoriamente precisa:
     - Separar no histórico do Git os commits gerados por IA de commits com código gerado/editado por você.
     - Todo commit com código gerado por IA deve ter a mensagem do commit iniciada por **`"IA: ..."`**
     - Incluir no repositório o **prompt utilizado para gerar o código**. Isso pode ser feito em um arquivo `PROMPTS.md`, ou o prompt pode ser colocado diretamente na mensagem do commit, com um identificador **`"PROMPT:"`** em maiúsculo.

4. **Código de Terceiros**
   - Se você copiar algum trecho de código de algum lugar, precisa:
     - Respeitar a licença de uso do código
     - Identificar o código com um comentário
     - Todo código pronto utilizado deve estar devidamente identificado através de comentários no código fonte entregue
     - Listar a **FONTE** de onde foi retirado cada trecho de código (a palavra **"FONTE"** deve estar em maiúsculo, para fácil localização)

5. **Assets (Imagens, Modelos, Texturas, Fontes, Áudio)**
   - Somente poderão ser adicionados ao repositório público no GitHub aqueles para os quais a dupla possua permissão de uso e redistribuição, de acordo com a respectiva licença.
   - Se a aplicação depender de algum arquivo extra que não possa ser incluído no GitHub por qualquer motivo:
     - O `README.md` deverá identificar **explicitamente** esse arquivo
     - Explicar como obtê-lo (ex: Moodle, fonte original, Google Drive)
     - Respeitar a licença do arquivo

6. **Plágio**
   - ⚠️ **GRAVE**: Qualquer utilização de código pronto ou de IA que não respeitar estas regras será considerado **plágio** e o trabalho receberá **nota zero**.
   - ⚠️ **GRAVE**: Qualquer cópia de código de trabalhos de colegas deste ou de semestres anteriores será considerado **plágio** e o trabalho receberá **nota zero**.

7. **Histórico Git**
   - O histórico do Git deve demonstrar um **avanço incremental** da implementação.
   - ❌ Não é aceitável um histórico que seja um commit único que inclui 100% do código já na sua versão final.
   - **Ambos os integrantes da dupla** devem possuir commits listados no histórico do Git.
   - Caso o trabalho seja desenvolvido com "pair programming," os integrantes da dupla devem se revezar em quem estará escrevendo o código e criando os commits.

## Requisitos Técnicos Obrigatórios

- **Interação em Tempo Real**
  - Sua aplicação deve possibilitar interação em tempo real.
  - Por exemplo, se você desenvolver um jogo, ele não pode ser "lento" a ponto de impactar negativamente a jogabilidade.

- **Objetivo e Lógica de Controle**
  - Sua aplicação deve possuir algum objetivo e lógica de controle **não-trivial**.
  - Por exemplo, um jogo de computador possui uma lógica não-trivial.
  - Uma aplicação que simplesmente carrega um modelo geométrico 3D e permite sua visualização é **trivial** e **não será aceita**.

- **Matrizes de Transformação**
  - Sua aplicação deve utilizar as matrizes que vimos em aula para:
    - **Model matrix**: Transformações geométricas
    - **Projection matrix**: Projeções
    - **View matrix**: Especificação do sistema de coordenadas da câmera
  - ❌ Você **NÃO** pode utilizar bibliotecas existentes para o cálculo de câmera, transformações, etc.
  - ❌ As seguintes funções **NÃO** podem ser utilizadas:
    ```
    gluLookAt(), gluOrtho2D(), gluPerspective(), gluPickMatrix(),
    gluProject(), gluUnProject(), glm::lookAt(), glm::ortho(),
    glm::perspective(), glm::pickMatrix(), glm::rotate(),
    glm::scale(), glm::translate(), dentre outras.
    ```

- **Interação com Usuário**
  - Sua aplicação deve possibilitar interação com o usuário através do **mouse** e do **teclado**.

### Componentes Gráficos Obrigatórios

Sua aplicação deve incluir implementação dos seguintes conceitos de Computação Gráfica:

1. **Malhas Poligonais Complexas**
   - Objetos virtuais representados através de malhas poligonais complexas (malhas de triângulos).
   - Quanto maior a variedade de modelos geométricos, melhor.
   - 💡 Veja a seção "Modelos 3D e Texturas Disponíveis na Web" na página principal de nosso Moodle para uma lista de locais onde você pode obter modelos 3D.

2. **Transformações Geométricas**
   - Transformações geométricas de objetos virtuais.
   - Através da interação com o teclado e/ou mouse, o usuário deve poder controlar transformações geométricas aplicadas aos objetos virtuais (não somente controle da câmera).

3. **Câmeras Virtuais**
   - Controle de câmeras virtuais.
   - No **mínimo** sua aplicação deve implementar **dois tipos de câmera consideravelmente distintos**.
   - Por exemplo, uma câmera look-at e uma câmera livre.

4. **Instâncias de Objetos**
   - No mínimo um objeto virtual deve ser copiado com duas ou mais instâncias, isto é, utilizando duas ou mais Model matrix aplicadas ao mesmo conjunto de vértices.

5. **Testes de Intersecção (Colisões)**
   - Testes de intersecção entre objetos virtuais.
   - Estes testes devem ter algum propósito dentro da lógica de sua aplicação.
   - Exemplo: em um jogo de corrida, o modelo virtual de um carro não pode atravessar a parede, e para tanto é necessário testar a intersecção entre estes dois objetos de modo a evitar esta intersecção.
   - Os testes de colisão devem ser implementados em um arquivo separado, nomeado `collisions.cpp` para projetos em C++. Para outras linguagens de programação, utilize a extensão correspondente.

6. **Iluminação**
   - Modelos de iluminação de objetos geométricos.
   - **Todos objetos** devem possuir algum tipo de iluminação **não-trivial**.

7. **Mapeamento de Texturas**
   - **Todos os objetos virtuais** de sua aplicação devem ter suas cores definidas através de texturas representadas por imagens (no mínimo **três imagens distintas**), ou por texturas procedurais.
   - ⚠️ Imagens de texturas "esticadas" de maneira não natural receberão **desconto de pontuação**.

8. **Curvas de Bézier**
   - No mínimo um objeto virtual de sua aplicação deve ter sua movimentação definida através de uma **curva de Bézier cúbica**.
   - O objeto deve se movimentar de forma suave ao longo do espaço em um caminho curvo (não reto).

9. **Animação Baseada no Tempo**
   - Animação de Movimento baseada no tempo.
   - **Todas as movimentações de objetos** (incluindo da câmera) devem ser computadas baseado no tempo (isto é, movimentações devem ocorrer sempre na mesma velocidade independente da velocidade da CPU onde o programa está sendo executado).

## Funcionalidades Extras
## Funcionalidades Extras

A dupla deve implementar **no mínimo uma funcionalidade extra** relacionada ao conteúdo da disciplina em sua aplicação. A funcionalidade pode ser escolhida entre as sugestões abaixo ou pode ser outra proposta pela dupla.

- A funcionalidade extra escolhida deve ser descrita no arquivo `SPEC.md` e documentada no relatório final (`README.md`).

### Sugestões de Funcionalidades Extras de Computação Gráfica e Interação

- ✨ Rasterização de curvas poligonais utilizando curvas de Bézier
- ✨ Rasterização de superfícies suaves utilizando patches de Bézier
- ✨ Sistema de partículas
- ✨ Sombras
- ✨ Animação esquelética (skinning)
- ✨ Billboards / Sprites
- ✨ Interface Gráfica (botões, etc.)
  - 💡 Sugestão de biblioteca: https://github.com/ocornut/imgui
- ✨ Rasterização de texto com fontes diversas
  - 💡 Sugestão de biblioteca: https://github.com/rougier/freetype-gl
- ✨ Seleção de objetos virtuais com o mouse (picking)
  - 💡 Exemplo de implementação: [Picking with custom Ray-OBB function](https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection)
- ✨ **<sua ideia aqui>** ...

### Outras Funcionalidades Extras Interessantes

As funcionalidades abaixo podem enriquecer a aplicação, mas **não contam como a funcionalidade extra obrigatória**, pois não estão diretamente relacionadas à Computação Gráfica:

- 🔊 Efeitos sonoros
  - 💡 Sugestão de biblioteca: https://github.com/mackron/miniaudio

## Apresentação Parcial

Nas aulas de laboratório dos dias **4 e 11 de novembro de 2026**, o professor irá passar por cada dupla no laboratório para que a dupla apresente uma versão em desenvolvimento do trabalho final, já compilando e executando, que já implemente alguns dos requisitos listados acima.

## Apresentação Final

Cada dupla irá apresentar seu trabalho para a turma nas aulas dos dias **7 e 9 de dezembro de 2026**. 

**Tempo de apresentação**: No máximo **10 minutos** por dupla.

**Ordem de apresentações**: Por sorteio nestes mesmos dias.

**Conteúdo da apresentação**:
- Obrigatoriamente deve conter a **demonstração da aplicação funcionando**
- Opcionalmente pode incluir uma **apresentação de slides**

**Equipamento**:
- Você pode utilizar o computador principal do laboratório ou conectar seu notebook pessoal no projetor.
- Se sua dupla for utilizar o computador do laboratório, **traga seu código fonte e binário compilado em um pen-drive**, para facilitar a cópia dos dados antes da apresentação.

**Qualidade de Avaliação**:
- A qualidade da apresentação do trabalho final, além da presença da dupla nos dias de apresentações de outros colegas, irá contar para a nota final do trabalho.
- Cada integrante da dupla irá receber **pontuação independente de participação**.
- ⚠️ Qualquer tipo de **plágio acarretará nota zero**.

## Entrega
## Entrega

Um dos integrantes da dupla deve fazer o envio no Moodle de um arquivo **ZIP**. Esse ZIP deve conter:

### 1. Arquivo `metadados-entrega.json`

Este arquivo deve conter os dados da entrega e apontar para o repositório com o código-fonte completo e documentado e para o local do vídeo.

**Formato**: JSON com as seguintes chaves:

| Chave | Descrição |
|-------|-----------|
| `titulo_trabalho` | Título do trabalho |
| `git_hash_commit_final` | Hash do commit final do trabalho |
| `url_github` | URL do repositório Git no GitHub |
| `local_do_video` | URL para o vídeo, URL para pasta em serviço de nuvem (Google Drive) ou caminho relativo para arquivo MP4 no ZIP |
| `periodo_letivo` | Período letivo da disciplina no formato `AAAA/S` |
| `integrantes` | Array com os integrantes da dupla |

**Cada integrante** deve possuir as seguintes informações:
- `nome`: Nome do integrante
- `cartao_ufrgs`: Número do cartão UFRGS
- `turma`: Turma (normalmente A, B ou U)
- `nome_de_usuario_no_github`: Nome de usuário no GitHub
- `email`: Email do integrante

**Exemplo de arquivo válido**:

```json
{
  "titulo_trabalho": "Título do Trabalho",
  "git_hash_commit_final": "a1b2c3d4e5f678901234567890abcdef12345678",
  "url_github": "https://github.com/usuario/repositorio",
  "local_do_video": "https://youtu.be/identificador-do-video",
  "periodo_letivo": "2026/2",
  "integrantes": [
    {
      "nome": "Nome do Primeiro Integrante",
      "cartao_ufrgs": "00123456",
      "turma": "A",
      "nome_de_usuario_no_github": "primeiro-integrante",
      "email": "primeiro.integrante@ufrgs.br"
    },
    {
      "nome": "Nome do Segundo Integrante",
      "cartao_ufrgs": "00987654",
      "turma": "A",
      "nome_de_usuario_no_github": "segundo-integrante",
      "email": "segundo.integrante@ufrgs.br"
    }
  ]
}
```

⚠️ **IMPORTANTE**: Realize um `git commit` seguido de um `git push` **ANTES** de obter o hash do seu commit final. Caso contrário o professor não conseguirá acessar seu commit final.

### 2. Arquivo do Vídeo (se necessário)

- Se `local_do_video` for um **caminho relativo**, o arquivo de vídeo em formato **MP4** deve estar incluído no ZIP.
- Se `local_do_video` contiver uma **URL publicamente acessível**, o arquivo do vídeo **não precisará** ser incluído no ZIP.

### Documentação Adicional

Além do conteúdo do ZIP, a entrega deve disponibilizar os seguintes itens:

#### 📄 Relatório `README.md` no GitHub

O relatório deve estar no GitHub e conter uma **descrição simples sobre o desenvolvimento do trabalho**.

⚠️ **Obs**: O uso de IA para escrever o relatório é **PROIBIDO**.

O relatório deve **obrigatoriamente incluir**:

1. **Descrição da Aplicação**
   - Um parágrafo descrevendo a aplicação que foi desenvolvida.

2. **Funcionalidade Extra**
   - Uma descrição da funcionalidade extra implementada pela dupla, conforme definida no arquivo `SPEC.md`.

3. **Contribuições da Dupla**
   - Um parágrafo listando as contribuições de cada membro da dupla para o trabalho.

4. **Uso de Ferramentas de IA**
   - Um parágrafo curto indicando se a dupla fez uso do ChatGPT (ou alguma outra ferramenta similar, como Claude, Gemini, LLaMa, GitHub Copilot, OpenAI Codex, etc.) para desenvolvimento do trabalho.
   - Descrevendo como a ferramenta foi utilizada e para quais partes do trabalho.
   - O parágrafo deve também incluir uma **análise crítica** descrevendo:
     - Quão útil a dupla achou a ferramenta
     - Onde ela auxiliou
     - Onde ela não auxiliou adequadamente

5. **Imagens de Funcionamento**
   - No mínimo **duas imagens** mostrando o funcionamento da aplicação.

6. **Manual de Utilização**
   - Um manual descrevendo a utilização da aplicação (atalhos de teclado, etc.).

7. **Instruções de Compilação e Execução**
   - Explicação de **todos os passos** necessários para compilação e execução da aplicação.

8. **Arquivos Extras Necessários**
   - Identificação de qualquer arquivo extra necessário que não esteja disponível no repositório GitHub.
   - Instruções explícitas para obtê-lo (ex: Moodle, fonte original, link para pasta no Google Drive).

#### 🎥 Vídeo de Apresentação

Um vídeo de **3 a 5 minutos** com uma apresentação do trabalho feito pela dupla, incluindo uma demonstração da aplicação gráfica.

**Ferramentas sugeridas para gravação**: OBS Studio

**Local de hospedagem**:
- ✅ Preferencialmente, utilize uma **URL para um vídeo no YouTube** (público ou "não listado")
- ✅ Também é permitido informar a **URL de uma pasta em um serviço de armazenamento em nuvem**, como o Google Drive
  - A dupla deve garantir que o link seja **publicamente acessível**
  - O professor deve conseguir abrir a pasta e assistir ao vídeo **sem solicitar permissão**
- ✅ Se você não quiser hospedar o vídeo na internet, pode incluí-lo em formato **MP4 no ZIP**, com compressão adequada
  - Informe seu **caminho relativo** na chave `local_do_video` (ex: `videos/apresentacao.mp4`)

### Prazo de Entrega

⏰ A entrega final **deve ser feita impreterivelmente até 23 horas e 55 minutos** do dia **11 de dezembro de 2026 (sexta-feira)**, utilizando o botão "Adicionar tarefa" no Moodle.

## Critérios de Avaliação

O professor irá utilizar uma planilha de avaliação com critérios similares ao que é mostrado abaixo:

### Critérios Técnicos (Nota da Dupla)
### Critérios Técnicos (Nota da Dupla)

- ✅ Interação em tempo real e desempenho adequado
- ✅ Objetivo e lógica de controle não triviais
- ✅ Interação através do mouse e do teclado
- ✅ Implementação própria das matrizes Model, View e Projection
- ✅ Malhas poligonais complexas
- ✅ Transformações geométricas de objetos controladas pelo usuário
- ✅ No mínimo duas câmeras consideravelmente distintas
- ✅ Instâncias de objetos
- ✅ Testes de intersecção com propósito na aplicação e implementados em `collisions.*`
- ✅ Modelos de iluminação não triviais em todos os objetos
- ✅ Mapeamento de texturas em todos os objetos
- ✅ Movimentação suave de pelo menos um objeto ao longo de um caminho curvo definido por uma curva de Bézier cúbica
- ✅ Animações baseadas no tempo ($\Delta t$)
- ✅ Implementação de funcionalidade extra obrigatória
- ✅ Ausência de bugs
- ✅ Estabilidade da aplicação durante a demonstração, sem encerramentos inesperados ou bugs que prejudiquem a apresentação
- ✅ Qualidade visual geral e similaridade às imagens de referência incluídas no arquivo `SPEC.md`

#### Exemplos de Bugs Não Desejáveis

- ❌ Z fighting
- ❌ Texturas esticadas
- ❌ Colisões que não funcionam corretamente
- ❌ Movimentação "travada" ou com "flickering"
- ❌ Entre outros

### Critérios de Entrega e Documentação (Nota da Dupla)

- ✅ Implementação de acordo com o arquivo `SPEC.md`
- ✅ Coerência entre o `SPEC.md`, a implementação e o `README.md`
- ✅ Completude e qualidade do relatório `README.md`
- ✅ Documentação da funcionalidade extra no `SPEC.md` e no `README.md`
- ✅ Arquivo `metadados-entrega.json` válido e completo
- ✅ Hash do commit final e URL do repositório GitHub válidos e acessíveis
- ✅ Vídeo acessível ao professor e com duração entre 3 e 5 minutos
- ✅ Facilidade de compilação e execução, com instruções corretas e reproduzíveis

### Critérios de Participação e Apresentação

- ✅ Qualidade da apresentação final *(nota da dupla)*
- ✅ Apresentações parciais nas datas previstas *(nota individual)*
- ✅ Participação nas apresentações finais *(nota individual)*
- ✅ Participação no desenvolvimento do código, de acordo com o histórico do GitHub *(nota individual)*

### Critérios de Conformidade

Os itens abaixo correspondem a regras **eliminatórias** ou sujeitas às **penalidades especiais** descritas anteriormente:

- ⚠️ Uso exclusivo de APIs gráficas de **baixo nível**, sem motores gráficos de alto nível
- ⚠️ **Ausência de plágio**
- ⚠️ **Identificação explícita** de código gerado por IA nos commits
- ⚠️ **Inclusão dos prompts** utilizados para geração de código
- ⚠️ **Identificação das fontes** de código e assets copiados e respeito às respectivas licenças
- ⚠️ **Histórico Git** demonstrando avanço incremental da implementação
- ⚠️ **Presença de commits** realizados pelos dois integrantes

---

**Versão**: 2026-09-16T14:44:04-0300