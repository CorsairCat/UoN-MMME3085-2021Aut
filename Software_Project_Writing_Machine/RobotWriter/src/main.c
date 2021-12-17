//  define if it is testing mode
#include "include/utility.h"
// define IO head file
#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#ifdef __RELEASE_MODE__
    #include "include/rs232.h"
#endif
#include "include/serial.h"
// define the function prototype
#include "include/main.h"

#define bdrate 115200               /* 115200 baud */

int main()
{
    //char mode[]= {'8','N','1',0};
    char buffer[100];

    //_________ Start Define of the Variables
    double outputOffsetX = 0;
    double outputOffsetY = 0;
    double generalScaler = 0.4; // scale the size of the word
    // int machineZaxisState = 0; // 0 is up, 1000 is down
    char charReadyToWrite;
    // font index array
    struct FontIndex FontIndexArray[128];
    // all lines of font file
    int fontFileLength = 0;
    int *memForFontData = NULL;

    // initial index of font to avoid non define issue
    for (int i = 0; i < 128; i++)
    {
        FontIndexArray[i].start_line = 0;
        FontIndexArray[i].line_num = 0;
    }

    // Generate the Index of font file and open the font file
    FILE *fpFont = NULL;
    // open font file
    fpFont = fopen("../asset/font/SingleStrokeFont.txt", "r");
    // genrerate the font index
    if (fpFont != NULL)
    {
        // call the function to generate the index
        fontFileLength = generateFontIndex(fpFont, FontIndexArray);
        // using the return value to identify any error during this process
        if (fontFileLength == 0)
        {
            // error handling if no index is created
            printf ("Unable to index the Font File\n");
            fclose(fpFont);
            exit (0);
        }
        else if (fontFileLength < 0)
        {
            // error handling if any error occured during the indexing
            printf ("Format Error Detected in the Font File, line %d\n", -fontFileLength);
            printf ("Reason: Index exceed the basic ASCII limit (1 - 128)\n");
            fclose(fpFont);
            exit (0);
        }
        else
        {
            // solve the font file from plain text to the int data in memory
            // allocate memory
            printf ("Trying to allocate %.2f kb memory...\n", (float) fontFileLength * 3 / 1024);
            memForFontData = malloc(fontFileLength * 3 * sizeof(int));
            memset(memForFontData, 0, fontFileLength * 3);
            // check if allocation works
            if (memForFontData == NULL)
            {
                // error handling if unable to use malloc()
                printf ("Memory allocate Failed\n");
                fclose(fpFont);
                exit (0);
            }
            else
            {
                // allocate success
                printf ("Successfully allocate %.2f kb memory\n", (float) fontFileLength * 3 / 1024);
            }
            // create the cache for the font file
            fontFileLength = createFontDataCache(fpFont, memForFontData);
            // error handling if error happens during the convert
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
        printf ("Unable to open the Font File\n");
        fclose(fpFont);
        exit (0);
    }
    // close the font file
    fclose(fpFont);

    // ask user to set the golbal scaler
    printf("Enter the Scale size of the font (0.4 is a good default): ");
    scanf("%lf", &generalScaler);
    // check if its a vaild scaler number
    while (generalScaler <= 0)
    {
        // error handle if the scaler number is smaller than 0
        printf("Scale size of the font should be positive:");
        scanf("%lf", &generalScaler);
    }

    // initialize the text file needs to print out
    FILE *fpText = NULL;
    // open text file
    fpText = fopen("../input/input.txt", "r");
    if (fpText == NULL)
    {
        // error hanbling
        printf("Failed to open the text file\n");
        exit(0);
    }

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

    //machineZaxisState = 0;
    // use fgetc to get next char inside the input file
    // while the file pointer didnt reach the end of the file
    while ((charReadyToWrite = (char) fgetc(fpText)) != EOF)
    {
        // call the function to generate the gcode of current character
        generateCharGcodeCommand(charReadyToWrite, &outputOffsetX, &outputOffsetY, buffer, memForFontData, FontIndexArray, generalScaler);
    }

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");
    // free the memory allocated before
    free(memForFontData);
    printf("Font cache cleared\n");
    // close the file of input
    fclose(fpText);

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

// generate the index from the file
int generateFontIndex(FILE *filePointer, struct FontIndex fontGcodeLineIndex[])
{
    // the buffer for storing the current line
    char fontReadBuff[255];
    // use this to reset the position of filepointer
    int index_counter = 0;
    // to accept the result from the convertion function
    // which are the char's ASCII number and the g-code line length of this character
    int currentCharASCII, charGcodeLength;
    // record the position of the character
    // this define where to start to convert string into integer
    int startPosOfNextConvert;
    // the pointer for transmit startPosOfNextConvert
    int *bufferCharPosPt;
    // get one line of the font file
    while (fgets(fontReadBuff, 255, (FILE*)filePointer))
    {
        // reset the position variables to 0 for a new circle
        startPosOfNextConvert = 0;
        charGcodeLength = 0;
        // check if this line is start with 999, which means this is the line define the start of a char and its g-code length
        if (fontReadBuff[0] == (char)57 && fontReadBuff[1] == (char)57 && fontReadBuff[2] == (char)57 )
        {
            // jump position to 4, because its 999 ASCII LENGTH
            // ascii number starts from position 5
            startPosOfNextConvert = 4;
            bufferCharPosPt = &startPosOfNextConvert;
            // convert the char's ascii num (max 128), max length = 3, start position is 4; (999 XXX)
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 20, &currentCharASCII))
            {
                //check if the converted result is between 0 -127
                if (currentCharASCII >= 0 && currentCharASCII < 128)
                {
                    // if its a char's ASCII, record it into the index array
                    fontGcodeLineIndex[currentCharASCII].start_line = index_counter;
                }
                else
                {
                    // if not, it means something is wrong, record the error line number and return
                    index_counter = -1 - index_counter;
                    break;
                }
            }
            // if its correct, jump to the start of the length and convert it to integer
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 20, &charGcodeLength))
            {
                // record the length into the index array
                fontGcodeLineIndex[currentCharASCII].line_num = charGcodeLength;
            }
        }
        // calculate the current line number
        index_counter += 1;
    }
    // reset the file pointer to beginning
    rewind(filePointer);
    return index_counter;
}

