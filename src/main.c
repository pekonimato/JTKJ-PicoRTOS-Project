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
const struct Morse_Pair {
    char letter;
    char morse[6];    
}Morse_Table[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, 
    {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}
};

const char TABLE_SIZE = sizeof(Morse_Table)/sizeof(Morse_Table[0]);

void interaction(int button);

bool valid_letter(char string[]);

char morse_to_letter();

void letter_to_morse();

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
    char morse_letter[5] = ""; //morsetettu kirjain jonka halutaan tarkistaa
    char morse_string[128] = ""; //lista johon morsekirjain lisätään tarkistuksen jälkeen
    
    //muutuja jolla seurataan viimeisimmän merkin indeksiä
    int morse_string_indx = 0;
    int letter_indx = 0;
    int string_indx = 0;

    while(1){
        if(latest_symbol){
            if(latest_symbol != " "){
                if(letter_indx < 5){
                    morse_letter[letter_indx] = latest_symbol;
                    letter_indx ++;
                }
            } else if(latest_symbol = " "){
                if(morse_to_letter(morse_letter)){
                    for(int i = 0; i < sizeof(morse_letter); i++){
                        morse_string[morse_string_indx] = morse_letter[i];
                    }
                    
                } 
            }
        }


        /*
        if(latest_symbol){
            morse_string[last_indx] = latest_symbol;
            string_indx ++;
            printf("morse jono tähän asti: %s\n", morse_string);
            latest_symbol = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        */
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

char morse_to_letter(char mstring[]){
    //finds the character of a morse string by going through the morse table and comparing the morse string with valid character morse strings.
    for(int i = 0; i < TABLE_SIZE; i++){
        if(strcmp(Morse_Table[i].morse, mstring) == 0){
            return Morse_Table[i].letter;
        }
    }
    return '\0';
}

void letter_to_morse(char mletter, char result[]){
    //finds the morse string for a character from the morse table
    for(int i = 0; i < TABLE_SIZE; i++){
        if(Morse_Table[i].letter == mletter){
            strcpy(result, Morse_Table[i].morse);
            return;
        }
    }
    strcpy(result, "\0");
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
