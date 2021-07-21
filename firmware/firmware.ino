#include<WiFi.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include"pitches.h"

#define WIFI_AP_NAME "ESP32 AP mode"
#define WIFI_AP_PASS "123456"

#define WIFI_STA_NAME "mrx01"
#define WIFI_STA_PASS "007007007"

#define MQTT_USERNAME "device"
#define MQTT_PASSWORD "sripatum"

#define MQTT_SERVER   "192.168.43.143"
#define MQTT_PORT     1883
#define MQTT_NAME     "ESP32_1"

#define timeSeconds 10

#define LED_BUILTIN 2
#define LED_PIN 23

#define SERVOSM_PIN_SIG  3

Servo myservo; //ประกาศตัวแปรแทน Servo

WiFiClient client;
PubSubClient mqtt(client);

//const char* mqttServer = "192.168.1.222";
//const int mqttPort = 1883 ;
const int motionSensor = 27;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;

int melody[] = {
NOTE_D4, NOTE_G4, NOTE_FS4, NOTE_A4,
NOTE_G4, NOTE_C5, NOTE_AS4, NOTE_A4,                   
NOTE_FS4, NOTE_G4, NOTE_A4, NOTE_FS4, NOTE_DS4, NOTE_D4,
NOTE_C4, NOTE_D4,0,                                 

NOTE_D4, NOTE_G4, NOTE_FS4, NOTE_A4,
NOTE_G4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_C5, NOTE_AS4, NOTE_A4,      //29               //8
NOTE_FS4, NOTE_G4, NOTE_A4, NOTE_FS4, NOTE_DS4, NOTE_D4,
NOTE_C4, NOTE_D4,0,                                       

NOTE_D4, NOTE_FS4, NOTE_G4, NOTE_A4, NOTE_DS5, NOTE_D5,
NOTE_C5, NOTE_AS4, NOTE_A4, NOTE_C5,
NOTE_C4, NOTE_D4, NOTE_DS4, NOTE_FS4, NOTE_D5, NOTE_C5,
NOTE_AS4, NOTE_A4, NOTE_C5, NOTE_AS4,             //58

NOTE_D4, NOTE_FS4, NOTE_G4, NOTE_A4, NOTE_DS5, NOTE_D5,
NOTE_C5, NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_C5, NOTE_AS4, NOTE_A4, NOTE_C5, NOTE_G4,
NOTE_A4, 0, NOTE_AS4, NOTE_A4, 0, NOTE_G4,
NOTE_G4, NOTE_A4, NOTE_G4, NOTE_FS4, 0,

NOTE_C4, NOTE_D4, NOTE_G4, NOTE_FS4, NOTE_DS4,
NOTE_C4, NOTE_D4, 0,
NOTE_C4, NOTE_D4, NOTE_G4, NOTE_FS4, NOTE_DS4,
NOTE_C4, NOTE_D4, END

};

// note durations: 8 = quarter note, 4 = 8th note, etc.
int noteDurations[] = {       //duration of the notes
8,4,8,4,
4,4,4,12,
4,4,4,4,4,4,
4,16,4,

8,4,8,4,
4,2,1,1,2,1,1,12,
4,4,4,4,4,4,
4,16,4,

4,4,4,4,4,4,
4,4,4,12,
4,4,4,4,4,4,
4,4,4,12,

4,4,4,4,4,4,
2,1,1,2,1,1,4,8,4,
2,6,4,2,6,4,
2,1,1,16,4,

4,8,4,4,4,
4,16,4,
4,8,4,4,4,
4,20,
};

int speed=90;  //higher value, slower notes

// shared variable
TaskHandle_t t0;
TaskHandle_t t1;
TaskHandle_t t2;
TaskHandle_t t3;
TaskHandle_t t4;
TaskHandle_t t5;
TaskHandle_t t6;
TaskHandle_t t7;

void setup() {
  pinMode(0, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);

  //create a task to handle button gpio 0 (core 0)
  xTaskCreatePinnedToCore(
    tButtonFunc,  /* Task function. */
    "Button",     /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t0,          /* Task handle to keep track of created task */
    0);           /* pin task to core 0 */
  delay(500);

  // create a task to handle led LED_BUILTIN (core 1)
  xTaskCreatePinnedToCore(
    tLedFunc,     /* Task function. */
    "Led",        /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t1,          /* Task handle to keep track of created task */
    1);           /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
    tApFunc,     /* Task function. */
    "ApF",        /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t2,          /* Task handle to keep track of created task */
    1);           /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
    tStaFunc,     /* Task function. */
    "Sta",        /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    0,            /* priority of the task */
    &t3,          /* Task handle to keep track of created task */
    0);           /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
    tServoFunc,     /* Task function. */
    "Ser",        /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t4,          /* Task handle to keep track of created task */
    1);           /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
    tMqttFunc,  /* Task function. */
    "Mqt",     /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t5,          /* Task handle to keep track of created task */
    0);           /* pin task to core 0 */
  delay(500);

  xTaskCreatePinnedToCore(
    tPirFunc,  /* Task function. */
    "Pir",     /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t6,          /* Task handle to keep track of created task */
    1);           /* pin task to core 0 */
  delay(500);

   xTaskCreatePinnedToCore(
    tSoundFunc,  /* Task function. */
    "Sou",     /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t7,          /* Task handle to keep track of created task */
    1);           /* pin task to core 0 */
  delay(500);
  
}

void loop() {
  // no coding here
  Serial.print("loop running on core ");
  Serial.println(xPortGetCoreID());
  delay(1000);
}

