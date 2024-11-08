#include <Button.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Déclaration des pins des boutons et LEDs
const int pinButtonVert = 12, pinButtonBleu = 0, pinButtonJaune = 2, pinButtonRouge = 14;
const int pinLedVert = 4, pinLedBleu = 13, pinLedJaune = 15, pinLedRouge = 16;
const int pinBuzzer = 5; // Buzzer

// Fréquences des notes pour chaque couleur
const int freqVert = 1000, freqBleu = 1200, freqJaune = 1400, freqRouge = 1600;

WiFiClient espClient;
PubSubClient client(espClient);

// Objets Button
Button buttonVert(pinButtonVert), buttonBleu(pinButtonBleu), buttonJaune(pinButtonJaune), buttonRouge(pinButtonRouge);

const char* ssid = "miguet_automation";
const char* password = "vincentbld196300";
const char* mqtt_server = "192.168.1.5";

unsigned long lastDebounceTimeVert = 0, lastDebounceTimeBleu = 0, lastDebounceTimeJaune = 0, lastDebounceTimeRouge = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  // Configuration des LEDs et des boutons
  pinMode(pinLedVert, OUTPUT);
  pinMode(pinLedBleu, OUTPUT);
  pinMode(pinLedJaune, OUTPUT);
  pinMode(pinLedRouge, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  
  pinMode(pinButtonVert, INPUT_PULLUP);
  pinMode(pinButtonBleu, INPUT_PULLUP);
  pinMode(pinButtonJaune, INPUT_PULLUP);
  pinMode(pinButtonRouge, INPUT_PULLUP);

  noTone(pinBuzzer);  // Désactiver le buzzer au démarrage
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("WiFi connecté");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("SimonGameClient")) {
      Serial.println("connecté");

      // Abonnement aux topics
      client.subscribe("home/simeo/led/#");
      client.subscribe("home/simeo/buzzer/command");

      Serial.println("Abonné aux topics MQTT");
    } else {
      Serial.print("échec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 1 seconde");
      delay(1000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Afficher le topic et le message reçu
  Serial.print("Message reçu sur le topic : ");
  Serial.println(topic);

  String topicStr = String(topic);
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Afficher le message reçu
  Serial.print("Message reçu : ");
  Serial.println(message);

  // Contrôle des LEDs via MQTT
  if (topicStr.startsWith("home/simeo/led/")) {
    int startIndex = String("home/simeo/led/").length();
    int endIndex = topicStr.lastIndexOf("/");
    String color = topicStr.substring(startIndex, endIndex);
    String command = message;

    // Contrôler la LED par couleur
    if (command == "on") {
      Serial.println("Commande ON reçue pour la couleur : " + color);
      controlLedByColor(color, true);  // Allumer la LED
    } else if (command == "off") {
      Serial.println("Commande OFF reçue pour la couleur : " + color);
      controlLedByColor(color, false);  // Éteindre la LED
    }
  }

  // Contrôle du buzzer via MQTT
  if (topicStr == "home/simeo/buzzer/command") {
    controlBuzzer(message);  // Contrôle du buzzer
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Lecture des boutons et gestion des LEDs associées
  unsigned long currentMillis = millis();

  handleButton(buttonVert, pinLedVert, "Vert", currentMillis, lastDebounceTimeVert, freqVert);
  handleButton(buttonBleu, pinLedBleu, "Bleu", currentMillis, lastDebounceTimeBleu, freqBleu);
  handleButton(buttonJaune, pinLedJaune, "Jaune", currentMillis, lastDebounceTimeJaune, freqJaune);
  handleButton(buttonRouge, pinLedRouge, "Rouge", currentMillis, lastDebounceTimeRouge, freqRouge);
}

// Fonction de gestion des boutons et LEDs
void handleButton(Button &button, int pinLed, const char *buttonName, unsigned long currentMillis, unsigned long &lastDebounceTime, int frequency) {
  if (button.pressed() && (currentMillis - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentMillis;
    digitalWrite(pinLed, HIGH);

    // Publier l'état du bouton via MQTT
    String topic = "home/simeo/button/" + String(buttonName) + "/state";
    String payload = "{\"state\": \"on\", \"button\": \"" + String(buttonName) + "\", \"timestamp\": " + String(currentMillis) + "}";
    client.publish(topic.c_str(), payload.c_str());

    playMelody(frequency);  // Jouer la note associée
  }

  if (button.released() && (currentMillis - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentMillis;
    digitalWrite(pinLed, LOW);

    // Publier l'état du bouton via MQTT
    String topic = "home/simeo/button/" + String(buttonName) + "/state";
    String payload = "{\"state\": \"off\", \"button\": \"" + String(buttonName) + "\", \"timestamp\": " + String(currentMillis) + "}";
    client.publish(topic.c_str(), payload.c_str());

    noTone(pinBuzzer);  // Arrêter la mélodie
  }
}

// Fonction pour contrôler les LEDs via MQTT
void controlLedByColor(String color, bool state) {
  int pinLed = -1;
  int frequency = 0;

  if (color == "Vert") {
    pinLed = pinLedVert;
    frequency = freqVert;
  } else if (color == "Bleu") {
    pinLed = pinLedBleu;
    frequency = freqBleu;
  } else if (color == "Jaune") {
    pinLed = pinLedJaune;
    frequency = freqJaune;
  } else if (color == "Rouge") {
    pinLed = pinLedRouge;
    frequency = freqRouge;
  }

  // Contrôler la LED correspondante
  if (pinLed != -1) {
    if (state) {  // Allumer la LED
      digitalWrite(pinLed, HIGH);
      Serial.println("LED " + color + " allumée.");
      playMelody(frequency);  // Jouer une mélodie
    } else {  // Éteindre la LED
      digitalWrite(pinLed, LOW);
      Serial.println("LED " + color + " éteinte.");
    }
  } else {
    Serial.println("Erreur : Couleur non reconnue.");
  }
}

// Contrôle du buzzer via MQTT
void controlBuzzer(String command) {
  if (command == "on") {
    tone(pinBuzzer, 1000);  // Jouer une note
  } else if (command == "off") {
    noTone(pinBuzzer);  // Arrêter la note
  }
}

// Fonction pour jouer une mélodie sur le buzzer
void playMelody(int frequency) {
  tone(pinBuzzer, frequency, 150);  // Jouer une note
  delay(150);
  noTone(pinBuzzer);
  delay(50);
  tone(pinBuzzer, frequency + 500, 150);  // Jouer une seconde note
  delay(150);
  noTone(pinBuzzer);
}
