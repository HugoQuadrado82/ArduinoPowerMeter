/*
  Developed by GnobarEl for tecnovlogger (c) 2020
  v 0.6 RC
*/

#include "EmonLib.h" //INCLUSÃO DE BIBLIOTECA
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

PZEM004Tv30 pzem(2,3);   /// Software Serial pin 10 (RX) & 11 (TX) for arduino uno

#define     CURRENT_CAL       15.30   //Valor de cabibração (Deve ser ajustado com um multimetro)
#define     LINE_FREQUENCY    50      //Frequencia
//#define     VOLTAGE_AC        240     //240v
#define     ruido             0.05    // Ruido causado pelo sensor de intensidade
#define     PF                1.0     //Power Factor 1.0
#define     measurementSample 500    // Intervalo de leitura do sendor. By default: #500 ms
#define     numberOfCycles    10     

#define EEADDR 166 // Start location to write EEPROM data.

const int   pinoSensor    = A2;
const float electicityPrice = 0.16;          // custo do kWh
      int   cycleCount    = 0;
      float power;
      float voltage;
      float current;
      float currentDraw;
      float totalkWh;
      float kWh;
      float hours;
      float cost;
      int   eeAddress = 0;            //Location we want the data to be put.
                                      // initial Time display is 00:00:00
      int h=00;
      int m=00;
      int s=00;
      
      // Digital LCD Constrast setting
      // For accurate Time reading, use Arduino Real Time Clock
      static uint32_t last_time, now = 0; // RTC

      struct MyObject {
        float field1;
        byte field2;
        char name[10];
};

      EnergyMonitor emon1; //CRIA UMA INSTÂNCIA

      //Special characters
      byte verticalLine[8]  = {B00100,B00100,B00100,B00100,B00100,B00100,B00100,B00100};  
      byte char1[8]         = {B00000,B00000,B00000,B00111,B00100,B00100,B00100,B00100};
      byte char2[8]         = {B00000,B00000,B00000,B11100,B00100,B00100,B00100,B00100};
      byte char3[8]         = {B00100,B00100,B00100,B00111,B00000,B00000,B00000,B00000};
      byte char4[8]         = {B00100,B00100,B00100,B11100,B00000,B00000,B00000,B00000};
      byte char5[8]         = {B00011,B00100,B01000,B11111,B01000,B11111,B00100,B00011};

void setup(){

      #if DEBUG == false      //check to false to enable debugging
        Serial.begin(9600);
      #endif 

      //Get the time
      int EEAddr = EEADDR;
      EEPROM.get(EEAddr,h); EEAddr +=sizeof(2);
      EEPROM.get(EEAddr,m); EEAddr +=sizeof(2);
      EEPROM.get(EEAddr,s); EEAddr +=sizeof(2);

     
      //read values from EEPROM
      Serial.print("Read float from EEPROM: ");
      //Get the float data from the EEPROM at position 'eeAddress'
      EEPROM.get(eeAddress, totalkWh);
  
      Serial.println(totalkWh,5);    //This may print 'ovf, nan' if the data inside the EEPROM is not a valid float.

      lcd.begin(20,4);
      createCustomCharacters();
      printFrame();  
      delay(2500);
      lcd.clear();
      now=millis(); // read RTC initial value
      emon1.current(pinoSensor, CURRENT_CAL);
}



