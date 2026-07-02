/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Addition to original blink sketch also turns on and off camera 1's tally indicator.

  Most Arduinos have an on-board LED you can control. On the Uno and
  Leonardo, it is attached to digital pin 13. If you're unsure what
  pin the on-board LED is connected to on your Arduino model, check
  the documentation at http://www.arduino.cc

  This example code is in the public domain.
*/

#include <BMDSDIControl.h>                                // need to include the library

BMD_SDITallyControl_Serial sdiTallyControl;
BMD_SDICameraControl_Serial sdiCameraControl;

// the setup function runs once when you press reset or power the board
void setup()
{
  delay(2500);
  Serial.begin(115200);
  Serial.println("Blackmagic Design SDI Control Shield");

  sdiTallyControl.begin();
  sdiTallyControl.setOverride(true);
  sdiCameraControl.begin();
  sdiCameraControl.setOverride(false);  // pass-through: read incoming camera control from SDI

  pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(13, OUTPUT);                                     // initialize digital pin 13 as an output
}

// the loop function runs over and over again forever
void loop()
{
  static bool ledState = false;
  byte buffer[256];       // Shield ICDATA max is 255 bytes
  int bytesRead;

  ledState = !ledState;
  digitalWrite(LED_BUILTIN, !ledState);                        // turn the LED ON

  sdiTallyControl.setCameraTally(                          // turn tally ON
    1,                                                     // Camera Number
    ledState,                                              // Program Tally
    false                                                  // Preview Tally
  );

  delay(100);                                             // leave it ON for 1 second

  if (sdiCameraControl.available()) {
    bytesRead = sdiCameraControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      Serial.print("CAMCTRL [");
      Serial.print(bytesRead);
      Serial.print(" bytes]: ");
      printHex(buffer, bytesRead);
    } else if (bytesRead < 0) {
      Serial.println("CAMCTRL buffer too small, flushing");
      sdiCameraControl.flushRead();
    } else {
      Serial.println("CAMCTRL empty packet");
    }
  }
}


void printHex(byte* data, int len) {
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}