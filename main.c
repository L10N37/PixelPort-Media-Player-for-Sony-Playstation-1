    /*
    ===========================================================
            Sony PlayStation 1 Source Code
    ===========================================================
                            TIM EXAMPLE
    -----------------------------------------------------------

      Developer / Programmer..............: PSXDEV.net
      Software Ddevelopment Kit...........: PSY-Q
      First Release.......................: 26/JANUARY/2013
      Last Release........................: 03/JULY/2013
      Version.............................: 1.2

      What was last changed: RAM address load option

      Copyright (C) 1994,1995 by Sony Computer Entertainment Inc.
              All Rights Reserved.

      Sony Computer Entertainment Inc. Development Department

                http://psxdev.net/

    -----------------------------------------------------------*/

    /*
    Spliced with // CDDA track playback example
    Schnappy 07-2021

    Added Media Player functionality - vajskids 2024
    randomising code stayed, but not used yet
    mostly rewritten
    */

    #include "common.h"
    // TIM image
    #include "background.h"
    #include <stdio.h>

    #define OT_LENGTH 1                             // ordering table length
    #define PACKETMAX (300)                         // the max num of objects
    #define MAX_VOLUME_CD 32767                     // maximum volume for an audio CD (0x7FFF)
    #define MAX_VOLUME_MASTER 16383                 // maximum master volume (0x3FFF)

    GsOT      WorldOrderingTable[2];                 // ordering table handlers
    GsOT_TAG  OrderingTable[2][1<<OT_LENGTH];        // ordering tables
    PACKET    GPUOutputPacket[2][PACKETMAX];         // GPU Packet work area

    GsSPRITE image1[2];                              // sprite handler
    
    unsigned long __ramsize =   0x00200000;          // force 2 megabytes of RAM
    unsigned long __stacksize = 0x00004000;          // force 16 kilobytes of stack

    unsigned char image[];       // bg image array converted from TIM file

    // Globals -----
    long int image1_addr;				// DRAM address storage of TIM file
    CdlLOC loc[100];                    // Array to store locations of tracks
    u_char cdInfoGetlocp[8];
    int currentbuffer; // double buffer holder
    int decimalValues[8];               // to hold decimal values from GETLOCP after conversion from BCD (uchar?)
    int CdVolume = MAX_VOLUME_CD;
    u_char shuffle = 0;
    u_char result[8];                    // general response storage
    int numTracks;                       // for storing number of audio tracks found on CD
    int debounceTimer= 0;                // controller input debounce timer
    unsigned int currentTrackTimeInSeconds = 0;  // to hold current track time in seconds
    int shuffledTracks[101];    // Max tracks 100, for shuffle/ built in CdPlay function we need an extra element to contain zero (could have implemented shuffle manually though)
    int repeat = 1;                     //track if repeat mode is on (repeat the CD when finished), on by default
    
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

    typedef struct 
    {
                int mins;
                int seconds;
    } TrackTime;
    
    void initFont()
    {
        static u_char fontColours[3] = {0x00, 0x00, 0x00};
        static int hue = 0; // Use an integer for hue cycling
        static int direction[3] = {1, 1, 1}; // Direction for each color channel (1 for increasing, -1 for decreasing)
        static int step = 1; // Step size for color change
        static int colorIndex = 0; // Index for current color channel
        static int brightThreshold = 200; // Brightness threshold to switch colors

        FntLoad(960, 256);  // Load basic font pattern
        FntLoad(FONTX, FONTY);                // Load font to VRAM at FONTX, FONTY
        FntOpen(10, 10, 320, 240, 0, 512);    // FntOpen(x, y, width, height, black_bg, max. nbr. chars)

        // Update RGB values based on hue for smooth transitions
        fontColours[colorIndex] += direction[colorIndex] * step;

        // Reverse direction if we hit bounds
        if (fontColours[colorIndex] >= 255) {
            fontColours[colorIndex] = 255;
            direction[colorIndex] = -1; // Change direction to decrease
        } else if (fontColours[colorIndex] <= 0) {
            fontColours[colorIndex] = 0;
            direction[colorIndex] = 1; // Change direction to increase
        }

        // Change color channel when bright enough
        if (fontColours[colorIndex] >= brightThreshold && direction[colorIndex] == 1) {
            colorIndex = (colorIndex + 1) % 3; // Move to the next color channel
        }

        fntColor.r = fontColours[0];
        fntColor.g = fontColours[1];
        fntColor.b = fontColours[2];
        FntColor(fntColor, fntColorBG);
    }
    
    void clearVRAM()
    {
      RECT rectTL;
      
      setRECT(&rectTL, 0, 0, 1024, 512);
      ClearImage2(&rectTL, 0, 0, 0);
      
        DrawSync(0); // ensure that the VRAM is clear before exiting
        
        printf("VRAM cleared!\n");
        return;
    }

    void initGraphics()
    {
      int SCREEN_WIDTH, SCREEN_HEIGHT;

      // puts display mask into the status specified by the mask (0 not displayed, 1 displayed)
      SetDispMask(1);
      
      // resets the graphic system (0 full reset, 1 cancels the current drawing and flushes the command buffer, 3 initialises the drawing engine while preserving the current display environment)
      ResetGraph(0);

      // clear all VRAM contents
      clearVRAM();

      // automatic video mode control
      if (*(char *)0xbfc7ff52=='E')  // SCEE string address
      {
        // PAL MODE
        SCREEN_WIDTH = 320;
        SCREEN_HEIGHT = 256;
        printf("Setting the PlayStation Video Mode to (PAL %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT,")");
        SetVideoMode(1);
        printf("Video Mode: PAL\n");
      }
      else
      {
        // NTSC MODE
        SCREEN_WIDTH = 320;
        SCREEN_HEIGHT = 240;
        printf("Setting the PlayStation Video Mode to (NTSC %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT,")");
        SetVideoMode(0);
        printf("Video Mode: NTSC\n");
      }

      // set the graphics mode resolutions
      GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsNONINTER|GsOFSGPU, 1, 0);

      // set the top left coordinates of the two buffers in video memory
      GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);

      // initialise the ordering tables
      WorldOrderingTable[0].length = OT_LENGTH;
      WorldOrderingTable[1].length = OT_LENGTH;
      WorldOrderingTable[0].org = OrderingTable[0];
      WorldOrderingTable[1].org = OrderingTable[1];
      
      GsClearOt(0,0,&WorldOrderingTable[0]);
      GsClearOt(0,0,&WorldOrderingTable[1]);

      printf("Graphics Initilised!\n");
    }

    // load tim image from ram into frame buffer slot
    // we use 2 arrays [0] and [1] to split the 320px image into a section of 256px and 64px because the PSX can not draw greater than 256px (load 'image.tim' into TimTool to see). this gives us a total image size of 320px.
    void initImage()
    {
      RECT            rect;                                    // RECT structure
      GsIMAGE         tim_data;                                // holds tim graphic info
        
      // put data from tim file into rect         
      //GsGetTimInfo ((u_long *)(image1_addr+4),&tim_data);
      GsGetTimInfo ((u_long *)(image+4),&tim_data);

      // load the image into the frame buffer
      rect.x = tim_data.px;                                    // tim start X coord
      rect.y = tim_data.py;                                    // tim start Y coord
      rect.w = tim_data.pw;                                    // data width
      rect.h = tim_data.ph;                                    // data height
      // load the tim data into the frame buffer
      LoadImage(&rect, tim_data.pixel);       

      // load the CLUT into the frame buffer
      rect.x = tim_data.cx;                                    // x pos in frame buffer
      rect.y = tim_data.cy;                                    // y pos in frame buffer
      rect.w = tim_data.cw;                                    // width of CLUT
      rect.h = tim_data.ch;                                    // height of CLUT
      // load data into frame buffer (DMA from DRAM to VRAM)
      LoadImage(&rect, tim_data.clut);                


      // initialise sprite
      image1[0].attribute=0x2000000;                           // 16 bit CLUT, all options off (0x1 = 8-bit, 0x2 = 16-bit)
      image1[0].x = 0;                                         // draw at x coord 0
      image1[0].y = 0;                                         // draw at y coord 0
      image1[0].w = 256;                                       // width of sprite
      image1[0].h = tim_data.ph;                               // height of sprite
      // texture page | texture mode (0 4-bit, 1 8-bit, 2 16-bit), semi-transparency rate, texture x, texture y in the framebuffer
      image1[0].tpage=GetTPage(1, 2, 320, 0);
      image1[0].r = 128;                                       // RGB Data
      image1[0].g = 128;
      image1[0].b = 128;
      image1[0].u=0;                                           // position within timfile for sprite
      image1[0].v=0;                                           
      image1[0].cx = tim_data.cx;                              // CLUT location x coord
      image1[0].cy = tim_data.cy;                              // CLUT location y coord
      image1[0].r=image1[0].g=image1[0].b=128;                 // normal luminosity
      image1[0].mx = 0;                                        // rotation x coord
      image1[0].my = 0;                                        // rotation y coord
      image1[0].scalex = ONE;                                  // scale x coord (ONE = 100%)
      image1[0].scaley = ONE;                                  // scale y coord (ONE = 100%)
      image1[0].rotate = 0;                                    // degrees to rotate   


      // initialise sprite
      image1[1].attribute=0x2000000;                           // 16 bit CLUT, all options off (0x1 = 8-bit, 0x2 = 16-bit)
      image1[1].x = 256;                                       // draw at x coord 0
      image1[1].y = 0;                                         // draw at y coord 0
      image1[1].w = 64;                                        // width of sprite
      image1[1].h = tim_data.ph;                               // height of sprite
      // texture page | texture mode (0 4-bit, 1 8-bit, 2 16-bit), semi-transparency rate, texture x, texture y in the framebuffer
      image1[1].tpage=GetTPage(1, 2, 576, 0);
      image1[1].r = 128;                                       // RGB Data
      image1[1].g = 128;
      image1[1].b = 128;
      image1[1].u=0;                                           // position within timfile for sprite
      image1[1].v=0;                                           
      image1[1].cx = tim_data.cx;                              // CLUT Location x coord
      image1[1].cy = tim_data.cy;                              // CLUT Location y coord
      image1[1].r=image1[1].g=image1[1].b=128;					// normal luminosity
      image1[1].mx = 0;                                        // rotation x coord
      image1[1].my = 0;                                        // rotation y coord
      image1[1].scalex = ONE;                                  // scale x coord (ONE = 100%)
      image1[1].scaley = ONE;                                  // scale y coord (ONE = 100%)
      image1[1].rotate = 0;                                    // degrees to rotate   

      // wait for all drawing to finish
      DrawSync(0);
    }


    void display()
    {
      // refresh the font
      FntFlush(-1);

      // get the current buffer
      currentbuffer=GsGetActiveBuff();

      // setup the packet workbase
      GsSetWorkBase((PACKET*)GPUOutputPacket[currentbuffer]);

      // clear the ordering table
      GsClearOt(0,0,&WorldOrderingTable[currentbuffer]);

      // insert sprites into the ordering table
      GsSortSprite(&image1[0], &WorldOrderingTable[currentbuffer], 0);
      GsSortSprite(&image1[1], &WorldOrderingTable[currentbuffer], 0);

      // wait for all drawing to finish
      DrawSync(0);

      // wait for v_blank interrupt
      VSync(0);

      // flip double buffers
      GsSwapDispBuff();

      // clear the ordering table with a background color
      GsSortClear(0,0,0,&WorldOrderingTable[currentbuffer]);

      // Draw the ordering table for the currentbuffer
      GsDrawOt(&WorldOrderingTable[currentbuffer]);

    }

    void printSpu()
    {

    // get SPU Attributes for printing
    SpuCommonAttr attr;
    SpuGetCommonAttr(&attr);
    // current CD volume
    printf("CD Volume Left: %d\n", attr.cd.volume.left), printf("CD Volume Right: %d\n", attr.cd.volume.right);
    printf("Reverb (on/off): %x\n", attr.cd.reverb);
    printf("CD Mix (on/off): %x\n", attr.cd.mix);

    }

    void initSpu(int applyVolumeCd, int applyVolumeMaster) 
    {
/*
        The volume mode is ‘direct mode’ and the range of the values which can be set to the mvolL and mvolR
        volumes is equal to that of the ‘direct mode’ in SpuSetVoiceAttr() (-0x4000 to 0x3fff).
*/      SpuSetCommonMasterVolume(applyVolumeMaster, applyVolumeMaster);

        //Set independently for left and right in the range -0x8000 - 0x7fff. If volume is negative, the phase is inverted.
        SpuSetCommonCDVolume(applyVolumeCd, applyVolumeCd);

        // No output without CD Mixing turned on
        SpuSetCommonCDMix(1);

        printSpu();
        
    }

    void shuffleFunction(int numTracks, int *shuffledTracks) {
        long rootCounterValue = GetRCnt(0);  // Get the root counter value

        if (rootCounterValue != -1) {
            srand((unsigned int)rootCounterValue);  // Seed the random number generator
        } else {
            printf("Error: Failed to get root counter value.\n");
            return; // Exit if seeding fails
        }

        // Start from index 1 to store track numbers
        for (int i = 1; i <= numTracks; i++) { // Fill from index 1 to numTracks
            int newTrack;
            do {
                newTrack = rand() % numTracks + 1; // Random value between 1 and numTracks
            } while (isValueInArray(newTrack, shuffledTracks, i)); // Ensure unique values

            shuffledTracks[i] = newTrack; // Store the unique value
            printf("\nTrack Order: %d\n", shuffledTracks[i]);
        }

        shuffledTracks[0] = numTracks; // Store total track count in index 0
        shuffledTracks[numTracks + 1] = 0; // Zero out the final array element (numTracks + 1)

        // Call CdPlay with repeat or stop mode
        if (repeat == 1) {
            CdPlay(2, shuffledTracks, decimalValues[index]); // Play with repeat
            printf("Shuffled tracks (repeat mode): 0x%02X\n", decimalValues[track]);
        } else if (repeat == 0) {
            CdPlay(1, shuffledTracks, decimalValues[index]); // Play and stop at the end
            printf("Shuffled tracks (stop mode): 0x%02X\n", decimalValues[track]);
        }
    }

    void playerInformationLogic(); // function dec here so we can call it as its below this function, rearrange later and remove this
    void readControllerInput()
    {
    
    int pad = 0x00;
    u_char trackValue;
    static int newVolumeLevelCd = MAX_VOLUME_CD;
    static int newVolumeLevelMaster = MAX_VOLUME_MASTER;
    trackValue = decimalValues[track]; 
    
    //slow down the polling
    if (debounceTimer%7 == 0)
    {                               
    pad = PadRead(0); // Read pads input. id is unused, always 0.
    }
    debounceTimer++;

        switch (pad) {
            /////////////////////////////////////   
            case PADLup:

                if (newVolumeLevelCd < MAX_VOLUME_CD) 
                {
                newVolumeLevelCd+=450; //tweak the steps!
                initSpu(newVolumeLevelCd, newVolumeLevelMaster);
                initSpu(newVolumeLevelCd, newVolumeLevelMaster);
                }
                printf("\nUP D-Pad pressed, volume increased\n");
                break;
            /////////////////////////////////////   
            case PADLdown:

                if (newVolumeLevelCd > 0) 
                {
                newVolumeLevelCd-=450; //tweak the steps!
                initSpu(newVolumeLevelCd, newVolumeLevelMaster);
                initSpu(newVolumeLevelCd, newVolumeLevelMaster);
                }
                printf("\nDown D-Pad pressed, volume decreased\n");
                break;
            /////////////////////////////////////   
            case PADLright: // Increment the track with wrap around to first track

                // Increment track value if not at last track
                if (trackValue < numTracks) // fix this, we can land on final track in shuffle mode but would still want to increment to next shuffled track
                {
                trackValue++;
                }
                // else if last track, go back to track 1, as long as shuffle isn't on
                if (trackValue == numTracks && shuffle == 0)
                {
                trackValue = 1;
                }
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]);
                printf("\nTrack value: %d\n", trackValue);
                printf("\nRight D-Pad pressed, increment track");
                break;
            /////////////////////////////////////   
            case PADLleft: // Decrement the track with wrap around to final track

                // if playing first track, go to final track, if shuffle is off!
                if (trackValue == 1 && shuffle == 0)
                {
                trackValue = numTracks;
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]);
                }
                // else decrement track value
                else if (trackValue <= numTracks && trackValue != 1 && shuffle == 0) //make sure shuffle isn't on
                {
                trackValue--;
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]);
                }
                printf("\nTrack value: %d\n", trackValue);
                printf("\nLeft D-Pad pressed, decrement track"); 
                break;
            /////////////////////////////////////      
            case PADRup:
                if (shuffle == 0)
                {
                    
                for (int i = 0; i < 101; i++) { // reset shuffledTracks array or locks up after turning on/off then on again
                shuffledTracks[i] = 0;
                }

                CdControlF (CdlStop, 0); // smoother?
                shuffleFunction(numTracks, shuffledTracks);
                shuffle = 1;
                }
                else if (shuffle == 1) // turn off shuffle
                {
                shuffle = 0;
                CdPlay(0, NULL, 0);
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]); //resume playing
                VSync( 3 );
                }
                break;
            /////////////////////////////////////   
            case PADRdown: // Play/Pause

            static u_char pause = 0;
            if (pause == 0) 
            {
            CdControlF (CdlPause, 0);
            pause = 1;
            printf("\nPAUSED");
            
            }
            else if (pause == 1)
            {
            CdControlF (CdlPlay, 0);
            pause = 0;
            printf("\nRESUMING PLAY FROM PAUSE");
            
            }
            break;
            /////////////////////////////////////   
            case PADRright: //Mute/ Demute

            static u_char mute = 0;
            if (mute == 0) 
            {
            CdControlF (CdlMute, 0);
            mute = 1;
            printf("\nAudio Muted");
            
            }
            else if (mute == 1)
            {
            CdControlF (CdlDemute, 0);
            mute = 0;
            printf("\nAudio Demuted");
            }
            break;
            /////////////////////////////////////   
            case PADRleft:
                        //   sq btn
                break;
            /////////////////////////////////////   
            case PADL1:
            CdControlF (CdlBackward, 0);
            while (pad == PADL1)
            {
                pad = PadRead(0);        // Keep re-reading the pad to check for release of forward
                printf("\nrewinding");
                playerInformationLogic();   //keep updating the track information
                display();                  // keep updating the display
            }               
            printf("\nrewind released");
            CdControlF (CdlPlay, 0); // Resume play, button released
                break;
            /////////////////////////////////////   
            case PADL2:
                        // L2
                break;
            /////////////////////////////////////   
            case PADR1:
            CdControlF (CdlForward, 0);
            while (pad == PADR1)
            {
                pad = PadRead(0); // Keep re-reading the pad to check for release of forward
                printf("\nfast forward");
                playerInformationLogic(); //keep updating the track information
                display();                // keep updating the display
            }               
            printf("\nfast forward released");
            CdControlF (CdlPlay, 0); // Resume play, button released
                break;
            /////////////////////////////////////   
            case PADR2:
                        // R2
                break;

            case PADstart:

                break;

            case PADselect:

                break;

            default:
                break;
        }
      }
          
    void calculateCurrentTrackLength() {
            // Get the current track number
            int currentTrack = decimalValues[track]; // Assuming this holds the current track number

            // Validate current track number
            if (currentTrack < 1 || currentTrack > numTracks) {
                printf("Invalid current track: %d\n", currentTrack);
                return;
            }

            // Temporary variables for BCD values and decimal conversion
            unsigned char bcdValues[2];
            int decimalValues[2];
            TrackTime trackStartTime[101]; // Local array to hold track start times
            int totalTimeInSeconds;

            // Get the total time (track 0)
            bcdValues[0] = loc[0].minute; // Total time minutes
            bcdValues[1] = loc[0].second; // Total time seconds
            convertBcdValuesToDecimal(bcdValues, decimalValues, 2);
            totalTimeInSeconds = (decimalValues[0] * 60) + decimalValues[1];

            // Get pregap from Track 1
            bcdValues[0] = loc[1].minute; // Track 1 minutes
            bcdValues[1] = loc[1].second; // Track 1 seconds
            convertBcdValuesToDecimal(bcdValues, decimalValues, 2);
            int pregapMinutes = decimalValues[0];
            int pregapSeconds = decimalValues[1];

            // Calculate track start times
            for (int i = 1; i <= numTracks; i++) {
                bcdValues[0] = loc[i].minute;
                bcdValues[1] = loc[i].second;
                convertBcdValuesToDecimal(bcdValues, decimalValues, 2);

                // Adjust for pregap
                int adjustedMinutes = decimalValues[0] - pregapMinutes;
                int adjustedSeconds = decimalValues[1] - pregapSeconds;

                // Handle seconds underflow
                if (adjustedSeconds < 0) {
                    adjustedMinutes--;
                    adjustedSeconds += 60;
                }

                // Handle minutes underflow
                if (adjustedMinutes < 0) {
                    adjustedMinutes = 0;
                    adjustedSeconds = 0;
                }

                trackStartTime[i].mins = adjustedMinutes;
                trackStartTime[i].seconds = adjustedSeconds;
            }

            // Calculate current track length
            int lengthMinutes, lengthSeconds;

            if (currentTrack < numTracks) {
                lengthMinutes = trackStartTime[currentTrack + 1].mins - trackStartTime[currentTrack].mins;
                lengthSeconds = trackStartTime[currentTrack + 1].seconds - trackStartTime[currentTrack].seconds;
            } else {
                // Last track length calculation using total time
                lengthMinutes = trackStartTime[0].mins - trackStartTime[currentTrack].mins;
                lengthSeconds = trackStartTime[0].seconds - trackStartTime[currentTrack].seconds;
            }

            // Handle seconds underflow for length
            if (lengthSeconds < 0) {
                lengthMinutes--;
                lengthSeconds += 60;
            }

            // Convert to total seconds
            currentTrackTimeInSeconds = (lengthMinutes * 60) + lengthSeconds;

            // Print the current track length in seconds
            printf("Track %02d Length: %02d:%02d (%d seconds)\n", currentTrack, lengthMinutes, lengthSeconds, currentTrackTimeInSeconds);
        }

        void getTableOfContents() {

            unsigned char bcdValues[2];
            int decimalValues[2];
            TrackTime trackStartTime[101]; // Array to hold track start times, including total time
            int trackLengths[101]; // Array to hold track lengths in seconds
            
            while ((numTracks = CdGetToc(loc)) == 0) 
            {
                FntPrint("No TOC found: please use CD-DA disc...\n");
            }

            // Print total time
            bcdValues[0] = loc[0].minute; // Total time minutes
            bcdValues[1] = loc[0].second; // Total time seconds
            convertBcdValuesToDecimal(bcdValues, decimalValues, 2);
            trackStartTime[0].mins = decimalValues[0];
            trackStartTime[0].seconds = decimalValues[1];
            printf("Total Time: %02d:%02d\n\n", trackStartTime[0].mins, trackStartTime[0].seconds);

            printf("Track starting positions:\n");

            // Get pregap from Track 1
            bcdValues[0] = loc[1].minute; // Track 1 minutes
            bcdValues[1] = loc[1].second; // Track 1 seconds
            convertBcdValuesToDecimal(bcdValues, decimalValues, 2);
            int pregapMinutes = decimalValues[0];
            int pregapSeconds = decimalValues[1];

            // Calculate track start times and store in trackStartTime
            for (int i = 1; i <= numTracks; i++) {
                bcdValues[0] = loc[i].minute;
                bcdValues[1] = loc[i].second;
                convertBcdValuesToDecimal(bcdValues, decimalValues, 2);

                // Deduct pregap
                int adjustedMinutes = decimalValues[0] - pregapMinutes;
                int adjustedSeconds = decimalValues[1] - pregapSeconds;

                // Handle seconds underflow
                if (adjustedSeconds < 0) {
                    adjustedMinutes--;
                    adjustedSeconds += 60;
                }

                // Handle minutes underflow
                if (adjustedMinutes < 0) {
                    adjustedMinutes = 0;
                    adjustedSeconds = 0;
                }

                trackStartTime[i].mins = adjustedMinutes;
                trackStartTime[i].seconds = adjustedSeconds;

                // Print the track start time
                printf("Track %02d | %02d:%02d\n", i, trackStartTime[i].mins, trackStartTime[i].seconds);
            }

            printf("\nTrack lengths:\n");

            // Calculate and print track lengths
            for (int i = 1; i < numTracks; i++) {
                int lengthMinutes = trackStartTime[i + 1].mins - trackStartTime[i].mins;
                int lengthSeconds = trackStartTime[i + 1].seconds - trackStartTime[i].seconds;

                // Handle seconds underflow for length
                if (lengthSeconds < 0) {
                    lengthMinutes--;
                    lengthSeconds += 60;
                }

                printf("Track %02d | %02d:%02d\n", i, lengthMinutes, lengthSeconds);
            }

            // Last track length calculation using total time
            int lastTrackLengthMinutes = trackStartTime[0].mins - trackStartTime[numTracks].mins;
            int lastTrackLengthSeconds = trackStartTime[0].seconds - trackStartTime[numTracks].seconds;

            // Handle seconds underflow for last track length
            if (lastTrackLengthSeconds < 0) {
                lastTrackLengthMinutes--;
                lastTrackLengthSeconds += 60;
            }

            // Adjust for the actual length of the last track
            if (lastTrackLengthMinutes < 0) {
                lastTrackLengthMinutes = 0;
                lastTrackLengthSeconds = 0;
            }

            printf("Track %02d | %02d:%02d\n", numTracks, lastTrackLengthMinutes, lastTrackLengthSeconds);
        }

    void playerInformationLogic() {
        u_char currentValues[8] = {0x00};
        CdControlB(CdlGetlocP, 0, currentValues);
        convertBcdValuesToDecimal(currentValues, decimalValues, 8); // convert return data from CdGetLocP from BCD to decimal

        // Print the CURRENT track information
        FntPrint("           %0d\n\n", numTracks); // Track Count main screen 
        FntPrint("           %0d\n\n", decimalValues[track]); // Current Track main screen

        if (shuffle == 0) {
            FntPrint("           OFF\n\n"); // Shuffle on or off? main screen
        } else {
            FntPrint("           ON\n\n");
        }

        FntPrint("           %02d:%02d\n", decimalValues[min], decimalValues[sec]); // time played, main screen

        // progress bar spacing
        FntPrint("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        
        // Calculate progress
        int totalSeconds = currentTrackTimeInSeconds; // Total track length in seconds
        int currentSeconds = (decimalValues[min] * 60) + decimalValues[sec]; // Current track time in seconds

        // Calculate the number of filled segments based on the total length
        int barLength = 37; // Length of the progress bar (37 dashes)
        int filledLength = (totalSeconds > 0) ? (currentSeconds * barLength) / totalSeconds : 0; // Calculate filled segments

        // Print filled and empty segments
        for (int i = 0; i < filledLength; i++) {
            FntPrint("=");
        }
        for (int i = filledLength; i < barLength; i++) {
            FntPrint("-");
        }
        FntPrint("\n");
        
        // Has the track changed? grab new tracks value in seconds if so
        static int hasTrackChanged = 0;
        static int oldTrack = 0;

        if (decimalValues[track] != 0) {   
            if (hasTrackChanged == 0) {
                oldTrack = decimalValues[track]; // old track now contains current track value
                hasTrackChanged = 1;
                calculateCurrentTrackLength(); // Grab the current track time in seconds
            }
        } 

        if (oldTrack != decimalValues[track]) { // if oldTrack variable is not equal to the current track being played
            hasTrackChanged = 0; // Now we can grab the new track's time in seconds
        }
/*
        if (shuffle == 1) //shuffle was turned on, we need to control cdlPlay's next track selection manually
        {
            if (tracksPlayed != numTracks)
            {
            while (currentSeconds == currentTrackTimeInSeconds) {}; // Lock up/ wait here on the last second of the track unless its the last track in the shuffle array / playlist                                                 
            u_char nextTrack = shuffledTracks[tracksPlayed+1];      // store our next track 
            CdControlF (CdlPlay, (u_char *)&nextTrack+1);  // Play the next trackk   
            }
            if (tracksPlayed == numTracks)
            {
                while (currentSeconds == currentTrackTimeInSeconds){};  // Lock up/ wait here on the last second of the last track, then stop the CD                                     
                CdControlF(CdlStop, 0);
            }
        }
*/
    }

    int main()
    {

      u_char CDDA = 1; // CDDA mode for CdlSetMode- requires pointer as mode parameter
   
      CdInit(); // Init extended CD system
      CdControlB (CdlSetmode, &CDDA, 0); // set CDDA playback mode
      SpuInit(); //The proper order of initialization is CDInit(), then SpuInit()

        /* p1042 LibRef47.pdf

        Initializes SPU. Called only once within the program. After initialization, the state of the SPU is:
        • Master volume is 0 for both L/R
        • Reverb is off; reverb work area is not reserved
        • Reverb depth and volume are 0 for both L/R
        • Sound buffer transfer mode is DMA transfer
        • For all voices: Key off
        • For all voices: Pitch LFO, noise, reverb functions not set
        • CD input volume is 0 for both L/R
        • External digital input volume is 0 for both L/R
        • DMA transfer initialization set
        The status of the sound buffer is indeterminate after initialization.

        */
      initSpu(MAX_VOLUME_CD, MAX_VOLUME_MASTER); // now set the desired SPU settings
      ResetCallback(); // initialises all system callbacks
      initGraphics(); // this will initialise the graphics
      initFont(); // initialise the psy-q debugging font
      initImage(); // initialise the TIM image
      PadInit(0); // Initialize pad.  Mode is always 0
      getTableOfContents(); // get TOC information from the CD

      CdControlB (CdlPlay, (u_char *)&loc[1], 0); // play track 1

        while (1) 
        {   
            initFont();
            playerInformationLogic();
            readControllerInput();
            display();
        }

        ResetGraph(3); // set the video mode back
        return 0;
      }


/*

add reverb on / off
add L/R balance
add volume level displays
add repeat on/off
accompanying background display editing (including shoulder buttons)
add disc swapping

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
*/
