
// ST7920-based 128x64 pixel graphic LCD demo program
// Written by D. Crocker, Escher Technologies Ltd.
// adapted for STM32-arduino by Matthias Diro and added some fonts and new commands (box, fillbox, fastVline, fastHline)
// library doesn't use SPI or some low level pin toggles --> too fast for the display
// setup Display:
// PSB must be set to ground for serial mode: maybe you must unsolder a resistor!
// only two lines are needed: R/W = Mosi, E=SCK, just solder RS (Slave select) to +5V, it isn't needed!
// maple mini user: you must set VCC to +5V, but you don't have it onboard!
#include "lcd7920_STM.h"
// some fonts
extern const PROGMEM LcdFont font5x5_STM;    // in glcd10x10.cpp
extern const PROGMEM LcdFont font5x7_STM;    // in glcs5x7.cpp

// Define the following as true to use SPI to drive the LCD, false to drive it slowly instead
#define LCD_USE_SPI    (false) // no SPI yet for STM32

const int MOSIpin = PB9;
const int SCLKpin = PB8;
 
// LCD
static Lcd7920 lcd(SCLKpin, MOSIpin, LCD_USE_SPI);
int xPos = 0;
float adcVal = 0;
float adcRatio = 0;
int currPoint = 0;
int prevPoint = 0;
int dataDivision = 1;//default == 1
int timeDivision = 0;//default == 0
int bufferSize = 512;
int sampleHigh = 2048;
int sampleLow = 2048;
uint32_t sampleAverage = 0;
int currDataDisplay = 1;//default == 1
int currMode = 0;//default == 0,volts
uint32_t previousTime1 = 0;
uint32_t previousTime2 = 0;
bool INITPINA1_STATE = 0;

//int runTime = 0;

uint16_t sampleBuffer1[512];

void collectVoltSamples();
void collectAmpSamples();
void increment();
void decrement();
void drawVoltFrame();//warning, does not clear previous image
void drawAmpFrame();
void scrollMode();

// Initialization
void setup()
{
    Serial1.begin(9600);
    lcd.begin(true);          // init lcd in graphics mode
    lcd.setFont(&font5x5_STM);
    
    lcd.fastHline(0, 31 , 127, PixelSet);
    unsigned char xPos = 0;
    float adcVal = 0;
    float adcRatio = 0;
    pinMode(PB1, INPUT_ANALOG);
    pinMode(PB0, INPUT_ANALOG);
    pinMode(PA2, INPUT_ANALOG);
    pinMode(PC15, INPUT);
    pinMode(PA0, INPUT);
    pinMode(PA1, INPUT);
    pinMode(PB4, INPUT_PULLUP);
    pinMode(PB5, INPUT_PULLUP);
    pinMode(PB6, INPUT_PULLUP);
    pinMode(PB7, INPUT_PULLUP);
    attachInterrupt(PC15, increment, FALLING);
    attachInterrupt(PA0, decrement, FALLING);//look into defining EXT interface 
    attachInterrupt(PA1, scrollMode, FALLING);//change or falling?
    
}


void loop()
{
  Serial1.println("main loop entered");
  if(currMode == 0)
  {
     collectVoltSamples();
     lcd.clear();
     drawVoltFrame();
  }
  else if(currMode == 1)
  {
     collectAmpSamples();
     lcd.clear();
     drawAmpFrame();
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM MODE ERROR!!!        ");
  }
  while((millis()-previousTime1) < 300);
  previousTime1 = millis();
    
}

