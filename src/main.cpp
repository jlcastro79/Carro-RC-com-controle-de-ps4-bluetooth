#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>

//pinos figurativos
#define LN298_IN1 27
#define LN298_IN2 26
#define LN298_EN 25

#define SERVO_PIN 14

#define PWM_CHANNEL 8
#define PWM_FREQ 1000
#define PWM_RES 8

#define MIN_PWM 0
#define MAX_PWM 255

#define LED_CHANNEL 13

#define SERVO_CENTER 90
#define SERVO_MIN 30
#define SERVO_MAX 150
#define joytstick_Deadzone 30

#define trigger_deadzone 80

bool isControllerConnected = false;
int pwm_Motorvalue = 0;
Servo steering;

//ControllerPtr myController[BP32_MAX_GAMEPADS];
ControllerPtr myController = nullptr;


void controllerConnected(ControllerPtr ctl){
   //for(int i = 0; i< BP32_MAX_GAMEPADS;i++){}
    if(myController == nullptr){
      myController = ctl;
      isControllerConnected = true;
      Serial.println("Controle conectado");
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                     properties.product_id);
      //break;
    } else {
       ctl->disconnect();
    }
  }

void controllerDisconnected(ControllerPtr ctl){
  //for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {}
    if (myController == ctl) {
        myController = nullptr;
        isControllerConnected = false;

        steering.write(SERVO_CENTER);
        ledcWrite(LED_CHANNEL, 0);
        Serial.println("Controle desconectado");
          //break;
    }
}

void stopMotors(){
  pwm_Motorvalue = 0;
  digitalWrite(LN298_IN1, LOW);
  digitalWrite(LN298_IN2, LOW);
  ledcWrite(PWM_CHANNEL, 0);
}

//R2
bool accelerateMotor(ControllerPtr myController){
   int speed = myController->throttle();
   Serial.printf("valor do pressionamento do R2: %d \n",speed);
   if(speed <= trigger_deadzone){
    return false;
   }
   pwm_Motorvalue = map(speed,trigger_deadzone,1023,MIN_PWM,MAX_PWM);
      
    Serial.print("valor do pwm:");
    Serial.println(pwm_Motorvalue);
    digitalWrite(LN298_IN1,HIGH);
    digitalWrite(LN298_IN2,LOW);

    ledcWrite(PWM_CHANNEL,pwm_Motorvalue);
    return true;
  }

//L2 
bool brakeMotor(ControllerPtr myController){
      int speed = myController->brake();
      Serial.printf("valor do pressionamento do L2: %d \n",speed);
      if(speed <= trigger_deadzone){
          return false;
      }
      pwm_Motorvalue = map(speed,trigger_deadzone,1023,MIN_PWM,MAX_PWM);
      
      Serial.print("valor do pwm:");
      Serial.println(pwm_Motorvalue);

      digitalWrite(LN298_IN1,LOW);
      digitalWrite(LN298_IN2,HIGH);

      ledcWrite(PWM_CHANNEL,pwm_Motorvalue);
      return true;
    }

//axisX = analogico esquerdo
void updateServo(ControllerPtr ctl){
  int axisX = ctl->axisX();
  int angle = SERVO_CENTER;

  Serial.printf("leitura atual do joystick da esquerda: %d \n",axisX);

  if(abs(axisX) > joytstick_Deadzone){
    angle = map(axisX,-511,512,SERVO_MIN,SERVO_MAX);
  }

  static int lastAngle = -1;
  if (angle != lastAngle) {
    steering.write(angle);
    Serial.printf("Angulo atual do servo: %d\n", angle);
    lastAngle = angle;
  }
}

void updateLed(ControllerPtr ctl) {
  int axisX = ctl->axisX();
  int brightness = map(abs(axisX), 0, 512, 0, 255);
  if (abs(axisX) <= joytstick_Deadzone) brightness = 0;
  ledcWrite(LED_CHANNEL, brightness);
}

void setup() {
  Serial.begin(115200);

  pinMode(LN298_IN1, OUTPUT);
  pinMode(LN298_IN2, OUTPUT);
  ledcSetup(LED_CHANNEL, 5000, 8);
  ledcAttachPin(LED_BUILTIN, LED_CHANNEL);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LN298_EN, PWM_CHANNEL);

  steering.setPeriodHertz(50);
  steering.attach(SERVO_PIN, 500, 2400);
  steering.write(SERVO_CENTER);

  BP32.setup(&controllerConnected, &controllerDisconnected);
}

void loop() {
  BP32.update();

  bool isMoving = false;
  if (isControllerConnected && myController != nullptr && myController->isConnected()) {
    Serial.println(isMoving ? true : false);
    isMoving = accelerateMotor(myController);

    if(!isMoving){
      isMoving = brakeMotor(myController);
      Serial.println(isMoving ? true : false);
    }

    updateServo(myController);
    updateLed(myController);
  }
  
  if(!isMoving){
    stopMotors();
  }
  delay(1);
}