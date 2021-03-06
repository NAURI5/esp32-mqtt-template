//ESP32-DevKitC V4
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html
//https://components101.com/microcontrollers/esp32-devkitc
//https://docs.platformio.org/en/latest/boards/espressif32/az-delivery-devkit-v4.html
/*
The pins D0, D1, D2, D3, CMD and CLK are used internally for communication between ESP32 and SPI flash memory. 
They are grouped on both sides near the USB connector. Avoid using these pins, as it may disrupt access to the SPI flash memory / SPI RAM.
*/
#include <Arduino.h>
#include "credentials.h"
#include "settings.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>    //ota
#include <ArduinoOTA.h> //ota

int SW_PIN = 37;   //GPIO 23?
int curr_pin = 20; //ERR 5off?, 6on?

WiFiClient espClient;
PubSubClient client(espClient); //MQTT client
char message_buff[100];         //received cmds

void setup_ota()
{
  //https://lastminuteengineers.com/esp32-ota-updates-arduino-ide/
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });
  ArduinoOTA.begin();
  Serial.println("OTA setup OK");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
}

boolean reconnect()
{ // Here is where we actually connect/re-connect to the MQTT broker and subscribe to our topics.
  if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
  {
    Serial.println("MQTT connected.");
    Serial.println(WiFi.RSSI());
    client.publish(prefixtopic, (String("Hello from ") + HOSTNAME).c_str()); // Sends topic and payload to the MQTT broker.
    for (int i = 0; i < (sizeof(subscribetopic) / sizeof(int)); i++)
    {                                                    // Subscribes to list of topics from the MQTT broker. This whole loop is well above my pay grade.
      String mypref = String(prefixtopic) + String("/"); // But first, we need to get our prefix.
      mypref.concat(subscribetopic[i]);                  // Concatenate prefix and the topic together with a little bit of pfm.
      client.subscribe((char *)mypref.c_str());          // Now, let's subscribe to that concatenated and converted mess.
      Serial.println(mypref);                            // Let's print out each subscribed topic, just to be safe.
    }
  } // if client.connected()
  else
  {
    Serial.println("MQTT not connected.");
    Serial.println(WiFi.RSSI());
  }
  return client.connected();
} // reconnect()

void networking()
{ // Asynchronous network connect routine with MQTT connect and re-connect. The trick is to make it re-connect when something breaks.
  static long lastReconnectAttempt = 0;
  static bool wifistart = false; // State variables for asynchronous wifi & mqtt connection.
  static bool wificonn = false;
  if (wifistart == false)
  {
    //WiFi.hostname(HOSTNAME); // //esp8266
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS); // Initialize WiFi on the ESP8266. We don't care about actual status.
    wifistart = true;
    Serial.println("wifi start");
  }
  if (WiFi.status() == WL_CONNECTED && wificonn == false)
  {
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP()); // This should just print once.
    //MDNS.begin(HOSTNAME);
    wificonn = true;
  }
  if (WiFi.status() != WL_CONNECTED)
    wificonn = false; // Toast the connection if we've lost it.
  if (!client.connected() && WiFi.status() == WL_CONNECTED)
  { // Non-blocking re-connect to the broker. This was challenging.
    if (millis() - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = millis();
      if (reconnect())
      { // Attempt to reconnect.
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    client.loop();
  }
} // networking()

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived: [");
  Serial.print(topic);
  Serial.println("]"); // Prints out anything that's arrived from broker and from the topic that we've subscribed to.
  int i;
  for (i = 0; i < length; i++)
    message_buff[i] = payload[i];
  message_buff[i] = '\0';                  // We copy payload to message_buff because we can't make a string out of payload.
  String msgString = String(message_buff); // Finally, converting our payload to a string so we can compare it.
  String myTopic = String(topic);

  if (myTopic == String(prefixtopic) + "/cmd")
  {
    if (msgString == "REBOOT")
    {
      ESP.restart();
    }
  }

} // callback()

void setup()
{
  delay(500);
  Serial.begin(115200);
  client.setServer(MQTT_SERVER, MQTT_PORT); // Initialize MQTT service. Just once is required.
  client.setCallback(callback);             // Define the callback service to check with the server. Just once is required.
  Serial.println("SETUP A");
  networking();
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("SETUP OTA");
  setup_ota();
  //pinMode(SW_PIN, OUTPUT); //sw_blink
  for (int j = 22; j < 40; j++)
  {
    pinMode(j, OUTPUT);
  }
  pinMode(22, OUTPUT);
  Serial.println("SETUP END");
}

void sw_blink()
{
  static int ledState = LOW;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000)
  {
    previousMillis = currentMillis;
    if (ledState == LOW)
    {
      ledState = HIGH;
      //Serial.print("CURR PIN ");
      //Serial.print(curr_pin);
      //Serial.println("SW HIGH");
    }
    else
    {
      ledState = LOW;
      //Serial.print("CURR PIN ");
      //Serial.print(curr_pin);
      //Serial.println("SW LOW");
    }
    digitalWrite(curr_pin, ledState);
    Serial.println(curr_pin);
    Serial.println(ledState);
    if (ledState == LOW)
    {
      curr_pin++;
    }
  }
}

void loop()
{
  //networking();
  //sw_blink();
  //ArduinoOTA.handle();

  digitalWrite(22, HIGH); //22=nuber 22 on board
  Serial.println("ledpin high");
  delay(2000);
  digitalWrite(22, LOW);
  Serial.println("ledpin low");
  delay(2000);
}