#ifndef TOOLS_H
#define TOOLS_H

#include "common.h"

// Define font coordinates
#define FONTX   960
#define FONTY   0

// byte we are interested in for CdlNop return 
#define cdlNopStatusByte result[0]

// Function prototypes
int bcdToDecimal(unsigned char bcd);
void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size);
int isValueInArray(int value, int *array, int size);
void checkDriveLidStatus();
void FntColor(CVECTOR fgcol, CVECTOR bgcol);
void getTableOfContents();
void playerInformationLogic();
void display();
void initFont();

// shared variables
extern u_char shuffle;
extern CdlLOC loc[100];
extern u_char result[8];  
extern CVECTOR fntColor;    // Foreground color
extern CVECTOR fntColorBG;  // Background color
extern int numTracks;
extern int shuffledTracks[101];
extern int decimalValues[8];
extern u_char result[8];

// for readability when accessing decimalValues variable
typedef enum {
    track,
    index,
    min,
    sec,
    frame,
    amin,
    asec,
    aframe
} TrackInfoIndex;

#endif