#include "timerISR.h"
#include "stdlib.h"
#include "helper.h"
#include "periph.h"

#include "bcast107_gameGraphics.h"
#include "bcast107_ST7735Display.h"

#ifndef BCAST107_GAMEFUNCTIONS_H
#define BCAST107_GAMEFUNCTIONS_H

// Joystick directions
#define neutral 0
#define up 1
#define down 2
#define left 3
#define right 4

// Game Object structs
struct _hitbox{
    char x;
    char y;
    unsigned char width;
    unsigned char height;
};

struct Game_Object{
    char x; // x position on the display
    char y; // y position on the display
    char min; // same object for cactus, pterodactyl and dino, so for dino min, max are Y values, for obstacles they are x values
    char max;
    const unsigned char* bitmap; // points to the current bitmap
    unsigned char bitmap_Num; // which bitmap is being used 
    unsigned char width; // width of bitmap
    unsigned char height; // height of bitmap
    char vx;  // x velocity
    char vy;  // y velocity
    unsigned char isCactus; // flags if the current object is a cactus
    unsigned long color; // tracks the color, used for obstacle generation
    _hitbox hitbox; // object hitbox
};

/* 
Reads the joystick and assigns it a directional value
Note: in this instance, the oriention of the joystick is such that the wires point in the up direction, thus,
Vx actually acts as the Y direction, and is labelled as such.
*/
unsigned char jstk_Read(){
    unsigned long jstk_Y = ADC_read(3);
    unsigned long jstk_X = ADC_read(2);
    
    if(jstk_Y > 896 && jstk_X < 722 && jstk_X > 150){
      return down;
    } else if(jstk_Y < 352 && jstk_X < 722 && jstk_X > 300){
      return up;
    } else if(jstk_X > 722 && jstk_Y < 896 && jstk_Y > 352){
      return left;
    } else if(jstk_X < 300 && jstk_Y < 896 && jstk_Y > 352){
      return right;
    } else if(jstk_X >  300 && jstk_X < 722 && jstk_Y > 352 && jstk_Y < 896) return 0;

    return neutral;
}

// Dinosaur initialization function
Game_Object init_Dino(){
    Game_Object dinosaur;
    dinosaur.x = 10;
    dinosaur.min = 66;
    dinosaur.max = 16;
    dinosaur.y = 66;
    dinosaur.bitmap = dino_bitmap1;
    dinosaur.bitmap_Num = 1;
    dinosaur.width = DINO_WIDTH;
    dinosaur.height = DINO_HEIGHT;
    dinosaur.vx = 0;
    dinosaur.vy = 8;

    return dinosaur;
}

// Cactus initialization function
Game_Object init_Cactus(){
    Game_Object cactus;
    cactus.x = 108;
    cactus.min = 108;
    cactus.max = -20;
    cactus.y = 91;        
    cactus.bitmap = cactus_bitmap1;
    cactus.bitmap_Num = 1;
    cactus.width = CACTUS_WIDTH;
    cactus.height = CACTUS_HEIGHT;
    cactus.isCactus = 1;
    cactus.color = DARK_GRAY;

    return cactus;
}

// Pterodactyl initialization function
Game_Object init_Pterodactyl(){
    Game_Object pterodactyl;
    pterodactyl.x = 108;
    pterodactyl.min = 108;
    pterodactyl.max = -20;
    pterodactyl.y = 73;        
    pterodactyl.bitmap = pterodactyl_bitmap1;
    pterodactyl.bitmap_Num = 1;
    pterodactyl.width = PTERODACTYL_WIDTH;
    pterodactyl.height = PTERODACTYL_HEIGHT;
    pterodactyl.isCactus = 0;
    pterodactyl.color = DARK_GRAY;

    return pterodactyl;
}

// Randomly selects which obstacle to generate, then returns it for use in the Obstacle task
Game_Object setObstacle(Game_Object cactus, Game_Object pterodactyl){
  unsigned char gen = rand() % 10;
  Game_Object obstacle;

  if(gen < 7){
    obstacle.x = cactus.x;
    obstacle.y = cactus.y;
    obstacle.min = cactus.min;
    obstacle.max = cactus.max;
    obstacle.bitmap = cactus.bitmap;
    obstacle.bitmap_Num = cactus.bitmap_Num;
    obstacle.width = cactus.width;
    obstacle.height = cactus.height; 
    obstacle.isCactus = cactus.isCactus;
  } else if(gen > 7) {
    obstacle.x = pterodactyl.x;
    obstacle.y = pterodactyl.y;
    obstacle.min = pterodactyl.min;
    obstacle.max = pterodactyl.max;
    obstacle.bitmap = pterodactyl.bitmap;
    obstacle.bitmap_Num = pterodactyl.bitmap_Num;
    obstacle.width = pterodactyl.width;
    obstacle.height = pterodactyl.height; 
    obstacle.isCactus = pterodactyl.isCactus;
  }
  return obstacle;
}

// Updates the object's hitbox based on the current location
void updateHitbox(_hitbox *hb, char x, char y, unsigned char width, unsigned char height) {
    hb->x = x;
    hb->y = y;
    hb->width = width;
    hb->height = height;
}

// Checks for collision based on the obstacle and dinosaur sizes and positions
char checkCollision(_hitbox *a, _hitbox *b) {
    if (a->x < b->x + b->width &&
        a->x + a->width > b->x &&
        a->y < b->y + b->height &&
        a->y + a->height > b->y) {
        return 1; // Collision detected
    }
    return 0; // No collision
}

#endif