#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "tkjhat/sdk.h"
#define DEFAULT_STACK_SIZE 2048

typedef enum {
IDLE,
COMPOSING,
RECEIVING,
PLAYBACK
} SystemState;

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

volatile int button_pressed = 0;
volatile char latest_symbol = '\0';
QueueHandle_t symbol_queue;

/*
struct DisplayInfo {
    char plaintext[128];
    char msg[512];
    char letter[6];
    char custom_text[16];
    bool show_table;

    bool refresh;
} global = {0};
*/

void buttons(uint gpio, uint32_t eventMask);

char morse_to_letter(char mstring[]);

void letter_to_morse(char mletter, char result[]);

bool demorse(char *morse, char *text);

void display_morse_table();

void play_morse(char *morse);

void buttons(uint gpio, uint32_t eventMask){
    //asettaa napin tilan
    if(!button_pressed){
        button_pressed = gpio;
    }
}

void sensor_task(void *pvParameters){
    //pohjalla pyörivä taski joka asettaa käsiteltävän merkin jos nappia painetaan
    while(1){
        if(button_pressed){ //jos jotain nappia on painettu

            if(button_pressed == SW1_PIN){      //jos pinettu nappi on 1

                char symbol = '\0';
                float ay;
                ICM42670_read_sensor_data(NULL, &ay, NULL, NULL, NULL, NULL, NULL);
                if(ay < 0){
                    latest_symbol = '.';
                    //xQueueSend(symbol_queue, '.', 0);
                    printf("__dot__");
                } else{
                    latest_symbol = '-';
                    //xQueueSend(symbol_queue, '-', 0);
                    printf("__dash__");
                }
            } 
            
            else{                             //jos painettu nappi on 2
                latest_symbol = ' ';
                printf("__space__");
            }

        }
        button_pressed = 0; //nollataan painettu nappi
         vTaskDelay(pdMS_TO_TICKS(150));
    }  
}