void loop(){
      voltage = pzem.voltage(); // Volts
      current = pzem.current(); // Amps
      power = pzem.power(); // Watts
      

      //emon1.calcVI(numberOfCycles,measurementSample); //FUNÇÃO DE CÁLCULO (20 SEMICICLOS / TEMPO LIMITE PARA FAZER A MEDIÇÃO)
      //currentDraw = emon1.Irms; //VARIÁVEL RECEBE O VALOR DE CORRENTE RMS OBTIDO
      //currentDraw = currentDraw-ruido; //VARIÁVEL RECEBE O VALOR RESULTANTE DA CORRENTE RMS MENOS O RUÍDO
      
      if(current < 0) { //SE O VALOR DA VARIÁVEL FOR MENOR QUE 0, FAZ 
          current = 0; //VARIÁVEL RECEBE 0
          cycleCount++; 
      } else {
          cycleCount++; 
     }

     //Save to EEPROM every 60 seconds
  
    if (cycleCount % 60 == 0) {
        Serial.print("Save kWh: ");
        Serial.println(totalkWh,5);
        EEPROM.put(eeAddress, totalkWh);          // Save the kWh
  
        int EEAddr = EEADDR;                      // Save the time
        EEPROM.put(EEAddr,h); EEAddr +=sizeof(2);
        EEPROM.put(EEAddr,m); EEAddr +=sizeof(2);
        EEPROM.put(EEAddr,s); EEAddr +=sizeof(2);
   
    }
       calculateThePower();
       debug();
       writeEnergyToDisplay();
       showClock();
}

void showClock() {
       // Update LCD Display
       // Print TIME in Hour, Min, Sec
       lcd.setCursor(0,0);
       if(h<10)lcd.print("0");// always 2 digits
       lcd.print(h);
       lcd.print(":");
       if(m<10)lcd.print("0");
       lcd.print(m);
       lcd.print(":");
       if(s<10)lcd.print("0");
       lcd.print(s);

 
      lcd.setCursor(0,1);// for Line 2

      while ((now-last_time) < 1000 ) // wait1000ms
        {
          now=millis();
        }

     last_time=now; // prepare for next loop 
     s=s+1; //increment sec. counting


      /* ---- manage seconds, minutes, hours am/pm overflow ----*/
       if(s==60){
        s=0;
        m=m+1;
       }
       if(m==60)
       {
        m=0;
        h=h+1;
       }
       if(h==13)
       {
        h=1;
       }
}

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to adress + 3.
void EEPROMWritelong(int address, long value)
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      }

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to adress + 3.
long EEPROMReadlong(long address)
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }



void calculateThePower(){
    
      //convert Amps to Watts
      //P(W) = PF × I(A) × V(V)
      power = PF * current * voltage; //Watts in real time
    
      //Convert Seconds to Hours
      //The time is hard coded. We are taking a measurement every second.
      hours = (float)1 / 3600;
    
      kWh = (power * hours) / 1000;
      totalkWh = totalkWh + kWh;
      
      cost = electicityPrice * totalkWh;
}

void writeEnergyToDisplay(){

      // Colunas, Linhas
      //lcd.setCursor(5,0); lcd.print("00:00:00");
  
      lcd.setCursor(11,0); lcd.print("PF:  ");
      lcd.setCursor(14,0); lcd.print(PF,1);
      lcd.setCursor(0,1); lcd.print("W:        ");
      lcd.setCursor(2,1); lcd.print(power,3);
      lcd.setCursor(10,1); lcd.print("kWh: ");
      lcd.setCursor(14,1); lcd.print(totalkWh,4);
      lcd.setCursor(0,2); lcd.print("V: ");
      lcd.setCursor(2,2); lcd.print(voltage);
      lcd.setCursor(10,2); lcd.print("T/");
      lcd.setCursor(12,2); lcd.write(byte(5));
      lcd.setCursor(13,2); lcd.print(":");
      lcd.setCursor(14,2); lcd.print(cost,4);
      lcd.setCursor(0,3); lcd.print("A:    ");
      lcd.setCursor(2,3); lcd.print(current);
     /* lcd.setCursor(6,3); lcd.print("* Menu *"); */
  }


void debug() {

      Serial.print("cycleCount: ");
      Serial.println(cycleCount);
      
      Serial.print("power: ");
      Serial.print(power,6);
      Serial.println(" W");
  
      Serial.print("hours: ");
      Serial.println(hours,10);
  
      Serial.print("totalKWh: ");
      Serial.println(totalkWh,10);
  
      Serial.print("Price: ");
      Serial.print(cost,6);
      Serial.println(" €");
      
      Serial.println("");
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
