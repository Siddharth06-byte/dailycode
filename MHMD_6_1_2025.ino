#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <EEPROM.h>
#include <time.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <WiFiManager.h>
#include <ArduinoOTA.h>

// ***************************************************************
// ***************************************************************
// ***************************************************************

#define AWS_IOT_PUBLISH_TOPIC   "MHMD/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "MHMD/sub"

#define EEPROM_SIZE 1024
#define DATA_ADDR 0 
#define THINGNAME "MHMD"   
                      
//const char WIFI_SSID[] = "CACTUS 5CW";               
//const char WIFI_PASSWORD[] = "Admin$c@ctu$5FL";           
const char AWS_IOT_ENDPOINT[] = "a3j8q5vaeajlzm-ats.iot.ap-south-1.amazonaws.com";       

bool wasConnected=false;

//////////
const char* gitlabToken = "glpat-EDZPygYR1SDohoR2yVJD";
const char* projectPath = "IOT%2FMeasuring_Equipment_Health_Monitoring_Device_MHMD"; // URL-encoded project path
const char* versionFilePath = "OTA%2Fversion%2Etxt"; // URL-encoded file path for version file
const char* binaryFilePath = "OTA%2Ffirmware%2Ebin"; // URL-encoded file path for binary file
const char* branch = "dev";
const char* currentVersion = "1.1";
const unsigned long updateinterval = 21600000;
unsigned long lastupdatecheck = 0;
///////////
 
// ***************************************************************
// ***************************************************************
// ***************************************************************

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17  
#define PZEM_SERIAL Serial2
#define CONSOLE_SERIAL Serial
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DUE_LED_PIN 4  // D15 pin for relay control
#define YELLOW_LED_PIN 5// D15 pin for relay control
#define GREEN_LED_PIN 6  // D15 pin for relay control
#define RELAY_PIN 8  // D15 pin for relay control

 float voltage;
 float current;
 float power;
 float energy;
 float frequency ;
 float pf;

 //***
 String EquipmentID;
 String HostID;
 String jsonPayload;
 String DueDate;
 int RequiredAction;
 String CurrentDate;
 String EquipmentName;
 String EquipmentStatus;
//***
char timeStr[25];
String isoTime;
struct tm timeinfo;

struct EquipmentData {
  int gmValueBad;
  int gmValueGood;
  int requiredAction;
  char pmChecklist[50];
  char equipmentID[20];
  char equipmentName[20];
  char pmFrequency[10];
  char dueDate[15];
  char calibrationDate[15];
  char gmFrequency[10];
  char CurrentDate[10];
};

EquipmentData equipmentData;




const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;   // GMT offset for India (UTC +5:30)
const int daylightOffset_sec = 0;    // India does not observe daylight saving time
// ***************************************************************
// ***************************************************************
// ***************************************************************
 // Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";
 
// Device Certificate                                               
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUdN1CyNLl5LEL7JSjcK2Rc1kAMmAwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MTExMzEwMjgw
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMPDP+NGRj76n+H/31uy
0qY74P820SIilSDN2gSYjgFnh+erXX/1fKVSELvqCcMyp2QsKlSoZbaYmkw4yORx
8qddtv/E06UgTE7IfKN4MJ5UodbUTLNWwh+0wdLanq5MybMkP+69ZgHurzsfOTuD
t2CHcCjTaQqrogIhkzdIzuDfmYkaW2JXvPn5uSaB2JIeqirvGl1AyfmzDvx7Jd6p
GHJIYRE1V0KbdkN4bkc9vI0SBd28h0T3mebEgXbEw2GqhIaQZS9sV+K7RYQT1fUd
qgOi4nZXBrXD+EWNHVHXiL/QFzIpDYi7DeD0INrsBqjaIwU8/TMWbrbe0tlrCv0u
HVsCAwEAAaNgMF4wHwYDVR0jBBgwFoAUpi0558PKDfaTvwMlR8CWh2qAqVIwHQYD
VR0OBBYEFPIsRILamIHH564rlCvYWQaQElAAMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA/+VA3S8t6hKswF6XCCgymsCOl
QfvSLI3psGPpYoFGABSsiq/g/OA2rFJJip/QXcrrqZROLsEovySidZOXN1JPsDhd
07eURgnYJWxvw84yh8OMZv/GM3Qo0AUglm2AbWHaWHDDgHKZjbLCZifPTExYSqMU
KEyg9oASo5p72DNOzhBOGOFU/u7gE4UHzNBLKDRvDcoKPJDADZbfK7rS2Yr3jXOG
ZkL4hNrZn+QeDAjAA9HCmL4xqQb1jI1heKVTsG3QQhtJEoEZQc1evQ32maITSgdz
6/3/zAztYW7k7CCad3FtGlRuPI0UoCgZ6durCaL53x5yyhIQZRHW8C0SEw8o
-----END CERTIFICATE-----
)KEY";
 
