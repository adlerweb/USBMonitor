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
#include <Wire.h>
#include <EEPROM.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

//#define DEBUG

#define LED 13
#define FET 2
#define SW 3
#define RE1 4
#define RE2 5

#define ADC_U A0
#define ADC_I A1
#define ADC_DP A3
#define ADC_DN A2
#define ADC_VCC A7
#define ADC_OV A6

#define BROWNOUT 0 //4.6V - write EEPROM and shut down

static const float version = 0.02;

unsigned long lasttime = millis();
unsigned int runtime=0;

unsigned long lastkey = 0;
boolean lastkeyact = false;

unsigned int u_min = 475;
unsigned int u_max = 525;

unsigned int ref_u = 610;

/*
 * 10A

  unsigned int ref_i = 497;
  unsigned int ref_i_o = 174;
  unsigned int ref_i_mva = 146;
*/

/*
 * 5A
*/
  unsigned int ref_i = 412;
  unsigned int ref_i_o = 145;
  unsigned int ref_i_mva = 131;
/**/

unsigned int ref_DP = 615;
unsigned int ref_DN = 613;
unsigned int ref_VCC = 616;
unsigned int ref_OV = 2175;

unsigned int volt=0;
signed int amp=0;
signed int mAh=0;
signed long mWh=0;

char mAh_c=0;
char mWh_c=0;

unsigned int lastVcc=0;

unsigned int DP=0;
unsigned int DN=0;

byte signaling = 0; //0=open; 1=data; 2=apple-500; 3=Apple-1000; 4=Apple-2000; 5=DCP

/*
 * When D+ = D− = 2.0 V, the device may pull up to 500 mA.
 * When D+ = 2.0 V and D− = 2.8 V, the device may pull up to 1 A of current.
 * When D+ = 2.8 V and D− = 2.0 V, the device may pull up to 2 A of current.
 */

char flags[4];
byte uov = 0;
byte uovAct = 0;

byte menu = 0;
byte menuAct = 0;
char uartBtn = 0x00;

unsigned int adc_u = 0;
unsigned int adc_i = 0;
unsigned int adc_DP = 0;
unsigned int adc_DN = 0;
unsigned int adc_VCC = 0;
unsigned int adc_OV = 0;

boolean running = false;
boolean output = false;

