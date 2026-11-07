/*         Brett Castro - bcast107@ucr.edu

 *         Discussion Section: 022

 *         Assignment: Custom Lab Project

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: 

 */

#include "bcast107_gameFunctions.h"
#include "bcast107_gameGraphics.h"
#include "timerISR.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "avr/eeprom.h"
#include "avr/sleep.h"
#include "avr/interrupt.h"
#include "helper.h"
#include "periph.h"
#include "LCD.h"
#include "bcast107_ST7735Display.h"

// GLOBAL VARIABLES

unsigned char direction; // 0 = - | 2 = U | 1 = D | 3 = L | 4 = R
unsigned char jump;      // Flag when a jump is detected
unsigned char duck;      // Flag when a duck is detected
unsigned char collision; // Flag when a collision is detected
unsigned char gameOn;    // Flag when a start button press is detected
unsigned char endGameScreen; // Flag when the end game screen is finished being displayed
uint16_t EEMEM savedHighscore;  // High score stored in EEPROM memory
uint16_t highscore = 0; // Highscore
uint16_t score; // Current game score
char Line1[16]; // First line of text to be displayed on 16x2 LCD display
char Line2[16]; // Second line of text to be displayed on 16x2 LCD display
unsigned char sleeping; // Flag when the sleep button is pressed

// GAME OBJECTS

Game_Object dinosaur; // Create game object for Dinosaur
Game_Object cactus; // Create game object for Cactus
Game_Object pterodactyl; // Create game object for Pterodactyl
Game_Object obstacle; // Create the current game obstacle

// STATE MACHINE LOGIC

#define NUM_TASKS 11 // Number of tasks being used

// Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


// Periods for each task
const unsigned long START_Period = 25;
const unsigned long JSTCK_Period = 25;
const unsigned long JUMP_DUCK_Period = 25;
const unsigned long DINO_Period = 25;
const unsigned long OBSTACLE_Period = 25;
const unsigned long COLLISION_Period = 25;
const unsigned long SCORE_Period = 50;
const unsigned long STRING_Period = 50;
const unsigned long DISPLAY_Period = 25;
const unsigned long LCD_Period = 50;
const unsigned long SLEEP_Period = 25;

const unsigned long GCD_PERIOD = 25;


task tasks[NUM_TASKS]; // declared task array

// Task enums and tick function declaration

enum START_States{START_Start, START_Off, START_On};
int START_Tick(int state);

enum JSTCK_States{ON};
int JSTCK_Tick(int state);

enum JUMP_DUCK_States{JUMP_DUCK_Start, JUMP_DUCK_Read, JUMP_DUCK_Jump_Action, JUMP_DUCK_Duck_Action};
int JUMP_DUCK_Tick(int state);

enum DINO_States{DINO_Start, DINO_Wait, DINO_Ascend, DINO_Descend, DINO_Duck, DINO_Reset};
int DINO_Tick(int state);

enum OBSTACLE_States{OBSTACLE_Start, OBSTACLE_Update, OBSTACLE_Reset};
int OBSTACLE_Tick(int state);

enum COLLISION_States{COLLISION_Start, COLLISION_Check, COLLISION_Reset};
int COLLISION_Tick(int state);

enum SCORE_States{SCORE_Start, SCORE_PreGame, SCORE_GameOn, SCORE_PostGame};
int SCORE_Tick(int state);

enum STRING_States{STRING_Start, STRING_PreGame, STRING_GameOn, STRING_PostGame};
int STRING_Tick(int state);

enum DISPLAY_States{DISPLAY_Start, DISPLAY_Gameplay, DISPLAY_Endgame};
int DISPLAY_Tick(int state);

enum LCD_States {LCD_Start, LCD_Line1, LCD_Line2};
int LCD_Tick(int state);

enum SLEEP_States{SLEEP_Start, SLEEP_Read, SLEEP_Press};
int SLEEP_Tick(int state);

// TIMER

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {               // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state);         // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                                  // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                            // Increment the elapsed time by GCD_PERIOD
	}
}