// tButtonFunc: check button for push and releae events
void tButtonFunc(void *params) {
  // local variables
  bool lastState = false;
  
  // setup
  Serial.print("tButtonFunc running on core ");
  Serial.println(xPortGetCoreID());
  
  // loop
  while (true) {
    bool curState = digitalRead(0) == LOW;
    if (!lastState && curState) {     // push
  Serial.print("tButtonFunc running on core ");
  Serial.println(xPortGetCoreID());
      Serial.println("tButtonFunc: push");  
    } else if (lastState && !curState) { // release
  Serial.print("tButtonFunc running on core ");
  Serial.println(xPortGetCoreID());
      Serial.println("tButtonFunc: release");
    }
    lastState = curState;
    delay(10);
  } 
}

// tLedFunc: blinks every 1000ms
void tLedFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tLedFunc running on core ");
  Serial.println(xPortGetCoreID());

  // loop
  while (true) {
  Serial.print("tLedFunc running on core ");
  Serial.println(xPortGetCoreID());
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
  }
}

// tLedFunc: blinks every 1000ms
void tApFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tApFunc running on core ");
  Serial.println(xPortGetCoreID());
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASS);

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // loop
  while (true) {
  Serial.print("tApFunc running on core ");
  Serial.println(xPortGetCoreID());
    delay(1000);
  }
}

// tLedFunc: blinks every 1000ms
void tStaFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tStaFunc running on core ");
  Serial.println(xPortGetCoreID());
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_STA_NAME);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

 
  // loop
  while (true) {
  Serial.print("tStaFunc running on core ");
  Serial.println(xPortGetCoreID());
    delay(1000);
  }
}

// tLedFunc: blinks every 1000ms
void tServoFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tServoFunc running on core ");
  Serial.println(xPortGetCoreID());
  
  myservo.attach(13); // D22 (กำหนดขาควบคุม Servo)
  
  // loop
  while (true) {
  Serial.print("tServoFunc running on core ");
  Serial.println(xPortGetCoreID());

  
  /*myservo.write(0); // สั่งให้ Servo หมุนไปองศาที่ 0
  delay(1000); // หน่วงเวลา 1000ms
  myservo.write(90); // สั่งให้ Servo หมุนไปองศาที่ 90
  delay(1000); // หน่วงเวลา 1000ms
  myservo.write(180); // สั่งให้ Servo หมุนไปองศาที่ 180
  delay(2000); // หน่วงเวลา 2000ms*/
  delay(1000);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
  
  }
}

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  Serial.println("MOTION DETECTED!!!");
  digitalWrite(LED_PIN, HIGH);
  startTimer = true;
  lastTrigger = millis();
}
// tLedFunc: blinks every 1000ms
void tPirFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tPirFunc running on core ");
  Serial.println(xPortGetCoreID());
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  // Set LED to LOW
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // loop
  while (true) {
  Serial.print("tPirFunc running on core ");
  Serial.println(xPortGetCoreID());
  // Current time
  now = millis();
  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("Motion stopped...");
    myservo.write(150);
    delay(500);
    myservo.write(20);
    delay(500);
    myservo.detach();
    digitalWrite(LED_PIN, LOW);
    startTimer = false;
  }
  
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
    
  }
}

// tLedFunc: blinks every 1000ms
void tSoundFunc(void *params) {
  // local variable
  
  // setup  
  Serial.print("tSoundFunc running on core ");
  Serial.println(xPortGetCoreID());
 if(startTimer = true){
  for (int thisNote = 0; melody[thisNote]!=-1; thisNote++) {

  int noteDuration = speed*noteDurations[thisNote];
  tone(3, melody[thisNote],noteDuration*.95);
  Serial.println(melody[thisNote]);

  delay(noteDuration);

  noTone(3);
  char tempString[20];
    String strVal = "Speaked";
    strVal.toCharArray(tempString, 20);
    mqtt.publish("device/ESP32_1/Order/command", tempString);
  }
}else{
  noTone(3);
  }

  // loop
  while (true) {
  Serial.print("tSoundFunc running on core ");
  Serial.println(xPortGetCoreID());
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
  }
}

void tMqttFunc(void *params) {
   // local variable
  
  // setup  
  Serial.print("tMqttFunc running on core ");
  Serial.println(xPortGetCoreID());
  // Loop until we're reconnected
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_STA_NAME);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
    // loop
  while (true) {
     Serial.print("tMqttFunc running on core ");
  Serial.println(xPortGetCoreID());
    delay(1000);
     if (mqtt.connected() == false) {
    Serial.print("MQTT connection... ");
    if (mqtt.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqtt.subscribe("device/ESP32_1/Order/command");
    } else {
      Serial.println("failed");
      delay(5000);
    }
  } else {
    mqtt.loop();
  }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String topic_str = topic, payload_str = (char*)payload;
  Serial.println("[" + topic_str + "]: " + payload_str);

  digitalWrite(LED_PIN, (payload_str == "ON") ? HIGH : LOW);

  if(payload_str == "Orderwait"){
     myservo.write(150);
    delay(500);
    myservo.write(20);
    delay(500);
    myservo.detach();
    char tempString[20];
    String strVal = "Orderedwait";
    strVal.toCharArray(tempString, 20);
    mqtt.publish("device/ESP32_1/Order/command", tempString);
  }
  lastTrigger = millis();
  if(lastTrigger = 10000){
      char tempString[20];
      String strVal = "Touched";
     strVal.toCharArray(tempString, 20);
    mqtt.publish("device/ESP32_1/Order/command", tempString);
  }
   if(payload_str == "Ordernow"){
    if(lastTrigger = 30000){
     myservo.write(150);
    delay(500);
    myservo.write(20);
    delay(500);
    myservo.detach();
    char tempString[20];
    String strVal = "Orderednow";
    strVal.toCharArray(tempString, 20);
    mqtt.publish("device/ESP32_1/Order/command", tempString);
    }
  }
  
}
