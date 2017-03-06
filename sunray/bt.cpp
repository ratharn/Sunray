/*
  Ardumower (www.ardumower.de)
  Copyright (c) 2013-2014 by Alexander Grau
  Copyright (c) 2013-2014 by Sven Gennat
  
  Private-use only! (you need to ask for a commercial-use)
 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  Private-use only! (you need to ask for a commercial-use)
*/

#include "bt.h"
#include "config.h"

enum {
  BT_UNKNOWN,
  BT_LINVOR_HC06, // Linvor / HC06
  BT_HC05,        // HC05                 
  BT_FBT06_MBTV4, // ModiaTek FBT06/MBTV4 
};


void BluetoothConfig::setConfigs(byte *config){
  for (int i=0; i < 24; i++){
    btTestConfig[i] = config[i];
  }
}

BluetoothConfig::BluetoothConfig(){
  btType = BT_UNKNOWN;
  btRate = 9600;    
#ifdef __AVR__
  byte configs[24] =  { SERIAL_8N1, SERIAL_5N1, SERIAL_6N1, SERIAL_7N1, 
                    SERIAL_5N2, SERIAL_6N2, SERIAL_7N2, SERIAL_8N2,
                    SERIAL_5E1, SERIAL_6E1, SERIAL_7E1, SERIAL_8E1,  
                    SERIAL_5E2, SERIAL_6E2, SERIAL_7E2, SERIAL_8E2,
                    SERIAL_5O1, SERIAL_6O1, SERIAL_7O1, SERIAL_8O1,
                    SERIAL_5O2, SERIAL_6O2, SERIAL_7O2, SERIAL_8O2 };               
  setConfigs(configs);
  btConfig = SERIAL_8N1;                                                
#else
  btTestConfig[1] = { 0 }; 
  btConfig = 0;  
#endif                            
}

void BluetoothConfig::writeBT(String s){
  DEBUG("send: "+s);
  BLUETOOTH.print(s);
}

void BluetoothConfig::readBT(){
  btResult = "";
  if (BLUETOOTH.available()){
    DEBUG(F("  received: "));
    while (BLUETOOTH.available()){
      btData=BLUETOOTH.read();
      btResult += char(btData);
      DEBUG(btData);
    }    
  }  
  //DEBUG("btResult=");
  //DEBUGLN(btResult);
}

void BluetoothConfig::writeReadBT(String s){
  writeBT(s);
  delay(2000);
  readBT();    
  int counter = 0;
  while ((btResult.startsWith(F("ERROR")) && counter < 4)){
    // retry 
    writeBT(s);
    delay(2000);    
    readBT();
    counter++;
  }        
  DEBUGLN();
}


void BluetoothConfig::setName(String name){
  boolean res = false;
  DEBUGLN();
  DEBUG(F("setting name "));
  DEBUG(name);
  DEBUGLN("...");
  switch (btType){
    case BT_LINVOR_HC06:
      writeReadBT("AT+NAME"+name);     
      res = btResult.startsWith(F("OKsetname"));
      break;
    case BT_HC05:
      writeReadBT("AT+NAME="+name+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);      
      break;
    case BT_FBT06_MBTV4:
      writeReadBT("AT+NAME"+name+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);
      break;  
  }    
  if (res) DEBUGLN(F("=>success"));
    else DEBUGLN(F("=>error setting name"));
}


void BluetoothConfig::setPin(int pin){
  boolean res = false;
  DEBUGLN();
  DEBUG(F("setting pin "));
  DEBUG(pin);
  DEBUGLN(F("..."));
  switch (btType){
    case BT_LINVOR_HC06:
      writeReadBT("AT+PIN"+String(pin));     
      res = (btResult.startsWith("OKsetPIN"));
      break;
    case BT_HC05:
      writeReadBT("AT+PSWD="+String(pin)+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);      
      break;
    case BT_FBT06_MBTV4:
      writeReadBT("AT+PIN"+String(pin)+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);      
      break;
  }  
  if (res) DEBUGLN(F("=>success"));
    else DEBUGLN(F("=>error setting pin"));
}


