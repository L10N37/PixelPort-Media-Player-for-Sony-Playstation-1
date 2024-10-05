#include "tools.h"
#include "libcd.h"
#include "types.h"
#include <stdbool.h>
#include <stdio.h>

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

    #define subsequentLidClose 0
    bool lidOpenAtBoot= false;

    ////////////  check cdlNopStatusByte ////////////
    CdControlB( CdlNop, 0, result);
    ////////////////////////////////////////////////

        // bail out of the function if lid open at boot/ app launch - prevents app lock up while searching for TOC (blank screen) until lid closure
        if (cdlNopStatusByte == CdlStatShellOpen && lidOpenAtBoot){
        lidOpenAtBoot=!lidOpenAtBoot;
        return;
        }

        if (shuffle && cdlNopStatusByte == CdlStatShellOpen) // if shuffle was on when you opened the drive lid
        {                                          
            shuffle = false;                       // toggle the shuffle flag to false in case shuffle was on at disc swap
            CdPlay(0, NULL, 0);// cancel shuffle mode
            for (int i = 0; i < 101; i++)         // reset shuffledTracks array elements to 0
            {        
            shuffledTracks[i] = 0;
            }
        }

    //  cdlNopStatusByte returns 0x02 (CdlStatStandby) if lid is closed at boot, 0x00 if closed after boot
    if  ((cdlNopStatusByte == CdlStatStandby && !shuffle) || (cdlNopStatusByte == subsequentLidClose && !shuffle))

    {   
        numTracks = 0;                         // init numTracks to 0;
        if (cdlNopStatusByte == CdlStatShellOpen) printf("\ndrive lid open- result[0] (CdlNop): %x", cdlNopStatusByte); //cdlNopStatusByte: 0
        while (cdlNopStatusByte == CdlStatShellOpen){initFont(); display();}; //hang here if lid is open
        if (cdlNopStatusByte == subsequentLidClose) printf("\ngetting toc - result[0] (CdlNop): %x", cdlNopStatusByte); //cdlNopStatusByte: 0
        if (cdlNopStatusByte == CdlStatStandby) printf("\nbooted up with drive lid closed, getting toc - result0 (CdlNop): %x", cdlNopStatusByte); // cdlNopStatusByte: 2
        while ((numTracks = CdGetToc(loc)) == 0){initFont(); display();}; // lock here until TOC is retrieved
        printf("\nTrack Count: %0d", numTracks );
        getTableOfContents(); // CD information calcs and print outs
        CdControlF (CdlPlay, &loc[1].track); // play track 1
    }
}

/*
p195 LibOver47.pdf

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

p178 LibOver47.pdf

Status Bit Assignments
The first byte of the result of almost all commands indicates CD-ROM status. The bit assignments of the
status byte are as shown below. Use the command CdlNop if you wish to obtain the CD-ROM status only.
Table 11-5: Bit Assignments of Status Byte
Symbol Code Details
CdlStatPlay 0x80 1: CD-DA playing
CdlStatSeek 0x40 1: seeking
CdlStatRead 0x20 1: reading data sector
CdlStatShellOpen 0x10 1: shell open*
CdlStatSeekError 0x04 1: error during seeking/reading
CdlStatStandby 0x02 1: motor rotating
CdlStatError 0x01 1: command issue error
*This flag is cleared by the CdlNOP command. Therefore, in order to decide if the cover is
currently open or not and before checking this flag, the CdlNOP command must be issued
at least once.
*/