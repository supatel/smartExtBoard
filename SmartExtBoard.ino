

/*
 * Created by Shreyas Patel
 * 
 * Email: shreyaspatel21789@gmail.com
 * 
 * Github: https://github.com/
 * 
 * Copyright (c) 2020 
 * 
 * This code is for personal use of watering household
 * plants
 *
 * IMPORTANT NOTE:
 * To run this script, your need to
 *  1) Enter your WiFi SSID and PSK below this comment
 *  2) Make sure to have certificate data available. You will find a
 *     shell script and instructions to do so in the library folder
 *     under extras/
 *
 * This script will install an HTTPS Server on your ESP32 with the following
 * functionalities:
 *  - Show simple page on web server root
 *  - 404 for everything else
 * The server will be run in a separate task, so that you can do your own stuff
 * in the loop() function.
 * Everything else is just like the Static-Page example
 */


/** Check if we have multiple cores */
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Include certificate data (see note above)
#include "cert.h"
#include "private_key.h"

// We will use wifi
#include <WiFi.h>

// Includes for the server
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ESPWebServerSecure.hpp>
#include <ESPmDNS.h>
#include <Update.h>

// The HTTPS Server comes in a separate namespace. For easier use, include it here.
using namespace httpsserver;

// Create an SSL certificate object from the files included above
SSLCert cert = SSLCert(
  example_crt_DER, example_crt_DER_len,
  example_key_DER, example_key_DER_len
);

// Create an SSL-enabled server that uses the certificate
//HTTPSServer secureServer = HTTPSServer(&cert);
ESPWebServerSecure server(443);

#define APMODE 1
//1. Change the following info
#if APMODE
#define WIFI_SSID "SmartExtensionBoard1"
#define WIFI_PASSWORD "12345678"
#else
#define WIFI_SSID "JioFi3_514C3A"
#define WIFI_PASSWORD "38kfuw53n0"
#endif

const char* host = "esp32";


//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = (5*3600) + 1800; //GMT + 5:30
//const int   daylightOffset_sec = 0;


// Define the pin the relay IN pin is connected to.
const int relay = 25;
const int relay2 = 26;
const int relay3 = 27;

unsigned char relay_cmd = 0;
unsigned char relay_cmd2 = 0;
unsigned char relay_cmd3 = 0;
//unsigned char prev_relay_cmd = 0;
//unsigned char remote_on_flag = 0;
//char timestampString[34] = "October 18 2020 17:45:18, Sunday";
void(* resetFunc) (void) = 0;//declare reset function at address 0

//void configModeCallback (WiFiManager *myWiFiManager) {
//  Serial.println("Entered config mode");
//  Serial.println(WiFi.softAPIP());
//  //if you used auto generated SSID, print it
//  Serial.println(myWiFiManager->getConfigPortalSSID());
//}

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED
//WebServer server(HTTP_PORT);

/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>ESP32 Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;

void setup()
{
  int wifiRetry = 0;
  Serial.begin(115200);

    //define relay output
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
    pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH);
    pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);
 
#if APMODE
  // create WiFi network
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
#else
  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
    wifiRetry++;
    if(wifiRetry > 300)
    {
      resetFunc(); //call reset 
  }
  }
//    //WiFiManager
//  //Local intialization. Once its business is done, there is no need to keep it around
//  WiFiManager wifiManager;
//  //reset settings - for testing
//  //wifiManager.resetSettings();
// 
//  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
//  wifiManager.setAPCallback(configModeCallback);
//  
//  //fetches ssid and pass and tries to connect
//  //if it does not connect it starts an access point with the specified name
//  //here  "AutoConnectAP"
//  //and goes into a blocking loop awaiting configuration
//  if(!wifiManager.autoConnect("esp8266 mischiantis test")) {
//    Serial.println("failed to connect and hit timeout");
//    //reset and try again, or maybe put it to deep sleep
////    ESP.reset();
//    resetFunc(); //call reset 
//    delay(1000);
//  } 
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
#endif

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  MDNS.addService("http", "tcp", 80);
  
  Serial.println(MDNS.hostname(1));
  Serial.print("mDNS responder started");
  //init and get the time
//  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//  printLocalTime();
  
  server.setServerKeyAndCert(example_key_DER, example_key_DER_len, 
                            example_crt_DER, example_crt_DER_len  );
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.on("/relay1", handleRelay1);
  server.on("/relay2", handleRelay2);
  server.on("/relay3", handleRelay3);
  server.begin();
}

void loop(void) {
  static unsigned int mscount = 0;
  server.handleClient();
  delay(1);
  mscount++;
  if(mscount >= 450)
  {
    mscount = 0;
//    printLocalTime();
//    if(0)
//    {
//      //Success
//      relay_cmd = ~relay_cmd;
//      Serial.print("Get int data success, int = ");
//      Serial.println(relay_cmd);
////      if(relay_cmd != prev_relay_cmd)
//      {
//        if(relay_cmd == 1)
//        {
////          remote_on_flag = 1;
//          Serial.print("Relay On ");
//          digitalWrite(relay, LOW);
//        }
//        else
//        {
//          Serial.print("Relay Off ");
//          digitalWrite(relay, HIGH);
//        }  
//      }
////      prev_relay_cmd = relay_cmd;
//  
//    }
  }
}

void handleRelay1() {
    //Success
    String out = "";
  relay_cmd = ~relay_cmd;
//  Serial.print("relay = ");
//  Serial.println(relay_cmd);
  //if(relay_cmd != prev_relay_cmd)
  {
    if(relay_cmd == 255)
    {
//      remote_on_flag = 1;
      out = "Relay On"; 
      Serial.print("Relay1 On ");
      digitalWrite(relay, LOW);
    }
    else
    {
      out = "Relay1 Off"; 
      Serial.print("Relay1 Off ");
      digitalWrite(relay, HIGH);
    }  
  }
//  prev_relay_cmd = relay_cmd;
  
  server.send(200, "text/plain", out);
}
void handleRelay2() {
    //Success
    String out = "";
  relay_cmd2 = ~relay_cmd2;
//  Serial.print("relay = ");
//  Serial.println(relay_cmd);
  //if(relay_cmd != prev_relay_cmd)
  {
    if(relay_cmd2 == 255)
    {
//      remote_on_flag = 1;
      out = "Relay2 On"; 
      Serial.print("Relay2 On ");
      digitalWrite(relay2, LOW);
    }
    else
    {
      out = "Relay2 Off"; 
      Serial.print("Relay2 Off ");
      digitalWrite(relay2, HIGH);
    }  
  }
//  prev_relay_cmd = relay_cmd;
  
  server.send(200, "text/plain", out);
}
void handleRelay3() {
    //Success
    String out = "";
  relay_cmd3 = ~relay_cmd3;
//  Serial.print("relay = ");
//  Serial.println(relay_cmd);
  //if(relay_cmd != prev_relay_cmd)
  {
    if(relay_cmd3 == 255)
    {
//      remote_on_flag = 1;
      out = "Relay3 On"; 
      Serial.print("Relay3 On ");
      digitalWrite(relay3, LOW);
    }
    else
    {
      out = "Relay3 Off"; 
      Serial.print("Relay3 Off ");
      digitalWrite(relay3, HIGH);
    }  
  }
//  prev_relay_cmd = relay_cmd;
  
  server.send(200, "text/plain", out);
}