// Interrupt functions to wake CPU from sleep mode
void interrupt_ON() {
    sei();

    PCIFR |= (1 << PCIF1);  // Clear any pending pin change interrupt

    PCMSK1 |= (1 << PCINT13); // Enable interrupt for PC5 (PCINT13)

    PCICR |= (1 << PCIE1);    // Enable Pin Change Interrupts for PCINT[14:8] (PORTC)
}

void interrupt_OFF(){
    PCICR &= ~(1 << PCIE1);

    PCMSK1 &= ~(1 << PCINT13);

    cli();
}

ISR(PCINT1_vect) {} // Triggered when interrupt is detected

// MAIN

int main(void) {
    // Initialize all inputs and ouputs
    DDRC   = 0x01;
    PORTC  = 0xFE;
  
    DDRB   = 0xFF;
    PORTB  = 0x00;
  
    DDRD   = 0xFF;
    PORTD  = 0x00;

    // Initialization functions
    ADC_init(); 
    DSP_SPI_init();
    DSP_init();
    lcd_init();

    // Initialize tasks here
    // e.g. 
    // tasks[0].period = ;
    // tasks[0].state = ;
    // tasks[0].elapsedTime = ;
    // tasks[0].TickFct = ;

    tasks[0].period = START_Period;
    tasks[0].state = START_Start;
    tasks[0].elapsedTime = START_Period;
    tasks[0].TickFct = &START_Tick;

    tasks[1].period = JSTCK_Period;
    tasks[1].state = ON;
    tasks[1].elapsedTime = JSTCK_Period;
    tasks[1].TickFct = &JSTCK_Tick;
    
    tasks[2].period = JUMP_DUCK_Period;
    tasks[2].state = JUMP_DUCK_Start;
    tasks[2].elapsedTime = JUMP_DUCK_Period;
    tasks[2].TickFct = &JUMP_DUCK_Tick;

    tasks[3].period = DINO_Period;
    tasks[3].state = DINO_Start;
    tasks[3].elapsedTime = DINO_Period;
    tasks[3].TickFct = &DINO_Tick;
    
    tasks[4].period = OBSTACLE_Period;
    tasks[4].state = OBSTACLE_Start;
    tasks[4].elapsedTime = OBSTACLE_Period;
    tasks[4].TickFct = &OBSTACLE_Tick;

    tasks[5].period = COLLISION_Period;
    tasks[5].state = COLLISION_Start;
    tasks[5].elapsedTime = COLLISION_Period;
    tasks[5].TickFct = &COLLISION_Tick;

    tasks[6].period = SCORE_Period;
    tasks[6].state = SCORE_Start;
    tasks[6].elapsedTime = SCORE_Period;
    tasks[6].TickFct = &SCORE_Tick;

    tasks[7].period = STRING_Period;
    tasks[7].state = STRING_Start;
    tasks[7].elapsedTime = STRING_Period;
    tasks[7].TickFct = &STRING_Tick;

    tasks[8].period = DISPLAY_Period;
    tasks[8].state = DISPLAY_Start;
    tasks[8].elapsedTime = DISPLAY_Period;
    tasks[8].TickFct = &DISPLAY_Tick;

    tasks[9].period = LCD_Period;
    tasks[9].state = LCD_Start;
    tasks[9].elapsedTime = LCD_Period;
    tasks[9].TickFct = &LCD_Tick;

    tasks[10].period = SLEEP_Period;
    tasks[10].state = SLEEP_Start;
    tasks[10].elapsedTime = SLEEP_Period;
    tasks[10].TickFct = &SLEEP_Tick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}

// TICK FUNCTIONS

/* 
This task observes whether or not the user has pressed the start button. 
Initally, the task sets the game state to off.
When the start button is pressed, if the end game screen is not being displayed, the gameOn flag is raised, and the game state is set to on. 
When the the collision flag is raised, the game state changes to off. 
When the system is sleeping, this task resets to the start state.
*/
int START_Tick(int state){
    switch (state){
        case START_Start:
            gameOn = 0;
            state = START_Off;
            break;
        case START_Off:
            if(GetBit(PINC, 1) == 0 && endGameScreen == 0) { gameOn = 1; state = START_On; }
            break;
        case START_On:
            if(collision == 1) { gameOn = 0; state = START_Off; }
            break;
        default: 
            break;
    }
    return state;
}

