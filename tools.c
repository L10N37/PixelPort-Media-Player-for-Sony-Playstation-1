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

// for random track order (shuffle)
int isValueInArray(int value, int *array, int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] == value) {
            return 1; // Value is already in the array (true)
        }
    }
    return 0; // Value not found (false)
}

void checkDriveLidStatus(){
    CdControlB( CdlNop, 0, result);
    while  (result[0] == 0) 
    {
        getTableOfContents(); 
        CdControlF (CdlPlay, (u_char *)&loc[1]); // Grab new CD's TOC and play track 1
        break;
    }
}
/*
Detecting a Swapped CD
To see whether the CD has been replaced, the following two tests should be performed: (1) determine
whether the cover has been opened; and (2) determine the spindle rotation. Either test can be performed
using the CdlNop command.
CdControlB( CdlNop, 0, result ); /* char result[ 8 ]; 
1. The opening and closing of the cover is reflected in the CdlStatShellOpen bit of result[0]. The
CdlStatShellOpen bit detects an open cover, and has the following settings:
Cover is open: always 1
Cover is closed: 1, the first time this condition is detected, 0 for subsequent times
Thus, if this bit makes a transition from 1 to 0, it can be assumed that the CD has been swapped.
2. Use the CdlNop command and wait for bit 1 of result [ 0 ] (0x02) to change to 1
*/

