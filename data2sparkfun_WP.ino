/*****************************************************************
based on: Phant_Ethernet.ino
Post data to SparkFun's data stream server system (phant) using
an Arduino and an Ethernet Shield.
Jim Lindblom @ SparkFun Electronics
Original Creation Date: July 3, 2014

This sketch uses an Arduino Uno to POST sensor readings to
SparkFun's data logging streams (http://data.sparkfun.com). A post
will be initiated whenever pin 3 is connected to ground.

Before uploading this sketch, there are a number of global vars
that need adjusting:
1. Ethernet Stuff: Fill in your desired MAC and a static IP, even
   if you're planning on having DCHP fill your IP in for you.
   The static IP is only used as a fallback, if DHCP doesn't work.
2. Phant Stuff: Fill in your data stream's public, private, and
data keys before uploading!

Hardware Hookup:
  * These components are connected to the Arduino's I/O pins:
    * D3 - Active-low momentary button (pulled high internally)
    * A0 - Photoresistor (which is combined with a 10k resistor
           to form a voltage divider output to the Arduino).
    * D5 - SPST switch to select either 5V or 0V to this pin.
  * A CC3000 Shield sitting comfortable on top of your Arduino.

Development environment specifics:
    IDE: Arduino 1.0.5
    Hardware Platform: RedBoard & PoEthernet Shield

This code is beerware; if you see me (or any other SparkFun
employee) at the local, and you've found our code helpful, please
buy us a round!

Much of this code is largely based on David Mellis' WebClient
example in the Ethernet library.

Distributed as-is; no warranty is given.
*****************************************************************/
#include <SPI.h> // Required to use Ethernet
#include <Ethernet.h> // The Ethernet library includes the client
#include <avr/pgmspace.h> // Allows us to sacrifice flash for DRAM

#ifndef READVCC_CALIBRATION_CONST
#define READVCC_CALIBRATION_CONST 1117799L  // 1120559L   1117799L
#endif

///////////////////////
// Ethernet Settings //
///////////////////////
// Enter a MAC address for your controller below.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE8 };
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(54,86,132,254);  // numeric IP for data.sparkfun.com
char server[] = "data.sparkfun.com";    // name address for data.sparkFun (using DNS)
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 2, 50);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

/////////////////
// Phant Stuff //
/////////////////
const String publicKey = "DJWNywlO70cAyYAVwXdm";
const String privateKey = "<fill in your private key>";
const byte NUM_FIELDS = 2;
const String fieldNames[NUM_FIELDS] = {"ubatt", "ucc"};
String fieldData[NUM_FIELDS];

//////////////////////
// Input Pins, Misc //
//////////////////////
long VCC;
#define CYCLE 60000    // every x Milliseconds

void setup()
{
  Serial.begin(115200);
  Serial.print("VCC =");
  VCC = readVcc();
  Serial.println(VCC);
  // Setup Input Pins:
  // Set Up Ethernet:
  setupEthernet();

  Serial.println(F("=========== Ready to Stream ==========="));
}

void loop()
{
  unsigned long lasttime = millis();
  // Gather data:
  VCC = readVcc();
  fieldData[0] = String(map(analogRead(A0),0,1023,0,VCC));
  fieldData[1] = String(VCC);
  Serial.println("Posting!");
  postData(); // the postData() function does all the work,
  // check it out below.
  while (millis() - lasttime < CYCLE) {}  // wait, do nothing
}

void postData() {
  // Make a TCP connection to remote host
  if (client.connect(server, 80)) {
    String message = "GET /input/";    // make string
    message += publicKey;
    message += "?private_key=";
    message += privateKey;
    for (int i = 0; i < NUM_FIELDS; i++) {
      message += "&";
      message += fieldNames[i];
      message += "=";
      message += fieldData[i];
    }
    message += " HTTP/1.1\n";
    message += "Host: ";
    message += server;
    message += "Connection: close\n\n";

    Serial.println(F("=========== send Message ==========="));
    Serial.println(message);
    client.print(message);
    Serial.println(F("=========== End of Message ==========="));
  }
  else {
    Serial.println(F("Connection failed"));
  }

  // Check for a response from the server, and route it
  // out the serial port.
  while (client.connected()) {
    if ( client.available() )
    {
      char c = client.read();
      Serial.print(c);
    }
  }
  Serial.println();
  client.stop();
}

void setupEthernet() {
  Serial.println("Setting up Ethernet...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

long readVcc() {
  long result;

  //not used on emonTx V3 - as Vcc is always 3.3V - eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB1286__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADCSRB &= ~_BV(MUX5);   // Without this the function always returns -1 on the ATmega2560 http://openenergymonitor.org/emon/node/2253#comment-11432
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);

#endif


#if defined(__AVR__)
  delay(2);                                        // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                             // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = READVCC_CALIBRATION_CONST / result;  //1100mV*1024 ADC steps http://openenergymonitor.org/emon/node/1186
  return result;
#elif defined(__arm__)
  return (3300);                                  //Arduino Due
#else
  return (3300);                                  //Guess that other un-supported architectures will be running a 3.3V!
#endif
}


