/* Example of driving servomotor and reading encoder signals in various ways */

#include <avr/io.h>  /* Needed to set up counter on pin 47 */
#include <SPI.h>     /* Needed to communicate with LS7366R (Counter Click) */

/* Serial input aspects are based closely upon: 
   http://forum.arduino.cc/index.php?topic=396450
   Example 4 - Receive a number as text and convert it to an int
   Modified to read a float */

/* LS7366R aspects very loosely based on concepts used in controlling
   the Robogaia 3-axis encoder shield though implementation is very different
   https://www.robogaia.com/3-axis-encoder-conter-arduino-shield.html */

/* Counting using Timer 5 (external counter input) based loosely on code from 
  https://forum.arduino.cc/index.php?topic=59396.0 written by bubuldino */

/* Pins used for L298 driver */
#define enA 13      /* PWM output, also visible as LED */
#define in1 8       /* H bridge selection input 1 */
#define in2 9       /* H bridge selection input 2 */
#define minPercent -100.0
#define maxPercent 100.0

/* Encoder input pins (used for state machine and interrupts) */
#define channelA 2
#define channelB 3

/* Used to to initiate SPI communication to LS7366R chip (Counter click) */
#define chipSelectPin 10

/* Size of buffer used to store received characters */
#define numChars 32

/* Intervals in milliseconds for user-defined timed loops */
#define printInterval 1000           

/* Global variables used in serial input */ 
char receivedChars[numChars];   // an array to store the received data
float dataNumber = 0;             // new for this version
boolean newData = false;

/* Global variables used for motor control and encoder reading */
double percentSpeed;
double encoderValue;

/* Used for state machine and encoder reading */
typedef enum states{state1=1, state2, state3, state4};
volatile long int count = 0;
volatile long int error = 0;
volatile states state;
bool channelAState, channelBState;

/* Used for handling overflows in Timer 5 */
volatile long int bigLaps;

/* Global variables used for loop timing */
unsigned long prevMillisPrint = 0;        /* stores last time values were printed */
unsigned long prevMillisControl = 0;      /* stores last time control action was updated */

/* Overlapping regions of memory used to convert four bytes to a long integer */
union fourBytesToLong
{
  long result;
  unsigned char bytes [4];
};

void setup() 
{
  Serial.begin(9600);
  Serial.println("Enter PWM duty cycle as a percentage (positive for forward, negative for reverse");

  /* Set encoder pins as input but with pullup resistors to be compatible with various encoders */
  pinMode(channelA, INPUT_PULLUP);
  pinMode(channelB, INPUT_PULLUP);

  channelAState = digitalRead(channelA);
  channelBState = digitalRead(channelB);

  initialiseEncoderStateMachine();  /* Find initial state based on inputs */
  
  /* Set up and initialise pin used for selecting LS7366R counter: hi=inactive */
  pinMode(chipSelectPin, OUTPUT);   
  digitalWrite(chipSelectPin, HIGH);

  SetUpLS7366RCounter();

  delay(100);

  /* Configure Timer 5 to count pulses on pin 47 */
  pinMode(47, INPUT_PULLUP);           // set pin to input with pullup resistor
  
  TCCR5A = 0; // No waveform generation needed. 
  TCCR5B = (1<<CS50) | (1<<CS51) | (1<<CS52); // Normal mode, clock from pin T5 on rising edge. T5 is Arduinos Pin 47
  TCCR5C = 0; // No force output compare. 
  TCNT5 = 0;  // Initialise counter register to zero.
  TIMSK5= (1<<TOIE5);  // Enable overflow interrupt
  sei();      // Enable all interrupts
  bigLaps = 0; // Initialise number of overflows
  
  /* Configure control pins for L298 H bridge */
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  /* Set initial rotation direction */
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  //attachInterrupt(digitalPinToInterrupt(channelA), updateEncoderStateMachine, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(channelB), updateEncoderStateMachine, CHANGE); 
}

void loop() 
{
  unsigned long currentMillis = millis();

  if (currentMillis - prevMillisPrint >= printInterval) {
    // save the last time you printed output
    prevMillisPrint = currentMillis;
    printLoop();
  }
  
  recvWithEndMarker();
  if(convertNewNumber())
  // Update value read from serial line
  {
     percentSpeed=dataNumber;
     driveMotorPercent(percentSpeed);
  }

  updateEncoderStateMachine();
}