void morse_string_task(void *pvParameters){
    /*
    Taski joka lisää merkin morsejonoon ja tarvittaessa lähettää.
    */

    char morse_letter[6] ; //morsetettu kirjain morsemerkkeina.
    char letter_len = 0;                //morsekirjaimen pituus. käytetään merkkien lisäämiseen ja pituuden tarkistamiseen

    char morse_string[512];    //lista johon morsekirjain lisätään tarkistuksen jälkeen
    int string_len = 0;                 //muuttuja jolla seurataan viimeisimmän merkin indeksiä
    
    char plaintext[256];

    while(1){

        //jos saadaan uusi merkki, aloitetaan tarkistukset
        if(latest_symbol){
            //kerätään merkkejä väliaikaiseen kirjaimen morsejonoon jos saatu merkki on piste tai viiva.
            if(latest_symbol == '.' || latest_symbol == '-'){
                if (letter_len < 5){
                    morse_letter[letter_len++] = latest_symbol;
                    if(latest_symbol == '.'){
                        buzzer_play_tone(1000, 100);
                    } else{
                        buzzer_play_tone(1000, 200);
                    }

                } else{ //jos saadaan yli 5 merkkiä ilman välilyöntiä, kirjain on liian pitkä ja aloitetaan uuden morsekirjaimen keruu.
                    letter_len = 0;
                    printf("__too long__");
                    buzzer_play_tone(1000, 500);
                }
                morse_letter[letter_len] = '\0';        //Lisää tyhjän merkin morsekirjaimen loppuun.
            } 
            
            //jos saadaan välimerkki tarkistetaan kerätyn kirjaimen laillisuus ja lisätään se morsejonoon jos se on laillinen.
            else if(latest_symbol == ' '){
                if(morse_to_letter(morse_letter)){      //jos morsekirjain on laillinen, lisataan sen merkit morsejonoon.
                    int i = 0;
                    while(morse_letter[i] != '\0'){
                        morse_string[string_len++] = morse_letter[i++];
                    }

                    morse_string[string_len++] = ' ';   //merkin jalkeen tulee aina valilyonti.
                    morse_string[string_len] = '\0';    //lisää tyhjän merkin morsejonon loppuun että jono luetaan oikein.

                    

                    demorse(morse_string, plaintext);
                    write_text_xy(0,56, plaintext);

                    buzzer_play_tone(1000, 50);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    buzzer_play_tone(1000, 50);

                } else{

                    printf("__Invalid morse character: %s__", morse_letter);
                    buzzer_play_tone(1000, 500);
                }
                morse_letter[0] = '\0';


                //jos kaks vikaa on space niin vaihdera
                if(string_len > 2 && (morse_string[string_len - 2] == ' ' && morse_string[string_len - 3] == ' ')) {
                    morse_string[string_len] = '\n';
                    string_len++;
                    morse_string[string_len] = '\0';

                    demorse(morse_string, plaintext);
                    printf(morse_string);

                    buzzer_play_tone(1000, 600);
                    clear_display();
                    write_text_xy(0 ,0, "Message sent!");

                    if(strlen(plaintext) < 14){//jos teksti on alle 12 merkkiä kirjoitetaan isolla
                        write_text(plaintext);  
                    } else{
                        write_text_xy(0, 32, plaintext);
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    play_morse(morse_string);
                    vTaskDelay(pdMS_TO_TICKS(1000));

                    //nollataan kaikki muuttujat viestin lahetyksen jalkeen 
                    string_len = 0;
                    morse_string[string_len] = '\0';
                    letter_len = 0;
                    morse_letter[letter_len] = '\0';
                    plaintext[0] = '\0';
                    display_morse_table();
                };
                letter_len = 0;
            }
            latest_symbol = '\0';
        }
       vTaskDelay(pdMS_TO_TICKS(100));
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


bool demorse(char *morse, char *text){

    char letter[8];
    int morse_index = 0;
    int text_index = 0;
    bool valid = true;

    while(morse[morse_index] != '\0' && morse[morse_index] != '\n'){
        char i = 0;
        while(morse[morse_index] != ' ' && morse[morse_index] != '\n' && morse[morse_index] != '\0'){
            if(i < 7){
            letter[i++] = morse[morse_index++];
            }
        }
        letter[i] = '\0';

        if(letter[0]){
            char plain_letter = morse_to_letter(letter);
            if(plain_letter){
                text[text_index++] = plain_letter;
            } else{ // jos kirjain ei ole laillinen korvataan se kysymysmerkillä ja asetetaan laillisuus vääräksi
                text[text_index++] = '?';
                valid = false;
            }
        } else if(morse[morse_index] == ' '){
            text[text_index++] = ' ';
        }
        if(morse[morse_index] == ' '){
            morse_index++;
        }
        
    }
    text[text_index++] = '\n';
    text[text_index] = '\0';
    return valid;
}

void display_morse_table(){
    char *table[] = {
    "a.-  b-... c-.-.d-..",
    "e.   f..-. g--. h....",
    "i..  j.--- k-.- l.-..",
    "m--  n-.   o--- p.--.",
    "q-.- r.-.  s... t-",
    "u..- v...- w.-- x-..-",
    "    y-.--   z--..",
     NULL
    };
    clear_display();
    int y = 0;
    for(int i = 0; table[i]; i++){
        write_text_xy(0, y, table[i]);
        y += 7;
    }
    
}

void play_morse(char *morse){
    for(int i = 0; morse[i] != '\0'; i++){
        char x = morse[i];
        if(x == '.'){
            buzzer_play_tone(1000, 100);
            vTaskDelay(pdMS_TO_TICKS(50));
        } else if(x == '-'){
            buzzer_play_tone(1000, 200);
            vTaskDelay(pdMS_TO_TICKS(50));
        }else if(x == ' '){
            vTaskDelay(pdMS_TO_TICKS(200));
        }      
    }
}

int main(){
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300);

    //initializing buttons
    init_sw1();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);
    init_sw2();
    gpio_set_irq_enabled_with_callback(SW2_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);

    init_display();
    init_buzzer();
    init_ICM42670();

    ICM42670_start_with_default_values();

    display_morse_table();

    TaskHandle_t myTaskHandle1 = NULL;
    TaskHandle_t myTaskHandle2 = NULL;
    
    xTaskCreate(sensor_task, "sensor_task", DEFAULT_STACK_SIZE, NULL, 2, &myTaskHandle1);
    xTaskCreate(morse_string_task, "morse_string_task", DEFAULT_STACK_SIZE, NULL, 3, &myTaskHandle2);

    vTaskStartScheduler();

    return 0;
}
