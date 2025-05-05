#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pio_matrix.pio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdio.h>
#include "pico/bootrom.h"

#define botaoB 6
#define MATRIX 7
#define green_pin_led 11
#define red_pin_led 13
#define blue_pin_led 12
#define BUTTON_A 5
#define BUZZER_A 10

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

static volatile bool night_mode = false;

/**
 * @brief Task para mudar o modo do semaforo
 */
void vButtonA()
{
    /**
     * Inicializa e configura o Botão A
     */
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_down(BUTTON_A);
    uint32_t last_time = 0;
    uint32_t current_time;

    while(true)
    {
        current_time = to_ms_since_boot(get_absolute_time());

        if ((!gpio_get(BUTTON_A)) && (current_time - last_time > 500))
        {
           last_time = current_time;
           night_mode = !night_mode;
           while (!gpio_get(BUTTON_A));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * Atualiza o estado do semaforo no LED RGB
 */
void vLedsRgb() 
{
    /**
     * Inicializa os pinos GPIOs para uso dos LEDs RGB
     */
    gpio_init(red_pin_led);
    gpio_set_dir(red_pin_led, GPIO_OUT);
    gpio_init(green_pin_led);
    gpio_set_dir(green_pin_led, GPIO_OUT);
    gpio_init(blue_pin_led);
    gpio_set_dir(blue_pin_led, GPIO_OUT);

    //Define os estados dos leds
    uint8_t states[3][3] = {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    uint8_t cont = 0;

    while (true)
    {
        if (!night_mode)
        {
            //Liga o LED com a cor atual do semaforo
            gpio_put(red_pin_led, states[cont][0]);
            gpio_put(green_pin_led, states[cont][1]);
            gpio_put(blue_pin_led, states[cont][2]);

            //Atualiza o valor do controlador de ciclo, mantendo sempre valores entre 0 e 2
            cont++;
            (cont > 2) ? cont = 0 : cont;
        
            //Aguarda 2 segundo antes de mudar para o próximo ciclo
            vTaskDelay(pdMS_TO_TICKS(2000));  
        }else {
            cont=0;
            //Mantém o LED amarelo piscando
            gpio_put(red_pin_led, true);
            gpio_put(green_pin_led, true);
            gpio_put(blue_pin_led, false);

            vTaskDelay(pdMS_TO_TICKS(500));

            gpio_put(red_pin_led, false);
            gpio_put(green_pin_led, false);
            gpio_put(blue_pin_led, false);
            
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
}

/**
 * @brief Task que controla o buzzer para emitir sons de acordo com o estado atual do semaforo
 */
void vBuzzer()
{
    /**
     * Configura o PWM para controle do buzzer
     */
    uint16_t WRAP = 1000;
    float diviser = 62.5;

    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(BUZZER_A);
    pwm_set_clkdiv(slice, diviser);
    pwm_set_wrap(slice, WRAP);
    pwm_set_gpio_level(BUZZER_A, 500);
    pwm_set_enabled(slice, false);

    while (true)
    {
        if(!night_mode)
        {
            //Vermelho
            pwm_set_enabled(slice, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set_enabled(slice, false);
            vTaskDelay(pdMS_TO_TICKS(1500));
            //Amarelo
            pwm_set_enabled(slice, true);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_enabled(slice, false);
            vTaskDelay(pdMS_TO_TICKS(500));
            //Verde
            pwm_set_enabled(slice, true);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }else {
            //Beep de 500ms a cada 2 segundos
            pwm_set_enabled(slice, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set_enabled(slice, false);
            vTaskDelay(2000);
        }
        
    }
}

uint32_t matrix_rgb(double r, double g, double b)
{
    unsigned char R = (unsigned char)(r * 255);
    unsigned char G = (unsigned char)(g * 255);
    unsigned char B = (unsigned char)(b * 255);

    return (G << 24) | (R << 16) | (B << 8);
}

/**
 * @brief Função para acender um LED da matriz
 */
void draw_on_matrix(uint32_t index_frame, uint32_t led_value, PIO pio, uint sm)
{
    const int frame[4][25] = {
        {
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0
        },
        {
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,2,0,0,
        0,0,0,0,0,
        0,0,0,0,0
        },
        {
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,3,0,0
        },
        {
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0
        }
    };

    for (uint32_t i = 0; i < 25; i++) {
        if (frame[index_frame][24-i] == 1) {
            led_value = matrix_rgb(1.0, 0.0, 0.0); 
        } else if(frame[index_frame][24-i]==2) {
            led_value = matrix_rgb(1.0, 1.0, 0.0);
        } else if(frame[index_frame][24-i]==3) {
            led_value = matrix_rgb(0.0,1.0,0.0);
        }else {
            led_value = matrix_rgb(0.0, 0.0, 0.0); 
        }
        pio_sm_put_blocking(pio, sm, led_value);
    }
}

/**
 * @brief Task que acende os LEDS na matriz de acordo com o estado atual do semaforo
 */
void vMatrixRgb()
{
    /**
     * Configuração da matriz de LEDS
     */
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, MATRIX);
    
    uint32_t led_value = 0;
    uint32_t index_frame = 0;

    while (true)
    {
        if(!night_mode)
        {
            draw_on_matrix(index_frame, led_value, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(2000));
            index_frame++;
            (index_frame > 2) ? index_frame = 0 : index_frame;
        }else {
            uint32_t index = 1;
            draw_on_matrix(index, led_value, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(500));
            index = 3;
            draw_on_matrix(index, led_value, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/**
 * @brief Desenha o semaforo no display ssd1306
 */
void draw_on_display(ssd1306_t ssd, bool cor)
{
    ssd1306_fill(&ssd, !cor);                        
    // retângulo de 20×60 em x=54, y=2
    ssd1306_line(&ssd, 54,  2, 54, 62, true);  // esquerda
    ssd1306_line(&ssd, 74,  2, 74, 62, true);  // direita
    ssd1306_line(&ssd, 54,  2, 74,  2, true);  // topo
    ssd1306_line(&ssd, 54, 62, 74, 62, true);  // base
    uint8_t h3 = 60 / 3;  // 20
    ssd1306_line(&ssd, 54,  2 + h3, 74,  2 + h3, true);
    ssd1306_line(&ssd, 54,  2 + 2*h3, 74, 2 + 2*h3, true);

    ssd1306_send_data(&ssd);                           // Atualiza o display
    //sleep_ms(735);
}

/**
 * @brief Task que simula o semaforo no display
 * A mudança de estados é representada pelas letras da cor atual do semaforo
 * R -> Red (Vermelho)
 * G -> Green (Verde)
 * Y -> Yellow (Amarelo)
 */
void vDisplay()
{
    /**
     * Configura o I2C e o display ssd1306
     */
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    
    gpio_pull_up(I2C_SDA);                                        
    gpio_pull_up(I2C_SCL);                                        
    ssd1306_t ssd;                                                
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
    ssd1306_config(&ssd);                                         
    ssd1306_send_data(&ssd);                                      
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;
    while (true)
    {
        if (!night_mode)
        {
            draw_on_display(ssd, cor);
            ssd1306_draw_char(&ssd, 'R', 60, 5);
            ssd1306_send_data(&ssd);
            vTaskDelay(pdMS_TO_TICKS(2000));

            draw_on_display(ssd, cor);
            ssd1306_draw_char(&ssd, 'Y', 60, 30);
            ssd1306_send_data(&ssd);
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            draw_on_display(ssd, cor);
            ssd1306_draw_char(&ssd, 'G', 60, 55);
            ssd1306_send_data(&ssd);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }else {
            draw_on_display(ssd, cor);
            ssd1306_draw_char(&ssd, 'Y', 60, 30);
            ssd1306_send_data(&ssd);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// Trecho para modo BOOTSEL com botão B
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL,
                                         true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    stdio_init_all();

    xTaskCreate(vButtonA, "ButtonA Task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLedsRgb, "LedsRGB Task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzer, "Buzzer Task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vMatrixRgb, "Matrix Task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplay, "Display Task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
