
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

//Add here necessary states
enum state { IDLE=1, VIIVA, PISTE};
enum state programState = IDLE;

static void example_task(void *arg){
    (void)arg;

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void sensor_task(void *pvParameters) {
    (void)pvParameters;

    float ax, ay, az;
    // Setting up the sensor.
    if(init_ICM42670() == 0) {
        printf("jeps, icm starttas oikeesti hienosti eikä mitään ongelmia jee jee\n");
    }
    else {
        printf("noups, tää paska ei startannu tai mikä tuo init onkaan\n");
    }

    while (1)
    {
        if(ICM42670_read_sensor_data(&ax, &ay, &az, NULL, NULL, NULL, NULL) == 0) {
            printf("Accel: X=%.2f, Y=%.2f, Z=%.2f\n", ax, ay, az);

            /*
            tässä me asetetaan tila laitteen asennon mukaan. jos laite on vaakassa asetetaan
            tilaksi VIIVA. jos laite on pystyssä asetetaan tilaksi PISTE.
            jossakin meillä on keskeytyskäsittelijä joka lukee napin ja heittää meidät merkinlähetys
            taskiin. siellä me tallennetaan tilaa vastaava merkki jonnekin.
            */

        }
        else {
            printf("ei toimi apua, tää ei toimi öö koska ei saanu dataa ehkä...\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    } 
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    printf("nyt lähtee\n");
    TaskHandle_t myExampleTask = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(sensor_task, "sensor_task", DEFAULT_STACK_SIZE, NULL, 2, &myExampleTask);

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