void setup() {
  unsigned int tRead=0;
  
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  pinMode(FET, OUTPUT);

  pinMode(SW, INPUT_PULLUP);
  pinMode(RE1, INPUT_PULLUP);
  pinMode(RE2, INPUT_PULLUP);

  pinMode(ADC_U, INPUT);
  pinMode(ADC_I, INPUT);
  pinMode(ADC_DP, INPUT);
  pinMode(ADC_DN, INPUT);
  pinMode(ADC_VCC, INPUT);
  pinMode(ADC_OV, INPUT);

  digitalWrite(LED, LOW);
  digitalWrite(FET, LOW);
  digitalWrite(ADC_U, LOW);
  digitalWrite(ADC_I, LOW);
  digitalWrite(ADC_DP, LOW);
  digitalWrite(ADC_DN, LOW);
  digitalWrite(ADC_VCC, LOW);
  digitalWrite(ADC_OV, LOW);

  analogReference(INTERNAL); //1.1V internal bandgap reference

  EEPROM.get(0, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) u_min = tRead;
  EEPROM.get(2, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) u_max = tRead;
  EEPROM.get(4, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_u = tRead;
  EEPROM.get(6, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_i = tRead;
  EEPROM.get(8, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_DP = tRead;
  EEPROM.get(10, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_DN = tRead;
  EEPROM.get(12, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_VCC = tRead;
  EEPROM.get(14, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) ref_OV = tRead;
  EEPROM.get(16, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) mAh = tRead;
  EEPROM.get(18, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) mWh = tRead;
  EEPROM.get(22, tRead);
  if(tRead > 0x0000 && tRead < 0xFFFF) runtime = tRead;
  
  Serial.begin(115200);

  Serial.println(F("#BOOT"));
  Serial.println(F("#USB POWER Monitor - www.adlerweb.info"));
  Serial.print(F("#Version: "));
  Serial.println(version);

  Serial.print(F("#[C] uMin: "));
  Serial.println(u_min);
  Serial.print(F("#[C] uMax: "));
  Serial.println(u_max);
  Serial.print(F("#[C] ref_u: "));
  Serial.println(ref_u);
  Serial.print(F("#[C] ref_i: "));
  Serial.println(ref_i);
  Serial.print(F("#[C] ref_DP: "));
  Serial.println(ref_DP);
  Serial.print(F("#[C] ref_DN: "));
  Serial.println(ref_DN);
  Serial.print(F("#[C] ref_VCC: "));
  Serial.println(ref_VCC);
  Serial.print(F("#[C] ref_OV: "));
  Serial.println(ref_OV);
  Serial.print(F("#[C] mAh: "));
  Serial.println(mAh);
  Serial.print(F("#[C] mWh: "));
  Serial.println(mWh);
  Serial.print(F("#[C] runtime: "));
  Serial.println(runtime);
  
  u8g2.begin();
  u8g2.firstPage();

  do {
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(11,13,"USB POWER MONITOR");
    u8g2.drawStr(11,25,"www.adlerweb.info");
  
    String out;
    out = "Version: ";
    out += version;
  
    char outc[out.length()+1];
    out.toCharArray(outc, sizeof(outc));
  
    u8g2.drawStr(25,50,outc);
  } while ( u8g2.nextPage() );

  delay(1000);
}


void dbug(String str) {
  #ifdef DEBUG
    Serial.print("D:");
    Serial.println(str);
  #endif
}


void loop() {
  unsigned int passed = (millis()-lasttime)/1000;
  
  getReadings();
  checkVcc();
  procUART();
  procSwitch();
  
  if(!u8g2.nextPage() && (millis()-lasttime)/1000 > 0) {
    dbug("FRAME");
    
    if(running && runtime < 0xFFFF) {
      runtime += (millis()-lasttime)/1000;
    }

    lasttime = millis();

    u8g2.firstPage();

    volt = procVolt();
    amp = procAmp();
    procmAh(passed);
    procmWh(passed);

    procDP();
    procDN();
    procSignaling();

    flags[0] = 'C';
    if(running) flags[0] = 'D';
    flags[1] = 'Q';
    if(output) flags[1] = 'S';
    
    switch(signaling) {
      case 1: //Data
        flags[2] = 'f';
        break;
      case 2: //Apple 0.5
      case 3: //Apple 1.0
      case 4: //Apple 2.0
        flags[2] = 'I';
        break;
      case 5: //DCP
        flags[2] = 'J';
        break;
      default: //0=OPEN
        flags[2] = 'H';
    }

    uovAct = uov;
    menuAct = menu;
    
    Serial.print('!');
    Serial.print(volt);
    Serial.print(';');
    Serial.print(amp);
    Serial.print(';');
    Serial.print(mAh);
    Serial.print(';');
    Serial.print(mWh);
    Serial.print(';');
    Serial.print(runtime);
    Serial.print(';');
    Serial.print(running);
    Serial.print(';');
    Serial.print(output);
    Serial.print(';');
    Serial.print(signaling);
    Serial.print(';');
    Serial.print(lastVcc);
    Serial.println();
  }

  switch(menuAct) {
    case 0:
      u8g2.setFont(u8g2_font_6x13_tr);
      u8g2.drawStr(38,13,"USB PWR MONITOR");
    
      drawVolt();
      drawAmp();
    
      drawWatt();
      
      drawAh();
      drawWh();
      
      drawTime(runtime);
    
      switch(uovAct) {
        case 1:
          u8g2.drawStr(115,63,"UV");
          break;
        case 2:
          u8g2.drawStr(115,63,"OV");
          break;
      }
    
      u8g2.setFont(u8g2_font_m2icon_9_tf);
      u8g2.drawStr(0,11,flags);
      break;
    case 1:
      u8g2.setFont(u8g2_font_6x13_tr);
      u8g2.drawStr(38,13,"USB PWR MONITOR");

      drawDP();
      drawDN();
      drawSignaling();
      drawVCC();
      
      u8g2.setFont(u8g2_font_m2icon_9_tf);
      u8g2.drawStr(0,11,flags);
      break;
    case 2:
      u8g2.setFont(u8g2_font_6x13_tr);
      u8g2.drawStr(38,13,"USB PWR MONITOR");
      u8g2.drawStr(0,25,"Lower Limit:");

      drawUMin();

      u8g2.setFont(u8g2_font_m2icon_9_tf);
      u8g2.drawStr(0,11,flags);
      break;
    case 3:
      u8g2.setFont(u8g2_font_6x13_tr);
      u8g2.drawStr(38,13,"USB PWR MONITOR");
      u8g2.drawStr(0,25,"Upper Limit:");

      drawUMax();

      u8g2.setFont(u8g2_font_m2icon_9_tf);
      u8g2.drawStr(0,11,flags);
      break;
  }
  
}

void getReadings(void) {
  #ifdef DEBUG
  Serial.print('.');
  #endif
  
  if(adc_u == 0) {
    adc_u = analogRead(ADC_U)*10;
  }else{
    adc_u = (adc_u + analogRead(ADC_U)*10) / 2;
  }
  
  if(adc_i == 0) {
    adc_i = analogRead(ADC_I)*10;
  }else{
    adc_i = (adc_i + analogRead(ADC_I)*10) / 2;
  }
  
  if(adc_DP == 0) {
    adc_DP = analogRead(ADC_DP)*10;
  }else{
    adc_DP = (adc_DP + analogRead(ADC_DP)*10) / 2;
  }
  
  if(adc_DN == 0) {
    adc_DN = analogRead(ADC_DN)*10;
  }else{
    adc_DN = (adc_DN + analogRead(ADC_DN)*10) / 2;
  }
  
  if(adc_VCC == 0) {
    adc_VCC = analogRead(ADC_VCC)*10;
  }else{
    adc_VCC = (adc_VCC + analogRead(ADC_VCC)) / 2;
  }
  
  if(adc_OV == 0) {
    adc_OV = analogRead(ADC_OV);
  }else{
    adc_OV = (adc_OV + analogRead(ADC_OV)*10) / 2;
  }
}

void checkVcc(void) {
  unsigned int vccchk = getVcc();

  if(vccchk <= BROWNOUT) {
    EEPROM.put(16, mAh);
    EEPROM.put(18, mWh);
    EEPROM.put(22, runtime);
    fetOff();
    digitalWrite(LED, HIGH);
    Serial.println(F("E:VCC"));
    Serial.flush();
    while(1) {}
  }
}

unsigned int getVcc(void) {
  unsigned int adco = (float)adc_VCC * ref_VCC / 10000;
  lastVcc = adco;
  adc_VCC = 0;
  return adco;
}

void procUART(void) {
  unsigned int temp=0;
  if(Serial.available()) {
    switch(Serial.read()) {
      case '1':
        fetOn();
        Serial.println(F("OK"));
        break;
      case '0':
        fetOff();
        Serial.println(F("OK"));
        break;
      case 'r':
        runtime = 0;
        mAh=0;
        mWh=0;
        mAh_c=0;
        mWh_c=0;
        uov=0;
        Serial.println(F("OK"));
        break;
      case 's':
        running = false;
        fetOff();
        Serial.println(F("OK"));
        break;
      case 'S':
        running = true;
        fetOn();
        Serial.println(F("OK"));
        break;
      case '<':
        temp = Serial.parseInt();
        if(temp > 0) {
          u_min = temp;
          EEPROM.put(0, u_min);
          Serial.println(F("OK"));
        }
        break;
        temp = Serial.parseInt();
        if(temp > 0) {
          u_max = temp;
          EEPROM.put(2, u_max);
          Serial.println(F("OK"));
        }
        break;
      case 'u':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_u = temp;
          EEPROM.put(4, ref_u);
          Serial.println(F("OK"));
        }
        break;
      case 'i':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_i = temp;
          EEPROM.put(6, ref_i);
          Serial.println(F("OK"));
        }
        break;
      case 'p':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_i = temp;
          EEPROM.put(8, ref_i);
          Serial.println(F("OK"));
        }
        break;
      case 'n':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_DN = temp;
          EEPROM.put(10, ref_DN);
          Serial.println(F("OK"));
        }
        break;
      case 'v':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_VCC = temp;
          EEPROM.put(12, ref_VCC);
          Serial.println(F("OK"));
        }
        break;
      case 'O':
        temp = Serial.parseInt();
        if(temp > 0) {
          ref_OV = temp;
          EEPROM.put(14, ref_OV);
          Serial.println(F("OK"));
        }
        break;
      case 'V':
        Serial.print(F("#Version: "));
        Serial.println(version);
        break;
      case 'm':
        uartBtn = 'm';
        break;
      case '+':
        uartBtn = '+';
        break;
      case '-':
        uartBtn = '-';
        break;
    }
  }
}