/*
This task reads the joystick position every tick, by calling the jstK_Read function, defined in the gameFunctions.h file.
The value of direction is used in the JUMP_DUCK task, and is always on.
*/
int JSTCK_Tick(int state){
    direction = jstk_Read();
    return state;
}

/*
This task reads the current joystick direction value and flags a jump if the direction is up, or a duck if the direction is down.
This task is always running, but the flag values are only used while the game is active.
*/
int JUMP_DUCK_Tick(int state){
    switch (state){
        case JUMP_DUCK_Start:
            jump = 0;
            duck = 0;
            state = JUMP_DUCK_Read;
            break;
        case JUMP_DUCK_Read:
            jump = 0;
            duck = 0;
            if(direction == up) state = JUMP_DUCK_Jump_Action;
            if(direction == down) state = JUMP_DUCK_Duck_Action;
            break;
        case JUMP_DUCK_Jump_Action:
            jump = 1;
            state = JUMP_DUCK_Read;
            break;
        case JUMP_DUCK_Duck_Action:
            duck = 1;
            if (direction != down ) state = JUMP_DUCK_Read;
            break;
        default:   
            break;
    }
    return state;
}

/*
This task handles all behavior relating to the dinosaur game object. 
The dinosaur is first initialized, and it's hitbox updated. 
When a game is started, the dinosaur waits until a jump or duck is detected, 
however the reference frame that the user observes follows that of the dinosaur.
Thus, the dinosaur's bitmap is updated to give an animation as though it is running. 
When a jump is detected, the dinosaur ascends and descends at particular set velocity.
When a duck is detected, the dinosaur ducks until the joystick is released.     
When a game is over, the dinosaur waits for the end game screen to end and then resets, waiting for a new game.
When the system is sleeping, the dinosaur is reset to its initial state, then waits.
*/
int DINO_Tick(int state){
    switch (state){
        case DINO_Start:
            dinosaur = init_Dino();
            if(gameOn == 1) state = DINO_Wait;
            break;
        case DINO_Wait:
            updateHitbox(&dinosaur.hitbox, dinosaur.x, dinosaur.y, dinosaur.width - 8, dinosaur.height - 18);
            if(gameOn == 1){
                if(dinosaur.bitmap_Num == 1) { dinosaur.bitmap = dino_bitmap2; dinosaur.bitmap_Num = 2; }
                else if(dinosaur.bitmap_Num == 2) {dinosaur.bitmap = dino_bitmap3; dinosaur.bitmap_Num = 3; } 
                else if(dinosaur.bitmap_Num == 3) {dinosaur.bitmap = dino_bitmap1; dinosaur.bitmap_Num = 1; } 
                else if(dinosaur.bitmap_Num == 4 || dinosaur.bitmap_Num == 5) {dinosaur.bitmap = dino_bitmap1; dinosaur.bitmap_Num = 1; } 


            } else {dinosaur.bitmap = dino_bitmap1; dinosaur.bitmap_Num = 1;}
            if(jump == 1 && gameOn == 1) { 
                state = DINO_Ascend; 
                dinosaur.bitmap = dino_bitmap1; 
                dinosaur.bitmap_Num = 1; 
            } else if (duck == 1 && gameOn == 1) { 
                state = DINO_Duck; 
                dinosaur.bitmap = dino_duck_bitmap1; 
                dinosaur.bitmap_Num = 4; 
            }
            break;
        case DINO_Ascend:
            dinosaur.vy = 8;
            dinosaur.y -= dinosaur.vy;

            updateHitbox(&dinosaur.hitbox, dinosaur.x, dinosaur.y, dinosaur.width - 8, dinosaur.height - 18);

            
            if(dinosaur.y >= dinosaur.max) state = DINO_Ascend;
            else if (dinosaur.y < dinosaur.max) state = DINO_Descend;
            if (collision == 1) state = DINO_Reset;
            break;
        case DINO_Descend:
            if(direction == down) dinosaur.vy = 12;
            else dinosaur.vy = 8;

            dinosaur.y += dinosaur.vy;
            if(dinosaur.y > dinosaur.min) dinosaur.y = dinosaur.min;

            updateHitbox(&dinosaur.hitbox, dinosaur.x + 8, dinosaur.y, dinosaur.width - 12, dinosaur.height - 18);

            
            if(dinosaur.y >= dinosaur.min) state = DINO_Wait;
            else if (dinosaur.y < dinosaur.min) state = DINO_Descend;
            if (collision == 1) state = DINO_Reset;
            break;
        case DINO_Duck:
            updateHitbox(&dinosaur.hitbox, dinosaur.x, dinosaur.y + 26, dinosaur.width, dinosaur.height - 30);
        
            if(score % 10 == 0){
                    dinosaur.bitmap = dino_duck_bitmap2;
                    dinosaur.bitmap_Num = 5;
            } else {
                dinosaur.bitmap = dino_duck_bitmap1;
                dinosaur.bitmap_Num = 4;
            }

            if(duck == 0) state = DINO_Wait;
            if (collision == 1) state = DINO_Reset;
            break;
        case DINO_Reset:
            if (endGameScreen == 0) state = DINO_Start;
        default:
            break;
    }
    return state;
}

