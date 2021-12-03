//  define if it is testing mode
//  __DEBUG_MODE__ or __RELEASE_MODE__
//#define __RELEASE_MODE__
#define __DEBUG_MODE__
// define IO head file
#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#ifdef __RELEASE_MODE__
    #include "include/rs232.h"
    #define Serial_Mode
#endif
#include "include/serial.h"
// define the function prototype
#include "include/main.h"

#define bdrate 115200               /* 115200 baud */

// define the global const number
#define _LINE_HEIGHT_OFFSET_ 30

int main()
{

    //char mode[]= {'8','N','1',0};
    char buffer[100];

    //_________ Start Define of the Variables
    int outputOffsetX = 0;
    int outputOffsetY = 0;
    // font index array
    struct FontIndex FontIndexArray[128];
    // all lines of font file
    int fontFileLength = 0;
    int *memForFontData = NULL;

    // Generate the Index of font file and open the font file
    FILE *fpFont = NULL;
    // open font file
    fpFont = fopen("../asset/font/SingleStrokeFont.txt", "r");
    // genrerate the font index
    if (fpFont != NULL)
    {
        fontFileLength = generateFontIndex(fpFont, FontIndexArray);
        if (fontFileLength == 0)
        {
            // error handling
            printf ("\nUnable to index the Font File\n");
            fclose(fpFont);
            exit (0);
        }
        else
        {
            // solve the font file from plain text to the int data in memory
            // allocate memory
            printf ("Trying to allocate %.2f kb memory...\n", (float) fontFileLength * 3 / 1024);
            memForFontData = malloc(fontFileLength * 3 * sizeof(int));
            // check if allocation works
            if (memForFontData == NULL)
            {
                // error handling
                printf ("Memory allocate Failed\n");
                fclose(fpFont);
                exit (0);
            }
            else
            {
                printf ("Successfully allocate %.2f kb memory\n", (float) fontFileLength * 3 / 1024);
            }
            // create the cache for the font file
            fontFileLength = createFontDataCache(fpFont, memForFontData);
            // error handling
            if (fontFileLength < 0)
            {
                printf ("Failed to read the format of Font File\n");
                fclose(fpFont);
                exit (0);
            }
            else
            {
                printf ("Font Data has been translate successfully\n");
            }
        }
    }
    else
    {
        printf ("\nUnable to open the Font File\n");
        fclose(fpFont);
        exit (0);
    }
    // close the font file
    fclose(fpFont);

    //_________ Initial the RS232 Serial Port

    // If we cannot open the port then give up immediately
    if ( CanRS232PortBeOpened() == -1 )
    {
        printf ("\nUnable to open the COM port (specified in serial.h) ");
        exit (0);
    }

    //_________ Initial the machine

    // Time to wake up the robot
    printf ("\nAbout to wake up the robot\n");

    // We do this by sending a new-line
    sprintf (buffer, "\n");
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    Sleep(100);

    // This is a special case - we wait  until we see a dollar ($)
    WaitForDollar();

    //________ Here ends the initial of machine

    printf ("\nThe robot is now ready to draw\n");
    // sending initial commands
    initializeWritingMachine(buffer);


    // These are sample commands to draw out some information - these are the ones you will be generating.
    sprintf (buffer, "G0 X-13.41849 Y0.000\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y-4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y0.0000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41089 Y4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X-7.17524 Y0\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X0 Y0\n");
    SendCommands(buffer);

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");

    return (0);
}


// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    //WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
    // getch(); // Omit this once basic testing with emulator has taken place
}

// genrerate the index from the file
int generateFontIndex(FILE *filePointer, struct FontIndex fontGcodeLineIndex[])
{
    char fontReadBuff[255];
    // use this to reset the position of filepointer
    int index_counter = 0;
    int currentReadCharNum, charGcodeLength;
    int currentBuffCharPos;
    int *bufferCharPosPt;
    while (fgets(fontReadBuff, 255, (FILE*)filePointer))
    {
        currentBuffCharPos = 0;
        charGcodeLength = 0;
        if (fontReadBuff[0] == (char)57 && fontReadBuff[1] == (char)57 && fontReadBuff[2] == (char)57 )
        {
            currentBuffCharPos = 4;
            // convert the char's ascii num, max length = 3, start position is 4; (999 XXX)
            bufferCharPosPt = &currentBuffCharPos;
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 3, &currentReadCharNum))
            {
                fontGcodeLineIndex[currentReadCharNum].start_line = index_counter;
            }
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 4, &currentReadCharNum))
            {
                fontGcodeLineIndex[currentReadCharNum].line_num = charGcodeLength;
            }
        }
        index_counter += 1;
    }
    // reset the file pointer to beginning
    rewind(filePointer);
    return index_counter;
}

int createFontDataCache(FILE *filePointer, int fontGcodeData[])
{
    rewind(filePointer);
    char fontReadBuff[255];
    // use this to reset the position of filepointer
    int index_counter = 0;
    int currentBuffCharPos;
    int *bufferCharPosPt;
    int tempResult;
    while (fgets(fontReadBuff, 255, (FILE*)filePointer))
    {
        currentBuffCharPos = 0;
        bufferCharPosPt = &currentBuffCharPos;
        for (int i = 0; i < 3; i++)
        {
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 6, &tempResult))
            {
                fontGcodeData[3*index_counter + i] = tempResult;
            }
            else
            {
                // failure generated
                printf("Format Error Found in Line %d;\n", (index_counter+1));
                index_counter = -1;
                break;
            }
        }
        if (index_counter < 0)
        {
            break;
        }
        else
        {
            index_counter += 1;
        }
    }
    return index_counter;
}

int initializeWritingMachine(char commandBuffer[])
{
    //These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands
    sprintf (commandBuffer, "G1 X0 Y0 F1000\n");
    SendCommands(commandBuffer);
    sprintf (commandBuffer, "M3\n");
    SendCommands(commandBuffer);
    sprintf (commandBuffer, "S0\n");
    SendCommands(commandBuffer);
    return 0;
}