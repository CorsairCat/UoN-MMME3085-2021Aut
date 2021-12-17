#include <stdio.h>
#include <stdlib.h>

// define the global const number
#define _LINE_HEIGHT_OFFSET_ 36
#define _MAX_LINE_WIDTH_ 600
#define _DEFAULT_FONT_WIDTH_ 18

// define the structure of font index
struct FontIndex {
    int start_line;
    int line_num;
};

void SendCommands (char *buffer );
// create font g code index
int generateFontIndex(FILE *filePointer, struct FontIndex fontGcodeLineIndex[]);
// create the int format g code data in memory from the text font file;
int createFontDataCache(FILE *filePointer, int fontGcodeData[]);
// send initial commands to writing robots
int initializeWritingMachine();
// create the next line of g code to be executed
// scale, change offset, output
int generateCharGcodeCommand(int charAsciiNum, double *selectedOffsetX, double *selectedOffsetY, char commandBuffer[], int fontDataCache[], struct FontIndex fontIndexArray[], double Scaler);
// update the offset of 0,0 point for next character
int updateCharactorOffsetPosition(double *selectedOffsetX, double *selectedOffsetY, double commandWidthChange, double commandHeightChange, double globalScaler);
int convertCharArrayToInt(char numarray[], int *startPosition, int charLength, int *returnValue);

// verify for system to create sleep() capibility
#ifdef _WIN32
    #include <windows.h>
#else // Unix like systems
    // activate the sleep function provided
    #include <unistd.h>
    void Sleep(double sleepTime);
    void Sleep(double sleepTime)
    {
        sleep(sleepTime/1000);
    }
#endif

// enable a debug printf function
#ifdef __FULL_DEBUG_MODE__
    #define debug_printf(x...) printf(x)
#else
    #define  debug_printf(x...)
#endif

// utility function for convert a char array less than length of 4 to a int
int convertCharArrayToInt(char numarray[], int *startPosition, int charLength, int *returnValue)
{
    int convertedNumber = 0;
    int isNegative = 0;
    int tempNumExacted;
    for (int i = 0; i < charLength; i++)
    {
        tempNumExacted = numarray[*startPosition + i] - 48;
        // verify if the char read in is a num
        if (tempNumExacted >= 0 && tempNumExacted < 10)
        {
            convertedNumber = convertedNumber * 10 + tempNumExacted;
            //printf("current num is %d, added result is %d; \n", tempNumExacted, convertedNumber);
        }
        // dealing with - leading negative value
        else if (tempNumExacted == -3 && i == 0)
        {
            isNegative = 1;
            //convertedNumber = 0;
        }
        else
        {
            // if it is not a number, means convert of current number has reach the end
            if (i == 0)
            {
                // error handling
                // no number has been converted, report failure
                *returnValue = 0;
                return 0;
            }
            else
            {
                // ending and return the result
                // update the char postion at the end of current number
                *startPosition += i + 1;
                if (isNegative)
                {
                    // change the value to negative if is defined
                    *returnValue = - convertedNumber;
                }
                else
                {
                    // normal return
                    *returnValue = convertedNumber;
                }
                return 1;
            }
            break;
        }
    }
    return 0;
}
