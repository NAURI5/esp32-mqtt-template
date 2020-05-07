#include <Arduino.h>
#include "credentials.h"
#include "settings.h"
int i;

void setup() {
  // put your setup code here, to run once:
  i=0;
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1009);
  i++;
  Serial.println(i);
}