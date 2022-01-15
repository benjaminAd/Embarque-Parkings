# Projet Parking, Développemen mobile et IoT

Pour utiliser ce projet vous devrez installer les librairies arduino suivantes :

- [LibYxml](https://www.arduino.cc/reference/en/libraries/libyxml/)
- [ArduinoJson](https://arduinojson.org/v6/doc/installation/)

Ensuite vous devrez créer 2 fichiers externes :

- wifiLogs.h

```
#ifndef WIFILOGS_H
#define WIFILOGS_H
const char *wifi_name="***";
const char *wifi_password="***";
#endif
```

- api.h

```
#ifndef APIKEYS_H
#define APIKEYS_H
const char *GOOGLE_API_KEY = "*******";
#endif
```

Pour pouvoir exécuter ce projet, il faudra posséder un ESP (8266 ou 32) possédant une carte réseau afin de pouvoir se connecter à internet et pouvoir faire les différents traitements.
