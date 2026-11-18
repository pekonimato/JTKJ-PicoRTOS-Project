#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 1024

volatile int button_pressed = 0;
volatile char latest_symbol = 0;

void interaction(int button);

void buttons(uint gpio, uint32_t eventMask){
    //asettaa napin tilan
    if(!button_pressed){
        button_pressed = gpio;
    }
}

void sensor_task(void *pvParameters){
    //pohjalla pyörivä taski joka asettaa käsiteltävän merkin jos nappia painetaan
    while(1){
        if(button_pressed){
            if(button_pressed == SW1_PIN){
                float ay;
                ICM42670_read_sensor_data(NULL, &ay, NULL, NULL, NULL, NULL, NULL);
                if(ay > 0){
                    latest_symbol = '.';
                    printf("dot\n");
                } else{
                    latest_symbol = '-';
                    printf("dash\n");
                }
            } else{
                latest_symbol = ' ';
                printf("space\n");
            }
            button_pressed = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void morse_string_task(void *pvParameters){
    /*
    Taski joka lisää merkin morsejonoon ja tarvittaessa lähettää.
    */

    //morse merkkien jono
    char morse_string[128] = "";
    //muutuja jolla seurataan viimeisen merkin indeksiä
    int last_indx = 0;

    while(1){
        if(latest_symbol){
            morse_string[last_indx] = latest_symbol;
            last_indx ++;
            printf("morse jono tähän asti: %s\n", morse_string);
            latest_symbol = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }


}

void interaction(int button){
    //napin painalluksen käsittelijä
    if(button == SW1_PIN){
        float ay;
        if(ICM42670_read_sensor_data(NULL, &ay, NULL, NULL, NULL, NULL, NULL) == 0) {
            printf("y = %f\n", ay);
        } else{
            printf("failed to get sensor data\n");
        }
    } else if(button == SW2_PIN){
        printf("space\n");
    } 
}

int main(){
    stdio_init_all();
    while (!stdio_usb_connected()){
        sleep_ms(10);
    } 
    init_hat_sdk();
    sleep_ms(300);

    //initializing buttons
    init_sw1();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);
    init_sw2();
    gpio_set_irq_enabled_with_callback(SW2_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);

    if(init_ICM42670() == 0) {
        printf("jeps, icm starttas oikeesti hienosti eikä mitään ongelmia jee jee\n");
    }

    ICM42670_start_with_default_values();

    TaskHandle_t myTaskHandle1 = NULL;
    TaskHandle_t myTaskHandle2 = NULL;
    
    xTaskCreate(sensor_task, "sensor_task", DEFAULT_STACK_SIZE, NULL, 2, &myTaskHandle1);
    xTaskCreate(morse_string_task, "morse_string_task", DEFAULT_STACK_SIZE, NULL, 2, &myTaskHandle2);

    vTaskStartScheduler();

    return 0;
}
