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
- Détection d'incendie (température qui augmente de +3°C en moins de 30mn) à l'aide des capteurs **LM35** et **BME680**.
- Envoi d'alertes via **MQTT** en cas de détection d'incendie.
- Envoi d'un SMS à un opérateur si une alerte n'est pas acquittée dans les 10 secondes.
- Activation d'un **buzzer** et d'une **LED** pour signaler l'alerte.
- Support du mode **light sleep** pour l'économie d'énergie (en cours de développement).

## Montage
Pour la réalisation du montage, nous avons dans un premier temps câblé les composants principaux qui sont le buzzer, la LED rouge, le capteur LM35 et le bouton. Nous avons par la suite ajouté un autre capteur plus précis qui est le BME680.

### Câblage des composants
- **Alimentation** : Chaque composant est alimenté avec 3,3V et GND.
- **Capteur LM35** :
  - Patte du milieu reliée à la pin VP.
- **Buzzer** :
  - Relié à la PIN 23.
- **LED** :
  - Reliée à la PIN 22.
- **Bouton** :
  - Relié à la PIN 21.
  
 ## Installation

### Étape 1 : Clonez le dépôt
1. Ouvrez votre terminal (ou invite de commande).
2. Exécutez les commandes suivantes pour cloner le projet :
   ```bash
   git clone https://github.com/votre-compte/ESP32-Fire-Alert-IoT.git
   cd ESP32-Fire-Alert-IoT