// Device Private Key                                              
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAw8M/40ZGPvqf4f/fW7LSpjvg/zbRIiKVIM3aBJiOAWeH56td
f/V8pVIQu+oJwzKnZCwqVKhltpiaTDjI5HHyp122/8TTpSBMTsh8o3gwnlSh1tRM
s1bCH7TB0tqerkzJsyQ/7r1mAe6vOx85O4O3YIdwKNNpCquiAiGTN0jO4N+ZiRpb
Yle8+fm5JoHYkh6qKu8aXUDJ+bMO/Hsl3qkYckhhETVXQpt2Q3huRz28jRIF3byH
RPeZ5sSBdsTDYaqEhpBlL2xX4rtFhBPV9R2qA6LidlcGtcP4RY0dUdeIv9AXMikN
iLsN4PQg2uwGqNojBTz9MxZutt7S2WsK/S4dWwIDAQABAoIBADvPUJqootelDOoo
md5TUUaWrtRqtBSgyj0+h2i2D1Tgg8ZFNgjdQNrlqtUu2RK5TCkv0TMSTBkwoJu/
EamyIrf9yuZ0BVHC/21mlaYAF6KYKuYO8OwuCptTR5CbWLXaIhnIbnwLg7n9aa4/
iOXlBUbJunvOs54d+B58kZTnesV7Ikk3s7fHHgZDcZ3gGxExp2vmDN3dlgEA5EMC
SDsSaMEJsjU8/+gJTX7GU+omzDL0iPpd0g9OZ8kcVAhpiIBHRyC/6T4g2uoeL0Kq
cB4lc3uezkebmhnmjrmyhn2FSHM37KhBuGiaXC06NrGuyJCtW+KeRXLVlDm+1VZv
zHAnjUECgYEA7hQg58J+rBRNrESAcyToj3kU61VGeUjcnR/sdOxzp8jAUWzTZFft
/7xe10zVJhF8v9kruif9GPXJcS4x2wMUhOP6DC61WlQ8CiQ4erAEFlCx02/vEsaE
ZPZ+vJrA81i44TwtU8wjBCT/nJwL7SwDTbDf5qJJ5t3WYbn+3V3ezrsCgYEA0n+t
BaBZQQDcQ+TVpUkLmRvS1c28J53qsXuCkVLU86EMkJUqW0haUDwaijMToALNxchW
MB70TTfcZxLKqlz6UdGSoJMKkUCye7zPUMdMUFHO2/sXtAo+ElCPxn+TzZz9GtQC
CHBYaI4j83daz0ahaL6LI9KXe9NRM7LfqBfgEeECgYAGBAwwzwFUZ87uo2x7fFtE
BNM6fxUgy26Hqd7XtYHWo4P4ir9ssawZ4jCjUBWyuI5map5oPcpvbwaADaLE3fcO
xJP06tk6RxadSv0CXKA90BwuM43e85U01+Wr7l0hyQQi7NpYW7gmyqiK1IaYn5wV
wnpl4YPsiX55SbHe9hhDMQKBgEYAE2fFlSjawmUJmkqm6eZ2rDxReHqsNzniWt88
7ng+XFFQaY+FlhMMib7fkW0qwYc+QfsqsA9lvjPAugjBdrTyvpILk/4Nx80CUucH
/8bODb5118IVjeLFS6q1Dk/YHwtm6W4o/h1291mlnVtEn7SXeOnmxF13Po+0sk/R
8L/BAoGBAOgoIQs45JOuBz7IQqmbea9U/Ga5ff5d+YFxpi/oZEvgGgWkPs2HAn/S
sOzrIHjf2hkLXz5+6Lr1d4Bnm2pQ7fyqupqSJbmq2bQB/4zrqBc5MS3gWbK90YWB
vyODyfw2MaGJ44wkSu/B9cKJFwvK8OXJpU+Xyheim7crspT92g7K
-----END RSA PRIVATE KEY-----
)KEY";

