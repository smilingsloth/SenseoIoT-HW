#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <PubSubClient.h>

unsigned long lastTime = 0;

// PubSubClient config
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// Serial Communication
String command; // String input from command prompt
char inByte; // Byte input from command prompto

char statusMessage[80];
boolean serialCommandReady = false;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "iot.eclipse.org";
char mqtt_port[6] = "1883";


char mqtt_subs_topic[40] = "larstest2/comm";
char mqtt_publ_topic[40] = "larstest2/stat";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("configmodeOn");
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    //clean FS, for testing
    //SPIFFS.format();
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_subs_topic, json["mqtt_subs_topic"]);
          strcpy(mqtt_publ_topic, json["mqtt_publ_topic"]);


        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_subs_topic("Subscribe Topic", "subs topic", mqtt_subs_topic, 40);
  WiFiManagerParameter custom_mqtt_publ_topic("Publish Topic", "publ topic", mqtt_publ_topic, 40);


  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(configModeCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_subs_topic);
  wifiManager.addParameter(&custom_mqtt_publ_topic);


  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    //ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  //tell config mode is off
  Serial.println("configmodeOff");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_subs_topic, custom_mqtt_subs_topic.getValue());
  strcpy(mqtt_publ_topic, custom_mqtt_publ_topic.getValue());


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_subs_topic"] = mqtt_subs_topic;
    json["mqtt_publ_topic"] = mqtt_publ_topic;


    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  // Begin Pubsub config
  //  unsigned short int mp2 = int(mqtt_port);
  int mp = atoi(mqtt_port);
  //uint16_t mp = uint16_t(mp2);
  //const int mp = mp2;
  Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  Serial.println("Port:");
  Serial.println(mp);
  Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

  client.setServer(mqtt_server, mp);
  client.setCallback(callback);

}
// PubSub callback
void callback(char* topic, byte* payload, unsigned int length) {
  String rcv;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    rcv.concat((char)payload[i]);
  }
  Serial.println(rcv);

  if (rcv.equalsIgnoreCase("reset")) {
    SPIFFS.format();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(1000);
    ESP.reset();
    //ESP.restart();
    delay(5000);
  }
  if (rcv.equalsIgnoreCase("makeLarge")) {
    Serial.println("makeLarge");
  }
  if (rcv.equalsIgnoreCase("makeSmall")) {
    Serial.println("makeSmall");
  }
  if (rcv.equalsIgnoreCase("status")) {
    Serial.println("status");
  }
  /*
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
    } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  */
}

void reconnect() {
  // Loop until we're reconnected
  int counter = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_publ_topic, "hello world");
      // ... and resubscribe
      client.subscribe(mqtt_subs_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      counter ++;

    }
    if (counter >= 3) {
      SPIFFS.format();
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      delay(1000);
      ESP.reset();
      delay(5000);
    }
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }


  handleSerial();

  /*long now = millis();
    if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_publ_topic, msg);
    }*/
  if (millis() - lastTime > 2000 && client.connected()) {
    char m[50];
    snprintf (m, 50, "heartbeat #%ld", millis());
    client.publish(mqtt_publ_topic, m);
    Serial.println(m);
    lastTime = millis();
  }
  //Serial.println("alive");
  //delay(500);*/

  client.loop();
  delay(50);

}

// Handle serial commands
void handleSerial() {
  
  char msg[100];
  if (Serial.available() > 0) {
    inByte = Serial.read();
    // only input if a letter, number, =,?,+ are typed!
    //if ((inByte >= 65 && inByte <= 90) || (inByte >= 97 && inByte <= 122) || (inByte >= 48 &&     inByte <= 57) || inByte == 43 || inByte == 61 || inByte == 63) {
      command.concat(inByte);
    //}
  }
  if (inByte == 10 || inByte == 13) {

    inByte = 0;
    //command="";
    serialCommandReady = true;
  }
  // check if error, report or andwer to status
  // new commands
if (serialCommandReady){
  Serial.print("received: ");
  Serial.println(command);
  if (command.startsWith("e:")) {
    // clear command
    Serial.println("error received");
    String msgS = command.substring(2);
    msgS.toCharArray(statusMessage, 80);
    char m[100];
    snprintf (m, 100, "error: %s", statusMessage);
    client.publish(mqtt_publ_topic, m);
    command = "";
  }
  else if (command.startsWith("s:")) {
    Serial.println("report received");
    String msgS = command.substring(2);
    msgS.toCharArray(statusMessage, 80);
    char m[100];
    snprintf (m, 100, "report: %s", statusMessage);
    client.publish(mqtt_publ_topic, m);
    // clear command
    command = "";
  }
  else if (command.startsWith("lid")){
    Serial.println("status received");
    String msgS = command.substring(2);
    msgS.toCharArray(statusMessage, 80);
    char m[100];
    snprintf (m, 100, "status: %s", statusMessage);
    client.publish(mqtt_publ_topic, m);
    command = "";
  }
  serialCommandReady=false;
  command="";
}


}
