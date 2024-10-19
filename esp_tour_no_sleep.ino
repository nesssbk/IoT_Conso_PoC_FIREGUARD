#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <HTTPClient.h>

// Paramètres du WiFi et d'Adafruit IO
#define WLAN_SSID       "TF4-iPhone"
#define WLAN_PASS       "AtBash5-CnLmSo"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // Utiliser 8883 pour SSL
#define AIO_USERNAME    "topiga"
#define AIO_KEY         "aio_ePiM34htp531YkejNp0K5y7Xp8IV"

// GPIOs
#define BUTTON_PIN 15
#define LED_PIN 16
#define BUZZER_PIN 26

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds Adafruit IO
Adafruit_MQTT_Subscribe forest_alert = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/forest.alert");
Adafruit_MQTT_Publish control_confirmation = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/control.confirmation");

// Variables globales
volatile bool button_pressed = false;
unsigned long alert_received_time = 0;
bool alert_active = false;

// Remplacez par vos identifiants API Free Mobile
const char* free_mobile_user = "54593799";
const char* free_mobile_pass = "AEkvHAQ4uWBNXr";

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  setup_wifi();

  mqtt.subscribe(&forest_alert);
}

void loop() {
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &forest_alert) {
      // Gérer l'alerte
      Serial.print("Alerte reçue : ");
      Serial.println((char *)forest_alert.lastread);
      alert_active = true;
      alert_received_time = millis();
      activate_alert();
    }
  }

  if (alert_active) {
    if (millis() - alert_received_time >= 10000) {
      // Temps écoulé, pas de réponse de l'opérateur
      send_sms_alert();
      alert_active = false;
      deactivate_alert();
    }
    if (button_pressed) {
      // L'opérateur a appuyé sur le bouton
      button_pressed = false;
      alert_active = false;
      deactivate_alert();

      // Envoyer une confirmation à l'ESP32 de la forêt
      if (!control_confirmation.publish("Alert acknowledged")) {
        Serial.println("Échec de l'envoi de la confirmation");
      } else {
        Serial.println("Confirmation envoyée");
      }
    }
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop si déjà connecté.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connexion au broker MQTT... ");

  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Nouvelle tentative de connexion MQTT dans 5 secondes...");
    mqtt.disconnect();
    delay(5000);
  }

  Serial.println("Connecté au broker MQTT !");
}

void setup_wifi() {
  delay(10);
  // Connexion WiFi standard
  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  unsigned long startAttemptTime = millis();

  // Attendre la connexion WiFi avec un timeout
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connecté. Adresse IP : ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Impossible de se connecter au WiFi");
  }
}

void buttonISR() {
  button_pressed = true;
}

void activate_alert() {
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
}

void deactivate_alert() {
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void send_sms_alert() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://smsapi.free-mobile.fr/sendmsg?user=" + String(free_mobile_user) + "&pass=" + String(free_mobile_pass) + "&msg=" + urlencode("Alerte incendie! Aucun acquittement de la tour de contrôle.");

    Serial.print("[HTTP] begin...\n");
    http.begin(url);

    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Pas de connexion WiFi, impossible d'envoyer le SMS");
  }
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += "%20";
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}