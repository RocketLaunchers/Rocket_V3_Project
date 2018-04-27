
/* Main Rocket program (V3)
  -Code has been sourced from various outlets and makers(Thank You!)
  -Code has been modified by the UTRGV Rocket Launchers Payload team
  -This is the main program

This code currently controls
     Temperature and Humidity Sensor
     CO2 Sensor
     Dust Sensor
     OLED Screen
     SD datalogging

Other features coming soon
     Timestamping
     Other misc. sensors
     TBD
*/

// include files
#include "Arduino.h"
#include "DHT.h"
#include "Wire.h"
#include "SeeedGrayOLED.h"
#include "avr/pgmspace.h"
#include "SPI.h"
#include "SD.h"
#include "Time.h"

//-------------------- definitions -----------------------
//     stores variables used in the program              -
//--------------------------------------------------------

// temp sensor
#define DHTPIN A0   // using pin A0
#define DHTTYPE DHT11

// SD card
#define DS1307_I2C_ADDRESS 0x68
#define TIME_MSG_LEN 11
#define TIME_HEADER 'T'


// CO_2 sensor
#define         MG_PIN                       (A2)     //define which analog input channel you are going to use
#define         BOOL_PIN                     (2)
#define         DC_GAIN                      (8.5)   //define the DC gain of amplifier
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milisecond) between each samples in
#define         ZERO_POINT_VOLTAGE           (0.220) //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         REACTION_VOLTGAE             (0.030) //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2


// globals
char floatStr[9];
char dataStr[24];
float CO2Curve[3]  =  {2.602,ZERO_POINT_VOLTAGE,(REACTION_VOLTGAE/(2.602-3))};
File myFile;
int hours = 0;
int minutes = 0;
int seconds = 0;

// motor driver variables
const int pinPwm = 11;
const int pinDir = 13;

//-------------------- Functions -----------------------------
//    Function prototypes and definitions are stored here    -
//------------------------------------------------------------

DHT dht(DHTPIN, DHTTYPE);   // controls temp