void drawVoltFrame()
{
  lcd.setFont(&font5x5_STM);
  lcd.fastHline(0, 31 , 128, PixelSet);
  for (xPos = 0; xPos < 128; xPos++)
  {
  adcVal = sampleBuffer1[ xPos ];
  adcRatio = (adcVal)/4095;
  currPoint = ((int)((1-adcRatio)*(64*dataDivision)))-(32*(dataDivision-1));//modify 64 to change position of wave?
  
  if(!xPos)
  {
    lcd.setPixel(xPos, currPoint, PixelSet);
  }
  else
  {
    lcd.line((xPos-1), prevPoint, xPos, currPoint, PixelSet);
  }
  
  lcd.setCursor(0, 0);
  
  if(currDataDisplay == 1)
  {
  int vppDiff = sampleHigh-sampleLow;
  float vppRatio = ((float)vppDiff)/4096;
  lcd.print(vppRatio*33);
  lcd.print(" VP-P  ");
  }
  else if(currDataDisplay == 2)
  {
  float sampleHighRatio = ((float)sampleHigh)/4096;
  lcd.print(((sampleHighRatio*3.3)-1.65)*10);
  lcd.print(" VMAX  ");
  }
  else if(currDataDisplay == 3)
  {
  float sampleLowRatio = ((float)sampleLow)/4096;
  lcd.print(((sampleLowRatio*3.3)-1.65)*10);
  lcd.print(" VMIN  ");
  }
  else
  {
  lcd.print("MODE ERROR!!!    ");  
  }
  
  prevPoint = currPoint;
  }
  
  lcd.flush();
}

void drawAmpFrame()
{
  lcd.setFont(&font5x7_STM);
    float sampleCurrRatio = (((((float)sampleBuffer1[0]-2048))*1650)/4096);//fix avg value

  for (xPos = 0; xPos < 128; xPos++)
  {
  adcVal = sampleBuffer1[ xPos ];
  adcRatio = (adcVal)/4095;
  currPoint = ((int)((1-((adcRatio)))*(64*(dataDivision*6))))-(32*((dataDivision*6)-1));//modify 64 to change position of wave?
  
  if(!xPos)
  {
    lcd.setPixel(xPos, currPoint, PixelSet);
  }
  else
  {
    lcd.line((xPos-1), prevPoint, xPos, currPoint, PixelSet);
  }
  
  lcd.setCursor(0, 0);
  if(currDataDisplay == 1)
  {
  lcd.print(sampleCurrRatio);
  lcd.print(" mA      ");
  //lcd.print("   ");
  }
  else if(currDataDisplay == 2)
  {
  float sampleHighRatio = (((float)sampleHigh-2048))/4096;
  lcd.print(((sampleHighRatio*1650))-5.64);
  //lcd.print("0");
  lcd.print(" mA MAX  ");
  }
  else if(currDataDisplay == 3)
  {
  float sampleLowRatio = ((float)(sampleLow-2048))/4096;
  lcd.print((sampleLowRatio*1650)-5.64);
  //lcd.print("0");
  lcd.print(" mA MIN  ");
  }
  else
  {
  lcd.print("MODE ERROR!!!    ");  
  }
  
  prevPoint = currPoint;
  }
  
  lcd.flush();
}

void increment()
{
   noInterrupts();
   bool PINB4_STATE = digitalRead(PB4);
   bool PINB5_STATE = digitalRead(PB5);
   bool PINB6_STATE = digitalRead(PB6);
   bool PINB7_STATE = digitalRead(PB7);
   if(!PINB4_STATE && PINB5_STATE && PINB6_STATE && PINB7_STATE)
   {
      dataDivision++;
      if(dataDivision > 3)
      {
         dataDivision = 3;
         //dataDivisionMax == true;
      }
   }
   if(PINB4_STATE && !PINB5_STATE && PINB6_STATE && PINB7_STATE)
   {
      timeDivision+=10;
      if(timeDivision >= 300)
      {
         timeDivision = 300; 
      }  
   }
   if(PINB4_STATE && PINB5_STATE && !PINB6_STATE && PINB7_STATE)
   {
      currDataDisplay++;
      if(currDataDisplay > 3)
      {
         currDataDisplay = 3; 
      }  
   }
   if(PINB4_STATE && PINB5_STATE && PINB6_STATE && !PINB7_STATE)
   {
      currMode+=1;
      if(currMode > 1)
      {
         currMode = 1; 
      }  
   }
   else
   {
      interrupts();
      return; 
   }

   for(uint32_t i = 0; i < 720000; i++)
   {
     asm("nop");  
   }
   while(!digitalRead(PC15));
   for(uint32_t i = 0; i < 720000; i++)
   {
     asm("nop"); 
   }
   interrupts();
}

