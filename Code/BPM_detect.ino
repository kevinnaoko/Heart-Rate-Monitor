/*
  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. 
*/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <LiquidCrystal_I2C.h>

MAX30105 particleSensor;
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

const byte RATE_SIZE = 6; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
int heartDisplay = 0;
long time;
int beatAvg;
int timeLeft;
float beatsPerMinute;

byte hollowHeart[] = {
  B00000,
  B01010,
  B10101,
  B10001,
  B10001,
  B01010,
  B00100,
  B00000
};

byte filledHeart[] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000
};

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  particleSensor.setup();                       //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);    //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);     //Turn off Green LED

  // Initialize Custom Chars
  lcd.createChar(0, hollowHeart);
  lcd.createChar(1, filledHeart);
}

void loop()
{
  time = millis();
  long irValue = particleSensor.getIR();
  
  // Heart icon - blink when a heartbeat is detected
  if (heartDisplay != 0)
  {
    heartDisplay--;
    lcd.setCursor(15,0);
    lcd.write(byte(1)); 
    Serial.print("A");
  }
  else
  {
    lcd.setCursor(15,0);
    lcd.write(byte(0)); 
    Serial.print("B");
  }

  // Main Function
  if (time <= 35000)
  {
    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      heartDisplay = 15;
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
      rates[rateSpot++] = (byte)beatsPerMinute;   //Store this reading in the array
      rateSpot %= RATE_SIZE;                      //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      }
    }

    // Serial monitor debug
    Serial.print(beatsPerMinute);
    Serial.print(" ");
    Serial.print(beatAvg);
    Serial.print(" ");

    // Simple dot animation
    if (time <= 15000)    
    {
      lcd.setCursor(0,0);
      lcd.print("BPM: ");
      
      if (time % 1000 <= 250)
      {
        lcd.print(".");
      }
      
      else if ((time % 1000 > 250) && (time % 1000 <= 500))
      {
        lcd.print(". .");
      }

      else if ((time % 1000 > 500) && (time % 1000 <= 750))
      {
        lcd.print(". . .");
      }

      else 
      {        
        lcd.print("      ");
      }
    }
    
    else
    {
      lcd.setCursor(0,0);
      lcd.print("BPM: ");
      lcd.print(beatAvg);
      lcd.print("    ");
    }

    // Display remaining time
    lcd.setCursor(0,1);
    lcd.print("Time Left: ");

    lcd.setCursor(14,1);
    timeLeft = (35000 - millis())/1000;
    lcd.setCursor(11,1);
    lcd.print(timeLeft);
    lcd.print(" ");

    if (irValue < 50000)
    {
      Serial.print(" No finger?");
    }
    
    Serial.println();
  }

  // Finished measuring
  else
  {
    Serial.println("Finished");
    lcd.setCursor(0,0);
    lcd.print("BPM: ");
    lcd.print(beatAvg);
    lcd.print("    ");

    lcd.setCursor(0,1);
    lcd.print("Sampling Done    ");
  }
}
