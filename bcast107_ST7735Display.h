#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "bcast107_gameGraphics.h"

#ifndef BCAST107_ST7735DISPLAY_H
#define BCAST107_ST7735DISPLAY_H

// Display control pins on PORTB
#define DC    PB1
#define RESET PB0
#define CS    PB2
#define MOSI  PB3
#define SCK   PB5

// Display dimensions
#define DSP_WIDTH  128
#define DSP_HEIGHT 128

// Offsets for display
#define X_OFFSET 2
#define Y_OFFSET 1

// Display SPI Initialization
void DSP_SPI_init(){
    DDRB |= (1 << MOSI) | (1 << SCK) | (1 << CS) | (1 << DC) | (1 << RESET);
    SPCR = (1 << SPE) | (1 << MSTR);
}

// Send one byte over SPI
void DSP_SPI_send(unsigned char data){
    SPDR = data;
    while(!(SPSR & (1 << SPIF)));
}

// Send command to display
void DSP_command(unsigned char cmd){
    PORTB &= ~(1 << CS);
    PORTB &= ~(1 << DC);
    DSP_SPI_send(cmd);
    PORTB |= (1 << CS);
}

// Send data to display
void DSP_data(unsigned char data){
    PORTB &= ~(1 << CS);
    PORTB |= (1 << DC);
    DSP_SPI_send(data);
    PORTB |= (1 << CS);
}

// Hardware reset the display
void DSP_reset(){
    PORTB |= (1 << RESET);
    _delay_ms(200);
}

// Set drawing window with offset
void DSP_setAddrWindow(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1){
    x0 += X_OFFSET;
    x1 += X_OFFSET;
    y0 += Y_OFFSET;
    y1 += Y_OFFSET;

    DSP_command(0x2A);
    DSP_data(0x00); 
    DSP_data(x0);
    DSP_data(0x00); 
    DSP_data(x1);

    DSP_command(0x2B);
    DSP_data(0x00); 
    DSP_data(y0);
    DSP_data(0x00); 
    DSP_data(y1);

    DSP_command(0x2C);
}

// Initialize the display
void DSP_init(){
    DSP_reset();

    DSP_command(0x01); 
    _delay_ms(150);
    DSP_command(0x11); 
    _delay_ms(200);

    DSP_command(0x3A);
    DSP_data(0x05);
    _delay_ms(10);

    DSP_command(0x29);
    _delay_ms(200);
}

// Fill rectangle with a color
void DSP_fillRect(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned short color){
    DSP_setAddrWindow(x0, y0, x1, y1);
    unsigned short n = (x1 - x0 + 1) * (y1 - y0 + 1);
    for (unsigned short i = 0; i < n; i++){
        DSP_data(color >> 8);
        DSP_data(color & 0xFF); 
    }
}

// Fill the entire screen white before outputting anything else
void DSP_fillScreen(unsigned short color){
    DSP_fillRect(0, 0, DSP_WIDTH - 1, DSP_HEIGHT - 1, color);
}

/*
Key function for gameplay -
This function draws out a bitmap onto the display, taking the desired x and y coordinates, and outputting each byte to the display in order.
The function prints row by row, following the strict width and height of the bitmap.
The function receives 2 color codes, one for the color of the game object, and one for the background color.
Since the display does not automatically reset any pixels, the bitmaps had to be large enough to cover the old pixels that were used for the old object position.
This resulted in the most significant, and only bug, which is a ghosting effect when the dinosaur and obstacle bitmaps overlap due to the bitmaps being printed over each other.
Due to the limited memory available on the arudino, printing more than 2 bitmaps, or attempting to reset the entire display every frame to avoid ghosting was impractical and resulted in large performance dropoffs.
*/
void DSP_drawBitmap(char x, char y, const unsigned char *bitmap, char width, char height, short color, short bg_color) {
    char bytes_per_row = width / 8;
    for (char row = 0; row < height; row++) {
        for (char byte = 0; byte < bytes_per_row; byte++) {
            char line = bitmap[row * bytes_per_row + byte];
            for (char bit = 0; bit < 8; bit++) {
                char px = x + byte * 8 + bit;
                char py = y + row;
                if (px < DSP_WIDTH && py < DSP_HEIGHT) {
                    DSP_setAddrWindow(px, py, px, py);
                    // Choose color based on bit value
                    if (line & (1 << (7 - bit))) {
                        DSP_data(color >> 8);
                        DSP_data(color & 0xFF);
                    } else {
                        DSP_data(bg_color >> 8);
                        DSP_data(bg_color & 0xFF);
                    }
                }
            }
        }
    }
}

// Turns off the backlights for both displays
void displayOff(){
    PORTC |= 0x01;
    PORTB &= 0xEF;
}


// Turns on the backlights for both displays
void displayOn(){
    PORTC &= 0xFE;
    PORTB |= 0x10;

}

#endif