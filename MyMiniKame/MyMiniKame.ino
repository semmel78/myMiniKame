#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>
#include "minikame.h"
#include <ArduinoJson.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Wifi Access Point configuration
const char* ssid = "mykame";
const char* password = "";
const char* http_username = "admin";
const char* http_password = "admin";
IPAddress   apIP(10, 0, 0, 1);  // Defining a static IP address: local & gateway
IPAddress   gateway(10, 0, 0, 1);  // Defining a static IP address: local & gateway
IPAddress   subnet(255, 0, 0, 0);

void parseData(String data);


String jsonMsg = "";
boolean newMsg = false;

MiniKame robot;
//WiFiServer server(81);
bool running = 0;
String input;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws



//flag to use from web update to reboot the ESP
bool shouldReboot = false;



void setup() {
  Serial.begin(57600);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(ssid, password); //Access point is open

  SPIFFS.begin();

  initServer();

  delay(1000);
  robot.init();
}

void loop() {
  /*    WiFiClient client = server.available();
      if (!client) {
          IPAddress myIP = WiFi.softAPIP();
          Serial.print("AP IP address: ");
          Serial.println(myIP);
          delay(1000);
      }
      while (client.connected()) {
          if (running){
              Serial.println("running");
              if (client.available()) {
                  while(client.available()) input = client.readStringUntil('+');
                  parseData(input);
              }
              else {
                  Serial.println("Keep Moving");
                  parseData(input);
              }
          }
          else{
              Serial.println("Normal mode");
              if (client.available()) {
                  while(client.available()) input = client.readStringUntil('+');
                  parseData(input);
              }
              else robot.home();
          }
      }
  */
  if(newMsg) parseData();
}

void parseData() {
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonMsg);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }      
  
  // Print values.
  int command = root["com"];
  int value = root["val"];
  Serial.print(command);
    Serial.print(" - ");
  Serial.println(value);
  newMsg = false;
  switch (command) {

    case 1: // Up
      robot.walk(1, 550);
      running = 1;
      break;

    case 2: // Down
      break;

    case 3: // Left
      robot.turnL(1, 550);
      running = 1;
      break;

    case 4: // Right
      robot.turnR(1, 550);
      running = 1;
      break;

    case 5: // STOP
      running = 0;
      break;

    case 6: // heart
      robot.pushUp(2, 2000);
      break;

    case 7: // fire
      robot.upDown(4, 250);
      break;

    case 8: // skull
      robot.jump();
      break;

    case 9: // cross
      robot.hello();
      break;

    case 10: // punch
      robot.frontBack(4, 200);
      break;

    case 11: // mask
      robot.dance(2, 1000);
      break;

    case 12: // mask
      robot.moonwalkL(2, 1000);
      break;

    default:
      robot.home();
      break;
  }
}

void initServer() {
  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // attach filesystem root at URL /fs
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);

  server.begin();
}

void onRequest(AsyncWebServerRequest *request) {
  //Handle Unknown Request
  Serial.printf("NOT_FOUND: ");
  if (request->method() == HTTP_GET)
    Serial.printf("GET");
  else if (request->method() == HTTP_POST)
    Serial.printf("POST");
  else if (request->method() == HTTP_DELETE)
    Serial.printf("DELETE");
  else if (request->method() == HTTP_PUT)
    Serial.printf("PUT");
  else if (request->method() == HTTP_PATCH)
    Serial.printf("PATCH");
  else if (request->method() == HTTP_HEAD)
    Serial.printf("HEAD");
  else if (request->method() == HTTP_OPTIONS)
    Serial.printf("OPTIONS");
  else
    Serial.printf("UNKNOWN");
  Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if (request->contentLength()) {
    Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for (i = 0; i < headers; i++) {
    AsyncWebHeader* h = request->getHeader(i);
    Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for (i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if (p->isFile()) {
      Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }

  request->send(404);
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    jsonMsg = "";
    if (info->final && info->index == 0 && info->len == len) {
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%u]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          jsonMsg += (char) data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          jsonMsg += buff ;
        }
      }
      Serial.printf("%s\n", jsonMsg.c_str());
      newMsg = true;
  
/*
      if (info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
*/
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0) {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%lu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %lu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          jsonMsg += (char) data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          jsonMsg += buff ;
        }
      }
      Serial.printf("%s\n", jsonMsg.c_str());

      if ((info->index + len) == info->len) {
        Serial.printf("ws[%s][%u] frame[%u] end[%lu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final) {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
