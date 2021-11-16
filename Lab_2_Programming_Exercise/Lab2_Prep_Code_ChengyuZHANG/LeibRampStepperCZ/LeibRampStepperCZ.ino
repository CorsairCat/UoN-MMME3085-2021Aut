/* Stepper motor demonstration program written by Arthur Jones,
   4 November 2018.  Implements a simplistic and ineffective ramping 
   algorithm but provides framework for implementation of LeibRamp 
   algorithm described by Aryeh Eiderman, http://hwml.com/LeibRamp.pdf

   Makes use of background work and some aspects of code developed
   by Choaran Wang, 2017-18. This in turn incorporates some ideas
   used in the AccelStepper library:
   https://www.airspayce.com/mikem/arduino/AccelStepper/

   Serial input aspects are based closely upon:
   http://forum.arduino.cc/index.php?topic=396450
   Example 4 - Receive a number as text and convert it to an int
   Modified to read a long */

// #define USEINTERRUPTS
const int stepPin = 13;
const int dirPin = 9;
const bool FWDS = true;
const bool BWDS = false;
#ifdef USEINTERRUPTS
const long ticksPerSec = 16000000; // Clock speed of Arduino
#else
const long ticksPerSec = 1000000; // microseconds in this case
#endif


/* Define permissible parameters for motor */
// For testing by watching LED: try movements in order of 100 steps
float accelSteps=20; /* leave this as a variable as we may over-write it */
const float minSpeed = 2.0;
const float maxPermissSpeed = 20.0;
const float maxAccel = 10.0;
const long stepLengthMus = 10000;

// For lab testing with real motor: try movements in the order of 3000 steps
 // float accelSteps=1000; /* leave this as a variable as we may over-write it */
 // const float minSpeed=10.0;
 // const float maxPermissSpeed=1000.0;
 // const float maxAccel=500.0;
 // const long stepLengthMus=100;

/* Intervals in milliseconds for user-defined timed loops */
const long printInterval = 1000;

/* Global variables used for loop timing */
unsigned long prevMillisPrint = 0;        /* stores last time values were printed */

/* Global variables used in serial input */
enum {numChars = 32};
char receivedChars[numChars];    /* an array to store the received data */
long dataNumber = 0;             /* Value read from serial monitor input */
boolean newData = false;

/* Global variables relating to stepper motor position counting etc. */
long stepsToGo;             /* Number of steps left to make in present movement */
long targetPosition;        /* Intended destination of motor for given movement */
volatile long currentPosition = 0;   /* Position in steps of motor relative to startup position */
double maxSpeed;            /* Maximum speed in present movement (not nec. max permitted) */
bool direction;             /* Direction of present movement: FWDS or BWDS */

volatile float p;   /* Step interval in clock ticks or microseconds */
float p1, ps;       /* Minimum and maximum step periods */
double deltaP;      /* You'll be able to get rid of this later */
double R;           /* Multiplying constant used in Eiderman's algorithm */

/* Global variable used for noting previous time of a step in timed loop */
long prevStepTime;

void setup()
{
    long stepsToGo = 0;
    currentPosition = 0;
    goToPosition(dataNumber);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    Serial.begin(9600);
    Serial.println("Enter target position in number of steps and hit return");

#ifdef USEINTERRUPTS
    cli();
    TCCR1A = 0; // No output compare
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12); //CTC mode
    OCR1A = 0; // Set to zero for the present time: catch this to switch interrupt off
    TCCR1B |= (1 << CS12);  // 256 prescaler: overwritten in ISR
    TIMSK1 |= (1 << OCIE1A); //enable timer compare interrupt
    sei();
#else
    prevStepTime = micros();
#endif
}

