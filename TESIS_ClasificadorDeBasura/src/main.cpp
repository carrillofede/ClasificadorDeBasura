#include <Arduino.h>


/****************************************************************************************************************************************
*****************************************************************************************************************************************
*******************************************       VARIABLES A UTILIZAR      *************************************************************
*****************************************************************************************************************************************
****************************************************************************************************************************************/

//Creamos una estructura Button la cual vamos a usar para las entradas GPIO
struct Button 
{
	const uint8_t PIN;            //Numero de Pin
	uint32_t numberKeyPresses;    //Cantidad de veces que fue presionado
	bool pressed;                 //Presionado (True or False)
};

Button button1 = {12, 0, false};  //Sensor del Metal
Button button2 = {13, 0, false};  //Sensor del Vidrio
Button button3 = {14, 0, false};  //Sensor del Carton/Madera
Button button4 = {4, 0, false};   //Sensor del Plastico

Button button5 = {27, 0, false};   //Sensor infrarrojo que da inicio al motor

//Definimos las variables para realizar la clasificacion:
volatile int metal = 0;
volatile int vidrio = 0;
volatile int carton = 0;
volatile int plastico = 0;

//Tiempo para antirrebote de las entradas GPIO
const unsigned long timeThreshold1 = 150;
const unsigned long timeThreshold2 = 150;
const unsigned long timeThreshold3 = 150;
const unsigned long timeThreshold4 = 150;
const unsigned long timeThreshold5 = 150;
unsigned long startTime = 0;         //GPIO plastico
unsigned long startTimeMotor = 0;     //GPIO arranque

//Variables PWM ServoMotores
const int freq = 50;            //Frecuencia del PWM
const int ledChannel1 = 0;      //Canal del servo 1 = 0 = timer0
const int ledChannel2 = 1;      //Canal del servo 2 = 1 = timer0
const int resolution = 13;      //13 bits de resolucion = 8191
const int ledPin1 = 16;         //Pin del servo 1
const int ledPin2 = 17;         //Pin del servo 2
volatile int dutyCycle1 = 0;    //Duty del servo 1 (20%=1623  ;  17,5% = 1420  ; 15% = 1218 ; 12,5% = 1015; 10% = 812)
volatile int dutyCycle2 = 0;    //Duty del servo 2 (20%=1623  ;  17,5% = 1420  ; 15% = 1218 ; 12,5% = 1015; 10% = 812)


//Variables PWM Motor
const int freqMotor = 4000;       //Frecuencia del PWM del motor
const int ledChannelMotor = 2;    //Canal del Motor = 2 = timer1
const int resolutionMotor = 13;   //13 bits de resolucion = 8191
const int ledPinMotor = 18;       //Pin del PWM del motor
volatile int dutyCycleMotor = 0;  //Duty del motor


//Variables del timer 2 para la clasificacion
volatile int interruptCounter;          //Variable que me indica si interrumpio
int totalInterruptCounter;              //Total de interrupciones del timer
hw_timer_t * timer = NULL;              //Crea la estructura del timer
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//Variables del timer 3 para la clasificacion
volatile int interruptCounter3;         //Variable que me indica si interrumpio
int totalInterruptCounter3;             //Total de interrupciones del timer
hw_timer_t * timer3 = NULL;             //Crea la estructura del timer
portMUX_TYPE timerMux3 = portMUX_INITIALIZER_UNLOCKED;


/****************************************************************************************************************************************
*****************************************************************************************************************************************
******************************************************       FUNCIONES      *************************************************************
*****************************************************************************************************************************************
****************************************************************************************************************************************/

//Funcion que genera el arranque suave del Motor
void inicio()
{
  /*Genera un for con un delay que va actualizando el duty del PWM que controla el motor
  de esta manera generamos un arranque suave y progresivo del mismo.*/

  Serial.printf("Inicia la cinta\n");
  for(int dutyMotor = 4095; dutyMotor <= 8191; dutyMotor+=2)
  {   
    ledcWrite(ledChannelMotor, dutyMotor);
    delay(1);
  }
  Serial.printf("LLego a la velocidad maxima\n");
}

