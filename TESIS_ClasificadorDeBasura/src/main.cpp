#include <Arduino.h>


struct Button 
{
	const uint8_t PIN;
	uint32_t numberKeyPresses;
	bool pressed;
};

Button button1 = {12, 0, false};  //Metal
Button button2 = {13, 0, false};  //Vidrio
Button button3 = {14, 0, false};  //Carton
Button button4 = {4, 0, false};   //Plastico

Button button5 = {27, 0, false};   //Inicio del motor

volatile int metal = 0;
volatile int vidrio = 0;
volatile int carton = 0;
volatile int plastico = 0;

const int timeThreshold1 = 150;
const int timeThreshold2 = 150;
const int timeThreshold3 = 150;
const int timeThreshold4 = 150;
const int timeThreshold5 = 150;
long startTime = 0;
long startTimeMotor= 0;

//Variables PWM ServoMotores
const int freq = 50;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int resolution = 13;
const int ledPin1 = 16;
const int ledPin2 = 17;
volatile int dutyCycle1 = 0; // (20%=1623  ;  17,5% = 1420  ; 15% = 1218 ; 12,5% = 1015; 10% = 812)
volatile int dutyCycle2 = 0;


//Variables PWM Motor
const int freqMotor = 4000;
const int ledChannelMotor = 2;
const int resolutionMotor = 13;
const int ledPinMotor = 18;
volatile int dutyCycleMotor = 0;


//Variables del timer 3 para la clasificacion
volatile int interruptCounter;
int totalInterruptCounter;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//Variables del timer 4 para la clasificacion
volatile int interruptCounter4;
int totalInterruptCounter4;
hw_timer_t * timer4 = NULL;
portMUX_TYPE timerMux4 = portMUX_INITIALIZER_UNLOCKED;

void inicio()
{
  for(int dutyMotor = 4095; dutyMotor <= 8191; dutyMotor+=2)
  {   
    ledcWrite(ledChannelMotor, dutyMotor);
    delay(1);
  }
  Serial.printf("PASO por inicio\n");
  // dutyCycleMotor = 4095;
  // ledcWrite(ledChannelMotor, dutyCycleMotor);
}

void detencion()
{
  for(int dutyMotor = 8191; dutyMotor >= 4095; dutyMotor-=2)
  {
    ledcWrite(ledChannelMotor, dutyMotor);
    delay(1);  
  }
  dutyCycleMotor = 0;
  ledcWrite(ledChannelMotor, dutyCycleMotor);
}

//Interrupciones de los sensores
void ARDUINO_ISR_ATTR isrSensor1() 
{
  button1.numberKeyPresses++;
  button1.pressed = true;
  metal = 1;
}
void ARDUINO_ISR_ATTR isrSensor2()
{
  button2.numberKeyPresses++;
	button2.pressed = true;
  vidrio = 1;
}
void ARDUINO_ISR_ATTR isrSensor3()
{
  button3.numberKeyPresses++;
	button3.pressed = true;
  carton = 1;
}
void ARDUINO_ISR_ATTR isrSensor4()
{
  if (millis() - startTime > timeThreshold4)
  {
    button4.numberKeyPresses++;
    button4.pressed = true;
    plastico = 1; 
    startTime = millis();
  }
}

//Interrupcion para arranque de la cinta
void ARDUINO_ISR_ATTR isrSensor5()
{
  if (millis() - startTimeMotor > timeThreshold5)
  {
    button5.pressed = true;
    //inicio();
    startTimeMotor = millis();
  }
}

