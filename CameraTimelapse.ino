// Author Teemu Mäntykallio, 2017-04-07

// Define pins
#define DIR_PIN1   13
#define STEP_PIN1  12                               // Step on rising edge
#define DIR_PIN2   14
#define STEP_PIN2  27                               // Step on rising edge

HardwareSerial Serial2(2);
#include <WiFi.h>
const char* ssid     = "Timelapse";
const char* password = "Password";
WiFiServer server(80);

#include <TMC2208Stepper.h>                       // Include library
TMC2208Stepper driver = TMC2208Stepper(&Serial2); // Create driver and use
// HardwareSerial0 for communication

unsigned long start1;
unsigned long start2;
long zeitU = 10000000.0;
int Steps = 10;
int delay1 =  100;
int delay2 = 1600;
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

  Serial2.begin(115200);                            // Init used serial port
  while (!Serial2);

  WiFi.softAP(ssid, password);

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.softAPIP());

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
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println();
            client.println("<!DOCTYPE HTML><html><head>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
            client.println("<body><h1>Timelapse WiFi Remote</h1>");
            client.println("<p>X-Achse <a href=\"up\"><button>Up</button></a>&nbsp;<a href=\"down\"><button>Down</button></a></p>");
            client.println("<p>Y-Achse <a href=\"left\"><button>Left</button></a>&nbsp;<a href=\"right\"><button>Right</button></a></p>");
            client.println("<p>Set <a href=\"StartPos\"><button>StartPos</button></a>&nbsp;<a href=\"EndPos\"><button>EndPos</button></a> to current Position</p>");
            client.print("<a href=\"/goToStart\">Go to the Startposition</a><br>");
            client.print("<a href=\"/goToEnd\">Go to the Endposition</a><br>");
            client.print("<form><table>");
            client.print("<tr><th>Time in s</th><th><input name=\"Time\" value=\"");client.print(zeitU/1000000);client.print("\" /></th></tr>");
            client.print("<tr><th>Steps per Click</th><th><input name=\"Steps\" value=\"");client.print(Steps);client.print("\" /></th></tr>");
            client.print("</table><br/><input type=\"submit\" value=\"Set\" />");
            client.println("</form><br>");
            client.println("<a href=\"start\"><button>Start</button></a>");
            client.println("<a href=\"reset\"><button>Reset</button></a>");
            client.println("<a href=\"ResetSpeed\"><button>Reset Speed</button></a>");
            client.println("</body></html>");
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine And PROCESS STUFF!!!
            if (currentLine.indexOf("ET") > 0 && currentLine.indexOf("/favicon.ico") <= 0) {
              Serial.println("LINE!!!");
              if (currentLine.indexOf("Time") > 0) {
                Serial.println(currentLine.substring(currentLine.indexOf("Time") + 5));
                String dataFileName = currentLine.substring(currentLine.indexOf("Time") + 5, currentLine.indexOf("&"));//from the end of Time to the next space
                char __dataFileName[sizeof(dataFileName)];
                dataFileName.toCharArray(__dataFileName, sizeof(__dataFileName));
                Serial.println(dataFileName);
                zeitU = atoi(__dataFileName) * 1000000;
              }
              if (currentLine.indexOf("Steps") > 0) {
                Serial.println("-------------");
                Serial.println(currentLine.substring(currentLine.indexOf("Steps") + 6));
                String dataFileName = currentLine.substring(currentLine.indexOf("Steps") + 6,
                                      currentLine.length()-8);//from the end of Time to the next space
                char __dataFileName[sizeof(dataFileName)];
                dataFileName.toCharArray(__dataFileName, sizeof(__dataFileName));
                Serial.println(dataFileName);
                Steps = atoi(__dataFileName);
              }
              else if (currentLine.indexOf("up") > 0)
                goalPosY += Steps * 16;
              else if (currentLine.indexOf("down") > 0 && goalPosY - 10 > 0)
                goalPosY -= Steps * 16;
              else if (currentLine.indexOf("left") > 0)
                goalPosX -= Steps;
              else if (currentLine.indexOf("right") > 0)
                goalPosX += Steps;
              else if (currentLine.indexOf("start") > 0) {
                if ((abs(posY - endPosY) != 0) && (abs(posX - endPosX) != 0)) {
                  delay1 = (int)(zeitU / (abs(posY - endPosY)));
                  delay2 = (int)(zeitU / (abs(posX - endPosX)));
                }
                Serial.println(delay1 + " " + delay2);
                goalPosX = endPosX;
                goalPosY = endPosY;
              }
              else if (currentLine.indexOf("goToStart") > 0) {
                goalPosX = startPosX;
                goalPosY = startPosY;
              }
              else if (currentLine.indexOf("goToEnd") > 0) {
                goalPosX = endPosX;
                goalPosY = endPosY;
              }
              else if (currentLine.indexOf("StartPos") > 0) {
                startPosX = posX;
                startPosY = posY;
              }
              else if (currentLine.indexOf("EndPos") > 0) {
                endPosX = posX;
                endPosY = posY;
              }
              else if (currentLine.indexOf("ResetSpeed") > 0) {
                driver.pdn_disable(1);                          // Use PDN/UART pin for communication
                driver.I_scale_analog(0);                       // Adjust current from the registers
                driver.rms_current(500);                        // Set driver current 500mA
                driver.toff(0x2);                               // Enable driver
                driver.microsteps(256);
                driver.mstep_reg_select(1);
                driver.en_spreadCycle(false);
                delay1 =  100;
                delay2 = 1600;
              }
              else if (currentLine.indexOf("reset") > 0) {
                driver.pdn_disable(1);                          // Use PDN/UART pin for communication
                driver.I_scale_analog(0);                       // Adjust current from the registers
                driver.rms_current(500);                        // Set driver current 500mA
                driver.toff(0x2);                               // Enable driver
                driver.microsteps(256);
                driver.mstep_reg_select(1);
                driver.en_spreadCycle(false);
                delay1 =  100;
                delay2 = 1600;
                startPosY = 0;
                startPosX = 0;
                endPosY = 0;
                endPosX = 0;
                goalPosY = 0;
                goalPosX = 0;
              }
            }
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
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