//Funcion que genera un detencion suave del Motor
void detencion()
{
  /*Genera un for con un delay que va actualizando el duty del PWM que controla el motor
  de esta manera generamos una detencion suave y progresiva del mismo. Cuando el motor
  esta detenido se cambia el duty por 0 para evitar ruidos de frecuencia en el motor.*/

  for(int dutyMotor = 8191; dutyMotor >= 4095; dutyMotor-=2)
  {
    ledcWrite(ledChannelMotor, dutyMotor);
    delay(1);  
  }
  dutyCycleMotor = 0;
  ledcWrite(ledChannelMotor, dutyCycleMotor);
}

/****************************************************************************************************************************************
*****************************************************************************************************************************************
**************************************************       INTERRUPCIONES     *************************************************************
*****************************************************************************************************************************************
****************************************************************************************************************************************/

//Interrupciones del sensor del metal
void ARDUINO_ISR_ATTR isrSensor1() 
{
  button1.numberKeyPresses++;       //Contador de activaciones
  button1.pressed = true;           //Boton presionado = True
  metal = 1;                        //Ponemos en uno la variable metal
}

//Interrupciones del sensor del vidrio
void ARDUINO_ISR_ATTR isrSensor2()
{
  button2.numberKeyPresses++;       //Contador de activaciones
	button2.pressed = true;           //Boton presionado = True
  vidrio = 1;                       //Ponemos en uno la variable vidrio
}

//Interrupciones del sensor de madera/carton
void ARDUINO_ISR_ATTR isrSensor3()
{
  button3.numberKeyPresses++;       //Contador de activaciones
	button3.pressed = true;           //Boton presionado = True
  carton = 1;                       //Ponemos en uno la variable madera/carton
}

//Interrupciones del sensor del plastico
void ARDUINO_ISR_ATTR isrSensor4()
{
  /*Este if sirve para hacer un antirebote define en starTime el tiempo al cual entro a la interrupcion
  Luego si se entra a la misma en un tiempo menor al definido en timeThreshold4 no se ejecuta nuevamente la interrupcion
  Esto sucede ya que millis tiene el valor real del tiempo y startTime el tiempo en el cual arranco*/

  if (millis() - startTime > timeThreshold4)
  {
    button4.numberKeyPresses++;       //Contador de activaciones
    button4.pressed = true;           //Boton presionado = True
    plastico = 1;                     //Ponemos en uno la variable plastico
    startTime = millis();             //Define en startTime el tiempo al cual se ejecuto la interrupcion
  }
}

//Interrupcion para arranque de la cinta
void ARDUINO_ISR_ATTR isrSensor5()
{
  if (millis() - startTimeMotor > timeThreshold5) //Definimos un antirebote
  {
    button5.pressed = true;           //Boton presionado = True
    startTimeMotor = millis();        //Define en startTime el tiempo al cual se ejecuto la interrupcion
  }
}

//Interrupcion del Timer 2 de clasificacion
void IRAM_ATTR onTimer() 
{
  //Se utilizan portENTER_CRITICAL_ISR y portEXIT_CRITICAL_ISR para habilitar y deshabilitar las interrupciones. 
  //Esto se realiza para evitar que otra interrupción ocurra al mismo tiempo que la interrupción actual
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;                   //Sumo 1 a la variable interruptCounter
  portEXIT_CRITICAL_ISR(&timerMux);
}

//Interrupcion del Timer 3 de detencion
void IRAM_ATTR onTimer3() 
{
  //Se utilizan portENTER_CRITICAL_ISR y portEXIT_CRITICAL_ISR para habilitar y deshabilitar las interrupciones. 
  //Esto se realiza para evitar que otra interrupción ocurra al mismo tiempo que la interrupción actual
  portENTER_CRITICAL_ISR(&timerMux3);
  interruptCounter3++;                   //Sumo 1 a la variable interruptCounter3
  portEXIT_CRITICAL_ISR(&timerMux3);
}


