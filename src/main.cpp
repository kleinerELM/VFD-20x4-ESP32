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

#define VFD_W 20
#define VFD_H 4

#define DHTREADINTERVAL 1*60*1000 // in min
#define MIN
#define DHTPIN 4     // what digital pin the DHT22 is conected to
DHT_Unified dht(DHTPIN, DHT22);


float h = 0;
float t = 0;
void read_dht() {
    Serial.print("reading DHT22 at Pin " + String(DHTPIN) + ": ");

    // Get humidity event and print its value.
    sensors_event_t event;
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    } else {
      h = event.relative_humidity;
      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.println(F("%"));
    }

    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    } else {
      t = event.temperature;
      Serial.print(F("Temperature: "));
      Serial.print(t);
      Serial.println(F("°C"));
    }
}

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
void vfd_lf() { Serial2.write(0x0A); }
// set cursor to the left-most-character
void vfd_cr() { Serial2.write(0x0D); }
void vfd_new_line() { vfd_lf(); vfd_cr(); }

// write modes
// left-to-right, bottom-to-top, auto CR+LF
void vfd_bottom_to_top() { Serial2.write(0x10); }
// left-to-right, top-to-bottom, auto CR+LF
void vfd_top_to_bottom() { Serial2.write(0x11); }
// CR & LF off, overwrite right-most-character
void vfd_cr_lf_off()     { Serial2.write(0x12); }
// Horizontal scoll mode (right-to-left) only if line is filled
void vfd_hor_scroll()    { Serial2.write(0x13); }

// cursor settings
bool cursor_visible = true;
void vfd_cursor_hide() {
  if (cursor_visible)  { Serial2.write(0x0E); }
}
void vfd_cursor_show() {
  if (!cursor_visible) { Serial2.write(0x0F); }
}
void vfd_cursor_toggle() {
  if (cursor_visible) {
    vfd_cursor_hide();
  } else {
    vfd_cursor_show();
  }
}
void vfd_cursor_home() { Serial2.write(0x16); }

// software reset display
void vfd_reset() {
  Serial2.write(0x14);
  delay(1200);
}

void vfd_cls() { Serial2.write(0x15); }

// select specific character sets (eng is standard)
//                   23 5B 5C 5D 5F 60
// scientific        #  =  ∑  µ  Ω  Δ
void vfd_charset_sci() { Serial2.write(0x1A); }
// english           #  [  \  ]  _  `
void vfd_charset_eng() { Serial2.write(0x1C); }
// general european  £  [  \  ]  _  `
void vfd_charset_eur() { Serial2.write(0x1D); }
// scandinavian      £  Ä  Ö  Å  _  `
void vfd_charset_sca() { Serial2.write(0x1E); }
// german            £  Ä  Ö  Ü  _  `
void vfd_charset_ger() { Serial2.write(0x1F); }

// the manual described 4 brightness states. However only 2 seem to work:
// 0, 1, 2 dim
// 3 bright
void vfd_brightness( int value ) {
  Serial2.write(0x19);
  if ( value == 1 ) {
    Serial2.write(0x4D);
  } else if ( value == 2 ) {
    Serial2.write(0x4E);
  } else if ( value == 3 ) {
    Serial2.write(0x4F);
  } else {
    Serial2.write(0x4C);
  }
}

int vfd_print( String text ) {
  text.replace('Ä', '[');
  text.replace('Ö', '\\');
  text.replace('Ä', ']');
  text.replace('°', '~');

  int len = text.length();
  int i = 0;
  if (len < VFD_W-1) {
    for (i=0;i<floor( (VFD_W - len) / 2 );i++) {
      Serial2.print(' ');
    }
  }

  for (int j=0;j<len;j++) {
    Serial2.print(text[j]);
    delay(100);
  }

  if (len < VFD_W ) {
    for (int k=0;k<(VFD_W-len-i);k++) {
      Serial2.print(' ');
    }
  }
  return len;
}

void vfd_println( String text ) {
  int len = vfd_print( text );
  vfd_new_line();
}

int last_read = millis();
void setup() {
  delay(1000);
  // debug-serial-connection
  Serial.begin(9600);

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
  read_dht();

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
  // at pin 16 and pin 17
  // there were noise issues using 9600 baud
  Serial2.begin(1200 , SERIAL_7O1, 16,17);
  vfd_cls();
}


char dht_string[21];
void loop() {
  if ( DHTREADINTERVAL < millis() - last_read ) {
    read_dht();
    last_read = millis();
  }

  vfd_brightness(3);
  vfd_cursor_hide();
  vfd_cr_lf_off();

  vfd_charset_ger();
  vfd_cursor_home();

  vfd_println("Willkommen am");
  vfd_println("F.A. Finger-Institut");
  vfd_println("im FIB/REM-Labor");
  vfd_println("an der");
  vfd_println("Bauhaus-Universit[t");
  vfd_print  ("Weimar");
  vfd_new_line();

  delay(1000);
  // vfd_cls();

  //vfd_new_line();
  snprintf(dht_string, 21, "%.1f ~C    %.1f %s", t, h, "%-RH");
  vfd_print(String(dht_string));
  //Serial2.print("°^<>|,;.:-_'");
  //vfd_new_line();
  //Serial2.print("+*~`!§$%&/()=?");

  delay(5000);
  vfd_cls();
}