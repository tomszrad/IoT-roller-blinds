#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>
#include <ArduinoJson.h>

const int stepsPerRevolution = 2048;  // liczba kroków na pełen obrót

// ULN2003 Motor Driver Pins dla pierwszego silnika
#define IN1_1 27
#define IN2_1 14
#define IN3_1 12
#define IN4_1 13

// ULN2003 Motor Driver Pins dla drugiego silnika
#define IN1_2 17
#define IN2_2 16
#define IN3_2 4
#define IN4_2 2

// ULN2003 Motor Driver Pins dla trzeciego silnika
#define IN1_3 32
#define IN2_3 33
#define IN3_3 25
#define IN4_3 26

// inicjalizacja bibliotek dla trzech silników
Stepper stepper1(stepsPerRevolution, IN1_1, IN3_1, IN2_1, IN4_1);
Stepper stepper2(stepsPerRevolution, IN1_2, IN3_2, IN2_2, IN4_2);
Stepper stepper3(stepsPerRevolution, IN1_3, IN3_3, IN2_3, IN4_3);

const char* WIFI_SSID = "_WIFI_SSID";
const char* WIFI_PASSWORD = "_WIFI_PASSWORD";
#define LED_PIN 2

// MQTT Broker Configuration
const char* MQTT_SERVER = "_MQTT_SERVER_IP";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "rollerblinds";
const char* MQTT_REACHED_TOPIC = "reached";
const char* MQTT_USER = "_MQTT_USERNAME";
const char* MQTT_PASSWORD = "_MQTT_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Initialize serial port
  Serial.begin(115200);

  // ustawienie prędkości dla każdego silnika
  stepper1.setSpeed(10);
  stepper2.setSpeed(10);
  stepper3.setSpeed(10);

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Start WiFi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);  // Blink LED while connecting
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    Serial.print(".");
  }

  // Connected to WiFi
  digitalWrite(LED_PIN, LOW);  // Turn LED off to indicate WiFi connected
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  reconnect();  // Initial attempt to connect to MQTT broker
}

void loop() {
  if (!client.connected()) {
    digitalWrite(LED_PIN, HIGH);  // Turn on LED if MQTT connection is lost
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Print the payload (message body)
  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println(); // Print a newline after the payload

  // Parse JSON payload
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Ensure the payload is an array with exactly three elements
  if (!doc.is<JsonArray>() || doc.size() != 3) {
    Serial.println("Invalid payload format. Expected an array of three numbers.");
    return;
  }

  // Extract step counts for each stepper motor
  int steps1 = doc[0];
  int steps2 = doc[1];
  int steps3 = doc[2];

  // Calculate the maximum steps required
  int maxSteps = max(abs(steps1), max(abs(steps2), abs(steps3)));

  // Move motors simultaneously
  for (int step = 0; step < maxSteps; step++) {
    if (step < abs(steps1)) {
      stepper1.step(steps1 > 0 ? 1 : -1);
    }
    if (step < abs(steps2)) {
      stepper2.step(steps2 > 0 ? 1 : -1);
    }
    if (step < abs(steps3)) {
      stepper3.step(steps3 > 0 ? 1 : -1);
    }
    delay(5); // Adjust delay as needed for your application
  }

  // Confirm the steps taken
  Serial.println("Stepper motors moved to new positions.");

  // Publish "done" message to the "reached" topic
  Serial.println("wysyłam done");
  client.publish(MQTT_REACHED_TOPIC, "done");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT broker");
      Serial.println("wysyłam done");
      client.publish(MQTT_REACHED_TOPIC, "done");
      client.subscribe(MQTT_TOPIC);
      Serial.println("Subscribed to topic: rollerblinds");
      digitalWrite(LED_PIN, LOW);  // Turn off LED to indicate MQTT connected
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
