#include <WiFi.h>
#include <Arduino.h>
#include <Servo.h>
#include <HX711.h>


// DOIT ESP32 DEVKIT V1
#define GPIO_13 13
#define GPIO_26 26
#define GPIO_25 25
#define GPIO_33 33
#define GPIO_32 32

#define DOUT_1 GPIO_32
#define SCK_1 GPIO_33

#define DOUT_2 GPIO_26
#define SCK_2 GPIO_25

#define SERVO_PIN GPIO_13

HX711 scale1;
HX711 scale2;

Servo servo;

const char *ssid = "NoU";
const char *password = "NoU";

String serverName = "localhost";
String serverPath = "/a/p/i/s/e/n/s/o/r";
const int serverPort = 80;

unsigned long lastTime = 0;
unsigned long lastTimeServo = 0;
unsigned long timerDelay = 10000; //10s
unsigned long servoDelay = 1000; //1s

WiFiClient client;

void setup() {
    Serial.begin(115200);
    delay(10);

    scale1.begin(DOUT_1, SCK_1);
    scale1.tare();
    scale1.set_scale(232.356);

    scale2.begin(DOUT_2, SCK_2);
    scale2.tare();
    scale2.set_scale(-232.356);

    servo.attach(SERVO_PIN);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println(".");
    }
    Serial.println("Connected to WiFi");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    unsigned long currentMillis = millis();
    if ((currentMillis - lastTime) >= timerDelay) {
        float food_weight = scale1.get_units(10);
        float water_weight = scale2.get_units(10);
        Serial.print("Food weight: ");
        Serial.print(food_weight);
        Serial.print(" g, Water weight: ");
        Serial.print(water_weight);
        Serial.println(" g");
        sendData(food_weight, water_weight);
        if (food_weight < 10) {
            servo.write(90);
            Serial.println("Scroll rotate 90");
            lastTimeServo = millis();
        }
        lastTime = currentMillis;
    }
    if ((lastTimeServo > 0) && ((millis() - lastTimeServo) >= servoDelay)) {
        servo.write(180);
        lastTimeServo = 0;
        Serial.println("Servo rotate 180");
    }
}

void sendData(float food_weight, float water_weight){
    if (WiFi.status() == WL_CONNECTED) {
        String getBody;
        String getAll;
        if (client.connect(serverName.c_str(), serverPort)) {
            Serial.println("Connection successful!");
            String head = "";
            String tail = "";
            client.println("POST " + serverPath + "?food_weight=" + String(food_weight) + "&water_weight=" + String(water_weight) + " HTTP/1.1");
            client.println("Host: " + serverName);
            client.println("Content-Length: 0");
            client.println("Content-Type: text/plain");
            client.println();
            client.print(head);
            client.print(tail);

            int timoutTimer = 10000;
            long startTimer = millis();
            boolean state = false;
            while ((startTimer + timoutTimer) > millis()) {
                Serial.print(".");
                delay(100);
                while (client.available()) {
                    char c = client.read();
                    if (c == '\n') {
                        if (getAll.length() == 0) {
                            state = true;
                        }
                        getAll = "";
                    } else if (c != '\r') {
                        getAll += String(c);
                    }
                    if (state == true) {
                        getBody += String(c);
                    }
                    startTimer = millis();
                }
                if (getBody.length() > 0) {
                    break;
                }
            }
            Serial.println();
            client.stop();
            Serial.println(getBody);
        } else {
            getBody = "Connection to " + serverName + " failed.";
            Serial.println(getBody);
        }
    } else {
        Serial.println("WiFi Disconnected");
    }
}