// transfer the font data in plain text to cache data stored in memory
int createFontDataCache(FILE *filePointer, int fontGcodeData[])
{
    // reset the file pointe again for sure its at the start of file
    rewind(filePointer);
    // set read line buffer
    char fontReadBuff[255];
    // record the index position for easier check
    int index_counter = 0;
    // to accept the result from the convertion function
    // which are the char's ASCII number and the g-code line length of this character
    int startPosOfNextConvert;
    int *bufferCharPosPt;
    int tempResult;
    // get one line of the font file
    while (fgets(fontReadBuff, 255, (FILE*)filePointer))
    {
        // reset the position to start
        startPosOfNextConvert = 0;
        bufferCharPosPt = &startPosOfNextConvert;
        // each line is construct by 3 int numbers
        // use a for loop to read the int in one line
        for (int i = 0; i < 3; i++)
        {
            // convert the numbers and move the CharPos to next start in the buffer
            if (convertCharArrayToInt(fontReadBuff, bufferCharPosPt, 20, &tempResult))
            {
                // saved it in to the pre allocated memory, each is 3* line number + number position
                fontGcodeData[3*index_counter + i] = tempResult;
            }
            else
            {
                // failure generated if the converted function failed
                // export the error message to user
                printf("Format Error Found in Line %d;\n", (index_counter+1));
                // record the error
                index_counter = -1;
                // exit loop
                break;
            }
        }
        // if any error generated in this loop, exit the whole function
        if (index_counter < 0)
        {
            break;
        }
        else
        {
            // set the counter to correct position
            index_counter += 1;
        }
    }
    // return status
    return index_counter;
}

// a hard coded initial for the writing machine
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