void procSwitch(void) {
  if(( lastkey+100) > millis()) return;
  
  if(lastkeyact) {
    if(digitalRead(SW) == HIGH && digitalRead(RE1) == HIGH && digitalRead(RE2) == HIGH) {
      lastkeyact = false;
      lastkey = millis()+50;
    }

    return;
  }
  
  if(digitalRead(SW) == LOW || uartBtn == 'm') {
    lastkeyact = true;
    lastkey = millis();

    if(menu == 2) {
      unsigned int temp=0;
      EEPROM.get(0, temp);

      if(temp != u_min) EEPROM.put(0, u_min);
    }
    if(menu == 3) {
      unsigned int temp=0;
      EEPROM.get(2, temp);

      if(temp != u_max) EEPROM.put(2, u_max);
    }
    
    menu++;
    if(menu > 3) menu = 0;
  }

  switch(menu) {
    case 0:
      if(digitalRead(RE1) == LOW || uartBtn == '+') {
        lastkeyact = true;
        lastkey = millis();
        if(running) {
          fetOff();
          running = false;
        }else{
          fetOn();
          uov=0;
          running = true;
        }
      }
      if(digitalRead(RE2) == LOW || uartBtn == '-') {
        lastkeyact = true;
        lastkey = millis();
        runtime = 0;
        mAh=0;
        mWh=0;
        mAh_c=0;
        mWh_c=0;
        uov=0;
      }
      break;
    case 1:
      break;
    case 2:
      if(digitalRead(RE1) == LOW || uartBtn == '+') {
        lastkeyact = true;
        lastkey = millis();
        u_min++;
      }
      if(digitalRead(RE2) == LOW || uartBtn == '-') {
        lastkeyact = true;
        lastkey = millis();
        u_min--;
      }
      break; 
    case 3:
      if(digitalRead(RE1) == LOW || uartBtn == '+') {
        lastkeyact = true;
        lastkey = millis();
        u_max++;
      }
      if(digitalRead(RE2) == LOW || uartBtn == '-') {
        lastkeyact = true;
        lastkey = millis();
        u_max--;
      }
      break;
  }
  uartBtn = 0x00;
}