/*
This task handles all behavior relating to the obstacles.
The obstacle's position value is constantly updating, so every tick it's posiiton is updated to allow for gameplay.
The obstacle task reads the current score value and updates the velocity of the obstacle every 1000 points to be faster, adding dynamic speed.
The task also has to vary which obstacle is being used. The pterodactyl requires a duck or jump, and the cactus requires a jump.
Using a random number generator there is a 30% chance a pterodactyl spawns, with the first obstacle always being a cactus.
The animations here are based on the score, since it has the same period as this task. 
When the game is over, the cactus waits for the end game screen to end, then resets, waiting for a new game.
WHen the system is sleeping, this task resets the obstacle to the starting cactus and waits.
*/
int OBSTACLE_Tick(int state){
    static char velo;
    static unsigned char j;
    switch (state){
        case OBSTACLE_Start:
            j = 0;
            cactus = init_Cactus();
            pterodactyl = init_Pterodactyl();
            obstacle = init_Cactus();
            obstacle.vx = 5;

            updateHitbox(&obstacle.hitbox, obstacle.x + 4, obstacle.y, obstacle.width - 18, obstacle.height);

            if(gameOn == 1) state = OBSTACLE_Update;
            break;
        case OBSTACLE_Update:
            obstacle.x -= obstacle.vx;
            velo = obstacle.vx;

            if(obstacle.isCactus == 1) updateHitbox(&obstacle.hitbox, obstacle.x + 4, obstacle.y, obstacle.width - 18, obstacle.height);
            if(obstacle.isCactus == 0) updateHitbox(&obstacle.hitbox, obstacle.x + 8, obstacle.y, obstacle.width - 18, obstacle.height - 4);

            if(score % 1000 == 0) j++;
            if((score + 1) % (1000 + j) == 0) obstacle.vx++;

            if(obstacle.isCactus == 0) {
                if(score % 10 == 0){
                    obstacle.bitmap = pterodactyl_bitmap2;
                } else obstacle.bitmap = pterodactyl_bitmap1;
            }     

            if(obstacle.x <= obstacle.max + 13) {
                obstacle.color = GRAY;
            } else obstacle.color = DARK_GRAY;

            if(obstacle.x <= obstacle.max) {
                obstacle = setObstacle(cactus, pterodactyl);
                obstacle.x = obstacle.min;
                obstacle.vx = velo;
            }
            
            if(collision == 1) state = OBSTACLE_Reset;
            break;
        case OBSTACLE_Reset:  
            if(endGameScreen == 0) state = OBSTACLE_Start;
            break;
        default:
            break;
    }
    return state;
}

/*
This task handles all collision interaction while a game is active. 
When the game is started, this task constantly checks the position values of the dinosaur and cactus hitboxes to ensure they are not overlapping.
If an overlap is detected, a collision has happened, and the game is over.
Thus, a flag is raised, and the task waits for the end game screen to end before resetting and waiting for the next game.
When the game is not on, or the system is sleeping, this task waits.
*/
int COLLISION_Tick(int state){
    switch (state){
        case COLLISION_Start:
            collision = 0;
            if(gameOn == 1) state = COLLISION_Check;
            break;
        case COLLISION_Check:
            if(checkCollision(&dinosaur.hitbox, &obstacle.hitbox) == 1 || sleeping == 1) { collision = 1; state = COLLISION_Reset; }
            break;
        case COLLISION_Reset:
            if(endGameScreen == 0) state = COLLISION_Start;
            break;
        default:
            break;
    }
    return state;
}

