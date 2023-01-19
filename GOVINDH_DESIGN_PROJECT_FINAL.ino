#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <TinyGPS++.h>

#include <HardwareSerial.h>
HardwareSerial SIM800L(2); // Use hardware serial port 2 Rx of module to 17,Tx to 16
HardwareSerial neogps(0);
TinyGPSPlus gps;
#define sensor 2

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "add_ssid"
#define WIFI_PASSWORD "add_password"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAaAEBwR-kip9g6mYLobTU4AnE1PJnb7Jc"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://govindh-project-ca9d3-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count=1;
int count1= 0;
bool signupOK = false;

const String PHONE = "+91**********";// set the phone number here*************************************************
int buzzer_timer = 0;
bool alarm1 = false;
boolean send_alert_once = true;
const float maxDistance = 30;// set the radius of the fence here in meters*******************************************
float initialLatitude = 16.3733646;     //latitude                  // set parent locations here ****************************************************
float initialLongitude = 80.5264669;   //longitude
float latitude, longitude;
void getGps(float& latitude, float& longitude);




void getGps(float& latitude, float& longitude)
{
  // Can take up to 60 seconds
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }
  }
  
  if (newData) //If newData is true
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    newData = false;
  }
  else {
    Serial.println("No GPS data is available");
    latitude = 0;
    longitude = 0;
  }
}





void sendAlert()
{
  //return;
  String sms_data;
  sms_data = "Alert! The object is outside the fense.\r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(latitude) + "," + String(longitude);

  //return;
  SIM800L.print("AT+CMGS=\""+PHONE+"\"\r");
  delay(1000);
  SIM800L.print(sms_data);
  delay(100);
  SIM800L.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
  delay(1000);
  Serial.println("SMS Sent Successfully.");
  
}

float getDistance(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc=0;
  float dist_calc2=0;
  float diflat=0;
  float diflon=0;

  // Calculations
  diflat  = radians(flat2-flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2)-(flon1));

  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
  dist_calc2 = cos(flat1);
  dist_calc2*=cos(flat2);
  dist_calc2*=sin(diflon/2.0);
  dist_calc2*=sin(diflon/2.0);
  dist_calc +=dist_calc2;

  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));
  
  dist_calc*=6371000.0; //Converting to meters

  return dist_calc;
}



void setup(){
  Serial.begin(115200);
   SIM800L.begin(9600); 
   neogps.begin(9600);
pinMode(sensor,INPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop(){
  if(digitalRead(sensor)==HIGH)
{
  Serial.print("leash is connected");
  delay(1000);
   Serial.println("\n");

   if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "pet/leash status", count)){         // path to fire base is   pet/leash status
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

 }


else
{
  Serial.print("leash is not connected");    
   Serial.println("\n");
    if (Firebase.RTDB.setInt(&fbdo, "pet/leash status", count1))
     {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
     else 
         {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("not connected to wifi");
    SIM800L.println("AT"); // Check if SIM800L is ready to receive commands
  delay(1000);
  if(SIM800L.find("OK")) {
    SIM800L.println("AT+CMGF=1"); // Set SMS mode to text mode
    delay(1000);
    SIM800L.print("AT+CMGS=\""+PHONE+"\"\r");
  delay(1000);
    getGps(latitude, longitude);
     float distance = getDistance(latitude, longitude, initialLatitude, initialLongitude);
  if(distance > maxDistance)
  {
     sendAlert();
  }
  SIM800L.print("ALERT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r");
SIM800L.print("leash is disconnected outside wifi range but inside fence\r");
    
    SIM800L.println((char)26); // Send message (CTRL+Z)
     delay(1000);
  }
  }
     }
    }
  