void initializeData() {
  equipmentData.gmValueBad = 0;
  equipmentData.gmValueGood = 0;
  equipmentData.requiredAction = 6;
  strcpy(equipmentData.pmChecklist, "Null");
  strcpy(equipmentData.equipmentID, "CI-TC-00E5EC");
  strcpy(equipmentData.equipmentName, "TRH");
  strcpy(equipmentData.pmFrequency, "Null");
  strcpy(equipmentData.dueDate, "13-12-2024");
  strcpy(equipmentData.calibrationDate, "3-12-2024");
  strcpy(equipmentData.gmFrequency, "Null");
  strcpy(equipmentData.CurrentDate,"Null");
}


WiFiManager wifimanager;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
void reconnect();
void printEquipmentData();
String getCurrentTime();
time_t parseDate(const String& dateStr);
int calculateDateDifference(String DueDate,String CurrentDate);
void CheckDueDate();
void setupOTA();

void connectAWS()
{
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
// 
//  Serial.println("Connecting to Wi-Fi");
// 
//  while (WiFi.status() != WL_CONNECTED)
//  {
//    delay(500);
//    Serial.print(".");
//  }
  wifimanager.resetSettings();
  wifimanager.autoConnect("MHMD_Device_Config");
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
  client.setCallback(callback);
    client.setBufferSize(2048);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

// ***************************************************************
// ***************************************************************
// ***************************************************************
 
void setup() {
  Serial.begin(9600);
   EEPROM.begin(EEPROM_SIZE);

  initializeData();
  saveToEEPROM();
  memset(&equipmentData, 0, sizeof(EquipmentData));
  readFromEEPROM();
  printEquipmentData();
  
  connectAWS();
  // Initialize relay pin
  pinMode(DUE_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(DUE_LED_PIN, LOW);  // Start with the LED off
  digitalWrite(YELLOW_LED_PIN, LOW);  // Start with the LED off
  digitalWrite(GREEN_LED_PIN, LOW);  // Start with the LED off
  digitalWrite(RELAY_PIN, LOW);  // Start with the relay off

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Loop forever if initialization fails
  }
  delay(2000);
  display.clearDisplay();
  
  // Display static text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Cavin Infotech");
  display.display();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    
  }
  isoTime = getCurrentTime();
    

  //***
    HostID=WiFi.macAddress();
    Serial.println(HostID);
    sendHostID(HostID.c_str());
   //***  
    
  setupOTA();
  
}

void displayData(const char* label, float value, int y_position) {
  display.setCursor(0, y_position);
  display.print(label);
  display.print(value);
}
// ***************************************************************
// ***************************************************************
// ***************************************************************

void publishMessage()
{
  
  StaticJsonDocument<200> doc;
  doc["EquipmentID"]=EquipmentID;
  doc["HostID"]=HostID;
  doc["voltage"] = voltage;
  doc["current"] = current; 
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["pf"] = pf;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 // Serial.println("Lastest Reading Stored!");
}
 
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}


