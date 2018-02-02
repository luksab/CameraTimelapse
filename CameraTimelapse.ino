// Author Teemu Mäntykallio, 2017-04-07

// Define pins
#define DIR_PIN1   13
#define STEP_PIN1  12                               // Step on rising edge
#define DIR_PIN2   14
#define STEP_PIN2  27                               // Step on rising edge

HardwareSerial Serial2(2);

#include <SPI.h>
#include "RF24.h"

#include <TMC2208Stepper.h>                       // Include library
TMC2208Stepper driver = TMC2208Stepper(&Serial2); // Create driver and use
// HardwareSerial0 for communication

RF24 radio(21, 22);
byte addresses[][6] = {"1Node", "2Node"};

unsigned long start1;
unsigned long start2;
long zeitU = 10000000.0;
int delay1 =  100;
int delay2 = 1600;
bool start = false;
long startPosY = 0;
long startPosX = 0;
long endPosY = 0;
long endPosX = 0;
long posY = 0;
long posX = 0;
long goalPosY = 0;
long goalPosX = 0;


void setup() {
  Serial.begin(115200);                           // Init used serial port
  while (!Serial);                                // Wait for port to be ready

  Serial.println("Hi!");

  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // LOW power
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);
  radio.startListening();

  Serial2.begin(115200);                            // Init used serial port
  while (!Serial2);

  // Prepare pins
  pinMode(DIR_PIN1, OUTPUT);
  pinMode(STEP_PIN1, OUTPUT);
  pinMode(DIR_PIN2, OUTPUT);
  pinMode(STEP_PIN2, OUTPUT);

  driver.pdn_disable(1);                          // Use PDN/UART pin for communication
  driver.I_scale_analog(0);                       // Adjust current from the registers
  driver.rms_current(500);                        // Set driver current 500mA
  driver.toff(0x2);                               // Enable driver
  driver.microsteps(256);
  driver.mstep_reg_select(1);
  driver.en_spreadCycle(false);

  digitalWrite(DIR_PIN1, LOW);
  digitalWrite(DIR_PIN2, LOW);
  start1 = micros();
  start2 = micros();
}

void loop() {
  if ( radio.available()) {
    int len = 0;
    len = radio.getDynamicPayloadSize();
    char cha[len] = "";
    radio.read( &cha, len );

    radio.stopListening();
    radio.write( &cha, strlen(cha) ); //send back controll
    radio.startListening();
    if (strlen(cha) > 0) {
      char** args = str_split(cha, ',');
      //for (int i=0;i<5;i++)
      //  Serial.println(args[i]);
      if (strcmp(args[0], "startPos") == 0) {
        startPosX = posX;
        startPosY = posY;
      }
      else if (strcmp(args[0], "endPos") == 0) {
        endPosX = posX;
        endPosY = posY;
      }
      else if (strcmp(args[0], "start") == 0) {
        start = true;
        if ((abs(posY - endPosY) != 0) && (abs(posX - endPosX) != 0)) {
          delay1 = (int)(zeitU / (abs(posY - endPosY)));
          delay2 = (int)(zeitU / (abs(posX - endPosX)));
        }
        Serial.println(delay1 + " " + delay2);
        goalPosX = endPosX;
        goalPosY = endPosY;
      }
      else if (strcmp(args[0], "goToStart") == 0) {
        start = true;
        goalPosX = startPosX;
        goalPosY = startPosY;
      }
      else if (strcmp(args[0], "setTime") == 0) {
        zeitU = atoi(args[1]) * 1000000;
      }
      else if (strcmp(args[0], "restart") == 0) {
        driver.pdn_disable(1);                          // Use PDN/UART pin for communication
        driver.I_scale_analog(0);                       // Adjust current from the registers
        driver.rms_current(500);                        // Set driver current 500mA
        driver.toff(0x2);                               // Enable driver
        driver.microsteps(256);
        driver.mstep_reg_select(1);
        driver.en_spreadCycle(false);
        delay1 =  100;
        delay2 = 1600;
        start = false;
        startPosY = 0;
        startPosX = 0;
        endPosY = 0;
        endPosX = 0;
        goalPosY = 0;
        goalPosX = 0;
      }
      else {
        //delay1 = atoi(args[0]);
        //delay2 = atoi(args[1]);
        goalPosX += atoi(args[0]);
        if (goalPosY + atoi(args[1]) > 0)
          goalPosY += atoi(args[1]);
      }
    }
  }
  if (!(micros() - start1 < delay1)) {
    start1 = micros();
    if (goalPosY > posY) {
      //Serial.println("X:"+posX);
      digitalWrite(DIR_PIN1, LOW);
      posY++;
      digitalWrite(STEP_PIN1, !digitalRead(STEP_PIN1));
    }
    else if (goalPosY < posY) {
      //Serial.println("X:"+posX);
      digitalWrite(DIR_PIN1, HIGH);
      posY--;
      digitalWrite(STEP_PIN1, !digitalRead(STEP_PIN1));
    }
  }
  if (!(micros() - start2 < delay2)) {
    start2 = micros();
    if (goalPosX > posX) {
      //Serial.println("Y:"+posY);
      digitalWrite(DIR_PIN2, HIGH);
      posX++;
      digitalWrite(STEP_PIN2, !digitalRead(STEP_PIN2));
    }
    else if (goalPosX < posX) {
      //Serial.println("Y:"+posY);
      digitalWrite(DIR_PIN2, LOW);
      posX--;
      digitalWrite(STEP_PIN2, !digitalRead(STEP_PIN2));
    }
  }
}

char** str_split(char* a_str, const char a_delim)
{
  char** result    = 0;
  size_t count     = 0;
  char* tmp        = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp)
  {
    if (a_delim == *tmp)
    {
      count++;
      last_comma = tmp;
    }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);

  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;

  result = (char**)malloc(sizeof(char*) * count);

  if (result)
  {
    size_t idx  = 0;
    char* token = strtok(a_str, delim);

    while (token)
    {
      assert(idx < count);
      *(result + idx++) = strdup(token);
      token = strtok(0, delim);
    }
    assert(idx == count - 1);
    *(result + idx) = 0;
  }

  return result;
}


