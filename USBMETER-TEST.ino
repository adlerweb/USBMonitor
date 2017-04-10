/*
* USBMeter Test Sketch
* Copyright (c) 2017, Florian Knodt - www.adlerweb.info
* 
* Based on U8G2 HelloWorld.ino 
* Copyright (c) 2016, olikraus@gmail.com
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, 
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice, this list 
*    of conditions and the following disclaimer.
*    
*  * Redistributions in binary form must reproduce the above copyright notice, this 
*    list of conditions and the following disclaimer in the documentation and/or other 
*    materials provided with the distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
*  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
*  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*
*/

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

unsigned int ref_u = 610;
unsigned int ref_i = 412;
unsigned int ref_i_o = 145;
unsigned int ref_i_mva = 131;
unsigned int ref_DP = 615;
unsigned int ref_DN = 613;
unsigned int ref_VCC = 616;
unsigned int ref_OV = 2175;

unsigned int AA0=0;
unsigned int AA1=0;
unsigned int AA2=0;
unsigned int AA3=0;
unsigned int AA6=0;
unsigned int AA7=0;

void setup(void) {
  pinMode(2, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);

  analogReference(INTERNAL);

  Serial.begin(115200);
  Serial.println("USBMonitor Self-Test");
  
  u8g2.begin();
}

void printV(unsigned int volt) {
  unsigned int temp = volt / 100;
  boolean srt = false;

  if(temp > 10) srt = true;
  
  u8g2.print(temp);
  u8g2.print('.');
  if(srt) {
    temp = (volt % 100) / 10;
  }else{
    temp = volt % 100;
    if(temp < 10) u8g2.print('0');
  }
  u8g2.print(temp);
}


void printV(signed int volt) {
  if(volt > 0) printV((unsigned int)volt);

  u8g2.print('-');
  volt *= -1;
  unsigned int temp = volt / 100;
  
  u8g2.print(temp);
  u8g2.print('.');
  temp = (volt % 100) / 10;
  u8g2.print(temp);
}

void loop(void) {
  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_unifont_t_latin);	// choose a suitable font
  
  u8g2.setCursor(10, 12);
  u8g2.print("SELF-TEST-MODE");

  u8g2.setCursor(0, 26);
  u8g2.print("S1:");
  u8g2.print((digitalRead(5) ? 1 : 0));
  
  u8g2.setCursor(42, 26);
  u8g2.print("S2:");
  u8g2.print((digitalRead(4) ? 1 : 0));
  
  u8g2.setCursor(85, 26);
  u8g2.print("S3:");
  u8g2.print((digitalRead(3) ? 1 : 0));

  u8g2.setCursor(0, 37);
  u8g2.print("A0:");
  //u8g2.print(analogRead(A0));
  unsigned int adco = (float)analogReadCache(AA0) * ref_u / 1000;
  printV(adco);

  u8g2.setCursor(64, 37);
  u8g2.print("A1:");
  //u8g2.print(analogRead(A1));
  signed int itmp=0;
  adco = (float)analogReadCache(AA1) * ref_i / 1000;
  itmp = adco - ref_i_o;
  itmp *= ref_i_mva;
  itmp /= 10;
  printV(itmp);

  u8g2.setCursor(0, 48);
  u8g2.print("A2:");
  //u8g2.print(analogRead(A2));
  adco = (float)analogReadCache(AA2) * ref_DP / 1000;
  printV(adco);

  u8g2.setCursor(64, 48);
  u8g2.print("A3:");
  //u8g2.print(analogRead(A3));
  adco = (float)analogReadCache(AA3) * ref_DN / 1000;
  printV(adco);

  u8g2.setCursor(0, 59);
  u8g2.print("A6:");
  //u8g2.print(analogRead(A6));
  adco = (float)analogReadCache(AA6) * ref_OV / 1000;
  printV(adco);

  u8g2.setCursor(64, 59);
  u8g2.print("A7:");
  //u8g2.print(analogRead(A7));
  adco = (float)analogReadCache(AA7) * ref_VCC / 1000;
  printV(adco);

  Serial.print((AA0));
  Serial.print(';');
  Serial.print((AA1));
  Serial.print(';');
  Serial.print((AA2));
  Serial.print(';');
  Serial.print((AA3));
  Serial.print(';');
  Serial.print((AA6));
  Serial.print(';');
  Serial.print((AA7));
  Serial.print(';');
  Serial.print(itmp);
  Serial.println();
    
  u8g2.sendBuffer();					// transfer internal memory to the display

  if(Serial.available() > 0) {
    char in = Serial.read();
    switch(in) {
      case '1':
      case 1:
        digitalWrite(2, HIGH);
        Serial.println("o1");
        break;
      case '0':
      case 0:
        digitalWrite(2, LOW);
        Serial.println("o0");
        break;
        
    }
  }

  //Wait 100ms to next display
  unsigned long lct = millis() + 500;
  while(millis() <= lct) {
    AA0 = (AA0 + (analogRead(A0)*10)) / 2;
    AA1 = (AA1 + (analogRead(A1)*10)) / 2;
    AA2 = (AA2 + (analogRead(A2)*10)) / 2;
    AA3 = (AA3 + (analogRead(A3)*10)) / 2;
    AA6 = (AA6 + (analogRead(A6)*10)) / 2;
    AA7 = (AA7 + (analogRead(A7)*10)) / 2;
  }
}

unsigned int analogReadCache(unsigned int out) {
  return out/10;
}
