# Projeto-final-Embarcatech
Este projeto consiste em um sistema de controle de posição para um servo motor utilizando um joystick analógico, baseado no microcontrolador Raspberry Pi Pico. O sistema inclui feedback visual via display OLED, alertas sonoros e customização de exibição através de botões.

# Funcionalidades
- Controle Preciso:
Mapeamento do movimento do joystick para a posição do servo motor de 0 a 180 graus.

- Interface Visual:
Exibição da posição atual do motor em tempo real no display SSD1306 via protocolo I2C.

- Alertas Sonoros: 
Acionamento de buzzers nos limites das extremidades do movimento do joystick.

- Inversão de Sentido: 
O botão do joystick permite inverter a lógica de direção do motor.

- Alternância de Unidades: 
O Botão A permite alternar a exibição no display entre a escala de 0 a 180 graus e a escala de -90 a 90 graus.

- Controle de Áudio: 
O Botão B permite ativar ou desativar os alertas sonoros dos buzzers.

# Componentes Utilizados
- Microcontrolador Raspberry Pi Pico

- Servo Motor Standard PWM

- Joystick Analógico

- Display OLED SSD1306 128x64

- 2 Buzzers Ativos/Passivos

- 2 Botões Pulsadores

# Configuração de Pinos

| Componente | Pino | Função |
| :--- | :--- | :--- |
| Servo Motor | 8 | Saída PWM 50 Hz |
| Joystick X | 27 | Entrada ADC (Leitura analógica) |
| Buzzer A | 21 | Alerta de fim de curso (Esquerda) |
| Buzzer B | 10 | Alerta de fim de curso (Direita) |
| Botão A | 5 | Alternar escala de unidade (-90 a 90) |
| Botão B | 6 | Mudo / Ativar som |
| Botão JS | 22 | Inverter direção de rotação |
| Display SDA | 14 | Comunicação I2C1 |
| Display SCL | 15 | Comunicação I2C1 |

# Detalhes Técnicos
- PWM e Duty Cycle
O servo motor opera em uma frequência de 50 Hz. O código está configurado para traduzir a leitura do ADC em um ciclo de trabalho que varia entre 2.5% para a posição 0 e 12.5% para a posição 180.

- Tratamento de Debounce
As interrupções dos botões implementam um controle de tempo via software. Um novo acionamento só é registrado se houver um intervalo mínimo de 100 milissegundos desde a última solicitação, garantindo estabilidade contra ruídos mecânicos.

- Zona Morta
O software implementa uma zona morta central e nas extremidades para evitar trepidações do motor em valores próximos aos limites de leitura do potenciômetro do joystick.

# Como Compilar
- Certifique-se de ter o Raspberry Pi Pico SDK configurado em seu ambiente.

- Inclua a biblioteca para o display SSD1306 no diretório do projeto.

- Crie um diretório para a build ou importe o projeto direto para o VsCode

- Compile o código e pronto!

# Exemplos:
- Simulação no wokwi:
https://wokwi.com/projects/461038225095903233

- Demonstração no youtube:
https://youtu.be/ktTMtsjbqmQ



