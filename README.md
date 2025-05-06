# Semáforo Inteligente com Modo Noturno e Acessibilidade

## Objetivo Geral

Criar um sistema multitarefas com o uso do **FreeRTOS**. Para isso, foi desenvolvido um semáforo com **modo noturno** onde cada periférico usado para representar o semáforo é implementado através de uma *task* separada. O estado do semáforo é exibido por diferentes periféricos:

- LED RGB
- Matriz de LEDs 5x5
- Display OLED SSD1306
- Buzzer
- Botão A

---

## Descrição Funcional

O sistema utiliza periféricos para representar visual e sonoramente o estado do semáforo.

Cada *task* verifica se o sistema está no **modo normal** ou **modo noturno**, e executa ações diferentes conforme o modo. A seguir, as funcionalidades de cada tarefa:

### `vButtonA`
- Controla o modo do semáforo (normal ou noturno).

### `vLedsRgb`
- Representa o semáforo usando um LED RGB.
- **Modo normal**: alterna entre vermelho, amarelo e verde a cada 2 segundos.
- **Modo noturno**: pisca amarelo (500ms ligado, 500ms desligado).

### `vMatrixLeds`
- Exibe o semáforo na matriz de LEDs 5x5.
- Funciona de forma similar à `vLedsRgb`.
- Utiliza as funções auxiliares:
  - `draw_on_matrix(...)`
  - `matrix_rgb(...)`

### `vDisplay`
- Exibe o semáforo no display OLED SSD1306.
- Mostra um retângulo dividido em três quadrados (verticalmente):
  - Vermelho (topo)
  - Amarelo (meio)
  - Verde (base)
- A letra inicial da cor atual (em inglês: R, Y, G) é exibida no quadrado correspondente.
- Utiliza a função auxiliar `draw_on_display(...)`.

### `vBuzzer`
- Emite sons com o buzzer de acordo com o estado do semáforo.
- No modo normal, a duração dos *beeps* muda conforme a cor atual.
- No modo noturno, segue um padrão diferente de som.

---

## Periféricos Usados

### SSD1306 (Display OLED)
- Usado como uma saída visual.
- Exibe um retângulo dividido em três seções (vermelha, amarela e verde).
- Indica a cor atual exibindo a letra da cor correspondente.

### Matriz de LEDs 5x5
- Segunda forma visual de representar o semáforo.
- **Modo normal**: alterna entre as três cores com mudança de posição.
- **Modo noturno**: exibe apenas a cor amarela piscando a cada 500ms.

### LED RGB
- Representa o estado do semáforo com cores.
- **Modo normal**: alterna entre vermelho, amarelo e verde.
- **Modo noturno**: pisca amarelo.

### Buzzer A
- Representação sonora.
- Emite sons com frequências distintas para cada cor.
- Adapta-se ao modo atual (normal ou noturno).

---

## Tasks

Cada tarefa é responsável por controlar um periférico, de forma concorrente, utilizando o **FreeRTOS** para garantir multitarefa eficiente.

---

## Compilação e Execução

1. **Pré-requisitos**:
   - Ter o ambiente de desenvolvimento para o Raspberry Pi Pico configurado (compilador, SDK, etc.).
   - CMake instalado.

2. **Compilação**:
   - Clone o repositório ou baixe os arquivos do projeto.
   - Navegue até a pasta do projeto e crie uma pasta de build:
     ```bash
     mkdir build
     cd build
     ```
   - Execute o CMake para configurar o projeto:
     ```bash
     cmake ..
     ```
   - Compile o projeto:
     ```bash
     make
     ```

3. **Upload para a placa**:
   - Conecte o Raspberry Pi Pico ao computador.
   - Copie o arquivo `.uf2` gerado para a placa.
