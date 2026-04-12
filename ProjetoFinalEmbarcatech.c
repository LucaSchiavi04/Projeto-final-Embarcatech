// Incluindo bibliotecas necessárias.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "ssd1306.h"
/*
 * Definindo pinos necessários
 *
 * SERVO_PIN: Pino PWM de 50 Hz para nosso servo motor. O duty cicle deste pino
 * varia de 2,5% a 12,5% para controlar nosso servo.
 *
 * ANALOG_X_PIN: Entrada analógica que faz a leitura do nosso analógico (joystick).
 * O controle da posição horizontal do analógico vai coordenar nosso servo.
 *
 * BUZZER_PIN: Duas saídas para os buzzers A e B. Quando o analógico está totalmente
 * virado para a esquerda, o buzzer A apita, e quando está para a direita, o buzzer
 * B apita.
 *
 * BUTTON_PIN: Duas entradas digitais para os botões A e B, o botão A diz a 
 * referência de posição que queremos (-90 a 90 graus ou 0 a 180 graus). O botão 
 * B desativa/ativa os buzzers.
 *
 * DISPLAY: Pinos do display SSD1306 (I2C)
 *
 * SLEEPTIME: tempo de sleep em ms para o código de inicialização
 *
 */
#define SERVO_PIN 8
#define ANALOG_X_PIN 27
#define BUZZER_B_PIN 10
#define BUZZER_A_PIN 21
#define BUTTON_B_PIN 6
#define BUTTON_A_PIN 5
#define DISPLAY_SDA 14
#define DISPLAY_SCL 15
#define SLEEPTIME 15

/*
 * Inicializando variáveis globais
 * 
 * wrap_servo: Valor do wrap para o PWM do servo motor.
 *
 * wrap_buzzer: Valor do wrap para o PWM dos buzzers.
 *
 * analog_finish: Flag que indica quando a próxima amostra de nosso ADC está
 * pronta.
 *
 * print_ready: Flag que indica quando devemos printar informações de nosso
 * interesse na porta USB e no display.
 *
 * mute_buzzer: Flag para mutar os buzzers insuportáveis.
 *
 * change_unit: Variável para mudar a referência das unidades mostradas no 
 * display.
 *
 * last_request_time: Variável de temporização para tratar debounce nas 
 * interrupções dos botões.
 *
 * Algumas variáveis são inicializadas com o valor "0" para evitar pontos 
 * flutuantes.
 */
uint16_t wrap_servo = 65535;
uint8_t wrap_buzzer = 255;
volatile bool analog_finish = false;
volatile bool print_ready = false;
volatile bool mute_buzzer = false;
volatile bool change_unit = false;
static volatile absolute_time_t last_request_time;

/*
 * Inicializando funções que precisamos
 */
void start();
bool analog_finish_callback(struct repeating_timer *t);
bool print_ready_callback(struct repeating_timer *t);
void button_callback(uint gpio, uint32_t events);