//Interrupcion del Timer 3 de clasificacion
void IRAM_ATTR onTimer() 
{
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

//Interrupcion del Timer 4 de detencion
void IRAM_ATTR onTimer4() 
{
  portENTER_CRITICAL_ISR(&timerMux4);
  interruptCounter4++;
  portEXIT_CRITICAL_ISR(&timerMux4);
}

void setup() {
  //Serial
	Serial.begin(115200);
  Serial.println("Inicializando...");

  //Interrupciones gpio de los sensores
	pinMode(button1.PIN, INPUT_PULLDOWN);
	attachInterrupt(button1.PIN, isrSensor1, RISING);
  pinMode(button2.PIN, INPUT_PULLDOWN);
	attachInterrupt(button2.PIN, isrSensor2, RISING);
  pinMode(button3.PIN, INPUT_PULLDOWN);
	attachInterrupt(button3.PIN, isrSensor3, RISING);
  pinMode(button4.PIN, INPUT_PULLDOWN);
	attachInterrupt(button4.PIN, isrSensor4, RISING);

  // configure LED PWM para servos
  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(ledPin1, ledChannel1);
  ledcWrite(ledChannel1, dutyCycle1);
  ledcSetup(ledChannel2, freq, resolution);
  ledcAttachPin(ledPin2, ledChannel2);
  ledcWrite(ledChannel2, dutyCycle2);

  // configure LED PWM para motor
  pinMode(button5.PIN, INPUT_PULLDOWN);
	attachInterrupt(button5.PIN, isrSensor5, RISING);

  ledcSetup(ledChannelMotor, freqMotor, resolutionMotor);
  ledcAttachPin(ledPinMotor, ledChannelMotor);
  ledcWrite(ledChannelMotor, dutyCycleMotor);

  // Timer 3 configuracion
  timer = timerBegin(2, 80, true); //Utilizamos el timer 2, con un prescaler de 80 y que cuente hacia arriba
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 2000000, false);
  timerAlarmDisable(timer);
  timerRestart(timer);
  timerStop(timer);

  // Timer 4 configuracion
  timer4 = timerBegin(3, 80, true); //Utilizamos el timer 2, con un prescaler de 80 y que cuente hacia arriba
  timerAttachInterrupt(timer4, &onTimer4, true);
  timerAlarmWrite(timer4, 4000000, false);
  timerAlarmDisable(timer4);
  timerRestart(timer4);
  timerStop(timer4);
}

void loop() {

  if (button4.pressed) 
  {
    timerAlarmEnable(timer);
    timerStart(timer);
    timerAlarmEnable(timer4);
    timerStart(timer4);
    button4.pressed = false;
	}

  if (button5.pressed) 
    {
      inicio();
      button5.pressed = false;
    }

  if (interruptCounter > 0) 
  {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    Serial.printf("Calculando material:.........\n");
    if (metal == 1)
    {
      Serial.printf("Paso un metal\n");
      dutyCycle1 = 812; // (20%=1623  ;  17,5% = 1420  ; 15% = 1218 ; 12,5% = 1015; 10% = 812)
      dutyCycle2 = 1623;
    }
    else if (vidrio == 1)
    {
      Serial.printf("Paso un vidrio\n");
      dutyCycle1 = 1015;
      dutyCycle2 = 1420;
    }
    else if (carton == 1)
    {
      Serial.printf("Paso un carton\n");
      dutyCycle1 = 1218;
      dutyCycle2 = 1218;
    }
    else if (plastico == 1)
    {
      Serial.printf("Paso un plastico\n");
      dutyCycle1 = 1420;
      dutyCycle2 = 1015;
    }
    metal = 0;
    vidrio = 0;
    carton = 0;
    plastico = 0;
    ledcWrite(ledChannel1, dutyCycle1);
    ledcWrite(ledChannel2, dutyCycle2);
    timerAlarmDisable(timer);
    timerRestart(timer);
    timerStop(timer);
  }

  if (interruptCounter4 > 0) 
  {
    portENTER_CRITICAL(&timerMux4);
    interruptCounter4--;
    portEXIT_CRITICAL(&timerMux4);
    Serial.printf("Se detuvo la cinta \n");    
    detencion();
    timerAlarmDisable(timer4);
    timerRestart(timer4);
    timerStop(timer4);
  }
}
