#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include "tkjhat/sdk.h"
#define DEFAULT_STACK_SIZE 2048

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

struct DisplayInfo {
    char plaintext[128];
    char msg[512];
    char letter[6];
    char custom_text[16];
    bool show_table;

    bool refresh;
} global = {0};


void buttons(uint gpio, uint32_t eventMask);

char morse_to_letter(char mstring[]);

void letter_to_morse(char mletter, char result[]);

void demorse(char *morse, char *text);

void display_message(char *line1, char *line2, char *line3);

void display_morse_table();

void display_text(char *list[]);

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
                float ay;
                ICM42670_read_sensor_data(NULL, &ay, NULL, NULL, NULL, NULL, NULL);
                if(ay < 0){
                    latest_symbol = '.';
                    printf("__dot\n__");
                    buzzer_play_tone(1000, 100);
                } else{
                    latest_symbol = '-';
                    printf("__dash\n__");
                    buzzer_play_tone(1000, 300);
                }
            } 
            
            else{                             //jos painettu nappi on 2
                latest_symbol = ' ';
                printf("__space\n__");
                buzzer_play_tone(1000, 50);
                sleep_ms(50);
                buzzer_play_tone(1000, 50);
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

    char *morse_letter = global.letter; //morsetettu kirjain morsemerkkeina.
    char letter_len = 0;                //morsekirjaimen pituus. käytetään merkkien lisäämiseen ja pituuden tarkistamiseen

    char *morse_string = global.msg;    //lista johon morsekirjain lisätään tarkistuksen jälkeen
    int string_len = 0;                 //muuttuja jolla seurataan viimeisimmän merkin indeksiä
    
    char *plaintext = global.plaintext;

    while(1){

        //jos saadaan uusi merkki, aloitetaan tarkistukset
        if(latest_symbol){
            //kerätään merkkejä väliaikaiseen kirjaimen morsejonoon jos saatu merkki on piste tai viiva.
            if(latest_symbol == '.' || latest_symbol == '-'){
                if (letter_len < 5){
                    morse_letter[letter_len++] = latest_symbol;

                } else{ //jos saadaan yli 5 merkkiä ilman välilyöntiä, kirjain on liian pitkä ja aloitetaan uuden morsekirjaimen keruu.
                    letter_len = 0;
                    printf("__too long\n__");
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
                    global.msg[string_len] = '\0';    //lisaa tyhjan merkin morsejonon loppuun.

                    demorse(morse_string, plaintext);
                    printf("__%s, %s\n__", morse_string, plaintext);
                    write_text_xy(0,56, plaintext);

                } else{

                    printf("__Invalid morse character: %s\n__", morse_letter);
                    buzzer_play_tone(1000, 500);
                }
                morse_letter[0] = '\0';


                //jos kaks vikaa on space niin vaihdera
                if(string_len > 2 && (morse_string[string_len - 2] == ' ' && morse_string[string_len - 3] == ' ')) {
                    morse_string[string_len] = '\n';
                    string_len++;
                    morse_string[string_len] = '\0';

                    demorse(morse_string, plaintext);
                    printf("%s", morse_string);

                    buzzer_play_tone(1000, 600);
                    clear_display();
                    write_text_xy(0 ,0, "Message sent!");
                    write_text(plaintext);
                    
                    //nollataan kaikki muuttujat viestin lahetyksen jalkeen 
                    string_len = 0;
                    morse_string[string_len] = '\0';
                    letter_len = 0;
                    morse_letter[letter_len] = '\0';
                    plaintext[0] = '\0';
                    
                    sleep_ms(2000);
                    display_morse_table();
                };
                letter_len = 0;
            }
            latest_symbol = '\0';
        }
       vTaskDelay(pdMS_TO_TICKS(150));
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


void demorse(char *morse, char *text){

    char letter[6];
    int morse_index = 0;
    int text_index = 0;

    while(morse[morse_index] != '\0'){
        char i = 0;
        while(morse[morse_index] != ' ' && morse[morse_index] != '\0' && i < 5){
            letter[i++] = morse[morse_index++];
        }
        letter[i] = '\0';

        if(letter[0]){
            if(letter[0] != '\n'){
                text[text_index++] = morse_to_letter(letter);
            } else{ 
                text[text_index++] = '\n';
            }
        } else if(morse[morse_index] == ' '){
            text[text_index++] = ' ';
        }

        if (morse[morse_index] == ' ') {
            morse_index++;
        }
    }

    text[text_index] = '\0';
}

void display_morse_table(){
    char *table[] = {
    "a.-  b-... c-.-.d-..",
    "e.   f..-. g--. h....",
    "i..  j.--- k-.- l.-..",
    "m--  n-.   o--- p.--.",
    "q-.- r.-.  s... t-",
    "u..- v...- w.-- x-..-",
    "y-.-- z--..", NULL
    };
    display_text(table);
    
}

void display_text(char *list[]){

    clear_display();
    int y = 0;
    for(int i = 0; list[i]; i++){
        write_text_xy(0, y, list[i]);
        y += 7;
    }
}

int main(){
    stdio_init_all();
    //while (!stdio_usb_connected()){
    //    sleep_ms(10);
    //} 
    init_hat_sdk();
    sleep_ms(300);

    //initializing buttons
    init_sw1();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);
    init_sw2();
    gpio_set_irq_enabled_with_callback(SW2_PIN, GPIO_IRQ_EDGE_FALL, true, buttons);

    init_display();

    init_buzzer();

    display_morse_table();

    if(init_ICM42670() == 0) {
        printf("__jeps, icm starttas oikeesti hienosti eikä mitään ongelmia jee jee\n__");
    }

    ICM42670_start_with_default_values();

    TaskHandle_t myTaskHandle1 = NULL;
    TaskHandle_t myTaskHandle2 = NULL;
    
    xTaskCreate(sensor_task, "sensor_task", DEFAULT_STACK_SIZE, NULL, 2, &myTaskHandle1);
    xTaskCreate(morse_string_task, "morse_string_task", DEFAULT_STACK_SIZE, NULL, 3, &myTaskHandle2);

    vTaskStartScheduler();

    return 0;
}
