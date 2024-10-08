#ifndef TOOLS_H
#define TOOLS_H

#include "common.h"

// Define font coordinates- remove this later and draw everything at manually selected locations with 'void drawTextAtPosition(int x, int y, const char* text)' in tools.c
#define FONTX   960
#define FONTY   0

// byte we are interested in for CdlNop return 
#define cdlNopStatusByte result[0]

// Function prototypes
void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size);
void checkDriveLidStatus();
void FntColor(CVECTOR fgcol, CVECTOR bgcol);
void getTableOfContents();
void playerInformationLogic();
void display();
void initFont();
void drawTextAtPosition(int x, int y, const char* text);
void shuffleFunction();
void selectCustomTracks();
void display();
void balanceControl();
void initSpu(int applyVolumeCdLeft, int applyVolumeCdRight);

int isValueInArray(int value, int *array, int size);
int convertToPercentage(int volume);
int shuffleModeSelection(int button);
int bcdToDecimal(unsigned char bcd);

// shared variables
extern u_char result[8];
extern u_char result[8];
extern CdlLOC loc[100];
extern CVECTOR fntColor;    // Foreground color
extern CVECTOR fntColorBG;  // Background color

extern unsigned char startBtnMenu[];
extern unsigned char background[];

extern int trackCount;
extern int numTracks;
extern int shuffledTracks[101];
extern int decimalValues[8];

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

typedef struct {
    bool repeat;                    
    bool shuffle;                   
    bool selectedShuffleMode;
    bool shuffleSelectionBreakEarly;
    bool reverb;
    bool mute;
} statusFlags;

extern statusFlags flag;

#endif