/*
  Developed by GnobarEl for tecnovlogger (c) 2020
  v 0.1
  References
  Debounce code: https://www.instructables.com/id/Arduino-Software-debouncing-in-interrupt-function/
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address


const int sensorIn = A0; //ACS712 Sensor --> Analog Pin 0
int mVperAmp = 66; // use 100 for 20A Module and 66 for 30A Module
double Voltage = 5;
double VRMS = 0;
double AmpsRMS = 0;
/*
 Since we dont have a PZEM we need to fake some values;
  - PF = 0.9
  - Main Voltage = 226V
  - Voltage Frequency = 50Hz;
*/
float PF = 1.0;
int mainVoltage = 226;
int Hz = 50;
int totalWatts = 0;
int cycleCount = 0;
float hours = 0;
float consumedkWh = 0;
float electricityPrice = 0.17; // €0.17 kWh
float totalPriceToPay = 0;

//Special characters
byte verticalLine[8]  = {B00100,B00100,B00100,B00100,B00100,B00100,B00100,B00100};  
byte char1[8]         = {B00000,B00000,B00000,B00111,B00100,B00100,B00100,B00100};
byte char2[8]         = {B00000,B00000,B00000,B11100,B00100,B00100,B00100,B00100};
byte char3[8]         = {B00100,B00100,B00100,B00111,B00000,B00000,B00000,B00000};
byte char4[8]         = {B00100,B00100,B00100,B11100,B00000,B00000,B00000,B00000};
byte char5[8]         = {B00011,B00100,B01000,B11111,B01000,B11111,B00100,B00011};

void setup()
{
  Serial.begin(9600);
  
  lcd.begin(20,4);
  createCustomCharacters();
  printFrame();  
  delay(2500);
  lcd.clear();
}


void loop()
{
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707;  //root 2 is 0.707
  AmpsRMS = (VRMS * 1000)/mVperAmp;

  //AC Amps to Kilowatts --> P(kW) = PF × I(A) × V(V)
  float watts = (PF * AmpsRMS * mainVoltage);
  // Amps to kWh -->P(kW) = PF × I(A) × V(V) / 1000
  float kWh = (PF * AmpsRMS * mainVoltage)/1000;
  // Total Cost (Euros) by hour of use --> Price by hour = kWh * electricityPrice;
  float priceByHour = kWh * electricityPrice;

  // Voltage
  lcd.setCursor(2,0); lcd.print("V: ");
  lcd.setCursor(4,0); lcd.print(mainVoltage);

  // Power Factor
  lcd.setCursor(1,1); lcd.print("PF: ");
  lcd.setCursor(4,1); lcd.print(PF,1);

  // KWh
  lcd.setCursor(0,2); lcd.print("kWh: ");
  lcd.setCursor(4,2); lcd.print(kWh);

  // Amps
  lcd.setCursor(12,0); lcd.print("A: ");
  lcd.setCursor(14,0); lcd.print(AmpsRMS);

  // Watts
  lcd.setCursor(12,1); lcd.print("W: ");
  lcd.setCursor(14,1); lcd.print(watts,1); lcd.print("   ");

  // Hz
  lcd.setCursor(11,2); lcd.print("Hz: ");
  lcd.setCursor(14,2); lcd.print(Hz);

  // Real time power cost
  lcd.setCursor(10,3); lcd.write(byte(5));
  lcd.setCursor(11,3); lcd.print("/h");
  lcd.setCursor(13,3); lcd.print(":");
  lcd.setCursor(14,3); lcd.print(priceByHour,3);

  //calcular total de watts consumidos segundo a segundo
  totalWatts = totalWatts + watts;
  hours = cycleCount / 3600;
  // convert total watts to kwh
  // E(kWh) = P(W) × t(hr) / 1000
  consumedkWh = (totalWatts * hours) / 1000;
  totalPriceToPay = consumedkWh * electricityPrice;

  // Total Cost
  lcd.setCursor(0,3); lcd.print("T/");
  lcd.setCursor(2,3); lcd.write(byte(5));
  lcd.setCursor(3,3); lcd.print(":");
  lcd.setCursor(4,3); lcd.print(totalPriceToPay,3);
  
  cycleCount++;
  delay(1000);  
}

float getVPP()
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here
 
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue)
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue)
       {
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;
   return result;
}





void printFrame()
{
  lcd.setCursor(1,0);
  lcd.print("------------------");
  lcd.setCursor(1,3);
  lcd.print("------------------");
  lcd.setCursor(0,1);
  lcd.write(byte(0));
  lcd.setCursor(0,2);
  lcd.write(byte(0));
  lcd.setCursor(19,1);
  lcd.write(byte(0));
  lcd.setCursor(19,2);
  lcd.write(byte(0));
  lcd.setCursor(0,0);
  lcd.write(byte(1));
  lcd.setCursor(19,0);
  lcd.write(byte(2));
  lcd.setCursor(0,3);
  lcd.write(byte(3));
  lcd.setCursor(19,3);
  lcd.write(byte(4));

  lcd.setCursor(4,1);
  lcd.print("tecnovlogger");
  lcd.setCursor(5,2);
  lcd.print("PowerMeter");
}

void createCustomCharacters()
{
  lcd.createChar(0, verticalLine);
  lcd.createChar(1, char1);
  lcd.createChar(2, char2);
  lcd.createChar(3, char3);
  lcd.createChar(4, char4);
  lcd.createChar(5, char5); 
}
