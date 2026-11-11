
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

void sensori_paski();

static void nabit(uint gpio, uint32_t eventMask){
    //tässä siis kun painetaan nappia, niin 
    if(gpio == SW1_PIN){
        printf("SW1 pressed\n");
        sensori_paski();

    } else if(gpio == SW2_PIN){
        printf("SW2 pressed\n");
        sensori_paski();
    }
    
}


void init_buttons(){
    //nappi1
    init_sw1();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, nabit);
    //nappi2
    init_sw2();
    gpio_set_irq_enabled_with_callback(SW2_PIN, GPIO_IRQ_EDGE_FALL, true, nabit);
}


static void example_task(void *arg){
    (void)arg;

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void idle_task(void *arg){

    (void)arg;

    while(1){
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}



void sensori_paski(){
    //tämä paska tulsotaa sensorin arvot
    float ax, ay, az, gx, gy, gz, t;
    // Setting up the sensor.
    if(init_ICM42670() == 0) {
        printf("jeps, icm starttas oikeesti hienosti eikä mitään ongelmia jee jee\n");
    }
    else {
        printf("noups, tää paska ei startannu tai mikä tuo init onkaan\n");
    }
    ICM42670_start_with_default_values();

        if(ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            printf("Accel: X=%f, Y=%f, Z=%f\n", ax, ay, az);
            printf("Gyro: X=%f, Y=%f, Z=%f\n", gx, gy, gz);
            printf("Temp: %2.2f\n", t);

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
}

void sensor_task(void *pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;
    // Setting up the sensor.
    if(init_ICM42670() == 0) {
        printf("jeps, icm starttas oikeesti hienosti eikä mitään ongelmia jee jee\n");
    }
    else {
        printf("noups, tää paska ei startannu tai mikä tuo init onkaan\n");
    }
    ICM42670_start_with_default_values();

        if(ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            printf("Accel: X=%f, Y=%f, Z=%f\n", ax, ay, az);
            printf("Gyro: X=%f, Y=%f, Z=%f\n", gx, gy, gz);
            printf("Temp: %2.2f\n", t);

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

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    } 
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_buttons();
    printf("nyt lähtee\n");
    TaskHandle_t myExampleTask = NULL;
    // Create the tasks with xTaskCreate
    xTaskCreate(idle_task, "idle_task", DEFAULT_STACK_SIZE, NULL, 2, &myExampleTask);

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