void callback(char* topic, byte* payload, unsigned int length) {
    String response;
    for (int i = 0; i < length; i++) {
        response += (char)payload[i];
    }
    Serial.print("Received response: ");
    Serial.println(response);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.f_str());
        return;
    }
   
  if(doc.containsKey("EquipmentID")&&doc.containsKey("HostID"))
  {
    EquipmentID = doc["EquipmentID"].as<String>();
    EquipmentName=doc["EquipmentName"].as<String>();
    Serial.println(EquipmentID);
    sendEquipmentID(String(EquipmentID).c_str());  
  }

  if (doc.containsKey("DueDate") && doc.containsKey("RequiredAction")) 
  {
    DueDate=doc["DueDate"].as<String>();
    EquipmentStatus=doc["Status"].as<String>();
    Serial.println(DueDate);
     int firstDash = DueDate.indexOf('-');
    int secondDash = DueDate.indexOf('-', firstDash + 1);
 
    if (firstDash > 0 && secondDash > firstDash) {
        String year = DueDate.substring(0, firstDash);
        String month = DueDate.substring(firstDash + 1, secondDash);
        String day = DueDate.substring(secondDash + 1);
 
        // If year is 4 digits and day has 1 or 2 digits, assume yyyy-m-d format
        if (year.length() == 4 && day.length() <= 2) {
            // Convert to d-m-yyyy format
            DueDate = day + "-" + month + "-" + year;
        }
    }
   
    Serial.println("Formatted DueDate: " + DueDate);
    RequiredAction=doc["RequiredAction"].as<int>();
    Serial.println(RequiredAction);



    
  equipmentData.gmValueBad = doc["GmValue/Bad"];
  equipmentData.gmValueGood = doc["GmValue/Good"];
  equipmentData.requiredAction =doc["RequiredAction"].as<int>();
  strcpy(equipmentData.pmChecklist, doc["PmChecklist"]);
  strcpy(equipmentData.equipmentID, doc["EquipmentID"]);
  strcpy(equipmentData.equipmentName, doc["EquipmentName"]);
  strcpy(equipmentData.pmFrequency,doc["PmFrequency"] );
  strcpy(equipmentData.dueDate, DueDate.c_str());
  strcpy(equipmentData.calibrationDate, doc["CalibrationDate"]);
  strcpy(equipmentData.gmFrequency,doc["GmFrequency"]);
  saveToEEPROM();

  
  }

  if (response.indexOf("Host_Master") != -1) {
    sendHostID(HostID.c_str()); // Call the function to send HostID
  }
  if (response.indexOf("Equipment_Master") != -1) {
    sendEquipmentID(String(EquipmentID).c_str()); // Call the function to send EquipmentID
  }
   
}


void sendHostID(const char* HostID) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    String message = String("{\"HostID\": \"") + HostID + "\"}";
    //Serial.println(message);
    client.publish(AWS_IOT_PUBLISH_TOPIC, message.c_str());
    Serial.println("Host ID sent!");
}

void sendEquipmentID(const char* EquipmentID) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    String message = String("{\"EquipmentID\": \"") + EquipmentID + "\"}";
     //Serial.println(message);
    client.publish(AWS_IOT_PUBLISH_TOPIC, message.c_str());
    Serial.println("EquipmentID sent!");
}

void SendSensorData(String jsonpayload) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    //Serial.println(jsonpayload);
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonpayload.c_str());
    Serial.println("Lastest Reading Stored!");
}

void sendstatus(String Status) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    String message = String("{\"Status\": \"") + Status + "\",\"EquipmentID\": \"" + EquipmentID + "\"}";
    //Serial.println(message);
    client.publish(AWS_IOT_PUBLISH_TOPIC, message.c_str());
    Serial.println("Equipment Status Send!");
}





String payloaddata()
{

  StaticJsonDocument<256> doc;

  // Populate the JSON document
  doc["EquipmentID"] = EquipmentID;
  doc["HostID"] = HostID;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["pf"] = pf;
  doc["TimeStamp"] = isoTime;

  // Serialize JSON to a String
  String jsonpayload;
  serializeJson(doc, jsonpayload);
  //Serial.println(jsonpayload);
    return jsonpayload;
  
}


void saveToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, equipmentData); // Save the entire structure
  EEPROM.commit();
  EEPROM.end();
}

void readFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, equipmentData); // Read the entire structure
  EEPROM.end();
}




