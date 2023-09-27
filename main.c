/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include <setjmp.h>

jmp_buf jmpbuf;

void custom_error_handler(const char *file, int line, const char *function, esp_err_t esp_error_code, const char *esp_error_message)
{
    printf("Error en función %s, archivo %s, línea %d\n", function, file, line);
    printf("Código de error ESP32: 0x%X\n", esp_error_code);
    printf("Mensaje de error: %s\n", esp_error_message);

    // Realiza acciones de recuperación o registro adicionales aquí si es necesario.

    // Realiza un salto a una ubicación específica (puedes ajustarla según tus necesidades)
    longjmp(jmpbuf, 1);
}

//DEFINICIONES PARA LAS ENTRADAS GPIO    *************************************************************************************************************************

#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT 0

int contador = 0;

//DEFINICIONES PARA PWM DE LOS SERVOS    *************************************************************************************************************************
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO1         (15) // Define the output GPIO
#define LEDC_OUTPUT_IO2         (2) // Define the output GPIO
#define LEDC_CHANNEL1           LEDC_CHANNEL_0
#define LEDC_CHANNEL2           LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY1              (1024) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095 (10% 819) (12,5% 1024) (7,5% 614)
#define LEDC_DUTY2              (205) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095 (5% 410) (2,5% 205)  (7,5% 614)
#define LEDC_FREQUENCY          (50) // Frequency in Hertz. Set frequency at 50 Hz

int servo1_duty = 0;
int servo2_duty = 0;



static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    ESP_LOGI("GPIO", "Interrupción en el GPIO %u\n", (unsigned int)gpio_num);
    printf("Entro en gpio\n");
    contador++;
    if (contador == 5)
    {
        contador = 1;
    }
    switch (contador)
    {
    case 1:
        servo1_duty = 205;
        servo2_duty = 1024;
        printf("Contador = 1\n");
        break;

    case 2:
        servo1_duty = 410;
        servo2_duty = 819;
        printf("Contador = 2\n");
        
        break;

    case 3:
        servo1_duty = 614;
        servo2_duty = 614;
        printf("Contador = 3\n");
        break;

    case 4:
        servo1_duty = 819;
        servo2_duty = 410;
        printf("Contador = 4\n");
        break;
    
    default:
        break;
    }

     //****************************************************      PWM SERVOS      *******************************************************
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL1, servo1_duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL1));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL2, servo2_duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL2));

}




//INICIALIZACION DE LOS GPIO *************************************************************************************************************************


static void gpio_init(void)
{
     gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL<<GPIO_INPUT_IO_0;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 2;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}



//INICIALIZACION DE LOS PWM DE LOS SERVOS *************************************************************************************************************************
static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer1 = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer1));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel1 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL1,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer2 = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer2));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL2,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO2,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
}




void app_main(void)
{
    if (setjmp(jmpbuf) == 0)
    {
        // Tu código principal aquí...
        printf("ERROR\n");
        // Si ocurre un error, se ejecutará el código en custom_error_handler
        // y luego se realizará un salto de vuelta a este punto.
    }

    printf("ARRANCO\n");
    //****************************************************      PWM SERVOS      *******************************************************
    // Set the LEDC peripheral configuration
    ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL1, LEDC_DUTY1));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL1));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL2, LEDC_DUTY2));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL2));

    gpio_init();
    //****************************************************      GPIO      *******************************************************

     //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_HIGH_LEVEL);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


    while(1)
    {
        
    }
}
