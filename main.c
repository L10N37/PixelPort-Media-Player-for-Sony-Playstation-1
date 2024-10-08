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
    */

    #include "common.h"

    // TIM images
    #include "background.h"
    #include "startBtnMenu.h"


    #include "libcd.h"
    #include "libetc.h"
    #include "libgpu.h"

    #define OT_LENGTH 1                             // ordering table length
    #define PACKETMAX (300)                         // the max num of objects
    #define MAX_VOLUME_CD 32767                     // maximum volume for an audio CD (0x7FFF)
    #define MAX_VOLUME_MASTER 16383                 // maximum master volume (0x3FFF), this can't be adjusted on the fly and displays 0 at all times, but no audio without init
    #define BALANCE_RANGE 100

    GsOT      WorldOrderingTable[2];                 // ordering table handlers
    GsOT_TAG  OrderingTable[2][1<<OT_LENGTH];        // ordering tables
    PACKET    GPUOutputPacket[2][PACKETMAX];         // GPU Packet work area

    GsSPRITE image1[2];                              // sprite handler
    
    unsigned long __ramsize =   0x00200000;          // force 2 megabytes of RAM
    unsigned long __stacksize = 0x00004000;          // force 16 kilobytes of stack

    unsigned char background[];         // bg image array converted from TIM file
    unsigned char startBtnMenu[];

    // Globals ------------------------------------------------------------------------------
    CdlLOC loc[100];                    // Array to store locations of tracks

    u_char cdInfoGetlocp[8];
    u_char result[8];                    // general response storage

    //long int image1_addr;				// DRAM address storage of TIM file, from loadTIM example, wasn't used

    unsigned int currentTrackTimeInSeconds = 0;  // to hold current track time in seconds

    int currentbuffer; // double buffer holder
    int decimalValues[8] = {0x00};       // to hold decimal values from GETLOCP after conversion from BCD (uchar?)
    int numTracks = 0x00;                // for storing number of audio tracks found on CD
    int debounceTimer= 0;                // controller input debounce timer
    int shuffledTracks[101] = {0x00};    // Max tracks 100, for shuffle/ built in CdPlay function we need an extra element to contain zero (could have implemented shuffle manually though)
    int balancePosition = 0; // Balance position (-50 to +50)

       statusFlags flag = {
        .repeat = true,                         // Repeat mode enabled
        .shuffle = false,                       // Shuffle mode disabled
        .selectedShuffleMode = false,           // shuffle mode flag, 0 for random, 1 for custom
        .shuffleSelectionBreakEarly = false,    // No early break in shuffle selection
        .reverb = false,                        // Reverb effect enabled
        .mute = false                           // Mute off
    };
    // --------------------------------------------------------------------------------------


    typedef struct 
    {
                int mins;
                int seconds;
    } TrackTime;


    // function decs
    void initImage(unsigned char *loadTimArray);
    void clearVRAM();
    // -------------

        void initSpu(int applyVolumeCdLeft, int applyVolumeCdRight) 
    {
        /*
        The volume mode is ‘direct mode’ and the range of the values which can be set to the mvolL and mvolR
        volumes is equal to that of the ‘direct mode’ in SpuSetVoiceAttr() (-0x4000 to 0x3fff).
        */
        SpuSetCommonMasterVolume(MAX_VOLUME_MASTER, MAX_VOLUME_MASTER);
        //Set independently for left and right in the range -0x8000 - 0x7fff. If volume is negative, the phase is inverted.
        SpuSetCommonCDVolume(applyVolumeCdLeft, applyVolumeCdRight);

        // No output without CD Mixing turned on
        SpuSetCommonCDMix(1);

        // get SPU Attributes for printing
        SpuCommonAttr attr;
        SpuGetCommonAttr(&attr);
        // current CD volume
        printf("CD Volume Left: %d\n", attr.cd.volume.left), printf("CD Volume Right: %d\n", attr.cd.volume.right);
        printf("Reverb (on/off): %x\n", attr.cd.reverb);
        printf("CD Mix (on/off): %x\n", attr.cd.mix);
    }

    void readControllerInput()
    {
    
    int pad = 0x00;
    u_char trackValue;
    static int newVolumeLevelCd = MAX_VOLUME_CD;
    trackValue = decimalValues[track]; 
    
    //slow down the polling
    if (debounceTimer%10 == 0)
    {                               
    pad = PadRead(0); // Read pads input. id is unused, always 0.
    }
    debounceTimer++;

        switch (pad) {
            /////////////////////////////////////   
            case PADLup:
                if (newVolumeLevelCd < MAX_VOLUME_CD) 
                {
                newVolumeLevelCd+=450; // applies the same volume to the left and right cd audio channels
                initSpu(newVolumeLevelCd, newVolumeLevelCd);
                }
                printf("\nUP D-Pad pressed, volume increased\n");                
                break;
            /////////////////////////////////////   
            case PADLdown:
                if (newVolumeLevelCd > 0) 
                {
                newVolumeLevelCd-=450; // applies the same volume to the left and right CD audio channels
                initSpu(newVolumeLevelCd, newVolumeLevelCd);
                }
                printf("\nDown D-Pad pressed, volume decreased\n");
                break;
            /////////////////////////////////////   
            case PADLright: // Increment the track with wrap around to first track
                // Increment track value if not at last track
                if (trackValue < numTracks) // shuffle can land on us last track with more to play in shuffledTracks array, but || with shuffle flag still trips out the player and resets it to track 1
                {
                trackValue++;
                }
                // else if last track, go back to track 1, as long as shuffle isn't on
                if (trackValue == numTracks && flag.shuffle == 0)
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
                if (trackValue == 1 && flag.shuffle == 0)
                {
                trackValue = numTracks;
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]);
                }
                // else decrement track value
                else if (trackValue <= numTracks && trackValue != 1 && flag.shuffle == 0) //make sure shuffle isn't on
                {
                trackValue--;
                CdControlF (CdlPlay, (u_char *)&loc[trackValue]);
                }
                printf("\nTrack value: %d\n", trackValue);
                printf("\nLeft D-Pad pressed, decrement track"); 
                break;
            /////////////////////////////////////      
            case PADRup:
            flag.shuffleSelectionBreakEarly = false;
            printf("\nShuffle button pressed (triangle)\n");

            if (!flag.shuffle) // shuffle is off, turn it on with chosen mode selection
            { 
               flag.selectedShuffleMode = shuffleModeSelection(pad); // grab chosen mode

                if (!flag.selectedShuffleMode && !flag.shuffleSelectionBreakEarly) // shuffle mode chosen: RANDOM
                { 
                CdControlF(CdlStop, 0); // smoother?
                shuffleFunction(); // Shuffle the tracks
                flag.shuffle = true; // Set shuffle mode to true
                printf("\nShuffle Mode: %d\n", flag.selectedShuffleMode);
                printf("\nShuffle Flag: %d\n", flag.shuffle);
                printf("\nChosen Tracks:\n");
                for (int i = 0; i <= numTracks+1; i++) {
                    printf("Track %d: %d\n", i, shuffledTracks[i]);
                }
                 return; // break out of the function so we don't turn off shuffle mode below 
                }
                
                if (flag.selectedShuffleMode && !flag.shuffleSelectionBreakEarly) // shuffle mode chosen : CUSTOM
                {
                selectCustomTracks();
                // Call CdPlay with repeat or stop mode
                CdControlF(CdlStop, 0); // smoother?
                if (flag.repeat == 1) {
                CdPlay(2, shuffledTracks, decimalValues[index]); // Play with repeat
                } else if (flag.repeat == 0) {
                CdPlay(1, shuffledTracks, decimalValues[index]); // Play and stop at the end
                }

                flag.shuffle = true; // Set shuffle mode to true
                printf("\nShuffle Mode: %d\n", flag.selectedShuffleMode);
                printf("\nShuffle Flag: %d\n", flag.shuffle);

                printf("\nChosen Tracks:\n");
                for (int i = 0; i <= numTracks+1; i++) {
                    printf("Track %d: %d\n", i, shuffledTracks[i]);
                }
                return; // break out of the function so we don't turn off shuffle mode below 
                }   
            }

            if (flag.shuffle || !flag.shuffleSelectionBreakEarly) // if shuffle is on or exit before selecting mode/ tracks, turn it off and reset the shuffledTracks array
            {
                // Reset shuffledTracks array
                for (int i = 0; i < 101; i++) 
                {
                    shuffledTracks[i] = 0;
                }

                CdPlay(0, NULL, 0); // turn off built in array player
                CdControlF(CdlPlay, (u_char *)&loc[trackValue]); // Resume playing from the current track
                flag.shuffle = false; // toggle shuffle mode flag
            }

            break;
            /////////////////////////////////////   
            case PADRdown: // Play/Pause
            static bool pause = false;
            if (pause == false) 
            {
            CdControlF (CdlPause, 0);
            pause = 1;
            printf("\nPAUSED");
            
            }
            else if (pause == true)
            {
            CdControlF (CdlPlay, 0);
            pause = 0;
            printf("\nRESUMING PLAY FROM PAUSE");
            
            }
            break;
            /////////////////////////////////////   
            case PADRright: //Mute/ Demute
            if (flag.mute == false) 
            {
            CdControlF (CdlMute, 0);
            flag.mute = true;
            printf("\nAudio Muted");
            
            }
            else if (flag.mute == true)
            {
            CdControlF (CdlDemute, 0);
            flag.mute = false;
            printf("\nAudio Demuted");
            }
            break;
            /////////////////////////////////////   
            case PADRleft:
            CdControlF(CdlStop, 0); // Stop the CD (FIX THIS, will stop and play again)
                break;
            /////////////////////////////////////   
            case PADL1:
            CdControlF (CdlBackward, 0);
            printf("\nrewinding");
            while (pad == PADL1)
            {
                pad = PadRead(0);        // Keep re-reading the pad to check for release of forward
                playerInformationLogic();   //keep updating the track information
                display();                  // keep updating the display
            }               
            printf("\nrewind released");
            CdControlF (CdlPlay, 0); // Resume play, button released
            break;
            /////////////////////////////////////   
            case PADL2: // L2
            balanceControl();
            break;
            /////////////////////////////////////   
            case PADR1:
            CdControlF (CdlForward, 0);
            printf("\nfast forward");
            while (pad == PADR1)
            {
                pad = PadRead(0); // Keep re-reading the pad to check for release of forward
                playerInformationLogic(); //keep updating the track information
                display();                // keep updating the display
            }               
            printf("\nfast forward released");
            CdControlF (CdlPlay, 0); // Resume play, button released
                break;
            /////////////////////////////////////   
            case PADR2: // R2
            balanceControl();
                break;
            /////////////////////////////////////   
            case PADstart:
            clearVRAM();
            initImage(startBtnMenu);
            while (pad == PADstart) {pad = PadRead(0);  display();};
            clearVRAM();
            initImage(background);
            display();
            break;
            /////////////////////////////////////   
            case PADselect:
            // p1060 LibRef47.pdf
            printf("\nSelect Pressed");
            if (flag.reverb) {
                SpuSetCommonCDReverb(0);
            } else {
                SpuSetCommonCDReverb(1);
            }
            flag.reverb = !flag.reverb; // Toggle the reverb state       
                break;
            /////////////////////////////////////   
            default:
                break;
        }
      }
    
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
        FntOpen(10, 15, 320, 240, 0, 512);    // FntOpen(x, y, width, height, black_bg, max. nbr. chars)

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
        printf("Setting the PlayStation Video Mode to (PAL %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT);
        SetVideoMode(1);
        printf("Video Mode: PAL\n");
      }
      else
      {
        // NTSC MODE
        SCREEN_WIDTH = 320;
        SCREEN_HEIGHT = 240;
        printf("Setting the PlayStation Video Mode to (NTSC %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT);
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

      printf("Graphics Initialised!\n");
    }

    // load tim image from ram into frame buffer slot
    // we use 2 arrays [0] and [1] to split the 320px image into a section of 256px and 64px because the PSX can not draw greater than 256px (load 'image.tim' into TimTool to see). this gives us a total image size of 320px.
    void initImage(unsigned char *loadTimArray)
    {
      RECT            rect;                                    // RECT structure
      GsIMAGE         tim_data;                                // holds tim graphic info
        
      // put data from tim file into rect         
      //GsGetTimInfo ((u_long *)(image1_addr+4),&tim_data);
      GsGetTimInfo ((u_long *)(loadTimArray+4),&tim_data); // +4, skip first few bytes and start at beginning of pixel data?

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

    void calculateCurrentTrackLength() {
            // Get the current track number
            int currentTrack = decimalValues[track];

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
        
        void getTableOfContents() { // check tools.c for gettoc in drive lid status function

            unsigned char bcdValues[2];
            int decimalValues[2];
            TrackTime trackStartTime[101]; // Array to hold track start times, including total time
            int trackLengths[101]; // Array to hold track lengths in seconds

            // Print total time
            bcdValues[0] = loc[0].minute; // Total time minutes
            bcdValues[1] = loc[0].second; // Total time seconds
            convertBcdValuesToDecimal(bcdValues, decimalValues, 2);
            trackStartTime[0].mins = decimalValues[0];
            trackStartTime[0].seconds = decimalValues[1];
            printf("\nTotal Time: %02d:%02d\n\n", trackStartTime[0].mins, trackStartTime[0].seconds);

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

        ////////////  check cdlNopStatusByte ////////////
        CdControlB( CdlNop, 0, result);
        ////////////////////////////////////////////////
        //printf("\nCdlNop: %x", cdlNopStatusByte); //cdlNopStatusByte: 0

        // Print the CURRENT track information if a disc is playing or in fast track forward/reverse, otherwise zero them out
        if (cdlNopStatusByte == CdlStatPlay + CdlStatStandby || cdlNopStatusByte == 2) { // fast forward/reverse, result[0] is 2
            FntPrint("    Track: %0d/%0d  %02d:%02d\n\n", decimalValues[track], numTracks, decimalValues[min], decimalValues[sec]);
        } else {
            FntPrint("    Track: %0d/%0d  %02d:%02d\n\n", 0, 0, 0, 0);
        }

        // Print shuffle status
        if (!flag.shuffle) {
            FntPrint("    Shuffle: OFF\n\n"); // Shuffle is off
        } else if (flag.shuffle) {
            // Print ON and the selected shuffle mode
            FntPrint("    Shuffle: ON- %s\n\n", flag.selectedShuffleMode == 0 ? "RANDOM" : "CUSTOM");
        }

        // Print reverb status
        if (!flag.reverb) {
            FntPrint("    Reverb: OFF\n\n"); // Reverb is off
        } else if (flag.reverb) {
            // Print ON
            FntPrint("    Reverb: ON\n\n");
        }
        
        // Print volume levels after conversion to a percentage value
        SpuCommonAttr attr;
        SpuGetCommonAttr(&attr);

        int leftVolume = attr.cd.volume.left;
        int rightVolume = attr.cd.volume.right;

        int leftPercentage = convertToPercentage(leftVolume);
        int rightPercentage = convertToPercentage(rightVolume);

        FntPrint("    CD Volume Left: %d%%\n", leftPercentage);
        FntPrint("    CD Volume Right: %d%%\n", rightPercentage);

        // Print mute status
        if (!flag.mute) {
            FntPrint("    Mute: OFF\n\n");
        } else if (flag.mute) {
            // Print ON
            FntPrint("    Mute: ON\n\n");
        }

        // progress bar spacing
        FntPrint("\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        
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
        
        // Has the track changed? grab new tracks value in seconds if so - might cause loss of progress bar if shuffle sets first track to one already playing, fix
        static bool hasTrackChanged = false;
        static u_char oldTrack = 0;

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
    }

    int main()
    {

      u_char CDDA = 1; // CDDA mode for CdlSetMode- requires pointer as mode parameter
   
      CdInit(); // Init extended CD system
      SpuInit(); //The proper order of initialization is CDInit(), then SpuInit()
      CdControlF (CdlSetmode, &CDDA); // set CDDA playback mode
        /* 
        p1042 LibRef47.pdf

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
      initSpu(MAX_VOLUME_CD, MAX_VOLUME_CD); // now set the desired SPU settings
      ResetCallback(); // initialises all system callbacks
      initGraphics(); // this will initialise the graphics
      initFont(); // initialise the psy-q debugging font
      initImage(background); // initialise the TIM image
      PadInit(0); // Initialize pad.  Mode is always 0

        while (1) 
        {   
            checkDriveLidStatus(); // getToc called in here
            initFont();
            readControllerInput();
            playerInformationLogic();
            display();
        }

        ResetGraph(3); // set the video mode back
        return 0;
      }


/*
fix shuffle bug where landing on final track in shuffledTrack array mid CD won't allow a manual (Right Dpad) track increment to next track/ array element of shuffledTrack
Unsure why it even increments correctly through the array up until that track comes up, it might be a bug in the SDK

add repeat on/off
*/