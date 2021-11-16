/* Test program for reading of thermistor, thermocouple and LVDT.
   K-type thermocouple functions written by Arthur Jones using 
   official NIST polynomial data from
   https://srdata.nist.gov/its90/download/type_k.tab */

#include <math.h> /* needed for exp() and pow() */

/* It is good practice to define things like pins used at the start
   so that you avoid hard-coded values (magic numbers) in code */
#define TCpin A0
#define ThermistorPin A1

// Announce static numbers using #define to avoid magic number
#define _DEG_K_AT_0DEG_C_ 273.15
#define _R_AT_25DEG_C_kOHM_ 10.0
#define _B_AT_25DEG_C_ 3975.0
#define _V_REF_THERMISTOR_ 3.3
 
/* Similarly, define any constant values e.g. Vref, B, R0 here to avoid
  need for "magic numbers" in code */

void setup() 
{
  	Serial.begin(9600);
}

void loop() 
{
  	/* Put your code here to read ADCs and convert ADC voltages to 
  	temperatures */

    // Define VRef
    float VoltageRef = 5.0;
    float VoltageThermistor, ResisThermistor, TemperatureThermistor_inDegK, TemperatureThermistor;
    float ThermoCoupleVoltage, EmfThermalCouple, EmfColdJunction, TemperatureAbsolute;

	// get input from pin using analogRead
	int ThermistorAnalogInput = analogRead(ThermistorPin);
  	int ThermoCoupleAnalogInput = analogRead(TCpin);
    
    // Calculate thermistor temperature in degrees C
    // get the real voltage from data read from adc
    VoltageThermistor = convertADCvalueToVoltage(ThermistorAnalogInput, VoltageRef);
    // get the thermistor's resistence from readed voltage
    ResisThermistor = 10.0 * _V_REF_THERMISTOR_ / VoltageThermistor - 10.0;
    // get result of temperature in K unit using the equation
	TemperatureThermistor_inDegK = 1.0 / (1.0 / (_DEG_K_AT_0DEG_C_ + 25.0) + 1.0 / _B_AT_25DEG_C_ * log(ResisThermistor / _R_AT_25DEG_C_kOHM_));
    // get the final temerature in C unit using the K to C function above
    TemperatureThermistor = convertDegKToDegC(TemperatureThermistor_inDegK);

    // Calculate thermocouple temperature in degrees C
    // get the real voltage from data read from adc
	ThermoCoupleVoltage = convertADCvalueToVoltage(ThermoCoupleAnalogInput, VoltageRef);
    // get the emf from the adc reading [unit in V]
    EmfThermalCouple = (ThermoCoupleVoltage - 0.35)/54.4;
    // get the emf of cold junction(from thermistor) [unit in mV]
    EmfColdJunction = NISTdegCtoMilliVoltsKtype(TemperatureThermistor);
    // using the NIST to get temperature [mV is the input unit so need convertion with EmfThermalCouple]
    TemperatureAbsolute = NISTmilliVoltsToDegCKtype((EmfThermalCouple * 1000) + EmfColdJunction);
    

	/* Display results.  Don't try to be clever by using printf etc.,
		or formatting statements, they simply don't work on the Arduino. Just use 
		the serial print statements given here, inserting your own code as needed */
	Serial.print("Thermistor temperature (deg C): ");
	Serial.println(TemperatureThermistor);  // Replace ... with your code, it won't compile until you do.
	Serial.print(" Thermocouple temperature with CJC (deg C): ");
	Serial.println(TemperatureAbsolute);  // Replace ... with your code, it won't compile until you do.
	Serial.println("\n");
	delay(1000);
}

/* You are encouraged to write a function to convert ADC value to 
   voltage: put it here and use it in your code above*/

/* Under no circumstances change any of the following code, it is fine as it is */
float NISTdegCtoMilliVoltsKtype(float tempDegC)
/* returns EMF in millivolts */
{
    int i;
    float milliVolts = 0;
    if(tempDegC >= -170 && tempDegC < 0)
    {
        const float coeffs[11] =
        {
            0.000000000000E+00,
            0.394501280250E-01,
            0.236223735980E-04,
            -0.328589067840E-06,
            -0.499048287770E-08,
            -0.675090591730E-10,
            -0.574103274280E-12,
            -0.310888728940E-14,
            -0.104516093650E-16,
            -0.198892668780E-19,
            -0.163226974860E-22
        };
        for (i=0; i<=10; i++)
        {
            milliVolts += coeffs[i] * pow(tempDegC,i);
        }
    }
    else if(tempDegC >= 0 && tempDegC <= 1372)
    {
        const float coeffs[10] =
        {
            -0.176004136860E-01,
            0.389212049750E-01,
            0.185587700320E-04,
            -0.994575928740E-07,
            0.318409457190E-09,
            -0.560728448890E-12,
            0.560750590590E-15,
            -0.320207200030E-18,
            0.971511471520E-22,
            -0.121047212750E-25
        };
        const float a0 =  0.118597600000E+00;
        const float a1 = -0.118343200000E-03;
        const float a2 =  0.126968600000E+03;

        for (i=0; i<=9; i++)
        {
            milliVolts += coeffs[i] * pow(tempDegC,i);
        }

        milliVolts += a0*exp(a1*(tempDegC - a2)*(tempDegC - a2));
    }
    else
    {
        milliVolts = 99E9;
    }
    return milliVolts;
}

float NISTmilliVoltsToDegCKtype(float tcEMFmV)  
// returns temperature in deg C.
{

        int i, j;
        float tempDegC = 0;
        const float coeffs[11][3] =
        {
          {0.0000000E+00,  0.000000E+00, -1.318058E+02},
         {2.5173462E+01,  2.508355E+01,  4.830222E+01},
         {-1.1662878E+00,  7.860106E-02, -1.646031E+00},
         {-1.0833638E+00, -2.503131E-01,  5.464731E-02},
         {-8.9773540E-01,  8.315270E-02, -9.650715E-04},
         {-3.7342377E-01, -1.228034E-02,  8.802193E-06},
         {-8.6632643E-02,  9.804036E-04, -3.110810E-08},
         {-1.0450598E-02, -4.413030E-05,  0.000000E+00},
         {-5.1920577E-04,  1.057734E-06,  0.000000E+00},
         {0.0000000E+00, -1.052755E-08,  0.000000E+00}
       };
       if(tcEMFmV >=-5.891 && tcEMFmV <=0 )
       {
           j=0;
       }
       else if (tcEMFmV > 0 && tcEMFmV <=20.644  )
       {
           j=1;
       }
       else if (tcEMFmV > 20.644 && tcEMFmV <=54.886  )
       {
           j=2;
       }
       else
       {
           return 99E99;
       }

       for (i=0; i<=9; i++)
        {
            tempDegC += coeffs[i][j] * pow(tcEMFmV,i);
        }
    return tempDegC;
}

/* Write a function here to convert ADC value to voltages.
Call it from the main() function below */
float convertADCvalueToVoltage(int ADCValue, float refVoltage)
{
    // pre-define the output variable
    float inputVoltage = 0.0;
	// force int to float to avoid get a result in int
    inputVoltage = (float)ADCValue * refVoltage / 1024.0;
    return inputVoltage;
}

/* Write a function to convert degrees K to degrees C
Call it from the main() function below */
float convertDegKToDegC(float degreeInK)
{
    // calculate the C deg from K using Dc = Dk - 273.15
    // where 273.15 is pre-defined in #define to avoid hard code of magic number
    return (degreeInK - _DEG_K_AT_0DEG_C_);
}

