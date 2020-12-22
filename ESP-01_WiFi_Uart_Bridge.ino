/*******************************************************************************
 *
 *  ESP-01 WiFi Uart Bridge test program
 *   (ESP-01 WiFi to Uart transport program)
 * 
 *******************************************************************************
 */

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <vector>

// definition
#define BITRATE   115200     // serial port bit rate
#define SCLPIN        0     // SCL pin
#define SDAPIN        2     // SDA pin
//
#define DSPIPIN       1     // ssid & password input
#define DSPSETP       2     // set wifi parameters
#define DSPCONT       3     // connected wifi
#define DSPWERR       4     // wifi connection error
//
#define RXWTPRD       5     // recieve from firmata period (mS)

// network parameters
const int networkport = 23;

// ssid & password manual setting (for manual setting)
// SSID and Password can set on the serial monitor if next items are blank
const char* fssid = "HITRON-0E30";      // Can set SSID on this item also
const char* fpassword = "1Banano1";  // Can set Password on this item also

// network instance
WiFiServer localServer(networkport);
WiFiClient localClient;

// set ssid & password
void setssid(char *ssid, char *pass, int maxlen){
  // variables
  int wpt;
  size_t len;
  
  // initial set
  ssid[0] = 0; pass[0] = 0;

  // check ssid and pasword setting
  Serial.println("\r\n\r\n\r\nSet SSID ? Press \"y\" to enter setting mode.");
  for(int i = 0 ; i < 100 ; i++){  // wait for key entering 5 sec.
    delay(50);
    yield();
    // check ssid setting mode
    if(Serial.available()){
      delay(10);
      len = Serial.available();
      uint8_t lbuf[len];
      Serial.readBytes(lbuf, len); // flush read buffer
      if((lbuf[0] != 'y') && (lbuf[0] != 'Y')) break;
      // input ssid 
      Serial.println(" Input SSID");
      while(!Serial.available()) delay(10);  // wait for ssid entering
      delay(10);
      len = Serial.available();
      if(len >= maxlen){          // check input length
       linput:
        uint8_t mbuf[len];
        Serial.readBytes(mbuf, len); // flush read buffer
        Serial.println(" Tool long. Can't set and stop here.");
        while(1);  // wait for WDT time out
      }
      // set ssid
      wpt = 0;
      while(Serial.available()){
        char ch = Serial.read();
        if(ch >= ' ') ssid[wpt++] = ch;
      }
      if(ssid[0] <= ' '){  // check alpha numeric
        ssid[0] = 0;
        break;
      }
      Serial.print("   SSID = ");
      Serial.println(ssid);
      // input password 
      Serial.println(" Input Password");
      while(!Serial.available()) delay(10);  // wait for password entering
      delay(10);
      len = Serial.available();
      if(len >= maxlen) goto linput;  // check input length
      // set password
      wpt = 0;
      while(Serial.available()){
        char ch = Serial.read();
        if(ch >= ' ') pass[wpt++] = ch;
      }
      if(pass[0] < ' '){  // check alpah numeric
        pass[0] = 0;
        break;
      }
      if(strlen(pass) > 0){
        Serial.print("   Password = ");
        Serial.println(pass);
      } else {
        Serial.print("   *** Password not set ***");
        pass[0] = 0;
      }
      break ;
    }
  }
  
  // end of process
  return;
}


// initial set
void setup() {
  // variables
  char sbuf[2][32] = {0,0};

  // wifi 1st contact
  WiFi.mode(WIFI_STA);
  if(fssid[0] > ' '){
   WiFi.begin(fssid, fpassword);
   delay(1000);
  } else WiFi.begin();
  delay(100);

  // setting ssid and password
  Wire.pins(SDAPIN, SCLPIN); // SDA, SCL
  Wire.begin();  
  

  // check ssid & passwrod setting
  Serial.begin(BITRATE);
  if(fssid[0] <= ' '){
    setssid(sbuf[0], sbuf[1], sizeof(sbuf)/2);
    if(sbuf[0][0]) WiFi.disconnect();
  }

  // Start wifi
  if(sbuf[0][0]){
    WiFi.mode(WIFI_STA);
    if(!sbuf[1][0]) WiFi.begin(sbuf[0]);
    else WiFi.begin(sbuf[0], sbuf[1]);
  }
 
  // wait wifi connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50) delay(500);
  if(i == 51){
    Serial.print("Could not connect to "); Serial.println(WiFi.SSID());
  }
  // start server
  localServer.begin();
  localServer.setNoDelay(true);

  // output connected wifi information to the serial monitor 
  Serial.print("Ready! Use 'Uart-WiFi Bridge ");
  Serial.print(WiFi.localIP());
  Serial.println(" to connect");

  // reset firmata unit
  delay(10);
  Serial.write(0xFF);
}

// main loop
void loop() {

  // check if there are any new clients
  if (localServer.hasClient()){
    if (!localClient.connected()){
      if(localClient) localClient.stop();
      localClient = localServer.available();
    }
  }

  // check client data arrival
  if (localClient && localClient.connected()){
    if(localClient.available()){
      size_t len = localClient.available();
      uint8_t sbuf[len];
      localClient.readBytes(sbuf, len);
      Serial.write(sbuf, len);
    }
  }

  // check UART response arrival
  int rlen = 0; std::vector<char> rbuf;
  while(Serial.available()){
   reread:
      rbuf.push_back(Serial.read());
      rlen ++;
  }
  // re-check serial port arrival data
  for(int i = 0 ; i < RXWTPRD ; i++){  // check data arrival complete
    delay(1);
    if(Serial.available()) goto reread;
  }

  // tx get serial data to wifi
  if(rlen && localClient && localClient.connected())
    localClient.write((char *)rbuf.data(), rlen);

  // for WDT reset
  delay(0);
}