/*
This task handles all behavior of updating the score. 
The task first attempts to read for a saved high score from EEPROM memory. It then sets up a current score, and waits for the game to start.
Before and after the game, the saved high score can be reset with the use of the left button. 
When a game starts, the current score is updated by 5 points every tick.
If the high score is exceeded by the current score during a game, the high score is replaced by the current score, and updates with it as the game continues.
After the game ends, the high score saved in EEPROM is updated or reset, and the task waits for the next game to start.
When the system is sleeping, the high score is updated if it was exceeded, then the score is reset for the next new game.
*/
int SCORE_Tick(int state){
    switch (state){
        case SCORE_Start:
            highscore = eeprom_read_word(&savedHighscore);
            score = 0;
            state = SCORE_PreGame;
            break;
        case SCORE_PreGame:
            if (gameOn == 1) state = SCORE_GameOn;
            if(GetBit(PINC, 4) == 1) { eeprom_update_word(&savedHighscore, 0); highscore = eeprom_read_word(&savedHighscore); }
            break;
        case SCORE_GameOn:
            if(score > highscore) highscore = score;
            if(collision == 0) score += 5;
            else state = SCORE_PostGame;
            break;
        case SCORE_PostGame:
            if(GetBit(PINC, 4) == 0) eeprom_update_word(&savedHighscore, highscore);
            else if(GetBit(PINC, 4) == 1) { eeprom_update_word(&savedHighscore, 0); highscore = eeprom_read_word(&savedHighscore); } 
            if(endGameScreen == 0) state = SCORE_Start;
            break;
        default:
            break;
    }
    return state;
}

/*
This task handles all behavior of the strings that are displayed on the 16x2 LCD screen.
Before a game starts, the task sets the strings to display " PRESS START " as well as the high score.
When a game is active, the task sets the strings to the current high score and the current score.
After a game, while the end game screen is being displayed, the task strings to "GAME OVER" as well as the score of the current game, then resets
to the pre-game state.
When sleeping, the strings are updated to be empty. 
*/
int STRING_Tick(int state){
    switch (state){
        case STRING_Start:
            state = STRING_PreGame;
            break;
        case STRING_PreGame:
            if(gameOn == 0){
                strcpy(Line1, "PRESS START   ");
                snprintf(Line2, sizeof(Line2), "HIGH: %u    ", highscore);
            } else if(gameOn == 1) state = STRING_GameOn;
            if (sleeping == 1) { strcpy(Line1, ""); strcpy(Line2, ""); }
            break;
        case STRING_GameOn:
            snprintf(Line1, sizeof(Line1), "HIGH: %u    ", highscore);
            snprintf(Line2, sizeof(Line2), "SCORE: %u    ", score);
            if (sleeping == 1) { strcpy(Line1, ""); strcpy(Line2, ""); }
            if(collision == 1) state = STRING_PostGame;
            break;
        case STRING_PostGame:
            strcpy(Line1, "GAME OVER  ");
            snprintf(Line2, sizeof(Line2), "SCORE: %u  ", score);
            if (sleeping == 1) { strcpy(Line1, ""); strcpy(Line2, ""); }
            if(endGameScreen == 0) state = STRING_PreGame;
            break;
        default:
            break;
    }
    return state;
}