void loop()
{
    unsigned long currentMillis = millis();
    unsigned long currentMicros;
    recvWithEndMarker();
    stepsToGo = computeStepsToGo();
    if (convertNewNumber())
    {
        /* Only get to this stage if there was new data to convert */
        if (stepsToGo <= 0)
        {
            /* Only get to this stage if not busy, otherwise will have thrown away input */
            goToPosition(dataNumber);

            /* ---------------------------------------------------------------------- */
            /* Start of pre-computation code											*/
            /* Add Leib Ramp formulae here */
                    
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
            
            
            /* End of pre-computation code                                            */
            /* ---------------------------------------------------------------------- */

#ifdef USEINTERRUPTS
            if (p != 0)
            {
                // Re-enable interrupts if non-zero steps
                TIMSK1 |= (1 << OCIE1A);
            }
#endif
        }
    }
#ifndef USEINTERRUPTS

    /* Timed loop for stepping, and associated coding */
    currentMicros = micros();
    if (currentMicros - prevStepTime >= p)
    {
        moveOneStep();
        prevStepTime = currentMicros;
        computeNewSpeed();
    }
#endif
    /* Timed loop for printing */
    if (currentMillis - prevMillisPrint >= printInterval)
    {
        /* Save the last time output was printed */
        prevMillisPrint = currentMillis;
        printLoop();
    }
}

void moveOneStep()
/* Move a single step, holding pulse high for delayMicroSeconds */
{
    if (p != 0) /* p=0 is code for "don't make steps" */
    {
        digitalWrite(stepPin, HIGH);
        if (direction == FWDS)
        {
        /* Is something missing here? */
        currentPosition++;
        }
        else
        {
        /* Is something missing here? */
        currentPosition--;
        }
        delayMicroseconds(stepLengthMus);
        digitalWrite(stepPin, LOW);
    }
}

void computeNewSpeed()
/* Calcuate new value of step interval p based on constants defined in loop() */
{
    /* You may need to declare some temporary variables for this function... */
    stepsToGo = computeStepsToGo();
    
    /* ----------------------------------------------------------------- */
    /* Start of on-the-fly step calculation code, executed once per step */
    /* Add Leib Ramp code here										   */
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

void recvWithEndMarker()
/* Receive data from serial port finishing with "newline" character.
   Based on http://forum.arduino.cc/index.php?topic=396450 Example 4 */
{
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    if (Serial.available() > 0) 
    {
        rc = Serial.read();

        if (rc != endMarker) 
        {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) 
        {
            ndx = numChars - 1;
        }
        }
        else 
        {
        receivedChars[ndx] = '\0'; // terminate the string
        ndx = 0;
        newData = true;
        }
    }
}

bool convertNewNumber()
/* Converts character string to long integer only if there are new
       data to convert, otherwise returns false */
{
    if (newData) 
    {
        dataNumber = 0.0;             // new for this version
        dataNumber = atol(receivedChars);   // new for this version
        newData = false;
        return true;
    }
    else
    {
        return false;
    }
}

void printLoop()
/* Print current position of stepper using timed loop */
{
    /* Sample all counters one after the other to avoid delay-related offsets */
    Serial.print("Current position = ");
    Serial.print(currentPosition);
    Serial.print("\r\n");
}

#ifdef USEINTERRUPTS
ISR(TIMER1_COMPA_vect)
/* Interrupt service routine which essentially just calls moveOneStep and computeNewSpeed.
   However, it also changes the prescale value on-the-fly so that the full range of possible
   step rates can be exploited, from around 0.25 Hz upwards, limited by step pulse width. */
{
    if (p == 0)
    {
        // Disable interrupt to avoid endless calling of interrupt if not needed
        TIMSK1 &= !(1 << OCIE1A);
    }
    moveOneStep();

    /* Adapt prescaler to keep OCR1A as large as possible within acceptable range */
    if (p < 65536)
    {
        // Prescaler 1
        OCR1A = (long)p - 1;
        TCCR1B = (TCCR1B & 0xF8) | 0x01;
    }
    else if (p < 524288)
    {
        // Prescaler 8
        OCR1A = ((long)p >> 3) - 1;
        TCCR1B = (TCCR1B & 0xF8) | 0x02;
    }
    else if (p < 4194304)
    {
        // Prescaler 64
        OCR1A = ((long)p >> 6) - 1;
        TCCR1B = (TCCR1B & 0xF8) | 0x03;
    }
    else if (p < 16777216)
    {
        // Prescaler 256
        OCR1A = ((long)p >> 8) - 1;
        TCCR1B = (TCCR1B & 0xF8) | 0x04;
    }
    else
    {
        // Prescaler 1024
        OCR1A = ((long)p >> 10) - 1;
        TCCR1B = (TCCR1B & 0xF8) | 0x05;
    }
    computeNewSpeed();
}
#endif