unsigned int procVolt(void) {
  unsigned int adco = (float)adc_u * ref_u / 10000;

  if(adco > 620) { //use secondary ADC
    adco = (float)adc_OV * ref_OV / 1000;
  }

  adc_u=0;
  adc_OV=0; 
  
  return adco;
}

signed int procAmp(void) {
  signed int adco=0;

  
  
  adco = (((signed long)adc_i * ref_i) - ((signed long)ref_i_o*10000)) * ((float)ref_i_mva/100000);
  /*adco = adco - ref_i_o;
  adco *= ref_i_mva;
  adco /= 10;*/

  adc_i = 0;
  
  if(!output) return 0;
  return adco;
}

void procmAh(unsigned int timedelta) {
  boolean neg = false;
  signed int tamp = amp;

  if(tamp < 0) {
    tamp *= -1;
    neg = true;
    mAh_c = 0;
  }
  
  signed long temp = mAh_c;

  temp += (signed long)(tamp * 10 * timedelta) / 36;

  if(neg) {
    mAh -= (temp/100);
  }else{
    mAh += (temp/100);
    mAh_c = temp % 100;
  }
}

void procmWh(unsigned int timedelta) {
  signed long temp = mWh_c;
  temp += (((signed long)amp * timedelta * volt) / 3600);
  mWh_c = temp % 10;
  mWh += (temp/10);
}

void procDP(void) {
  DP = (float)adc_DP * ref_DP / 10000;
  adc_DP = 0;
}

void procDN(void) {
  DN = (float)adc_DN * ref_DN / 10000;
  adc_DN = 0;
}

