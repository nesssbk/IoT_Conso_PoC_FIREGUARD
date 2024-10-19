#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_BME680.h>
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "esp_sleep.h"

// Paramètres du WiFi et d'Adafruit IO
#define WLAN_SSID       "TF4-iPhone"
#define WLAN_PASS       "AtBash5-CnLmSo"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // Utiliser 8883 pour SSL
#define AIO_USERNAME    "topiga"
#define AIO_KEY         "aio_ePiM34htp531YkejNp0K5y7Xp8IV"

// GPIOs pour les capteurs et actionneurs
#define BME680_I2C_SDA 21
#define BME680_I2C_SCL 22
#define LM35_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO 36
#define LM35_ADC_GPIO 36
#define BUZZER_PIN 25

// Variables globales
RTC_DATA_ATTR float last_bme680_temp = 0.0;  // Conservée pendant le sommeil
float current_bme680_temp = 0.0;
RTC_DATA_ATTR uint32_t ulp_threshold_adc = 0;

// Objets pour le WiFi et MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds Adafruit IO
Adafruit_MQTT_Publish forest_data = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/forest.data");
Adafruit_MQTT_Publish forest_alert = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/forest.alert");
Adafruit_MQTT_Subscribe control_confirmation = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/control.confirmation");

// Objet pour le capteur BME680
Adafruit_BME680 bme;

// Remplacez par vos identifiants API Free Mobile
const char* free_mobile_user = "54593799";
const char* free_mobile_pass = "AEkvHAQ4uWBNXr";

void setup() {
  // Initialisation de la communication série
  Serial.begin(115200);

  // Initialisation du capteur BME680
  Wire.begin(BME680_I2C_SDA, BME680_I2C_SCL);
  if (!bme.begin()) {
    Serial.println("Erreur d'initialisation du BME680 !");
    while (1);
  }
  // Configuration du BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setGasHeater(320, 150); // 320°C pendant 150ms

  // Configuration de l'ADC pour le LM35
  analogSetWidth(12);
  analogSetAttenuation(ADC_11db);

  // Test initial du LM35
  float initial_temp = read_lm35_temperature();
  Serial.print("Température initiale LM35 : ");
  Serial.print(initial_temp);
  Serial.println(" °C");

  if (initial_temp < -10 || initial_temp > 50) {
    Serial.println("ERREUR : La lecture du LM35 semble incorrecte. Vérifiez le branchement.");
    while(1); // Arrêtez le programme si la lecture semble erronée
  }

  // Configuration du buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Buzzer éteint

  // Vérifier la cause du réveil
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_ULP) {
    Serial.println("Réveillé par le ULP (LM35)");
    // Le ULP a détecté une température élevée via le LM35
    handle_temperature_alert();
  } else {
    Serial.println("Démarrage normal ou réveillé par le timer");
    // Première exécution ou réveil par le timer
    measure_and_send_data();
  }

  // Configuration du ULP
  init_ulp_program();
}

void loop() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.print("Wakeup reason: ");
  Serial.println(wakeup_reason);

  if (wakeup_reason == ESP_SLEEP_WAKEUP_ULP) {
    Serial.println("Réveillé par le ULP (LM35)");
    handle_temperature_alert();
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Réveillé par le timer");
    measure_and_send_data();
  } else {
    Serial.println("Réveil inattendu");
  }

  // Vérification manuelle de la température
  float current_temp = read_lm35_temperature();
  Serial.print("Température actuelle : ");
  Serial.print(current_temp);
  Serial.println(" °C");

  if (current_temp >= last_bme680_temp + 3.0) {
    Serial.println("Alerte de température détectée manuellement!");
    handle_temperature_alert();
  }

  init_ulp_program();
  esp_sleep_enable_ulp_wakeup();
  esp_sleep_enable_timer_wakeup(30 * 1000000ULL); // 30 seconds for testing

  Serial.println("Entrée en light sleep dans 5 secondes...");
  delay(5000);
  esp_light_sleep_start();
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

void measure_and_send_data() {
  // Lecture du BME680
  if (! bme.performReading()) {
    Serial.println("Échec de la lecture du BME680");
    return;
  }
  current_bme680_temp = bme.temperature;
  float humidity = bme.humidity;

  // Stocker la dernière température pour comparaison avec le LM35
  last_bme680_temp = current_bme680_temp;

  // Connexion WiFi
  setup_wifi();

  // Envoi des données via MQTT
  if (WiFi.status() == WL_CONNECTED) {
    MQTT_connect();
    // Envoyer les données au feed forest.data
    String dataPayload = String(current_bme680_temp) + "," + String(humidity);
    if (!forest_data.publish(dataPayload.c_str())) {
      Serial.println("Échec de l'envoi des données sur AIO");
    } else {
      Serial.println("Données envoyées sur AIO : " + dataPayload);
    }
    mqtt.disconnect();
  } else {
    Serial.println("Pas de connexion WiFi, impossible d'envoyer les données MQTT");
  }
}

