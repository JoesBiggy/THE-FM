// Import required libraries
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ESPmDNS.h>
#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <elegantWebpage.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Import webpage
#include "index.h"

#define INTERNAL_LED 2

// Replace with your network credentials
const char *ssid = "JUSTIN";
const char *password = "91705112";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *globalClient = NULL;

// PLAYMODES

int playmode = 0; // 0 = NONE/OFF

int playmodeTimer = 0;

//  STEPPER
int motor_stufe = 0;
int motor_stufe_max = 30;
int motor_speed = 0;
int motor_speed_stufe = 0;
int motor_speed_max = 20000;
unsigned long timeNow = 0;
unsigned long previousAccel = 0;
int interval = 1;
AccelStepper stepper(2, 5, 18);

bool motor_state = false;

const char *PARAM_MESSAGE = "message";

void senddata()
{
  DynamicJsonDocument doc(1024);
  doc["motor_speed"] = motor_speed;
  doc["motor_speed_stepper"] = stepper.speed();
  doc["motor_stufe"] = String(motor_stufe) + "/" + String(motor_stufe_max);
  doc["motor_state"] = motor_state;
  switch (playmode)
  {
  case 0:
    doc["playmode"] = "SPEED CTRL";
    break;
  case 1:
    doc["playmode"] = "START STOP";
    break;
  case 2:
    doc["playmode"] = "WAVE";
    break;
  case 3:
    doc["playmode"] = "EDGING";
    break;
  }
  String response;
  serializeJson(doc, response);
  Serial.println(response);
  ws.textAll(String(response));
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void syncSpeed()
{
  motor_speed = (motor_speed_max / motor_stufe_max) * motor_stufe;
  motor_speed_stufe = (motor_speed_max / motor_stufe_max) * motor_stufe;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "addspeed") == 0)
    {
      if (motor_stufe >= 0 && motor_stufe < motor_stufe_max)
      {
        motor_stufe++;
        syncSpeed();
      }
    }
    if (strcmp((char *)data, "addspeed5") == 0)
    {
      if (motor_stufe >= 0 && motor_stufe + 5 <= motor_stufe_max)
      {
        motor_stufe += 5;
        syncSpeed();
      }
    }
    if (strcmp((char *)data, "remspeed") == 0)
    {
      if (motor_stufe > 0 && motor_stufe <= motor_stufe_max)
      {

        if (motor_stufe == 0)
        {
          motor_speed = 0;
        }
        else
        {
          motor_stufe--;
          syncSpeed();
        }
      }
    }
    if (strcmp((char *)data, "remspeed5") == 0)
    {
      if (motor_stufe - 5 >= 0 && motor_stufe <= motor_stufe_max)
      {

        if (motor_stufe == 0)
        {
          motor_speed = 0;
        }
        else
        {
          motor_stufe -= 5;
          syncSpeed();
        }
      }
    }
    if (strcmp((char *)data, "motor_on") == 0)
    {
      syncSpeed();
      digitalWrite(19, HIGH);
      motor_state = true;
      stepper.enableOutputs();
    }
    if (strcmp((char *)data, "motor_off") == 0)
    {
      motor_speed = 0;
      // motor_stufe = 0;
      motor_state = false;
    }

    // MODES

    if (strcmp((char *)data, "mode0") == 0)
    {
      playmode = 0;
      playmodeTimer = 0;
    }
    if (strcmp((char *)data, "mode1") == 0)
    {
      playmode = 1;
      playmodeTimer = 0;
    }
    if (strcmp((char *)data, "mode2") == 0)
    {
      playmode = 2;
      playmodeTimer = 0;
    }
    if (strcmp((char *)data, "mode3") == 0)
    {
      playmode = 3;
      playmodeTimer = 0;
    }

    // RESTART
    if (strcmp((char *)data, "reboot") == 0)
    {
      ESP.restart();
    }
    // if (strcmp((char*)data, "motor_change") == 0) {
    //
    //   delay(1000);
    // }

    if (strcmp((char *)data, "motor_stop") == 0)
    {
      motor_speed = 0;
      motor_stufe = 0;
      syncSpeed();
      motor_state = false;
      digitalWrite(19, LOW);
      stepper.setMaxSpeed(0);
      stepper.disableOutputs();
    }
    if (strcmp((char *)data, "enableota") == 0)
    {
      AsyncElegantOTA.begin(&server);
    }

    // ------------------------------------------------

    senddata();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  Serial.begin(19200);
  pinMode(INTERNAL_LED, OUTPUT);
  pinMode(19, OUTPUT);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.println("Connecting to WiFi..");
    if (digitalRead(INTERNAL_LED) == HIGH)
    {
      digitalWrite(INTERNAL_LED, LOW);
    }
    else
    {
      digitalWrite(INTERNAL_LED, HIGH);
    }
  }
  while (!MDNS.begin("controller"))
  {
    Serial.println("Starting mDNS...");
    delay(1000);
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  digitalWrite(INTERNAL_LED, HIGH);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });
  server.on("/setmaxspeed", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        String message;
        String htmlmessage;
        if (request->hasParam("speed")) {
            message = request->getParam("speed")->value();
            motor_speed_max = message.toInt();
        } 
        htmlmessage = "<ul>  <li>motor_speed_max "+String(motor_speed_max)+"</li>  </ul> <br> <a href=\"/\"> BACK </a>";
        request->send(200, "html", htmlmessage); });

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Start server
  server.begin();

  stepper.setMaxSpeed(0);

  stepper.setSpeed(0);

  stepper.disableOutputs();
}

