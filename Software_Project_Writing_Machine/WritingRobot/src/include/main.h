#include <stdio.h>
#include <stdlib.h>

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
// create the next line of g code to be executed
int updateGcodeTargetPosition(int gcodeLineNum, int currentXOffset, int currentYOffset, char gCodeCommand[], int lastTimeReturnValue);
// update the offset of 0,0 point for next character
int updateCharactorOffsetPosition(int *tempOffsetX, int *tempOffsetY);
int convertCharArrayToInt(char numarray[], int *startPosition, int charLength, int *returnValue);

// verify for system to create sleep() capibility
#ifdef _WIN32
    #include <windows.h>
#else // Unix like systems
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
    int readCharNum, isNegative = 0;
    int tempNum;
    for (int i = 0; i < charLength; i++)
    {
        tempNum = numarray[*startPosition + i] - 48;
        // verify if its a num
        if (tempNum < 10 && tempNum >= 0)
        {
            readCharNum = readCharNum * 10 + tempNum;
        }
        // dealing with - leading negative value
        else if (tempNum == -3 && i == 0)
        {
            isNegative = 1;
        }
        else
        {
            if (i == 0)
            {
                *returnValue = 0;
                return 0;
            }
            else
            {
                *startPosition += i + 1;
                if (isNegative)
                {
                    *returnValue = - readCharNum;
                }
                else
                {
                    *returnValue = readCharNum;
                }
                return 1;
            }
            break;
        }
    }
    return 0;
}