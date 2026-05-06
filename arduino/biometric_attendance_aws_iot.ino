/*

 IoT-Based Biometric Attendance System
----------------------------------------------------
 Hardware  : ESP8266 NodeMCU + R307 Fingerprint Sensor
 Cloud     : AWS IoT Core
 Protocol  : MQTT

 Developed By:
    - Anagha R S
    - Anna Sebastian

 Guided By:
    Dr. Bijoy A Jose

*/


// Library Imports

#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"


// Time Configuration

#define TIME_ZONE -5


// AWS IoT MQTT Topics

#define AWS_IOT_PUBLISH_TOPIC   "user/fingerprint/response"
#define AWS_IOT_SUBSCRIBE_TOPIC "user/fingerprint/request"


// AWS IoT Client Setup

WiFiClientSecure net;

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

PubSubClient client(net);


// Fingerprint Sensor Setup

SoftwareSerial mySerial(D2, D1); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);


// Enrollment Configuration

int currentId = 1;
const int maxId = 127;

bool enrollmentSuccess = false;


// Time Variables

time_t now;
time_t nowish = 1510592825;


// Function Declarations
void connectAWS();
void messageReceived(char *topic, byte *payload, unsigned int length);
void publishFingerprintId(int fingerprint_id);
void enrollFingerprint();
bool convertToTemplate();
void NTPConnect(void);


// Configure NTP Time

void NTPConnect(void)
{
    Serial.print("Configuring SNTP Time");

    configTime(
        TIME_ZONE * 3600,
        0 * 3600,
        "pool.ntp.org",
        "time.nist.gov"
    );

    now = time(nullptr);

    while (now < nowish)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }

    Serial.println(" Done!");

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    Serial.print("Current Time: ");
    Serial.print(asctime(&timeinfo));
}


// Handle Incoming MQTT Messages

void messageReceived(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");

    String receivedMessage;

    for (int i = 0; i < length; i++)
    {
        receivedMessage += (char)payload[i];
    }

    Serial.println(receivedMessage);

    // Parse incoming JSON message
    StaticJsonDocument<512> doc;

    DeserializationError error =
        deserializeJson(doc, receivedMessage);

    if (error)
    {
        Serial.println("Failed to parse incoming MQTT message.");
        return;
    }

    // Check for enrollment request
    const char *username = doc["username"];

    if (username && strcmp(username, "enroll") == 0)
    {
        Serial.println("Enrollment request received.");
        enrollFingerprint();
    }
    else
    {
        Serial.println("Invalid or unrecognized request.");
    }
}


// Connect ESP8266 to AWS IoT

void connectAWS()
{
    delay(3000);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.println();
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println();
    Serial.println("WiFi Connected Successfully");

    // Configure Time
    NTPConnect();

    // Configure SSL Certificates
    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    // Configure MQTT Server
    client.setServer(MQTT_HOST, 8883);
    client.setCallback(messageReceived);

    Serial.println("Connecting to AWS IoT Core");

    while (!client.connect(THINGNAME))
    {
        Serial.print(".");
        delay(1000);
    }

    if (client.connected())
    {
        Serial.println();
        Serial.println("AWS IoT Connected Successfully");

        // Subscribe to Topic
        client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    }
    else
    {
        Serial.println();
        Serial.println("AWS IoT Connection Failed");
    }
}


// Publish Fingerprint ID to AWS IoT

void publishFingerprintId(int fingerprint_id)
{
    StaticJsonDocument<512> doc;

    doc["fingerprint_id"] = fingerprint_id;

    char jsonBuffer[512];

    serializeJson(doc, jsonBuffer);

    Serial.println("Publishing Fingerprint ID to AWS IoT Core...");

    bool result =
        client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

    if (result)
    {
        Serial.println("Fingerprint ID Published Successfully");
        Serial.println(jsonBuffer);
    }
    else
    {
        Serial.println("Failed to Publish Fingerprint ID");
    }
}


// Setup Function

void setup()
{
    Serial.begin(115200);

    mySerial.begin(57600);

    Serial.println();
    Serial.println("------------------------------------");
    Serial.println(" Biometric Attendance System ");
    Serial.println("-------------------------------------");

    // Connect to AWS IoT
    connectAWS();

    // Initialize Fingerprint Sensor
    finger.begin(57600);

    if (finger.verifyPassword())
    {
        Serial.println("Fingerprint Sensor Detected");
    }
    else
    {
        Serial.println("Fingerprint Sensor Not Found");
        Serial.println("Please Check Hardware Connections");

        while (1);
    }
}


// Main Loop

void loop()
{
    now = time(nullptr);

    // Maintain MQTT Connection
    client.loop();

    // Reconnect WiFi if Disconnected
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi Disconnected. Reconnecting...");
        connectAWS();
    }
}


// Fingerprint Enrollment Function

void enrollFingerprint()
{
    Serial.print("Enrolling Fingerprint ID: ");
    Serial.println(currentId);

    // Check Maximum Fingerprint Limit
    if (currentId > maxId)
    {
        Serial.println("Maximum Fingerprint Limit Reached");

        publishFingerprintId(currentId);

        return;
    }

    int p = -1;

    // Capture Fingerprint Image
    while (p != FINGERPRINT_OK)
    {
        p = finger.getImage();

        switch (p)
        {
            case FINGERPRINT_OK:
                Serial.println("Fingerprint Image Captured");
                break;

            case FINGERPRINT_NOFINGER:
                Serial.println("No Finger Detected");
                break;

            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication Error");
                break;

            case FINGERPRINT_IMAGEFAIL:
                Serial.println("Imaging Error");
                break;

            default:
                Serial.println("Unknown Error");
                break;
        }
    }

    // Convert Image to Template
    if (!convertToTemplate())
    {
        Serial.println("Template Creation Failed");

        publishFingerprintId(currentId);

        return;
    }

    // Search Existing Fingerprint
    p = finger.fingerSearch();

    if (p == FINGERPRINT_OK)
    {
        Serial.print("Fingerprint Already Exists with ID: ");
        Serial.println(finger.fingerID);

        return;
    }

    Serial.println("Fingerprint Not Found. Proceeding with Enrollment");

    // Store Fingerprint
    int storeResult = finger.storeModel(currentId);

    if (storeResult == FINGERPRINT_OK)
    {
        Serial.println("Fingerprint Stored Successfully");

        enrollmentSuccess = true;

        currentId++;
    }
    else
    {
        Serial.println("Failed to Store Fingerprint");
    }

    // Publish Fingerprint ID
    publishFingerprintId(currentId - 1);
}


// Convert Fingerprint Image to Template

bool convertToTemplate()
{
    int p = finger.image2Tz(1);

    if (p != FINGERPRINT_OK)
    {
        Serial.println("Failed to Convert First Image");

        return false;
    }

    Serial.println("Remove Finger and Place Again");

    delay(2000);

    p = -1;

    // Capture Second Fingerprint Image
    while (p != FINGERPRINT_OK)
    {
        p = finger.getImage();
    }

    p = finger.image2Tz(2);

    if (p != FINGERPRINT_OK)
    {
        Serial.println("Failed to Convert Second Image");

        return false;
    }

    // Create Fingerprint Model
    p = finger.createModel();

    if (p != FINGERPRINT_OK)
    {
        Serial.println("Failed to Create Fingerprint Model");

        return false;
    }

    return true;
}