// execute the G code for a input Character
int generateCharGcodeCommand(int charAsciiNum, double *currentOffsetX, double *currentOffsetY, char commandBuffer[], int fontDataCache[], struct FontIndex fontIndexArray[], double Scaler)
{
    // variables to store the last g command code which is next offset 
    double currentFontWidth = 0;
    double currentFontHeight = 0;
    // the tempory line number to store which line is used
    int currentExecutedLineNum = 0;
    // the status of the Z position, deine if pen is up or down
    int penZAxisStatus = 0; // reset to 0 for each input
    // add a error handling here to avoid non character g code
    // check this char do have some g code to output
    if (fontIndexArray[charAsciiNum].start_line >= 0)
    {
        // set the machine to offset with pen up
        sprintf (commandBuffer, "S0\n");
        SendCommands(commandBuffer);
        // move the pen to the default offset position to avoid any problem which might occur
        sprintf (commandBuffer, "G0 X%.3f Y%.3f F1000\n", *currentOffsetX, *currentOffsetY);
        SendCommands(commandBuffer);
        // use a for loop to execute the gcode stored in cache
        for (int exeCommand = 0; exeCommand < fontIndexArray[charAsciiNum].line_num; exeCommand++)
        {
            // move the line to the absolute position in memory cache using the relative line number and cached index
            currentExecutedLineNum = fontIndexArray[charAsciiNum].start_line + exeCommand + 1;
            // if the target pen status (up and down) is not same as current, 
            // send the pen down command seperately before executing the position move command
            if (fontDataCache[currentExecutedLineNum * 3 + 2] != penZAxisStatus)
            {
                // send pen move down/up command
                sprintf (commandBuffer, "S%d\n", fontDataCache[currentExecutedLineNum * 3 + 2] * 1000);
                SendCommands(commandBuffer);
                // set the pen status to the target pen status
                penZAxisStatus = fontDataCache[currentExecutedLineNum * 3 + 2];
                // keep in same line
                exeCommand -= 1;
            }
            else
            {
                // if pen status didnt changed, just send the command to move the position 
                // product the scaler to generate the correct position command 
                sprintf (commandBuffer, "G%d X%.3f Y%.3f\n", penZAxisStatus, *currentOffsetX + Scaler * fontDataCache[currentExecutedLineNum * 3], *currentOffsetY + Scaler * fontDataCache[currentExecutedLineNum * 3 + 1]);
                SendCommands(commandBuffer);
            }
            // if the line is the last line of current character's g code, thus, just transfer the position into the offset moving variables
            if (exeCommand == fontIndexArray[charAsciiNum].line_num - 1)
            {
                // set the position use the global scaler
                currentFontWidth = Scaler * fontDataCache[currentExecutedLineNum * 3];
                currentFontHeight = Scaler * fontDataCache[currentExecutedLineNum * 3 + 1];
            }
        }
    }
    else
    {
        // if this code didnt have a g-code, just using the default command and move it to next position
        // jump to next words position if this code is not defined
        currentFontWidth = Scaler * _DEFAULT_FONT_WIDTH_;
        currentFontHeight = 0;
    }
    // call the function to update the offset for next character
    updateCharactorOffsetPosition(currentOffsetX, currentOffsetY, currentFontWidth, currentFontHeight, Scaler);
    return 1;
}

// flash the default offset for next character
int updateCharactorOffsetPosition(double *currentOffsetX, double *currentOffsetY, double commandWidthChange, double commandHeightChange, double globalScaler)
{
    // check if its meet the max available line width
    if ((*currentOffsetX + commandWidthChange) < globalScaler * _MAX_LINE_WIDTH_)
    {
        // if not exceed the limits, add the x increasement to the global offset
        *currentOffsetX += commandWidthChange;
    }
    else
    {
        // the width is exceed, 
        // line change is required
        *currentOffsetY -= globalScaler * _LINE_HEIGHT_OFFSET_;
        *currentOffsetX = 0;
    }
    if (commandHeightChange != 0)
    {
        // if have height change, (example: \n)
        // the x should be move back to the start
        *currentOffsetY += commandHeightChange;
        *currentOffsetX = 0;
    }
    return 1;
}
