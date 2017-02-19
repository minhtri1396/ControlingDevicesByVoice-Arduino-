#include <SoftwareSerial.h>
SoftwareSerial BTserial(2, 3); // TX | RX

int device1 = 8;
int device2 = 12;
int device3 = 13;

void setup(void){
  /** initialize */
  Serial.begin(9600);
  BTserial.begin(38400); // bluetooth uses baund 38400
  pinMode(device1, OUTPUT);
  pinMode(device2, OUTPUT);
  pinMode(device3, OUTPUT);
}

void loop(void){
  if(BTserial.available() > 0){
    char device = BTserial.read(); // read information from bluetooth
    Serial.print(device);
    
    if('1' == device) digitalWrite(device1, !digitalRead(device1)); // gate for device 1
    else if('2' == device) digitalWrite(device2, !digitalRead(device2)); // gate for device 2
    else if('3' == device) digitalWrite(device3, !digitalRead(device3)); // gate for device 3
  }
}