/****************************************************************************************************************************************
*****************************************************************************************************************************************
***************************************************     CODIGO PRINCIPAL    *************************************************************
*****************************************************************************************************************************************
****************************************************************************************************************************************/


void setup() {
  //Configuro el serial
	Serial.begin(115200);
  Serial.println("Inicializando...");

  //Interrupciones gpio de los sensores
  //Sensor 1
	pinMode(button1.PIN, INPUT_PULLDOWN);                   //Establece el sensor1 como entrada con resistencia de Pulldown incluida
	attachInterrupt(button1.PIN, isrSensor1, RISING);       //Activa la interrupion del sensor 1 y la llama isrSensor1. La misma se genera con un flanco ascendente
  //Sensor 2
  pinMode(button2.PIN, INPUT_PULLDOWN);
	attachInterrupt(button2.PIN, isrSensor2, RISING);
  //Sensor 3
  pinMode(button3.PIN, INPUT_PULLDOWN);
	attachInterrupt(button3.PIN, isrSensor3, RISING);
  //Sensor 4
  pinMode(button4.PIN, INPUT_PULLDOWN);
	attachInterrupt(button4.PIN, isrSensor4, RISING);

  //Configura LED PWM para servos
  //Servo 1
  ledcSetup(ledChannel1, freq, resolution);               //Configura el PWM en el canal 0, con una frecuencia de 50 Hz y una resolucion de 13 bits 
  ledcAttachPin(ledPin1, ledChannel1);                    //Se activa la interrupcion
  ledcWrite(ledChannel1, dutyCycle1);                     //Se configura un Duty Cycle inicial

  //Servo 2
  ledcSetup(ledChannel2, freq, resolution);               //Lo mismo para el servo 2
  ledcAttachPin(ledPin2, ledChannel2);
  ledcWrite(ledChannel2, dutyCycle2);

  //Configura entrada que da inicio al motor
  pinMode(button5.PIN, INPUT_PULLDOWN);                   //Establece el sensor infrarrojo como entrada con resistencia de Pulldown incluida
	attachInterrupt(button5.PIN, isrSensor5, RISING);       //Activa la interrupion del sensor y la llama isrSensor5. La misma se genera con un flanco ascendente       

  //Configura LED PWM para motor
  ledcSetup(ledChannelMotor, freqMotor, resolutionMotor);
  ledcAttachPin(ledPinMotor, ledChannelMotor);
  ledcWrite(ledChannelMotor, dutyCycleMotor);

  // Timer 2 configuracion
  timer = timerBegin(2, 80, true);                        //Utilizamos el timer 2, con un prescaler de 80 y que cuente hacia arriba
  timerAttachInterrupt(timer, &onTimer, true);            //Se activa la interrupcion con nombre onTimer y que interrumpa en el borde
  timerAlarmWrite(timer, 2000000, false);                 //Se define que el contador sea 2.000.000 lo que es equivalente a 2 segundos
  timerAlarmDisable(timer);                               //Se desactiva la alarma
  timerRestart(timer);                                    //Se resetea el timer
  timerStop(timer);                                       //Se detiene para luego ser usado

  // Timer 3 configuracion
  timer3 = timerBegin(3, 80, true);                       //Utilizamos el timer 3, con un prescaler de 80 y que cuente hacia arriba
  timerAttachInterrupt(timer3, &onTimer3, true);;         //Se activa la interrupcion con nombre onTimer3 y que interrumpa en el borde
  timerAlarmWrite(timer3, 4000000, false);                //Se define que el contador sea 4.000.000 lo que es equivalente a 4 segundos
  timerAlarmDisable(timer3);                              //Se desactiva la alarma
  timerRestart(timer3);                                   //Se resetea el timer
  timerStop(timer3);                                      //Se detiene para luego ser usado
}

/****************************************************************************************************************************************
*****************************************************************************************************************************************
***************************************************          LOOOP          *************************************************************
*****************************************************************************************************************************************
****************************************************************************************************************************************/