/*****************************  MGRead *********************************************
Input:   mg_pin - analog channel
Output:  output of SEN-000007
Remarks: This function reads the output of SEN-000007
************************************************************************************/
float MGRead(int mg_pin)
{
    int i;
    float v=0;

    for (i=0;i<READ_SAMPLE_TIMES;i++) {
        v += analogRead(mg_pin);
        delay(READ_SAMPLE_INTERVAL);
    }
    v = (v/READ_SAMPLE_TIMES) *5/1024 ;
    return v;
}
/*****************************  MQGetPercentage **********************************
Input:   volts   - SEN-000007 output measured in volts
         pcurve  - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(MG-811 output) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int  MGGetPercentage(float volts, float *pcurve)
{
   if ((volts/DC_GAIN )>=ZERO_POINT_VOLTAGE) {
      return -1;
   } else {
      return pow(10, ((volts/DC_GAIN)-pcurve[1])/pcurve[2]+pcurve[0]);
   }
}

void updateTime()
{
    seconds = seconds + 1;
    if(seconds == 60)
    {
        minutes = minutes + 1;
        seconds = 0;

        if(minutes == 60)
        {
            hours = hours + 1;
            minutes = 0;
        }
    }
}

//---------------- put your setup code here, to run once:----------------------
void setup()
{
    Serial.begin(9600);

    // temp sensor initialization
    dht.begin();

    // SD Card code
    Serial.println("Initializing SD card...");

    // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
    // Note that even if it's not used as the CS pin, the hardware SS pin
    // (10 on most Arduino boards, 53 on the Mega) must be left as an output
    // or the SD library functions will not work.
    pinMode(10, OUTPUT);

    if (!SD.begin(10))
    {
        Serial.print("Init failed!");
        return;
    }

    Serial.println("initialization done.");
    floatStr[0] = '\0';
    analogReference(INTERNAL);

    // CO2 code
    pinMode(BOOL_PIN, INPUT);               //set pin to input
    digitalWrite(BOOL_PIN, HIGH);           //turn on pullup resistors

    // OLED code
    Wire.begin();
    SeeedGrayOled.init(SH1107G);              //initialize SEEED OLED display
    SeeedGrayOled.clearDisplay();             //Clear Display.
    SeeedGrayOled.setNormalDisplay();         //Set Normal Display Mode
    SeeedGrayOled.setVerticalMode();          // Set to vertical mode for displaying text

    SeeedGrayOled.setTextXY(0,0);             //set Cursor to 1st line, 0th column
    SeeedGrayOled.setGrayLevel(1);            //Set Grayscale level. Any number between 0 - 15.
    SeeedGrayOled.putString("Data Readings"); //Print Title

    // motor driver code
    pinMode(pinPwm, OUTPUT);
    pinMode(pinDir, OUTPUT);
}

//---------------- put your main code here, to run repeatedly:-----------------
void loop()
{
    // variables
    int percentage;
    float volts;
    float h;
    float t;

    updateTime();

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile)
    {
        Serial.println("File opened for writing.");
    }
    else
    {
        // if the file didn't open, print an error:
        Serial.println("error opening test.txt");
        return;
    }

    // Get volts and percentage for CO2
    volts = MGRead(MG_PIN);
    percentage = MGGetPercentage(volts,CO2Curve);

    // Get Humidity and Temperature from sensor
    h = dht.readHumidity();
    t = dht.readTemperature();

    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(t) || isnan(h))
    {
        Serial.println("Failed to read from DHT");
    }
    else
    {
        //---------- Output data ----------

        // Output data to file
        myFile.print("Humidity: ");
        myFile.print(h);
        myFile.print("\t Temperature: ");
        myFile.print(t);

        SeeedGrayOled.setTextXY(1,0);  //set Cursor to 2nd line, 0th column

        // Output data to serial output window
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print("  ");
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" *C");
        Serial.print("   ");
        Serial.print("CO2: ");
        if (percentage == -1)
        {
            Serial.print( "<400" );

            // also output to OLED and SD (prevents repeat code)
            SeeedGrayOled.putString("CO2: ");
            SeeedGrayOled.putString("<400 ppm");

            myFile.print("\tCO2: <400 ppm ");
        }
        else
        {
            Serial.print(percentage);

            // also output to OLED and SD (prevents repeat code)
            SeeedGrayOled.putString("CO2: ");
            SeeedGrayOled.putNumber(percentage);
            SeeedGrayOled.putString(" ppm");

            myFile.print("\tCO2: ");
            myFile.print(percentage);
            myFile.print(" ppm ");
        }

        // Print time to serial monitor
        Serial.print("Time: ");
        Serial.print(hours);
        Serial.print(":");

        if(minutes < 10)
        {
            Serial.print("0");
            Serial.print(minutes);
        }
        else
        {
            Serial.print(minutes);
        }

        Serial.print(":");

        if(seconds < 10)
        {
            Serial.print("0");
            Serial.print(seconds);
        }
        else
        {
            Serial.print(seconds);
        }
        Serial.print("\n");

        // Print temp and humi to OLED
        SeeedGrayOled.setTextXY(2,0);  //set Cursor to 3rd line, 0th column
        SeeedGrayOled.putString("H: ");
        SeeedGrayOled.putNumber(h);
        SeeedGrayOled.putString(" ");

        SeeedGrayOled.putString("T: ");
        SeeedGrayOled.putNumber(t);
        SeeedGrayOled.putString("C");

        // Display time on OLED
        SeeedGrayOled.setTextXY(3,0);  //set Cursor to 4th line, 0th column
        SeeedGrayOled.putNumber(hours);
        SeeedGrayOled.putString(":");
        if(minutes < 10)
        {
            SeeedGrayOled.putString("0");
            SeeedGrayOled.putNumber(minutes);
        }
        else
        {
            SeeedGrayOled.putNumber(minutes);
        }

        SeeedGrayOled.putString(":");

        if(seconds < 10)
        {
            SeeedGrayOled.putString("0");
            SeeedGrayOled.putNumber(seconds);
        }
        else
        {
            SeeedGrayOled.putNumber(seconds);
        }

        // Display time to file
        myFile.print("Time: ");
        myFile.print(hours);
        myFile.print(":");

        if(minutes < 10)
        {
            myFile.print("0");
            myFile.print(minutes);
        }
        else
        {
            myFile.print(minutes);
        }

        myFile.print(":");

        if(seconds < 10)
        {
            myFile.print("0");
            myFile.print(seconds);
        }
        else
        {
            myFile.print(seconds);
        }
        myFile.print("\n");
    }

    myFile.close();

    // motor driver code
    SeeedGrayOled.setTextXY(3,0);
    if(t >= 37)
    {
      SeeedGrayOLED.putString("Its cooling");
      analogWrite(pinPwm, -20);
      digitalWrite(pinDir, HIGH);
      Serial.println("Is cooling");
    }
    else
    {
      SeeedGrayOLED.putString("Its heating");
      analogWrite(pinPwm, -20);
      digitalWrite(pinDir, HIGH);
      Serial.println("Is heating");
    }
}
