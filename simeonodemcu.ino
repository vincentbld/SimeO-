#include <Button.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Déclaration des pins des boutons avec des noms clairs pour NodeMCU
const int pinButtonVert = 12;  // D6 sur NodeMCU
const int pinButtonBleu = 0;   // D3 sur NodeMCU
const int pinButtonJaune = 2;  // D4 sur NodeMCU
const int pinButtonRouge = 14; // D5 sur NodeMCU

// Déclaration des pins des LEDs associées aux boutons
const int pinLedVert = 4;      // D2 sur NodeMCU
const int pinLedBleu = 13;     // D7 sur NodeMCU
const int pinLedJaune = 15;    // D8 sur NodeMCU
const int pinLedRouge = 16;    // D0 sur NodeMCU (remplace D9 car non supporté)

// Déclaration du pin pour le buzzer (utiliser un pin disponible, comme GPIO5)
const int pinBuzzer = 5;       // SS est mappé sur GPIO5

// Création des objets Button pour chaque bouton
Button buttonVert(pinButtonVert);
Button buttonBleu(pinButtonBleu);
Button buttonJaune(pinButtonJaune);
Button buttonRouge(pinButtonRouge);

// Configuration WiFi et MQTT
const char* ssid = "miguet_automation";
const char* password = "vincentbld196300";
const char* mqtt_server = "192.168.1.5";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastDebounceTimeVert = 0;
unsigned long lastDebounceTimeBleu = 0;
unsigned long lastDebounceTimeJaune = 0;
unsigned long lastDebounceTimeRouge = 0;
const unsigned long debounceDelay = 50;

void mqttCallback(char* topic, byte* payload, unsigned int length);
void allLedsOn();
void controlLedByColor(String color);
void handleButton(Button &button, int pinLed, const char *buttonName, unsigned long currentMillis, unsigned long &lastDebounceTime);
void playMelody();

void setup() {
  Serial.begin(115200);

  pinMode(pinButtonVert, INPUT_PULLUP);
  pinMode(pinButtonBleu, INPUT_PULLUP);
  pinMode(pinButtonJaune, INPUT_PULLUP);
  pinMode(pinButtonRouge, INPUT_PULLUP);

  pinMode(pinLedVert, OUTPUT);
  pinMode(pinLedBleu, OUTPUT);
  pinMode(pinLedJaune, OUTPUT);
  pinMode(pinLedRouge, OUTPUT);

  pinMode(pinBuzzer, OUTPUT);
  noTone(pinBuzzer);

  digitalWrite(pinLedVert, LOW);
  digitalWrite(pinLedBleu, LOW);
  digitalWrite(pinLedJaune, LOW);
  digitalWrite(pinLedRouge, LOW);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  client.subscribe("simeo/all");
  client.subscribe("simeo/color/#");
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("SimonGameClient")) {
      Serial.println("connecté");
      client.subscribe("simeo/all");
      client.subscribe("simeo/color/#");
    } else {
      Serial.print("échec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "simeo/all") {
    if (message == "on") {
      allLedsOn();
    }
  } else if (String(topic).startsWith("simeo/color/")) {
    String color = String(topic).substring(12);
    controlLedByColor(color);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  buttonVert.read();
  buttonBleu.read();
  buttonJaune.read();
  buttonRouge.read();

  handleButton(buttonVert, pinLedVert, "Vert", currentMillis, lastDebounceTimeVert);
  handleButton(buttonBleu, pinLedBleu, "Bleu", currentMillis, lastDebounceTimeBleu);
  handleButton(buttonJaune, pinLedJaune, "Jaune", currentMillis, lastDebounceTimeJaune);
  handleButton(buttonRouge, pinLedRouge, "Rouge", currentMillis, lastDebounceTimeRouge);

  delay(50);
}

void handleButton(Button &button, int pinLed, const char *buttonName, unsigned long currentMillis, unsigned long &lastDebounceTime) {
  if (button.pressed() && (currentMillis - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentMillis;
    Serial.print("Bouton ");
    Serial.print(buttonName);
    Serial.println(" appuyé");
    digitalWrite(pinLed, HIGH);
    String topic = "simeo/";
    topic += buttonName;
    client.publish(topic.c_str(), "on");

    playMelody();
  }
  if (button.released() && (currentMillis - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentMillis;
    Serial.print("Bouton ");
    Serial.print(buttonName);
    Serial.println(" relâché");
    digitalWrite(pinLed, LOW);
    String topic = "simeo/";
    topic += buttonName;
    client.publish(topic.c_str(), "off");

    noTone(pinBuzzer);
  }
}

void allLedsOn() {
  digitalWrite(pinLedVert, HIGH);
  digitalWrite(pinLedBleu, HIGH);
  digitalWrite(pinLedJaune, HIGH);
  digitalWrite(pinLedRouge, HIGH);
  tone(pinBuzzer, 1000);
  delay(5000);
  noTone(pinBuzzer);
  digitalWrite(pinLedVert, LOW);
  digitalWrite(pinLedBleu, LOW);
  digitalWrite(pinLedJaune, LOW);
  digitalWrite(pinLedRouge, LOW);
}

void controlLedByColor(String color) {
  int pinLed = -1;
  if (color == "Vert") {
    pinLed = pinLedVert;
  } else if (color == "Bleu") {
    pinLed = pinLedBleu;
  } else if (color == "Jaune") {
    pinLed = pinLedJaune;
  } else if (color == "Rouge") {
    pinLed = pinLedRouge;
  }

  if (pinLed != -1) {
    digitalWrite(pinLed, HIGH);
    tone(pinBuzzer, 1000, 200);
    delay(200);
    digitalWrite(pinLed, LOW);
  }
}

void playMelody() {
  tone(pinBuzzer, 1000, 200);
  delay(250);
  tone(pinBuzzer, 1500, 200);
}
