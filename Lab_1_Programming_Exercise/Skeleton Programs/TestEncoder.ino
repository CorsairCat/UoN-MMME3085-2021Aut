/* Test program for incremental encoder state machine code */
/* Serial input aspects are based closely upon: 
   http://forum.arduino.cc/index.php?topic=396450
   Example 4 - Receive a number as text and convert it to an int
   Modified to read a float */

long int count = 0;
long int error = 0;
enum states{state1=1, state2, state3, state4};
bool channelAState;
bool channelBState;

int state;
const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data

boolean newData = false;

void setup() {
    Serial.begin(9600);
    Serial.println("Enter initial state as a 2-digit number e.g. 01 then hit return");
    do {
      recvWithEndMarker();
    } while (!newData);
    channelAState = receivedChars[0]!='0';
    channelBState = receivedChars[1]!='0';
    
    initialiseEncoderStateMachine();  
    Serial.print(state);
    Serial.print('\n');

    Serial.println("Now keep entering state as a 2-digit number e.g. 01 then hit return.");
    newData = false;
}

void loop() {
    recvWithEndMarker();
    if (newData)
    {
        channelAState = receivedChars[0]!='0';
        channelBState = receivedChars[1]!='0';
        updateEncoderStateMachine();
        Serial.print("State: ");
        Serial.print((int)state);
        Serial.print(" Count: ");
        Serial.print(count);
        Serial.print(" Error: ");
        Serial.print(error);
        Serial.write('\n');
        newData = false;
    }
    
}

void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
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

void initialiseEncoderStateMachine()
{
  /* If initially A is 0 and B is 0, system starts in State 1
     If initially A is 1 and B is 0, system starts in State 2
     If initially A is 1 and B is 1, system starts in State 3
     If initially A is 0 and B is 1, system starts in State 4 */

  if (channelAState)
  {
      if(channelBState)
      {
        state = state3;
      }
      /* else ....  lots of code goes here */
  }
}

void updateEncoderStateMachine()
{
  switch (state)
  {
     case state1:
     /* If A is 0 and B is 0, do nothing and stay in State 1
        If A is 1 and B is 0, add 1 to main counter and go to State 2
        If A is 0 and B is 1, subtract 1 to main counter and go to State 4
        If A is 1 and B is 1, do nothing to main counter but add 1 to error counter and go to state 3 */

     if (channelAState && !channelBState)
     {
        count++;
        state = state2;
     }
     /* else ....  lots of code goes here */
     break; /* don't forget break at the end of each case! */
     /* other cases follow */
  }
}