int timer = 0;

void loop()
{
  if (stepper.speed() < motor_speed && motor_state == true)
  {
    timeNow = millis();
    if (timeNow > previousAccel + interval)
    {
      previousAccel = timeNow;
      stepper.setSpeed(stepper.speed() + motor_speed_max / 2500);
      stepper.setMaxSpeed(motor_speed);
    }
  }
  if (stepper.speed() > motor_speed)
  {
    timeNow = millis();
    if (timeNow > previousAccel + interval)
    {
      previousAccel = timeNow;
      stepper.setSpeed(stepper.speed() - motor_speed_max / 2500);
      stepper.setMaxSpeed(stepper.speed() - motor_speed_max / 2500);
    }
  }
  stepper.runSpeed();

  if (timer == 25000)
  {
    ws.cleanupClients();
    // if(WiFi.status() != 3)
    //{
    //  ESP.restart();
    //}
    timer = 0;
  }
  if (playmodeTimer == 6000000)
  {
    playmodeTimer = 0;
  }

  // PLAYMODES
  if (playmode == 0)
  {
    playmodeTimer = 0;
  }
  if (playmode == 1)
  {
    _MODE_STARTSTOP(playmodeTimer);
    playmodeTimer = playmodeTimer + 1;
  }
  if (playmode == 2)
  {
    _MODE_WAVE(playmodeTimer);
    playmodeTimer = playmodeTimer + 1;
  }
  if (playmode == 3)
  {
    _MODE_EDGING(playmodeTimer);
    playmodeTimer = playmodeTimer + 1;
  }
  timer = timer + 1;
}

void _MODE_STARTSTOP(int i)
{
  switch (i)
  {
  case 0:
    motor_speed = motor_speed_stufe;
    break;

  case 1000000:
    motor_speed = 0;
    break;

  case 2000000:
    motor_speed = motor_speed_stufe;
    break;

  case 3000000:
    motor_speed = 0;
    break;

  case 4000000:
    motor_speed = motor_speed_stufe;
    break;

  case 5000000:
    motor_speed = 0;
    break;

  case 6000000:
    motor_speed = motor_speed_stufe;
    break;
  }
}

void _MODE_WAVE(int i)
{
  switch (i)
  {
  case 0:
    motor_speed = motor_speed_stufe;
    break;

  case 1000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 2000000:
    motor_speed = motor_speed_stufe;
    break;

  case 3000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 4000000:
    motor_speed = motor_speed_stufe;
    break;

  case 5000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 6000000:
    motor_speed = motor_speed_stufe;
    break;
  }
}

void _MODE_EDGING(int i)
{
  switch (i)
  {
  case 0:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 1000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 2000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 3000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 4000000:
    motor_speed = motor_speed_stufe / 4;
    break;

  case 5000000:
    motor_speed = motor_speed_stufe;
    break;

  case 6000000:
    motor_speed = motor_speed_stufe;
    break;
  }
}