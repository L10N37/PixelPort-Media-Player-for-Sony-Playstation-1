#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>
#include "libgte.h" // Include the existing definition

// Define font coordinates
#define FONTX   960
#define FONTY   0

// Function prototypes
int bcdToDecimal(unsigned char bcd);
void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size);

// Declare external color vectors for font without defining them
extern CVECTOR fntColor;    // Foreground color
extern CVECTOR fntColorBG;  // Background color
void FntColor(CVECTOR fgcol, CVECTOR bgcol);

#endif // TOOLS_H