void reconnect() {
    while (!client.connected()) {
      offlineCheckDueDate();
      checkWiFi();
        Serial.print("Attempting MQTT connection...");
        // Try to connect to the MQTT broker
        if (client.connect("ArduinoClient")) { // Replace "ArduinoClient" with a unique client ID
            Serial.println("Connected to MQTT broker!");
            
            // Subscribe to the desired topic upon successful connection
            client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
            sendEquipmentID(String(equipmentData.equipmentID).c_str());
            
        } else {
            Serial.print("Failed to connect. rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
           // delay(5000); // Wait before retrying
        }
    }
}

//***************************************************************
//***************************************************************
//***************************************************************

void loop() 
{

  checkWiFi();
   if (millis() - lastupdatecheck > updateinterval) {
        lastupdatecheck = millis();
        checkForUpdate(); // Check for updates
    }

  if (wasConnected == false)
  {
    offlineCheckDueDate();
  }
  else {
 
  isoTime = getCurrentTime();
   
  if(!(DueDate.isEmpty()))
  {
    Serial.println(DueDate);
    CheckDueDate();
  }
   
  // Read sensor data
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();
  
  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf)) {
    //CONSOLE_SERIAL.println("Error reading data from PZEM sensor.");
  voltage = 230.0;
  current = 2.00;
  power = 0.5;
  energy = 34.25;
  frequency = 50.0;
  pf = 1.0;

////  // Print sensor values to console
////    CONSOLE_SERIAL.print("Voltage: "); CONSOLE_SERIAL.println(voltage);
////    CONSOLE_SERIAL.print("Current: "); CONSOLE_SERIAL.println(current);
////    CONSOLE_SERIAL.print("Power: "); CONSOLE_SERIAL.println(power);
////    CONSOLE_SERIAL.print("Energy: "); CONSOLE_SERIAL.println(energy, 3);
////    CONSOLE_SERIAL.print("Frequency: "); CONSOLE_SERIAL.println(frequency, 1);
////    CONSOLE_SERIAL.print("PF: "); CONSOLE_SERIAL.println(pf);
//
//    // Update OLED display
//    display.clearDisplay();
//    displayData("Voltage: ", voltage, 0);
//    displayData("Current: ", current, 10);
//    displayData("Power: ", power, 20);
//    displayData("Energy: ", energy, 30);
//    displayData("Freq: ", frequency, 40);
//    displayData("PF: ", pf, 50);
//    display.display();
//    
  }
//else {
//    // Print sensor values to console
////    CONSOLE_SERIAL.print("Voltage: "); CONSOLE_SERIAL.println(voltage);
////    CONSOLE_SERIAL.print("Current: "); CONSOLE_SERIAL.println(current);
////    CONSOLE_SERIAL.print("Power: "); CONSOLE_SERIAL.println(power);
////    CONSOLE_SERIAL.print("Energy: "); CONSOLE_SERIAL.println(energy, 3);
////    CONSOLE_SERIAL.print("Frequency: "); CONSOLE_SERIAL.println(frequency, 1);
////    CONSOLE_SERIAL.print("PF: "); CONSOLE_SERIAL.println(pf);
//
//    // Update OLED display
//    display.clearDisplay();
//    displayData("Voltage: ", voltage, 0);
//    displayData("Current: ", current, 10);
//    displayData("Power: ", power, 20);
//    displayData("Energy: ", energy, 30);
//    displayData("Freq: ", frequency, 40);
//    displayData("PF: ", pf, 50);
//    display.display();
//  }
//
//
////  // Toggle relay every 2 seconds
////  static unsigned long lastToggleTime = 0;
////  unsigned long currentTime = millis();
////
////  if (currentTime - lastToggleTime >= 2000) {  // Check if 2 seconds have passed
////    lastToggleTime = currentTime;
////
////    // Toggle relay state
////    static bool relayState = false;
////    relayState = !relayState;  // Switch the relay state
////    digitalWrite(DUE_LED_PIN, relayState ? HIGH : LOW);  // Turn relay on/off
////    digitalWrite(GREEN_LED_PIN, relayState ? HIGH : LOW);  // Turn relay on/off
////    digitalWrite(YELLOW_LED_PIN, relayState ? HIGH : LOW);  // Turn relay on/off
////    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);  // Turn relay on/off
////  }
    if(EquipmentID!=0)
      publishMessage();
  
    
    
    
    if(EquipmentID!=0)
    {
    jsonPayload=payloaddata();
    SendSensorData(jsonPayload);
    }


    memset(&equipmentData, 0, sizeof(EquipmentData));
    readFromEEPROM();
    printEquipmentData();
    
    client.loop();
    ArduinoOTA.handle();

  }
 
}

 
 //currentDate
 String getCurrentTime()
 {
   if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "0";
  }
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  Serial.println(timeStr);
  

  char dateStr[11];  // "YYYY-MM-DD" is 10 characters + null terminator
  strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo); // Only the date part
  
  CurrentDate = String(dateStr); // Store date in CurrentDate variable
  strcpy(equipmentData.CurrentDate, CurrentDate.c_str());
  saveToEEPROM();
  Serial.println(CurrentDate);
  

  return String(timeStr);

}