void driveMotorPercent(double percentSpeed)
/* Output PWM and H bridge signals based on positive or negative duty cycle % */
{
      percentSpeed = constrain(percentSpeed, -100, 100);
      int regVal = map(percentSpeed, -100, 100, -255, 255);
      analogWrite(enA, (int)abs(regVal));
      digitalWrite(in1, regVal>0);
      digitalWrite(in2, !(regVal>0));
}

void printLoop()
/* Print count and control information */
{
   /* Sample all counters one after the other to avoid delay-related offsets */
   long encoderCountFromLS7366R = readEncoderCountFromLS7366R();
   long encoderCountFromStateMC = count;
   long stateMCerror = error;
   long timer5Count = TCNT5 + bigLaps*65536;
   Serial.print("Count from LS7366R = ");
   Serial.print(encoderCountFromLS7366R);
   Serial.print(" from state m/c = ");
   Serial.print(encoderCountFromStateMC);
   Serial.print(" State m/c errors = ");
   Serial.print(stateMCerror);
   Serial.print(" Count from LS7366R/4 = ");
   Serial.print(encoderCountFromLS7366R/4);
   Serial.print(" from Timer 5 = ");
   Serial.print(timer5Count);
   Serial.print(" Percent speed = ");
   Serial.print(percentSpeed);
   Serial.print("\r\n");
}
 
long readEncoderCountFromLS7366R()
/* Reads the LS7366R chip to obtain up/down count from encoder.  Reads four
   bytes separately then concverts them to a long integer using a union */
{
    fourBytesToLong converter; /* Union of four bytes and a long integer */
    
    digitalWrite(chipSelectPin,LOW); /* Make LS7366R active */
    
    SPI.transfer(0x60); // Request count
    converter.bytes[3] = SPI.transfer(0x00); /* Read highest order byte */
    converter.bytes[2] = SPI.transfer(0x00); 
    converter.bytes[1] = SPI.transfer(0x00); 
    converter.bytes[0] = SPI.transfer(0x00); /* Read lowest order byte */
    
    digitalWrite(chipSelectPin,HIGH); /* Make LS7366R inactive */
   
    return converter.result;
}


void SetUpLS7366RCounter(void)
/* Initialiseds LS7366R hardware counter on Counter Click board to read quadrature signals */
{
    /* Control registers in LS7366R - see LS7366R datasheet for this and subsequent control words */
    unsigned char IR = 0x00, MRD0=0x00;
    
    // SPI initialization
    SPI.begin();
    //SPI.setClockDivider(SPI_CLOCK_DIV16);      // SPI at 1Mhz (on 16Mhz clock)
    delay(10);

   /* Configure as free-running 4x quadrature counter */
   digitalWrite(chipSelectPin,LOW); /* Select chip and initialise transfer */
   /* Instruction register IR */
   IR |= 0x80;   /* Write to register (B7=1, B6=0) */
   IR |= 0x08;   /* Select register MDR0: B5=0, B4=0, B3=1 */
   SPI.transfer(IR); /* Write to instruction register */ 
   /* Mode register 0 */
   MRD0 |= 0x03;    /* 4x quadrature count: B0=1, B1=1 */
   /* B2=B3=0: free running.  B4=B5=0: disable index. */
   /* B6=0: asynchronous index.  B7: Filter division factor = 1. */
   SPI.transfer(MRD0);
   digitalWrite(chipSelectPin,HIGH); 

   /* Clear the counter i.e. set it to zero */
   IR = 0x00; /* Clear the instructino register IR */
   digitalWrite(chipSelectPin,LOW); /* Select chip and initialise transfer */
   IR |= 0x20; /* Select CNTR: B5=1,B4=0,B3=0; CLR register: B7=0,B6=0 */
   SPI.transfer(IR); /* Write to instruction register */ 
   digitalWrite(chipSelectPin,HIGH); 
   
}

