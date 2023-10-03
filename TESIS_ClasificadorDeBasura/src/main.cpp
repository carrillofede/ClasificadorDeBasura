#include <Arduino.h>


struct Button {
	const uint8_t PIN;
	uint32_t numberKeyPresses;
	bool pressed;
};

Button button1 = {12, 0, false};  //Metal
Button button2 = {13, 0, false};  //Vidrio
Button button3 = {14, 0, false};  //Carton
Button button4 = {4, 0, false};   //Plastico

volatile int metal = 0;
volatile int vidrio = 0;
volatile int carton = 0;
volatile int plastico = 0;

const int timeThreshold1 = 150;
const int timeThreshold2 = 150;
const int timeThreshold3 = 150;
const int timeThreshold4 = 150;
long startTime = 0;


//Variables PWM ServoMotores
const int freq = 50;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int resolution = 13;
const int ledPin1 = 16;
const int ledPin2 = 17;
volatile int dutyCycle1 = 0; // (20%=1024  ;  17,5% = 819  ; 15% = 614 ; 12,5% = 410; 10% = 205)
volatile int dutyCycle2 = 0;


//Variables PWM Motor


//Interrupciones de los sensores
void isrSensor1() 
{
  if (millis() - startTime > timeThreshold1)
  {
    button1.numberKeyPresses++;
	  button1.pressed = true;
    metal = 1;
    startTime = millis();
  }
}
void isrSensor2()
{
  if (millis() - startTime > timeThreshold2)
  {
    button2.numberKeyPresses++;
	  button2.pressed = true;
    vidrio = 1;
    startTime = millis();
  }
}
void isrSensor3()
{
  if (millis() - startTime > timeThreshold3)
  {
    button3.numberKeyPresses++;
	  button3.pressed = true;
    carton = 1;
    startTime = millis();
  }
}
void isrSensor4()
{
  if (millis() - startTime > timeThreshold4)
  {
    button4.numberKeyPresses++;
    button4.pressed = true;
    plastico = 1;
    startTime = millis();
  }
}

void setup() {
  //Serial
	Serial.begin(115200);

  //Interrupciones gpio
	pinMode(button1.PIN, INPUT);
	attachInterrupt(button1.PIN, isrSensor1, RISING);
  pinMode(button2.PIN, INPUT);
	attachInterrupt(button2.PIN, isrSensor2, RISING);
  pinMode(button3.PIN, INPUT);
	attachInterrupt(button3.PIN, isrSensor3, RISING);
  pinMode(button4.PIN, INPUT);
	attachInterrupt(button4.PIN, isrSensor4, RISING);

  // configure LED PWM functionalitites
  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(ledPin1, ledChannel1);
  ledcWrite(ledChannel1, dutyCycle1);
  ledcSetup(ledChannel2, freq, resolution);
  ledcAttachPin(ledPin2, ledChannel2);
  ledcWrite(ledChannel2, dutyCycle2);

}

void loop() {

  if (button4.pressed) {
    Serial.printf("Calculando material:.........\n");
    if (metal == 1)
    {
      Serial.printf("Paso un metal\n");
      dutyCycle1 = 205;
      dutyCycle2 = 1024;
    }
    else if (vidrio == 1)
    {
      Serial.printf("Paso un vidrio\n");
      dutyCycle1 = 410;
      dutyCycle2 = 819;
    }
    else if (carton == 1)
    {
      Serial.printf("Paso un carton\n");
      dutyCycle1 = 614;
      dutyCycle2 = 614;
    }
    else if (plastico == 1)
    {
      Serial.printf("Paso un plastico\n");
      dutyCycle1 = 819;
      dutyCycle2 = 410;
    }
    metal = 0;
    vidrio = 0;
    carton = 0;
    plastico = 0;
    ledcWrite(ledChannel1, dutyCycle1);
    ledcWrite(ledChannel2, dutyCycle2);
    button4.pressed = false;
	}
}