///DueDate
void CheckDueDate()
{
  int diffdate=calculateDateDifference(DueDate,CurrentDate);

   digitalWrite(DUE_LED_PIN, LOW); 
 // digitalWrite(YELLOW_LED_PIN, LOW);  
  //digitalWrite(GREEN_LED_PIN, LOW);
  
   Serial.println(diffdate);
      
  if (diffdate<=0)
  {
    digitalWrite(DUE_LED_PIN,HIGH); 
    digitalWrite(RELAY_PIN,LOW);
    EquipmentStatus="Over Due";
    sendstatus(EquipmentStatus); 
    Serial.println("Red led");   
  }

  else if(diffdate >=0 && diffdate > RequiredAction)
  {
    digitalWrite(DUE_LED_PIN, LOW); 
    digitalWrite(RELAY_PIN,HIGH);
    EquipmentStatus="Good";
    sendstatus(EquipmentStatus);
    Serial.println("Green led");
  }
  else if (diffdate >=0 && diffdate <= RequiredAction)
  {
    digitalWrite(DUE_LED_PIN, HIGH); 
    delay(50);
    digitalWrite(DUE_LED_PIN, LOW); 
    delay(50);
    digitalWrite(RELAY_PIN,HIGH);
    EquipmentStatus="Due date above to reach";
    sendstatus(EquipmentStatus);
    Serial.println("Yellow led");
  }
  
}


///calculateDateDifference
int calculateDateDifference(String DueDate,String CurrentDate)
{
  
  time_t dueTime = parseDate(DueDate);
  time_t currentTime = parseDate(CurrentDate);

  if (dueTime == -1 || currentTime == -1) {
    Serial.println("Error: Invalid date");
    return -1; // Error in parsing the date
  }

  // Calculate difference in days
  double secondsDifference = difftime(dueTime, currentTime);
  int daysDifference = secondsDifference / (60 * 60 * 24); // Convert seconds to days
  
  return daysDifference;
}  



//parse
time_t parseDate(const String& dateStr) 
{
  int day, month, year;

  // Parse the date in DD-MM-YY format
  sscanf(dateStr.c_str(), "%d-%d-%d", &day, &month, &year);

  // Adjust the year from 2 digits to 4 digits (assuming 2000-2099)
  year += 2000;
  
  // Set up tm structure
  struct tm timeinfo = {};
  timeinfo.tm_year = year - 1900;  // Year since 1900
  timeinfo.tm_mon = month - 1;     // Month (0-11)
  timeinfo.tm_mday = day;          // Day of the month

  // Convert tm structure to time_t
  return mktime(&timeinfo);
}



void printEquipmentData() 
{
  Serial.println("Equipment Data:");
  Serial.print("gmValueBad: "); Serial.println(equipmentData.gmValueBad);
  Serial.print("gmValueGood: "); Serial.println(equipmentData.gmValueGood);
  Serial.print("requiredAction: "); Serial.println(equipmentData.requiredAction);
  Serial.print("pmChecklist: "); Serial.println(equipmentData.pmChecklist);
  Serial.print("equipmentID: "); Serial.println(equipmentData.equipmentID);
  Serial.print("equipmentName: "); Serial.println(equipmentData.equipmentName);
  Serial.print("pmFrequency: "); Serial.println(equipmentData.pmFrequency);
  Serial.print("dueDate: "); Serial.println(equipmentData.dueDate);
  Serial.print("calibrationDate: "); Serial.println(equipmentData.calibrationDate);
  Serial.print("gmFrequency: "); Serial.println(equipmentData.gmFrequency);
  Serial.print("currentDate: "); Serial.println(equipmentData.CurrentDate);
}