void loop() {

  /*Como el sensor4 detecta todos los materiales y se encuentra en la ultima posicion es el ultimo que se activa.
  Aprovechando esto, disparamos un timer que le de unos segundos para realizar la clasificacion. Esto se hace porque se puede presentar el 
  caso que el sensor 4 se active antes del 3. Para solucionar este problema es que se implemento un retardo para realizar la clasificacion
  un tiempo despues de que el sensor 4 se active.
  */
  if (button4.pressed)                       //Si el sensor 4 se activa, se disparan los dos timer, el de clasificacion y el de detencion de la cinta
  {
    //Arranca timer 2
    timerAlarmEnable(timer);                 //Se activa la interrupcion del timer 2
    timerStart(timer);                       //Arranca el timer 2
    //Arranca timer 3
    timerAlarmEnable(timer3);                //Se activa la interrupcion del timer 2
    timerStart(timer3);                      //Arranca el timer 2

    button4.pressed = false;                 //Se pone en false la variable del boton presionado para un futuro
	}

  /*Sensor de Inicio:
  El mismo se activa cuando detecta un objeto en la posicion inicial de la cinta. Lo que hace es realizar un arranque suave del motor.*/
  if (button5.pressed) 
    {
      inicio();
      button5.pressed = false;
    }

  /*CLASIFICACION:
  Pasado el tiempo definido para evitar una mala clasificacion, el timer interrumpe y se procede a realizar la clasificacion.
  Si el sensor 1 fue activado inevitablemente el objeto es metalico ya que es el unico material que este sensor detecta.
  Si el sensor 1 no fue activado pero si el 2 el objeto es vidrio.
  Si el sensor 1 y 2 no fueron activados pero si el 3 el objeto es carton.
  Por ultimo si el sensor 1,2 y 3 no fueron activados pero si el 4 el objeto es plastico.
  A su vez cuando detecta el tipo de material actualiza la posicion de los servos para su clasificacion.
  Una vez realizada la clasificacion se restablecen las variables para realizar una nueva clasificacion en un futuro.
  Por ultimo apaga el timer.*/
  if (interruptCounter > 0) 
  {
    portENTER_CRITICAL(&timerMux);        //Se bloquea momentaneamente las interrupciones
    interruptCounter--;                   //Se resta 1 al interruptCounter para que en un futuro vuelva a interrumpir
    portEXIT_CRITICAL(&timerMux);         //Vuelven a la normalidad las interrupciones

    Serial.printf("Calculando material:.........\n");

    if (metal == 1)
    {
      Serial.printf("Paso un metal\n");
      //Cambia la variable dutyCycle 1 y 2 para luego actualizar el duty de los servos
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

    //Reestablece la variables
    metal = 0;
    vidrio = 0;
    carton = 0;
    plastico = 0;

    //Actualiza la posicion de los servos
    ledcWrite(ledChannel1, dutyCycle1);
    ledcWrite(ledChannel2, dutyCycle2);

    //Apaga el timer2 y su interrupcion
    timerAlarmDisable(timer);
    timerRestart(timer);
    timerStop(timer);
  }


  /*DETENCION DE LA CINTA:
  Cuando pasan 4 segundos luego de que se activo el ultimo sensor la cinta se detiene, esto se realiza para ahorrar energia y que no quede siempre en circulacion.
  */
  if (interruptCounter3 > 0) 
  {
    portENTER_CRITICAL(&timerMux3);         //Se bloquea momentaneamente las interrupciones
    interruptCounter3--;                    //Se resta 1 al interruptCounter para que en un futuro vuelva a interrumpir
    portEXIT_CRITICAL(&timerMux3);          //Vuelven a la normalidad las interrupciones

    Serial.printf("Se detuvo la cinta \n");

    detencion();                            //Se llama a la funcion detencion para que realice una detencion suave

    //Apaga el timer3 y su interrupcion    
    timerAlarmDisable(timer3);
    timerRestart(timer3);
    timerStop(timer3);
  }
}