void handle_temperature_alert() {
  // Le ULP a détecté une température élevée via le LM35

  // Lecture de la température du LM35
  float lm35_temp = read_lm35_temperature();
  Serial.print("Température LM35 : ");
  Serial.print(lm35_temp);
  Serial.println(" °C");

  // Activer le buzzer
  activate_buzzer();

  // Connexion WiFi
  setup_wifi();

  // Envoi d'une alerte via MQTT
  if (WiFi.status() == WL_CONNECTED) {
    MQTT_connect();

    // Envoyer l'alerte au feed forest.alert
    String alertPayload = String(lm35_temp);
    if (!forest_alert.publish(alertPayload.c_str())) {
      Serial.println("Échec de l'envoi de l'alerte sur AIO");
    } else {
      Serial.println("Alerte envoyée sur AIO : " + alertPayload);
    }

    // S'abonner au feed control.confirmation
    mqtt.subscribe(&control_confirmation);

    unsigned long startTime = millis();
    bool confirmation_received = false;
    while (millis() - startTime < 10000) {
      // Attendre une confirmation de la tour de contrôle
      Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription(5000))) {
        if (subscription == &control_confirmation) {
          Serial.print("Confirmation reçue : ");
          Serial.println((char *)control_confirmation.lastread);
          confirmation_received = true;
          break;
        }
      }
      if (confirmation_received) break;
    }

    // Si pas de confirmation, envoyer un SMS via l'API Free
    if (!confirmation_received) {
      send_sms_alert(lm35_temp);
    } else {
      Serial.println("Confirmation reçue de la tour de contrôle");
    }
    mqtt.disconnect();
  } else {
    Serial.println("Pas de connexion WiFi, impossible d'envoyer l'alerte MQTT");
  }

  // Désactiver le buzzer
  deactivate_buzzer();
}

void activate_buzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void deactivate_buzzer() {
  digitalWrite(BUZZER_PIN, LOW);
}

float read_lm35_temperature() {
  int adc_value = analogRead(LM35_ADC_GPIO);
  float voltage = (adc_value / 4095.0) * 3.3; // Assuming 3.3V reference voltage
  float temperature = voltage * 100.0; // LM35 provides 10mV per degree Celsius

  Serial.print("ADC: ");
  Serial.print(adc_value);
  Serial.print(", Voltage: ");
  Serial.print(voltage, 3);
  Serial.print("V, Température LM35: ");
  Serial.print(temperature);
  Serial.println("°C");

  return temperature;
}

void send_sms_alert(float temperature) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    const char* host = "smsapi.free-mobile.fr";
    String message = "Alerte incendie! Temp: " + String(temperature) + "C";

    Serial.print("Connexion à ");
    Serial.println(host);

    if (client.connect(host, 443)) {
      String url = "/sendmsg?user=" + String(free_mobile_user) + "&pass=" + String(free_mobile_pass) + "&msg=" + urlencode(message);
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
      Serial.println("Requête d'envoi du SMS envoyée");
    } else {
      Serial.println("Connexion échouée");
    }
    // Attendre une brève période pour s'assurer que la requête est envoyée
    delay(1000);
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

void init_ulp_program() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(LM35_ADC_CHANNEL, ADC_ATTEN_DB_11);

  float threshold_temp = last_bme680_temp + 3.0;
  float threshold_voltage = threshold_temp * 0.01; // LM35 : 10 mV par °C
  ulp_threshold_adc = (threshold_voltage / 3.3) * 4095.0;

  Serial.print("Seuil de température : ");
  Serial.print(threshold_temp);
  Serial.println("°C");
  Serial.print("Seuil ADC : ");
  Serial.println(ulp_threshold_adc);

  const ulp_insn_t ulp_prog[] = {
    I_ADC(R0, ADC1_CHANNEL_0, 0),
    I_MOVI(R1, ulp_threshold_adc & 0xFFFF),
    M_BGE(1, 0),
    I_WAKE(),
    M_LABEL(1),
    I_HALT()
  };

  size_t size = sizeof(ulp_prog) / sizeof(ulp_insn_t);
  ESP_ERROR_CHECK(ulp_process_macros_and_load(0, ulp_prog, &size));
  ESP_ERROR_CHECK(ulp_run(0));
  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
}