/*
This task handles the displaying to the ST7735 display.
It first fills the display white, then draws the bitmaps of the dinosaur and obstacle.
The task does not read when the game starts, and instead just displays the current bitmaps, 
in their respective positions, which are updated by their own tasks.
When the game ends, the task displays the last positions of the dinosaur and cactus, flashing it on the display before resetting.
When the system is sleeping, the display is set to show the starting screen with the dinosaur and cactus, but will have no backlight.
The backlight itself is handed in the Sleep task.
*/
int DISPLAY_Tick(int state){
    static unsigned char i;
    switch (state){
        case DISPLAY_Start:
            i = 0;
            endGameScreen = 0;
            DSP_fillScreen(GRAY); // background
            if (collision == 0) state = DISPLAY_Gameplay;
            break;
        case DISPLAY_Gameplay:

            DSP_drawBitmap(dinosaur.x, dinosaur.y, dinosaur.bitmap, dinosaur.width, dinosaur.height, DARK_GRAY, GRAY);
            DSP_drawBitmap(obstacle.x, obstacle.y, obstacle.bitmap, obstacle.width, obstacle.height, obstacle.color, GRAY);


            if(collision == 1 && sleeping == 0) { endGameScreen = 1; state = DISPLAY_Endgame; }
            else if(collision == 1 && sleeping == 1) { state = DISPLAY_Start; }
            break;
        case DISPLAY_Endgame:
            if(i % 2 == 0){
                DSP_drawBitmap(dinosaur.x, dinosaur.y, dinosaur.bitmap, dinosaur.width, dinosaur.height, BLACK, GRAY);
                DSP_drawBitmap(obstacle.x, obstacle.y, obstacle.bitmap, obstacle.width, obstacle.height, BLACK, GRAY);
            } else {               
                DSP_drawBitmap(dinosaur.x, dinosaur.y, dinosaur.bitmap, dinosaur.width, dinosaur.height, GRAY, GRAY);
                DSP_drawBitmap(obstacle.x, obstacle.y, obstacle.bitmap, obstacle.width, obstacle.height, GRAY, GRAY);
            }

            i++;

            if (i >= 20) { endGameScreen = 0; state = DISPLAY_Start; }
            break;
        default: 
            break;
    }
    return state;
}

/*
This task handles all behavior relating to the 16x2 LCD display.
It simply prints the value of Line1, then Line2, then repeats indefinitely.
When the system is sleeping, it clears the LCD.
*/
int LCD_Tick(int state) {
    switch(state) {
        case LCD_Start:
            lcd_clear();
            lcd_clear();
            if(sleeping == 0) state = LCD_Line1;
            break;
        case LCD_Line1:
            lcd_goto_xy(0, 0);
            lcd_write_str(Line1);
            if(sleeping == 0) state = LCD_Line2;
            else if (sleeping == 1){ lcd_clear(); state = LCD_Start; }
            break;
        case LCD_Line2:
            lcd_goto_xy(1,0);
            lcd_write_str(Line2);
            if(sleeping == 0) state = LCD_Line1;
            else if (sleeping == 1) { lcd_clear(); state = LCD_Start; }
            break;
        default:
            break;
    }
    return state;
}

/*
This task handles all behavior relating to the system sleeping.
When it detects a button press on the right button, it triggers the system to sleep, turning off both displays 
(handled by the L293D a dual H bridge motor driver repurposed to act similarly to a MOSFET, allowing for control over the backlighting)
The system also turns the CPU off, specifically, it puts the CPU in power down mode, the deepest sleep mode that the CPU can go.
The system then waits for an interrupt activated by the same button, which wakes the system and resets it.
Since the off button can be pressed at any time, it will simply reset the game to pre-game state.
If a game was ended due to the off button being pressed, a new high score will still be saved.
*/
int SLEEP_Tick(int state){
    switch (state){
        case SLEEP_Start:
            sleeping = 0;
            displayOn();
            state = SLEEP_Read;
            break;
        case SLEEP_Read:
            // Reads the right button and triggers sleep
            if(GetBit(PINC, 5) == 1){
                sleeping = 1;
                displayOff();
                state = SLEEP_Press;
            }
            break;
        case SLEEP_Press:
            if(GetBit(PINC, 5) == 0 && sleeping == 1){
                
                set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Deepest sleep - wakes on external interrupt

                interrupt_ON(); // Allow for the right button to act as an interrupt to wake the system

                sleep_enable(); // Allow CPU to enter sleep

                sleep_cpu();  // Enter sleep - CPU stops here until interrupt

                sleep_disable(); // Resume after wake

                interrupt_OFF(); // Stops the system from seeing the right button as an interrupt as a security measure

                sleeping = 0;

                state = SLEEP_Start;
            }
            break; 
        default:
            break;
    }
    return state;
}