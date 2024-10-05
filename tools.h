#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>
#include "libgte.h"

// Define font coordinates
#define FONTX   960
#define FONTY   0

// Function prototypes
int bcdToDecimal(unsigned char bcd);
void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size);
int isValueInArray(int value, int *array, int size);
void checkDriveLidStatus();

extern CdlLOC loc[100];         
extern u_char result[8];  
extern CVECTOR fntColor;    // Foreground color
extern CVECTOR fntColorBG;  // Background color
void FntColor(CVECTOR fgcol, CVECTOR bgcol);
void getTableOfContents();

#endif