void decrement()//risk of stack overflow from nested interrupts
{
   noInterrupts();
   bool PINB4_STATE = digitalRead(PB4);
   bool PINB5_STATE = digitalRead(PB5);
   bool PINB6_STATE = digitalRead(PB6);
   bool PINB7_STATE = digitalRead(PB7);
   if(!PINB4_STATE && PINB5_STATE && PINB6_STATE && PINB7_STATE)
   {
      dataDivision--;
      if(dataDivision < 1)
      {
         dataDivision = 1;  
      }
   }
   if(PINB4_STATE && !PINB5_STATE && PINB6_STATE && PINB7_STATE)
   {
      timeDivision-=10;
      if(timeDivision <= 0)
      {
         timeDivision = 0; 
      }  
   }
   if(PINB4_STATE && PINB5_STATE && !PINB6_STATE && PINB7_STATE)
   {
      currDataDisplay--;
      if(currDataDisplay < 1)
      {
         currDataDisplay = 1; 
      }  
   }
   if(PINB4_STATE && PINB5_STATE && PINB6_STATE && !PINB7_STATE)
   {
      currMode-=1;
      if(currMode < 0)
      {
         currMode = 0; 
      }  
   }
   else
   {
      interrupts();
      return; 
   }
   
   for(uint32_t i = 0; i < 720000; i++)
   {
     asm("nop");  
   }
   while(!digitalRead(PA0));
   for(uint32_t i = 0; i < 720000; i++)
   {
     asm("nop");  
   }
   interrupts();
}


void scrollMode()
{
   while(!digitalRead(PA1))
   {
      lcd.clear();
      uint16_t rawCursorPos = 4095-analogRead(PB0);
      int cursorCenter = ((rawCursorPos+1)/32);
      lcd.fastVline(cursorCenter-1, 0, 64, PixelSet);
      lcd.fastVline(cursorCenter+1, 0, 64, PixelSet);
      float selectedSampleVolt = ((float)sampleBuffer1[cursorCenter])/4096;
      float selectedSampleAmp = (((float)sampleBuffer1[cursorCenter])-2048)/4096;

      if(currMode == 0)
      {
         drawVoltFrame();
         lcd.setCursor(58, 0);
         lcd.setFont(&font5x5_STM);
         lcd.print(((selectedSampleVolt*3.3)-1.65)*10);
         lcd.print("V ");
         lcd.flush();
      }
      else
      {
         drawAmpFrame();
         lcd.setCursor(58, 0);
         lcd.setFont(&font5x5_STM);
         lcd.print((selectedSampleAmp*1650));//-5.64);
         lcd.print("mA ");
         lcd.flush();
         
      }
      
      for(uint32_t i = 0; i < 720000; i++)//50000//likely soucr of issue
      {
        asm("NOP");
      }
      
      }
}

void collectVoltSamples()
{
  sampleHigh = 2048;
  sampleLow = 2048;
  for (unsigned int sample = 0; sample < bufferSize; sample++)
  {
    sampleBuffer1[sample] = analogRead(PB1); 
    if(sampleBuffer1[sample] > sampleHigh)
    {
      sampleHigh = sampleBuffer1[sample];  
    }
    if(sampleBuffer1[sample] < sampleLow)
    {
      sampleLow = sampleBuffer1[sample];  
    }

    for(int count = 0; count < timeDivision; count++)
    {
      int dummyCapture = analogRead(PB1);
    }
    
  }  
}

void collectAmpSamples()
{
  sampleHigh = 0;
  sampleLow = 0;
  sampleAverage = 0;
  for (unsigned int sample = 0; sample < bufferSize; sample++)
  {
    sampleBuffer1[sample] = analogRead(PA2); 
    if(sampleBuffer1[sample] > sampleHigh)
    {
      sampleHigh = sampleBuffer1[sample];  
    }
    if(sampleBuffer1[sample] < sampleLow)
    {
      sampleLow = sampleBuffer1[sample];  
    }

    sampleAverage += sampleBuffer1[sample];
    
    for(int count = 0; count < timeDivision; count++)
    {
      int dummyCapture = analogRead(PA2);
    }
    
  }
  sampleAverage /= 512;  
}
