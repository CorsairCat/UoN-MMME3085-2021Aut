#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// verify for system
#ifdef _WIN32
    #include <windows.h>
#endif
// simulate a windows.h for mac
#ifdef __MACH__
    #include "windows.h"
#endif

/* Only needed in Windows program to maintain compatibility with Arduino version of C/C++ */
#ifdef _WIN32
    #define bool BOOL
    #define true 1
    #define false 0
#endif
const bool FWDS = true;
const bool BWDS = false;

const long ticksPerSec = 1000; // ms on PC
// on Arduino it is 1E6 for micros (for s/w) or 1.6E7 for 62.5 ns ticks (for h/w)

/* Function prototypes: */
/* PC only, don't need function prototypes on Arduino as they get added within compilation process */
void moveOneStep();
void computeNewSpeed();
long computeStepsToGo();
void goToPosition(long newPosition);
void printLoop();
long millis(void);

/* Note: we are using global variables ONLY to preserve compatibility with the Arduino program structure.
   They should not normally be used in C or C++ programs as they make for a poor software design. */
/* Global variables relating to stepper motor position counting etc. */
long stepsToGo;             /* Number of steps left to make in present movement */
long targetPosition;        /* Intended destination of motor for given movement */
volatile long currentPosition = 0;   /* Position in steps of motor relative to startup position */
float maxSpeed;            /* Maximum speed in present movement (not nec. max permitted) */
bool direction;             /* Direction of present movement: FWDS or BWDS */

/* Global variables used in simplistic and Leib Ramp algorithms */
volatile float p;   /* Step interval in clock ticks or microseconds */
float p1, ps;       /* Minimum and maximum step periods */
float deltaP;      /* You'll be able to get rid of this later */
float R;           /* Multiplying constant used in Eiderman's algorithm */

/* Global variable used for noting previous time of a step in timed loop and for calculating speed and accel */
long prevStepTime=0;
long millisAtStart;
float prevSpeed=0.0;

/* Define permissible parameters for motor */
// For testing on PC only, not for use in Arduino program: try movements in order of 50-100 steps
float accelSteps=20; /* leave this as a variable as we may over-write it */
const float minSpeed = 1.0;
const float maxPermissSpeed = 20.0;
const float maxAccel = 10.0;

int main()
{
    unsigned long currentMillis = millis();
    prevStepTime = 0;
    long positionToMoveTo;
    while(true)
    {
        printf("Enter position to move to in profile (or 999 to terminate)\n");
        scanf("%ld", &positionToMoveTo);
        if (positionToMoveTo==999) break;
        printf("        Time (s),  Speed (steps/s), Accel (steps/s^2),  Posit'n (steps), Step time (ticks)\n");

        goToPosition(positionToMoveTo);

        /* --------------------------------------------------------------------- */
        /* Start of pre-computation code - only executed once per profile        */
		
		// STEP 1																 
        // Define number of steps in acceleration phase using Equation (3) 
        maxSpeed = maxPermissSpeed;
		accelSteps= (maxSpeed * maxSpeed - minSpeed * minSpeed) /(2 * maxAccel);
		
        stepsToGo = computeStepsToGo();
        //maxSpeed = maxPermissSpeed;
        if (2 * accelSteps > stepsToGo)
        {
			// STEP 2 
			// Define maximum speed in profile and number of steps in acceleration phase 
            // Use Equations (4) and (5)
            maxSpeed = sqrt(minSpeed * minSpeed + maxAccel * stepsToGo);
            accelSteps = (float) (long) stepsToGo / 2;
        }

		// STEPS 3 and 5														  
        // Step 3 
        p1 = ((float)ticksPerSec) / sqrt(minSpeed * minSpeed + 2 * accelSteps);
        // Step 5
        R = ((float)maxAccel) / (ticksPerSec * ticksPerSec);
        // Calculate initial value of and p1 and R    Set p = p1 
        p = p1;

        // Step 4
        ps = ((float)ticksPerSec) / maxSpeed; //  STEP 4: leave it as it is.
		
        /* End of pre-computation code                                    */
        /* -------------------------------------------------------------- */
		
		
        millisAtStart = millis(); /* Needed only to tabulate speed vs. time */

        /* Timed loop for stepping, and associated coding */
        while(stepsToGo > 0)
        {
            currentMillis = millis();
            if (currentMillis - prevStepTime >= p)
            {
               moveOneStep();
               prevStepTime = currentMillis;
               computeNewSpeed();
            }
        }
    }
    return 0;
}

long millis(void)
/* Only needed for compatibility with Arduino program because millis() is not a native Windows API function */
{
    return GetTickCount();
}

void moveOneStep()
/* Move a single step.  If this were running on the Arduino we would holding pulse high for delayMicroSeconds */
{
    if (p != 0) /* p=0 is code for "don't make steps" */
    {
        // Print to screen instead of making a step
        if (direction == FWDS)
        {
        /* Something missing here but only because we are not using any real step output. Don't add it to PC program. */
        currentPosition++;
        }
        else
        {
        /* Something also missing here but only because we are not using any real step output. Don't add it to PC program. */
        currentPosition--;
        }
        /* Something also missing here but only because we are not using any real step output. Don't add it to PC program. */
        /* Instead of actually making step, print out parameters for current step */
        float speed = (float)(ticksPerSec)/p;
        float accel = (float)(ticksPerSec)*(speed-prevSpeed)/p;
        printf("%16.3f, %16.3f,  %16.3f, %16ld, %16.3f\n", 0.001*(millis()-millisAtStart), speed, accel, currentPosition, p);
        prevSpeed = speed;
    }
}


void computeNewSpeed()
/* Calcuate new value of step interval p based on constants defined in loop() */
{
    /* You may need to declare some temporary variables for this function... */
    stepsToGo = computeStepsToGo();

    /* ----------------------------------------------------------------- */
    /* Start of on-the-fly step calculation code, executed once per step */
    // define a temporary variable
    float m;
    //  STEP 6a														  
    if (stepsToGo == 0)
    {
        p = 0; // Not actually a zero step interval, used to switch stepping off
        return;
    }
    else if (stepsToGo >= accelSteps && (long)p > (long)ps) // Changed to make ramps even length
    /* Speeding up */
    {
        /* Equation (9) */
        m = - R;
        
    }
    else if (stepsToGo <= accelSteps)
    /* Slowing down */
    {
        /* Equation 10 */
        m = R;
        
    }
    else
    /* Running at constant speed */
    {
        /* Equation (11)	*/
        m = 0;
    }
    
    // STEP 6b, c and d using Equations (12) and (13)  
    float q;
    q = m * p * p;
    // update p to new
    p = p * (1 + q + 1.5 * q * q);
    // if p > p1, p = p1
    if (p > p1)
    {
        p = p1;
    }
    
    /* End of on-the-fly step calculation code */
    /* ----------------------------------------------------------------- */
}

long computeStepsToGo()
/* Work out how far the stepper motor still needs to move */
{
    if (direction == FWDS)
    {
        return targetPosition - currentPosition;
    }
    else
    {
        return currentPosition - targetPosition;
    }
}

void goToPosition(long newPosition)
/* Set the target position and determine direction of intended movement */
{
    targetPosition = newPosition;
    if (targetPosition - currentPosition > 0)
    {
        direction = FWDS;
    }
    else
    {
        direction = BWDS;
    }
}

//void printLoop()
///* Print current position of stepper using timed loop */
//{
//    /* Sample all counters one after the other to avoid delay-related offsets */
//    printf("Current position = %ld \n",currentPosition);
//}
