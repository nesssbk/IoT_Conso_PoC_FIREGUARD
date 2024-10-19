#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_BME680.h>
#include <HTTPClient.h>

// Paramètres du WiFi et d'Adafruit IO
#define WLAN_SSID       "TF4-iPhone"
#define WLAN_PASS       "AtBash5-CnLmSo"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "topiga"
#define AIO_KEY         "aio_ePiM34htp531YkejNp0K5y7Xp8IV"

// GPIOs pour les capteurs
#define BME680_I2C_SDA 21
#define BME680_I2C_SCL 22
#define LM35_ADC_GPIO 36

// Variables globales
float last_bme680_temp = 0.0;
unsigned long last_bme680_read = 0;
unsigned long last_lm35_read = 0;

// Objets pour le WiFi et MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds Adafruit IO
Adafruit_MQTT_Publish forest_data = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/forest.data");
Adafruit_MQTT_Publish forest_alert = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/forest.alert");

// Objet pour le capteur BME680
Adafruit_BME680 bme;

void setup() {
  Serial.begin(115200);

  // Initialisation du capteur BME680
  Wire.begin(BME680_I2C_SDA, BME680_I2C_SCL);
  if (!bme.begin()) {
    Serial.println("Erreur d'initialisation du BME680 !");
    while (1);
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setGasHeater(320, 150);

  // Configuration de l'ADC pour le LM35
  analogSetWidth(12);
  analogSetAttenuation(ADC_11db);

  setup_wifi();
}

void loop() {
  unsigned long current_time = millis();

  // Lecture du BME680 toutes les 30 minutes
  if (current_time - last_bme680_read >= 30 * 60 * 1000) {
    measure_and_send_data();
    last_bme680_read = current_time;
  }

  // Lecture du LM35 toutes les 5 secondes
  if (current_time - last_lm35_read >= 5000) {
    float lm35_temp = read_lm35_temperature();
    Serial.print("Température LM35 : ");
    Serial.print(lm35_temp);
    Serial.println(" °C");

    if (lm35_temp >= last_bme680_temp + 3.0) {
      handle_temperature_alert(lm35_temp);
    }

    last_lm35_read = current_time;
  }

  delay(100);  // Petit délai pour éviter une utilisation excessive du CPU
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;

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
  if (!bme.performReading()) {
    Serial.println("Échec de la lecture du BME680");
    return;
  }
  last_bme680_temp = bme.temperature;
  float humidity = bme.humidity;

  MQTT_connect();

  String dataPayload = String(last_bme680_temp) + "," + String(humidity);
  if (!forest_data.publish(dataPayload.c_str())) {
    Serial.println("Échec de l'envoi des données sur AIO");
  } else {
    Serial.println("Données envoyées sur AIO : " + dataPayload);
  }
}

void handle_temperature_alert(float alert_temp) {
  MQTT_connect();

  String alertPayload = String(alert_temp);
  if (!forest_alert.publish(alertPayload.c_str())) {
    Serial.println("Échec de l'envoi de l'alerte sur AIO");
  } else {
    Serial.println("Alerte envoyée sur AIO : " + alertPayload);
  }

  // Effectuer une nouvelle mesure avec le BME680
  measure_and_send_data();
}

float read_lm35_temperature() {
  int adc_value = analogRead(LM35_ADC_GPIO);
  float voltage = (adc_value / 2048.0) * 3.3;
  return voltage * 100.0;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion à ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());
}