void procSignaling(void) {
  if(DP > 180 && DP < 220 && DN > 180 && DN < 220) {
    signaling = 2; //Apple 0.5A
  }else if(DP > 180 && DP < 220 && DN > 260 && DN < 300) {
    signaling = 3; //Apple 1.0A
  }else if(DP > 260 && DP < 300 && DN > 180 && DN < 220) {
    signaling = 4; //Apple 2.0A
  }else{ //Check if pins are shorted for DCP
    digitalWrite(ADC_DP, LOW);
    pinMode(ADC_DP, OUTPUT);
    delay(2);
    if(analogRead(ADC_DN) > 16) {
      signaling = 1; //Nope - looks like something else is pulling it HIGH, propably data
    } else{
      pinMode(ADC_DP, INPUT_PULLUP);
      delay(2);
      if(analogRead(ADC_DN) > 16) {
        signaling = 0; //Nothing changed - propably not connected
      } else{
        signaling = 5; //Looks like DCP
      }
    }
    digitalWrite(ADC_DP, LOW);
    pinMode(ADC_DP, INPUT);
  }
}

void drawVolt(void) {
  String out;

  unsigned int adc = volt;

  if(uov==0) {
    if(adc < u_min) {
      Serial.println("UV");
      uov=1;
      fetOff();
      running = false;
    }
    if(adc > u_max) {
      Serial.println("OV");
      uov=2;
      fetOff();
      running = false;
    }
  }

  out =  "U: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,25,outc);
}

void drawAmp(void) {
  String out;

  signed int adc = amp;

  out =  "I: ";
  if(adc < 0) {
    out += '-';
    adc *= -1;
  }
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "A";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(64,25,outc);
}

void drawWatt() {
  String out;

  signed int adc = (signed long)((signed long)volt * (signed long)amp) / 100;

  out =  "P: ";
  
  if(adc < 0) {
    out += '-';
    adc *= -1;
  }
  
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "W";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,37,outc);
}

void drawAh(void) {
  String out = "";

  signed int adc = mAh;

  if(adc<0) {
    out += '-';
    adc*=-1;
  }

  out += adc/1000;
  out += '.';
  adc %= 1000;
  if(adc < 100) out += '0';
  if(adc < 10) out += '0';
  out += adc;
  out += "Ah";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,50,outc);
}

void drawWh(void) {
  String out = "";

  signed int adc = mWh;

  if(adc<0) {
    out += '-';
    adc*=-1;
  }

  out += adc/1000;
  out += '.';
  adc %= 1000;
  if(adc < 100) out += '0';
  if(adc < 10) out += '0';
  out += adc;
  out += "Wh";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(64,50,outc);
}

void drawTime(unsigned int calctime) {
  String out;

  int days = calctime / 86400;
  calctime %= 86400;

  int hours = calctime / 3600;
  calctime %= 3600;

  int minutes = calctime / 60;
  calctime %= 60;

  out  = days;
  out += "d ";
  out += hours;
  out += "h ";
  out += minutes;
  out += "m ";
  out += calctime;
  out += "s";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,63,outc);
}

void drawDP(void) {
  String out;

  unsigned int adc = DP;

  out =  "D+: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,25,outc);
}

void drawDN(void) {
  String out;

  unsigned int adc = DN;

  out =  "D-: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(64,25,outc);
}

void drawSignaling(void) {
  String out;

  switch(signaling) {
    case 0:
      out = "Not connected";
      break;
    case 1:
      out = "Data Connection";
      break;
    case 2:
      out = "Apple 0.5A";
      break;
    case 3:
      out = "Apple 1.0A";
      break;
    case 4:
      out = "Apple 2.0A";
      break;
    case 5:
      out = "USB Charger";
      break;
    default:
      out = "Unknown";
  }

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,37,outc);
}

void drawVCC(void) {
  String out;

  unsigned int adc = lastVcc;

  out =  "Vcc: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,63,outc);
}

void drawUMin(void) {
  String out;

  unsigned int adc = u_min;

  out =  "Umin: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,50,outc);
}

void drawUMax(void) {
  String out;

  unsigned int adc = u_max;

  out =  "Umax: ";
  out += adc/100;
  out += '.';
  adc %= 100;
  if(adc < 10) out += '0';
  out += adc;
  out += "V";

  char outc[out.length()+1];
  out.toCharArray(outc, sizeof(outc));

  u8g2.drawStr(0,50,outc);
}

void fetOff(void) {
  output = false;
  digitalWrite(FET, LOW);
}

void fetOn(void) {
  output = true;
  digitalWrite(FET, HIGH);
}


