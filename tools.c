int bcdToDecimal(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void convertBcdValuesToDecimal(unsigned char bcdValues[], int decimalValues[], int size) {
    for (int i = 0; i < size; i++) {
        decimalValues[i] = bcdToDecimal(bcdValues[i]);
    }
}

unsigned char decimalToBCD(int decimal) { //positive decimal to BCD, 100 limit, no bounds checks
    if (decimal == 100) {
        return 0x64; // BCD for 100

    // BCD conversion for numbers from 0 to 99
    return ((decimal / 10) << 4) | (decimal % 10);
    }
}

////////////////////////////////////////////////////Font stuff (https://wiki.arthus.net/?psxdev-change_font_color)
#define FONTX   960
#define FONTY   0
// Two color vectors R,G,B
CVECTOR fntColor;
CVECTOR fntColorBG;

void FntColor(CVECTOR fgcol, CVECTOR bgcol )
{
    // The debug font clut is at tx, ty + 128
    // tx = bg color
    // tx + 1 = fg color
    // We can override the color by drawing a rect at these coordinates
    // 
    // Define 1 pixel at 960,128 (background color) and 1 pixel at 961, 128 (foreground color)
    RECT fg = { FONTX+1, FONTY + 128, 1, 1 };
    RECT bg = { FONTX, FONTY + 128, 1, 1 };
    // Set colors
    ClearImage(&fg, fgcol.r, fgcol.g, fgcol.b);
    ClearImage(&bg, bgcol.r, bgcol.g, bgcol.b);
}
///////////////////////////////////////////////////