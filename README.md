# Projeto-final-Embarcatech
Este projeto consiste em um sistema de controle de posição para um servo motor utilizando um joystick analógico, baseado no microcontrolador Raspberry Pi Pico. O sistema inclui feedback visual via display OLED, alertas sonoros e customização de exibição através de botões.

#Componentes Utilizados
Microcontrolador Raspberry Pi Pico

Servo Motor Standard PWM

Joystick Analógico

Display OLED SSD1306 128x64

2 Buzzers Ativos/Passivos

2 Botões Pulsadores

#Configuração de Pinos
Componente,   Pino    GPIO/Função
Servo Motor,  8,      Saída PWM 50 Hz
Joystick,     27,     Entrada ADC
Buzzer A,     21,     Fim de curso
Buzzer B,     10,     Fim de curso
Botão A,      5,      Alternar escala de unidade
Botão B,      6,      Mudo / Ativar som
Botão JS,	    22,	    Inverter direção

#Display SSD1306
Pino SDA,     Pino SCL,   Barramento
14,           15,         I2C1

#Detalhes Técnicos
PWM e Duty Cycle
O servo motor opera em uma frequência de 50 Hz. O código está configurado para traduzir a leitura do ADC em um ciclo de trabalho que varia entre 2.5% para a posição 0 e 12.5% para a posição 180.

Tratamento de Debounce
As interrupções dos botões implementam um controle de tempo via software. Um novo acionamento só é registrado se houver um intervalo mínimo de 100 milissegundos desde a última solicitação, garantindo estabilidade contra ruídos mecânicos.

Zona Morta
O software implementa uma zona morta central e nas extremidades para evitar trepidações do motor em valores próximos aos limites de leitura do potenciômetro do joystick.

#Como Compilar
Certifique-se de ter o Raspberry Pi Pico SDK configurado em seu ambiente.

Inclua a biblioteca para o display SSD1306 no diretório do projeto.

Crie um diretório para a build. Ou importe o projeto:
<img width="566" height="488" alt="image" src="https://github.com/user-attachments/assets/cde30abb-d2f6-4978-9eb3-1ffbc15d02e9" />

Compile o código e pronto!
