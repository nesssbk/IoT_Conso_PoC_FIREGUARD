# IoT_Conso_PoC_FIREGUARD

## Description
Ce projet implémente un système d'alerte incendie basé sur l'ESP32, avec des notifications MQTT et un mode d'économie d'énergie (light sleep). L'alerte peut être déclenchée et contrôlée à distance via un système MQTT, et un message SMS est envoyé en cas de détection d'incendie. Ce système utilise des capteurs de température et de qualité de l'air pour détecter les incendies.

## Matériel nécessaire
- ESP32
- LED
- Buzzer
- Bouton poussoir
- **Capteur de température LM35**
- **Capteur de qualité de l'air BME680**
- Connexion WiFi

## Logiciels
- Arduino IDE
- Bibliothèque Adafruit MQTT (installée via le gestionnaire de bibliothèques Arduino)
- Bibliothèque WiFi (incluse avec l'ESP32)
- Bibliothèque Adafruit BME680 (pour gérer les lectures de ce capteur)

## Fonctionnalités principales
- Détection d'incendie à l'aide des capteurs **LM35** et **BME680**.
- Envoi d'alertes via **MQTT** en cas de détection d'incendie.
- Envoi d'un SMS à un opérateur si une alerte n'est pas acquittée dans les 10 secondes.
- Activation d'un **buzzer** et d'une **LED** pour signaler l'alerte.
- Support du mode **light sleep** pour l'économie d'énergie (en cours de développement).

## Codes du projet
Le projet se compose de quatre codes principaux :
- **esp_foret** : Code pour le fonctionnement standard de l'ESP32 (envoi des données).
- **esp_foret_no_sleep** : Code pour le fonctionnement de l'ESP32 (envoi des données) sans mode light sleep.
- **esp_tour** : Code pour le fonctionnement standard de l'ESP32 (récéption des données).
- **esp_tour_no_sleep** : Code pour le fonctionnement de l'ESP32 (récéption des données) sans mode light sleep.

## Montage
Câblage des composants sur l'ESP32 (avec une plaque à trou):
Buzzer : Pin 23
LED : Pin 22
Bouton : Pin 21
Capteur LM35 : La patte du milieu reliée à la pin VP


## Installation

### 1. Clonez le dépôt
Exécutez les commandes suivantes pour cloner le projet :
```bash
git clone https://github.com/nesssbk/IoT_Conso_PoC_FIREGUARD.git
cd IoT_Conso_PoC_FIREGUARD