void BluetoothConfig::setBaudrate(long rate){    
  DEBUGLN();
  DEBUG(F("setting baudrate "));
  DEBUG(rate);
  DEBUGLN(F("..."));
  byte n=4;
  boolean res = false;
  switch (btType){
    case BT_LINVOR_HC06:
      switch (rate){
        case 1200: n=1; break;
        case 2400: n=2; break;
        case 4800: n=3; break;
        case 9600: n=4; break;
        case 19200: n=5; break;
        case 38400: n=6; break;
        case 57600: n=7; break;
        case 115200: n=8; break;
      }      
      writeReadBT(F("AT+PN")); // no parity
      writeReadBT("AT+BAUD"+String(n));     
      res = (btResult.startsWith("OK"+String(rate)));
      break;
    case BT_HC05:
      writeReadBT("AT+UART="+String(rate)+",0,0"+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);          
      break;
    case BT_FBT06_MBTV4:
      switch (rate){
        case 1200: n=1; break;
        case 2400: n=2; break;
        case 4800: n=3; break;
        case 9600: n=4; break;
        case 19200: n=5; break;
        case 38400: n=6; break;
        case 57600: n=7; break;
        case 115200: n=8; break;
      }          
      writeReadBT("AT+BAUD"+String(n)+"\r\n");     
      res = (btResult.indexOf(F("OK")) != -1);
      break;
  }  
  if (res){
    btRate = rate;
    DEBUGLN(F("=>success"));
  } else {
    DEBUGLN(F("=>error setting baudrate"));
  } 
}  

boolean BluetoothConfig::detectBaudrate(boolean quickBaudScan){
  DEBUGLN();
  DEBUGLN(F("detecting baudrate..."));
  for (int i=0; i < 8; i++){    
    switch(i){
      case 0: btRate = 9600; break;
      case 1: btRate = 38400; break;      
      case 2: btRate = 19200; break;      
      case 3: btRate = 57600; break;            
      case 4: btRate = 115200; break;                  
      case 5: btRate = 4800; break;
      case 6: btRate = 2400; break;      
      case 7: btRate = 1200; break;            
    }      
    for (int j=0; j < sizeof btTestConfig; j++){
      btConfig = btTestConfig[j];
      DEBUG(F("trying baudrate "));
      DEBUG(btRate);
      DEBUG(F(" config "));
      DEBUG(j);
      DEBUGLN(F("..."));
      #ifdef __AVR__
        BLUETOOTH.begin(btRate, btConfig);
      #else
        BLUETOOTH.begin(btRate);      
      #endif
      writeReadBT(F("AT"));  // linvor/HC06 does not want a terminator!    
      if (btResult.startsWith(F("OK"))){
        DEBUGLN(F("=>success"));
        return true;
      }
      writeReadBT(F("AT\r\n"));  // HC05 wants a terminator!          
      if (btResult.startsWith(F("OK"))){
        DEBUGLN(F("=>success"));
        return true;
      }      
      if (quickBaudScan) break;      
    }
    //writeReadBT("ATI1\n"); // BTM info   
    //writeReadBT("ATZ0\n"); // BTM factory    
    //writeReadBT("ATC0\r\nATQ1\r\nATI1\r\n"); // BTM    
  }
  DEBUGLN(F("=>error detecting baudrate"));
  return false;
}

void BluetoothConfig::detectModuleType(){
  DEBUGLN();
  DEBUGLN(F("detecting BT type..."));
  writeReadBT(F("AT+VERSION"));  
  if (btResult.startsWith(F("OKlinvor"))){
    DEBUGLN(F("=>it's a linvor/HC06"));
    btType = BT_LINVOR_HC06;
    return;
  }     
  writeReadBT(F("AT+VERSION?\r\n"));  
  if (btResult.indexOf(F("OK")) != -1){
    DEBUGLN(F("=>must be a HC03/04/05 ?"));      
    btType = BT_HC05;
  }  
  writeReadBT(F("AT+VERSION\r\n"));
  if (btResult.indexOf(F("ModiaTek")) != -1){
    DEBUGLN(F("=>it's a FBT06/MBTV4"));
    btType = BT_FBT06_MBTV4;
  }
}

void BluetoothConfig::setParams(String name, int pin, long baudrate, boolean quickBaudScan) {
  //delay(2000);
  DEBUGLN(F("HC-03/04/05/06/linvor/ModiaTek Bluetooth config programmer"));
  DEBUGLN(F("NOTE for HC05: Connect KEY pin to 3.3V!"));
  DEBUGLN(F("NOTE for HC06/linvor: Do NOT pair/connect (LED must be blinking)"));
  DEBUGLN(F("NOTE for FBT06/MBTV4: First you have to solder the PIO11 pin to VCC (PIN 12) which is 3.3 Volts using a thin wire."));
 
  if (detectBaudrate(quickBaudScan)){
    detectModuleType();
    if (btType != BT_UNKNOWN){      
      setName(name);
      setPin(pin);
      setBaudrate(baudrate);
      DEBUGLN(F("You may restart BT module now!"));  
    }
  }
}



