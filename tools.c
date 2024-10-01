#include "common.h"

// Global instances for font colors
CVECTOR fntColor;      // Foreground color instance
CVECTOR fntColorBG;    // Background color instance

// Font position coordinates for changing system font colors
#define FONTX   960
#define FONTY   0

//////////////////////////////////////////////////// Font stuff (https://wiki.arthus.net/?psxdev-change_font_color)

void FntColor(CVECTOR fgcol, CVECTOR bgcol) {
    // Define 1 pixel at FONTX, FONTY + 128 (background color) and at FONTX + 1, FONTY + 128 (foreground color)
    RECT fg = { FONTX + 1, FONTY + 128, 1, 1 }; // Ensure RECT is correctly defined
    RECT bg = { FONTX, FONTY + 128, 1, 1 };     // Ensure RECT is correctly defined
    
    // Set colors
    ClearImage(&fg, fgcol.r, fgcol.g, fgcol.b); // Make sure ClearImage is declared
    ClearImage(&bg, bgcol.r, bgcol.g, bgcol.b); // Make sure ClearImage is declared
}

///////////////////////////////////////////////////

int bcdToDecimal(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size) {
    for (int i = 0; i < size; i++) {
        decimalValues[i] = bcdToDecimal(bcdValues[i]);
    }
}