void recvWithEndMarker() 
/* Receive data from serial port finishing with "newline" character. 
   Based on http://forum.arduino.cc/index.php?topic=396450 Example 4 */
{
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    if (Serial.available() > 0) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

bool convertNewNumber() 
/* Converts character string to floating point number only if there are new
       data to convert, otherwise returns false */
{
    if (newData) {
        dataNumber = 0.0;             // new for this version
        dataNumber = atof(receivedChars);   // new for this version
        newData = false;
        return true;
    }
    else
    {
       return false;
    }
}

void initialiseEncoderStateMachine()
/* User written code to initialise state of state machine code based on input states */
{
    /* If initially A is 0 and B is 0, system starts in State 1
       If initially A is 1 and B is 0, system starts in State 2
       If initially A is 1 and B is 1, system starts in State 3
       If initially A is 0 and B is 1, system starts in State 4 */
    // initial all state using the first input
    if (channelAState)
    {
        if(channelBState)
        {
            state = state3;
        }
        else
        {
            state = state2;
        }
    }
    else
    {
        if(channelBState)
        {
            state = state4;
        }
        else
        {
            state = state1;
        }
    }
}

void updateEncoderStateMachine()
/* User written code to update state and increment count of state machine  */
{
  channelAState = digitalRead(channelA);
  channelBState = digitalRead(channelB);
  
    switch (state)
    {
    case state1:
        /* If A is 0 and B is 0, do nothing and stay in State 1
           If A is 1 and B is 0, add 1 to main counter and go to State 2
           If A is 0 and B is 1, subtract 1 to main counter and go to State 4
           If A is 1 and B is 1, do nothing to main counter but add 1 to error counter and go to state 3 */

        if (channelAState && !channelBState)
        {
            // 10 -> state 2
            count++;
            state = state2;
        }
        else if (!channelAState && channelBState)
        {
            // 01 -> state 4
            count--;
            state = state4;
        }
        else if (channelAState && channelBState)
        {
            // 11 -> state 3
            error++;
            state = state3;
        }
        else
        {
            // 00 -> state 1
            // do nothing, stay in state 1
        }
        /* else ....  lots of code goes here */
        break; /* don't forget break at the end of each case! */
    case state2:
        /* If A is 0 and B is 0, subtract 1 to main counter and go to state 1
           If A is 1 and B is 0, do nothing and stay in State 2
           If A is 1 and B is 1, add 1 to main counter and goto State 3
           If A is 0 and B is 1, do nothing to main counter but add 1 to error counter and go to State 4
            */
        if (channelAState && channelBState)
        {
            // 11 -> state 3
            count++;
            state = state3;
        }
        else if (!channelAState && channelBState)
        {
            // 01 -> state 4
            error++;
            state = state4;
        }
        else if (!channelAState && !channelBState)
        {
            // 00 -> state 1
            count--;
            state = state1;
        }
        else
        {
            // 10 -> state 2
            // do nothing, stay in state 2
        }
        break;
    case state3:
        /* If A is 0 and B is 0, do nothing to main counter 
                but add 1 to error counter and go to state 1
           If A is 1 and B is 0, subtract 1 to main counter and go to State 2
           If A is 0 and B is 1, add 1 to main counter and go to State 4
           If A is 1 and B is 1, do nothing and stay in state 3 */
        if (channelAState && !channelBState)
        {
            // 10 -> state2
            count--;
            state = state2;
        }
        else if (!channelAState && channelBState)
        {
            // 01 -> state 4
            count++;
            state = state4;
        }
        else if (!channelAState && !channelBState)
        {
            // 00 -> state 1
            error++;
            state = state1;
        }
        else
        {
            // 11 -> state 3
            // do nothing, stay in state 3
        }
        break;
    case state4:
        /* If A is 0 and B is 0, add 1 to main counter and go to state 1
           If A is 1 and B is 0, do nothing to main counter but add 1 to error counter and go to State 2
           If A is 1 and B is 1, subtract 1 to main counter and goto State 3
           If A is 0 and B is 1, do nothing and stay in State 4
            */
        if (channelAState && channelBState)
        {
            // 11 -> state 3
            count--;
            state = state3;
        }
        else if (channelAState && !channelBState)
        {
            // 10 -> state 2
            error++;
            state = state4;
        }
        else if (!channelAState && !channelBState)
        {
            // 00 -> state 1
            count++;
            state = state1;
        }
        else
        {
            // 01 -> state 4
            // do nothing, stay in state 2
        }
        break;
    }
}

ISR(TIMER5_OVF_vect )
{
  //when this runs, you had 65536 pulses counted.
  bigLaps++; 
}

