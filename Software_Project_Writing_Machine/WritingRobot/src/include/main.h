#include <stdio.h>
#include <stdlib.h>

void SendCommands (char *buffer );
// create font g code index
int generateFontIndex(FILE *filepointer, int fontGcodeLineIndex[]);
// create the next line of g code to be executed
int updateGcodeTargetPosition(int gcodeLineNum, int currentXOffset, int currentYOffset, char gCodeCommand[ ], int lastTimeReturnValue);
// update the offset of 0,0 point for next character
int updateCharactorOffsetPosition(int *tempOffsetX, int *tempOffsetY);

// define the structure of font index
struct FontIndex {
    int start_line;
    int length;
};

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