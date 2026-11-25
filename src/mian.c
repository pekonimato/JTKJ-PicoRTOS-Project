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
    {' ', "\0"},
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

char morse_to_letter(char mstring[]);

void letter_to_morse(char mletter, char result[]);

void decode_morse_string(const char *morse_in, char *plaintext_out);

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

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void morse_string_task(void *pvParameters){
    /*
    Taski joka lisää merkin morsejonoon ja tarvittaessa lähettää.
    */

    //morse merkkien jono
    char morse_letter[6] = ""; //morsetettu kirjain joka halutaan tarkistaa
    char morse_string[255] = ""; //lista johon morsekirjain lisätään tarkistuksen jälkeen
    
    //muuttuja jolla seurataan viimeisimmän merkin indeksiä
    char string_index = 0;
    //morsekirjaimen viimeisen merkin indeksi. käytetään merkkien lisäämiseen ja pituuden tarkistamiseen
    char temp_index = 0;


    while(1){

        //jos saadaan uusi merkki, aloitetaan tarkistukset
        if(latest_symbol){

            //kerätään merkkejä väliaikaiseen kirjaimen morsejonoon jos saatu merkki on piste tai viiva
            if(latest_symbol == '.' || latest_symbol == '-'){
                if (temp_index < 5){
                    morse_letter[temp_index] = latest_symbol;
                 temp_index ++;
                } else{
                    morse_letter[0] = '\0';
                    temp_index = 0;
                    printf("too long\n");
                    buzzer_play_tone(3000, 1000);
                    buzzer_play_tone(2000, 1000); //Essi ja Laura teki soundia :D
                    buzzer_play_tone(1000, 1000);
                } 
            } 
            
            //jos saadaan välimerkki tarkistetaan saatu morsekirjain ja lisätään se oikeaan merkkijonoon.
            else if(latest_symbol == ' '){
                //asettaa morsekirjaimen viimesen merkin että jono luetaan oikein ja nollaa morsekirjaimen indeksilaskurin
                morse_letter[temp_index] = '\0';
                temp_index = 0;

                if(morse_to_letter(morse_letter)){
                    int i = 0;
                    while(morse_letter[i] != '\0'){
                        morse_string[string_index] = morse_letter[i];
                        i++, string_index++;
                    }
                    morse_string[string_index] = ' ';
                    string_index++;
                    printf("%s\n", morse_string);
                    char st[255];
                    decode_morse_string(morse_string, st);
                    printf("%s\n", st);

                } 
                else{
                    printf("character invalid\n");  //Essi ja Laura teki soundia :D
                    buzzer_play_tone(2000, 100);
                }
                morse_letter[0] = '\0';
                
                //jos kaks vikaa on space niin vaihdera
                if(string_index > 2 && morse_string[string_index - 1] == ' ' && morse_string[string_index - 2] == ' ') {
                    morse_string[string_index] = '\n';
                    string_index++;
                    morse_string[string_index] = '\0';
                };

            }
        }

       vTaskDelay(pdMS_TO_TICKS(500));
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



//GEMININ TEKEMÄ TESTIFUNKTIO!!! poistetaan/tehdään uudestaan, ei tarvitse huomioida
void decode_morse_string(const char *morse_in, char *plaintext_out) {
    char buffer[16]; // Max standard Morse character length is well under 16
    size_t buf_idx = 0;
    size_t out_idx = 0;

    for (size_t i = 0; morse_in[i] != '\0'; i++) {
        if (morse_in[i] == '.' || morse_in[i] == '-') {
            // Collect dots and dashes for the current letter
            if (buf_idx < sizeof(buffer) - 1) {
                buffer[buf_idx++] = morse_in[i];
            }
        } else if (morse_in[i] == ' ') {
            if (buf_idx > 0) {
                // First space after dots/dashes ends the letter
                buffer[buf_idx] = '\0';
                plaintext_out[out_idx++] = morse_to_letter(buffer);
                buf_idx = 0;
            } else {
                // Subsequent space (or leading space) adds a space to plaintext
                plaintext_out[out_idx++] = ' ';
            }
        }
    }

    // Handle the final letter if the input doesn't end with a space
    if (buf_idx > 0) {
        buffer[buf_idx] = '\0';
        plaintext_out[out_idx++] = morse_to_letter(buffer);
    }

    // Null-terminate the output string
    plaintext_out[out_idx] = '\0';
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

    init_buzzer();

    init_display();
    write_text("I am a apple");

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