// FUNÇÃO MAIN
int main()
{
	stdio_init_all(); // Ativando comunicação USB com o computador.
	printf("Inicializando pinos da placa...\n");
	start(); // Inicializando pinos da placa.

	/*
	 * Inicializando variáveis locais
	 *
	 * analog_read: Valor da leitura do ADC de 12 bits (ou seja, ira armazenar
	 * valores de 0 a 4095), que ira realizar a leitura da posição de nosso
	 * analógico (também chamado de joystick).
	 *
	 * position: Diz a posição em graus do nosso servo motor, de acordo com o 
	 * analógico.
	 *
	 * compensation: Variável que serve para "deslocar" o início da leitura de
	 * nosso sensor analógico. Como o 0º do motor é com 2,5% de duty cicle, 
	 * devemos compensar esses 2,5% de duty cicle do resultado da leitura do ADC.
	 * Sem isso, quando colocamos o analógico para esquerda, antes de o analógico
	 * chegar no seu fim, o servo motor já estaria no 0º.
	 *
	 * scale: Variável que serve para "alongar" a nossa faixa de valores do ADC.
	 * Seu objetivo é consegui colocar a faixa de valores de 0 a 4095 dentro da 
	 * faixa de 2,5% a 12,5% de duty cicle para nosso servo motor. Isso permite
	 * que o analógico consiga representar com um nível satisfatório de precisão
	 * a posição do servo motor em 0º quando ele está totalmente para a esquerda,
	 * e 180º quando ele está totalmente para a direita.
	 *
	 * conversion: Variável que faz todas as operações necessárias na leitura do
	 * analógico, fazendo a compensação da posição inicial e o escalonamento da 
	 * faixa de valores.
	 *
	 * duty_cicle: Variável que representa o duty cicle de nosso PWM de 50 Hz.
	 * Para que o servo motor fique em 0º, precisamos de um duty cicle de 2,5%,
	 * e para que fique em 180º, precisamos de 12,5% de duty cicle.
	 *
	 * *words: Ponteiro com apenas umas palavras que são usadas na abertura do 
	 * display (apenas frufru).
	 * 
	 * buff: buffer para armazenar o valor da leitura "position" no "*info".
	 *
	 * *info: Ponteiro do tipo char que guarda os valores da variável "position"
	 * em um formato que possamos colocar no display.
	 *
	 * Algumas variáveis são inicializadas com o valor "0" para evitar pontos 
	 * flutuantes.
	 */
	uint16_t analog_read = 0;
	float position = 0;
	uint16_t compensation = 1638;
	float scale = 1.6005;
	float conversion = 0;
	float duty_cicle = 0;
	const char *words[] = {"SSD1306", "DISPLAY", "DRIVER"};
	char buff[7];
	char *info[] = {"0"};

	// Inicialização do display
	ssd1306_t disp;
	disp.external_vcc = false;
	ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
	ssd1306_clear(&disp);

	// Abertura do display (apenas estético)
	/*
	for (int y = 0; y < 31; ++y) {
		ssd1306_draw_line(&disp, 0, y, 127, y);
		ssd1306_show(&disp);
		sleep_ms(SLEEPTIME);
		ssd1306_clear(&disp);
	}
	for (int y = 0, i = 1; y >= 0; y += i) {
		ssd1306_draw_line(&disp, 0, 31 - y, 127, 31 + y);
		ssd1306_draw_line(&disp, 0, 31 + y, 127, 31 - y);
		ssd1306_show(&disp);
		sleep_ms(SLEEPTIME);
		ssd1306_clear(&disp);
		if (y == 32) i = -1;
	}
	for (int i = 0; i < sizeof(words) / sizeof(char *); ++i) {
		ssd1306_draw_string(&disp, 8, 24, 2, words[i]);
		ssd1306_show(&disp);
		sleep_ms(800);
		ssd1306_clear(&disp);
	}
	for (int y = 31; y < 63; ++y) {
		ssd1306_draw_line(&disp, 0, y, 127, y);
		ssd1306_show(&disp);
		sleep_ms(SLEEPTIME);
		ssd1306_clear(&disp);
	}
	*/
	// Contador que nos informa quando a amostra de nosso ADC está pronta.
	repeating_timer_t analog_timer;
	add_repeating_timer_ms(5, analog_finish_callback, NULL, &analog_timer);

	// Contador que nos informa quando devemos printar algo na porta serial.
	repeating_timer_t print_timer;
	add_repeating_timer_ms(500, print_ready_callback, NULL, &print_timer);

	// Loop principal
	while (true) {
		if(analog_finish == true) {
		// Realizando leitura do ADC e fazendo correções.
		analog_read = adc_read();

		// Convertendo o valor da leitura em graus (0 a 180).
		position = (180.0*analog_read)/4095.0;
		if(change_unit == true) {
			// Se o botão A for apertado, mostra a referência de -90 a 90.
			position = position - 90.0;
		}
		// Armazenando o valor da posição no ponteiro *info.
		sprintf(buff, "%.1f", position);
		*info = buff;

		// Convertendo nossa leitura analógica para "caber" no wrap do PWM.
		conversion = (analog_read*scale) + compensation;
		duty_cicle = (100.0*conversion/65535.0);

		// Configurando uma zona morta para nosso analógico.
		if(conversion > 4800 && conversion < 5100) {
			pwm_set_gpio_level(SERVO_PIN, 4916);
		} else if(conversion < 1700) {
			pwm_set_gpio_level(SERVO_PIN, 1638);
			if(mute_buzzer == false)
				pwm_set_gpio_level(BUZZER_A_PIN, 127);
		} else if(conversion > 8100) {
			pwm_set_gpio_level(SERVO_PIN, 8192);
			if(mute_buzzer == false)
				pwm_set_gpio_level(BUZZER_B_PIN, 127);
		} else {
			pwm_set_gpio_level(SERVO_PIN, conversion);
			pwm_set_gpio_level(BUZZER_A_PIN, 0);
			pwm_set_gpio_level(BUZZER_B_PIN, 0);
		}
		// Desativando a flag da interrupção.
		analog_finish = false;
		}

		if(print_ready == true) {
			/*
			 * O valor da leitura real ainda será printado na porta serial, mas
			 * o motor ira receber a filtragem da zona morta.
			 */
			printf("Nível do PWM: %.3f%%\nLeitura analógica: %d\nPosição: %.1fº\n", 
			duty_cicle, analog_read, position);

			// Gravação do display
			ssd1306_draw_string(&disp, 8, 10, 2, "POSITION:");
			ssd1306_draw_string(&disp, 8, 30, 2, info[0]);
			ssd1306_show(&disp); 
			ssd1306_clear(&disp);

			// Desativando a flag da interrupção
			print_ready = false;
		}
	sleep_ms(2);
	}
}

