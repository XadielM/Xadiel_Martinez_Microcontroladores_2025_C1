#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/portmacro.h"

#define LED_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_0

static TimerHandle_t blink_timer;
static TimerHandle_t button_timer;

// Notification for synchronization
static TaskHandle_t button_task_handle = NULL;

void IRAM_ATTR button_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
 
    if (gpio_get_level(BUTTON_PIN) == 0) {
        // Cuando se presiona el boton - start timer
        xTimerStartFromISR(button_timer, &xHigherPriorityTaskWoken);
    } else {
        // Al soltar el boton- stop timer y notificar
        xTimerStopFromISR(button_timer, &xHigherPriorityTaskWoken);
        vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void button_timer_callback(TimerHandle_t xTimer) {
    //Aqui se cuenta cuanto duro el boton presionado
    gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
}

void blink_timer_callback(TimerHandle_t xTimer) {
    // Optional: Additional blinking logic if needed
    gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
}

void button_task(void *arg) {
    button_task_handle = xTaskGetCurrentTaskHandle();
    
    while (1) {
        //Esperar la notificacion de que se solto el boton
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            //La duracion del boton presionado se obtenida del timer
            uint32_t duration = xTimerGetPeriod(button_timer);
            
            //Empieza el timer de parpadeo mientras presiona el boton
            xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(duration), 0);
            xTimerStart(blink_timer, 0);
            
            //Esperar a la duracion del parpadeo
            vTaskDelay(pdMS_TO_TICKS(duration));
            
            //Detener el parpadeo
            xTimerStop(blink_timer, 0);
            gpio_set_level(LED_PIN, 0);
        }
    }
}

void app_main() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_ANYEDGE);

    //Crear timers
    button_timer = xTimerCreate("ButtonTimer", pdMS_TO_TICKS(50), pdTRUE, NULL, button_timer_callback);
    blink_timer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(500), pdTRUE, NULL, blink_timer_callback);

    // Install ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    // Crear el boton task
    xTaskCreate(button_task, "Button Task", 2048, NULL, 10, NULL);
}