void offlineCheckDueDate()
{
   
    checkWiFi();
    memset(&equipmentData, 0, sizeof(EquipmentData));
    readFromEEPROM();
 
  int diffdate=calculateDateDifference(equipmentData.dueDate,equipmentData.CurrentDate);
 
   digitalWrite(DUE_LED_PIN, LOW);
//  digitalWrite(YELLOW_LED_PIN, LOW);  
//  digitalWrite(GREEN_LED_PIN, LOW);
 
   Serial.println(diffdate);
     
  if (diffdate<=0)
  {
    digitalWrite(DUE_LED_PIN,HIGH);
    digitalWrite(RELAY_PIN,LOW);
    //EquipmentStatus="Over Due";
    //sendstatus(EquipmentStatus);
    Serial.println("offline Red led");  
  }
 
  else if(diffdate >=0 && diffdate > equipmentData.requiredAction)
  {
     digitalWrite(DUE_LED_PIN,LOW);
    digitalWrite(RELAY_PIN,HIGH);
    //EquipmentStatus="Good";
    //sendstatus(EquipmentStatus);
    Serial.println("Green led");
  }
  else if (diffdate >=0 && diffdate <= equipmentData.requiredAction)
  {
   
    digitalWrite(DUE_LED_PIN,HIGH);
     delay(100);
      digitalWrite(DUE_LED_PIN,LOW);
      delay(100);
    digitalWrite(RELAY_PIN,HIGH);
    //EquipmentStatus="Due date above to reach";
    //sendstatus(EquipmentStatus);
    Serial.println("Yellow led");
  }
 
}
////////////////////////////////////////////////////////////////////////
void setupOTA() {
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
 
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
 
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
 
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
 
  ArduinoOTA.begin();
  Serial.println("Ready for OTA updates");
}








///////////////////////////////////////////////////////////////////////////////////////

void checkForUpdate() {
 if (WiFi.status() == WL_CONNECTED) {
   String fetchedVersion = fetchFileContent(projectPath, versionFilePath, branch, gitlabToken);
   Serial.println("Fetched version:");
   Serial.println(fetchedVersion);

   if (fetchedVersion != currentVersion) {
     Serial.println("Version mismatch. Starting OTA update...");
     performOTAUpdate(projectPath, binaryFilePath, branch, gitlabToken);
   } else {
     Serial.println("Version matches. No update needed.");
   }
 }
}

String fetchFileContent(const char* projectPath, const char* filePath, const char* branch, const char* token) {
 HTTPClient http;
 String url = String("https://scm.ckdigital.in/api/v4/projects/") + projectPath + "/repository/files/" + filePath + "/raw?ref=" + branch;
 http.begin(url);
 http.addHeader("PRIVATE-TOKEN", token);

 int httpResponseCode = http.GET();
 String payload = "";

 if (httpResponseCode > 0) {
   payload = http.getString();
 } else {
   Serial.print("Error on HTTP request: ");
   Serial.println(httpResponseCode);
 }

 http.end();
 return payload;
}

void performOTAUpdate(const char* projectPath, const char* filePath, const char* branch, const char* token) {
 HTTPClient http;
 String url = String("https://scm.ckdigital.in/api/v4/projects/") + projectPath + "/repository/files/" + filePath + "/raw?ref=" + branch;
 http.begin(url);
 http.addHeader("PRIVATE-TOKEN", token);

 int httpResponseCode = http.GET();

 if (httpResponseCode > 0) {
   int contentLength = http.getSize();
   bool canBegin = Update.begin(contentLength);

   if (canBegin) {
     Serial.println("Begin OTA. This may take a while...");
     WiFiClient *client = http.getStreamPtr();
     size_t written = Update.writeStream(*client);

     if (written == contentLength) {
       Serial.println("Written : " + String(written) + " successfully");
     } else {
       Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
     }

     if (Update.end()) {
       Serial.println("OTA done!");
       if (Update.isFinished()) {
         Serial.println("Update successfully completed. Rebooting.");
         ESP.restart();
       } else {
         Serial.println("Update not finished? Something went wrong!");
       }
     } else {
       Serial.println("Error Occurred. Error #: " + String(Update.getError()));
     }
   } else {
     Serial.println("Not enough space to begin OTA");
   }
 } else {
   Serial.print("Error on HTTP request: ");
   Serial.println(httpResponseCode);
 }

 http.end();
}


unsigned long previousMillis = 0;
unsigned long timerInterval=3000;
void checkWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= timerInterval) {
            previousMillis = currentMillis;
            Serial.println("WiFi disconnected. Attempting to reconnect...");
            WiFi.disconnect();
            delay(100);
            WiFi.reconnect(); // Attempt to reconnect using WiFi library
            delay(500);
            if (WiFi.status() != WL_CONNECTED) {
                //wifiManager.setConfigPortalTimeout(30);
               // wifiManager.autoConnect("CK_Device_Config");
                Serial.println("Reconnection failed.");
                wasConnected = false; // Update the connection flag
               // delay(5000);
            }
        }
    } else if (!wasConnected) {
        Serial.println("Reconnected to WiFi!");
       // initializeWebServer();
        wasConnected = true; // Update the connection status
    }
}


 