/*
 * Função para inicializar todos os pinos e configurações do Raspberry Pi 
 * Pico que precisamos antes do main.
 */
void start()
{
	// Coletando os slices dos pinos de PWM.
	uint slice_servo = pwm_gpio_to_slice_num(SERVO_PIN);
	uint slice_buzzer_a = pwm_gpio_to_slice_num(BUZZER_A_PIN);
	uint slice_buzzer_b = pwm_gpio_to_slice_num(BUZZER_B_PIN);

	// Configurando os pinos para PWM.
	gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
	gpio_set_function(BUZZER_A_PIN, GPIO_FUNC_PWM);
	gpio_set_function(BUZZER_B_PIN, GPIO_FUNC_PWM);

	// Ajustando o divisor e wrap dos PWMs.
	pwm_set_clkdiv(slice_servo, 38.15);
	pwm_set_clkdiv(slice_buzzer_a, 255.91);
	pwm_set_clkdiv(slice_buzzer_b, 255.91);

	pwm_set_wrap(slice_servo, wrap_servo);
	pwm_set_wrap(slice_buzzer_a, wrap_buzzer);
	pwm_set_wrap(slice_buzzer_b, wrap_buzzer);

	// Inicializando o PWM e buzzers com 0% de ciclo de trabalho.
	pwm_set_gpio_level(SERVO_PIN, 0);
	pwm_set_enabled(slice_servo, true);
	pwm_set_gpio_level(BUZZER_A_PIN, 0);
	pwm_set_enabled(slice_buzzer_a, true);
	pwm_set_gpio_level(BUZZER_B_PIN, 0);
	pwm_set_enabled(slice_buzzer_b, true);

	// Inicializando ADC do joystick.
	adc_init();
	adc_gpio_init(ANALOG_X_PIN);
	adc_select_input(1);

	// Configurando pinos de entrada.
	gpio_init(BUTTON_A_PIN);
	gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
	gpio_pull_up(BUTTON_A_PIN);

	gpio_init(BUTTON_B_PIN);
	gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
	gpio_pull_up(BUTTON_B_PIN);

	// Inicializando interrupções de botões.
	gpio_set_irq_enabled_with_callback(
		BUTTON_A_PIN,
		GPIO_IRQ_EDGE_FALL,
		true,
		&button_callback
	);
	gpio_set_irq_enabled(
		BUTTON_B_PIN,
		GPIO_IRQ_EDGE_FALL,
		true
	);

	// Configurando protocolo I2C para o display SSD1306.
	i2c_init(i2c1, 400000);
	gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
	gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(DISPLAY_SDA);
	gpio_pull_up(DISPLAY_SCL);
}

/*
 * Rotina de tratamento de interrupção que nos avisa quando a amostra de
 * nosso ADC está pronta (a cada 10ms).
 */
bool analog_finish_callback(struct repeating_timer *t)
{
	analog_finish = true; // Flag para ser tratada em nosso main.
	return true; // Retorna verdadeiro para que o timer continue depois.
}

/*
 * Rotina de tratamento de interrupção que evita com que seja mandado 
 * uma quantia enorme de mensagens na porta serial (USB) e no display
 * (a cada 800ms).
 */
bool print_ready_callback(struct repeating_timer *t)
{
	print_ready = true; // Flag para ser tratada em nosso main.
	return true; // Retorna verdadeiro para que o timer continue depois.
}

/*
 * Rotina de tratamento de interrupção para os botões A e B. Dentro
 * da rotina de interrupção, temos um tratamento de debounce com um
 * temporizador. O tratamento da interrupção só vai acontecer quando
 * o último click do botão foi feito 100ms depois do click anterior,
 * evitando os "clicks fantasmas" causados pelo debounce.
 */
void button_callback(uint gpio, uint32_t events) {
	absolute_time_t now = get_absolute_time();
	if(gpio == BUTTON_B_PIN) {
		if (absolute_time_diff_us(last_request_time, now) > 100000) {
		last_request_time = now;
		mute_buzzer = !mute_buzzer;
		}
	} else if(gpio == BUTTON_A_PIN) {
		if (absolute_time_diff_us(last_request_time, now) > 100000) {
		last_request_time = now;
		change_unit = !change_unit;
		}
	}
}