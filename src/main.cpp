#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPUpdateServer.h>
  #define HOSTIDENTIFY  "esp8266"
  #define mDNSUpdate(c)  do { c.update(); } while(0)
  using WebServerClass = ESP8266WebServer;
  using HTTPUpdateServerClass = ESP8266HTTPUpdateServer;
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #include "HTTPUpdateServer.h"
  #define HOSTIDENTIFY  "esp32_h"
  #define mDNSUpdate(c)  do {} while(0)
  using WebServerClass = WebServer;
  using HTTPUpdateServerClass = HTTPUpdateServer;
#endif
#include <WiFiClient.h>
#include <AutoConnect.h>

// DHT22
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTREADINTERVAL 5
#define DHTPIN 4     // what digital pin the DHT22 is conected to
DHT_Unified dht(DHTPIN, DHT22);

//MQTT
#define MQTT_DEVICE "Vacuum-fluorescent-display"

// Fix hostname for mDNS. It is a requirement for the lightweight update feature.
static const char* host = HOSTIDENTIFY "-webupdate";
#define HTTP_PORT 80

// ESP8266WebServer instance will be shared both AutoConnect and UpdateServer.
WebServerClass  httpServer(HTTP_PORT);

#define USERNAME "Florian"   //*< Replace the actual username you want */
#define PASSWORD "fckgwrhqq2"   //*< Replace the actual password you want */
// Declare AutoConnectAux to bind the HTTPWebUpdateServer via /update url
// and call it from the menu.
// The custom web page is an empty page that does not contain AutoConnectElements.
// Its content will be emitted by ESP8266HTTPUpdateServer.
HTTPUpdateServerClass httpUpdater;
AutoConnectAux  update("/update", "Update");

// Declare AutoConnect and the custom web pages for an application sketch.
AutoConnect     portal(httpServer);
AutoConnectAux  hello;

static const char AUX_AppPage[] PROGMEM = R"(
{
  "title": "Humidity sensor",
  "uri": "/",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>Humidity sensor</h2>",
      "style": "text-align:center;color:#2f4f4f;padding:10px;"
    },
    {
      "name": "content",
      "type": "ACText",
      "value": "DHT22 auf NodeMCU32s."
    }
  ]
}
)";

// line feed
void vfd_lf() { Serial1.write(0x0A); }
// set cursor to the left-most-character
void vfd_cr() { Serial1.write(0x0D); }

// write modes
// left-to-right, bottom-to-top, auto CR+LF
void vfd_bottom_to_top() { Serial1.write(0x10); }
// left-to-right, top-to-bottom, auto CR+LF
void vfd_top_to_bottom() { Serial1.write(0x11); }
// CR & LF off, overwrite right-most-character
void vfd_cr_lf_off()     { Serial1.write(0x12); }
// Horizontal scoll mode (right-to-left) only if line is filled
void vfd_hor_scroll()    { Serial1.write(0x13); }

// software reset display
void vfd_reset() { Serial1.write(0x14); }

// cursor settings
bool cursor_visible = true;
void vfd_cursor_hide() {
  if (cursor_visible)  { Serial1.write(0x0E); }
}
void vfd_cursor_show() {
  if (!cursor_visible) { Serial1.write(0x0F); }
}
void vfd_cursor_toggle() {
  if (cursor_visible) {
    vfd_cursor_hide();
  } else {
    vfd_cursor_show();
  }
}
void vfd_cursor_home() { Serial1.write(0x16); }

void vfd_reset() {
  Serial1.write(0x14);
  delay(1200);
}

// select specific character sets (eng is standard)
//                   23 5B 5C 5D 5F 60
// scientific        #  =  ∑  µ  Ω  Δ
void vfd_charset_sci() { Serial1.write(0x1A); }
// english           #  [  \  ]  _  `
void vfd_charset_eng() { Serial1.write(0x1C); }
// general european  £  [  \  ]  _  `
void vfd_charset_eur() { Serial1.write(0x1D); }
// scandinavian      £  Ä  Ö  Å  _  `
void vfd_charset_sca() { Serial1.write(0x1E); }
// german            £  Ä  Ö  Ü  _  `
void vfd_charset_ger() { Serial1.write(0x1F); }

void setup() {
  delay(1000);
  // debug-serial-connection
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println("---Humidity sensor (" + String(MQTT_DEVICE) + ")---");
  Serial.println("last build: " __DATE__ ", " __TIME__);
  Serial.println();
  Serial.println("-Settings-");
  Serial.println("reading DHT-value every " + String(DHTREADINTERVAL) + " min");
  Serial.println();

  dht.begin();
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value);  Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value);  Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value);  Serial.println(F(" %"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value);  Serial.println(F(" %"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F(" %"));
  Serial.println(F("------------------------------------"));
  Serial.println();

  // Prepare the ESP8266HTTPUpdateServer
  // The /update handler will be registered during this function.
  httpUpdater.setup(&httpServer, USERNAME, PASSWORD);

  // Load a custom web page for a sketch and a dummy page for the updater.
  hello.load(AUX_AppPage);
  portal.join({ hello, update });

  Serial.println("-WiFi-");
  if (portal.begin()) {
    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.println(" WiFi connected!");
        Serial.printf( " HTTPUpdateServer ready: http://%s.local/update\n", host);
        Serial.println();
    }
    else
      Serial.println("Error setting up MDNS responder");
  }

  // VFD serial connection
  Serial1.begin(9600);
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
}