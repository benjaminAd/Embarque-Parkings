#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <yxml.h>
#include <stdlib.h>
#include "parkings.h"
#include "wifiLogs.h"

// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};

ESP8266WiFiMulti WiFiMulti;

// Données de géolocalisation : https://data.montpellier3m.fr/dataset/parkings-en-ouvrage-de-montpellier
// Identifants : https://data.montpellier3m.fr/dataset/disponibilite-des-places-dans-les-parkings-de-montpellier-mediterranee-metropole
const parking_t parkings[] = {
                             /* ID,           name,                      longitude,         latitude */
                             { "FR_MTP_ANTI",  "Antigone",                   3.888818930000000, 43.608716059999999 },
                             { "FR_MTP_COME",  "Comédie",                    3.879761960000000, 43.608560920000002 },
                             { "FR_MTP_CORU",  "Corum",                      3.882257730000000, 43.613888209999999 },
                             { "FR_MTP_EURO",  "Europa",                     3.892530740000000, 43.607849710000004 },
                             { "FR_MTP_FOCH",  "Foch Préfecture",            3.876570840000000, 43.610749120000001 },
                             { "FR_MTP_GAMB",  "Gambetta",                   3.871374360000000, 43.606951379999998 },
                             { "FR_MTP_GARE",  "Saint Roch",                 3.878550720000000, 43.603291489999997 },
                             { "FR_MTP_TRIA",  "Triangle",                   3.881844180000000, 43.609233840000002 },
                             { "FR_MTP_ARCT",  "Arc de Triomphe",            3.873200750000000, 43.611002669999998 },
                             { "FR_MTP_PITO",  "Pitot",                      3.870191170000000, 43.612244939999997 },
                             { "FR_MTP_CIRC",  "Circé Odysseum",             3.917849500000000, 43.604953770000002 },
                             { "FR_MTP_SABI",  "Sabines",                    3.860224600000000, 43.583832630000003 },
                             { "FR_MTP_GARC",  "Garcia Lorca",               3.890715800000000, 43.590985089999997 },
                             { "FR_MTP_SABL",  "Notre Dame de Sablassou",    3.922295360000000, 43.634191940000001 },
                             { "FR_MTP_MOSS",  "Mosson",                     3.819665540000000, 43.616237159999997 },
                             { "FR_STJ_SJLC",  "Saint-Jean-le-Sec",          3.837931200000000, 43.570822249999999 },
                             { "FR_MTP_MEDC",  "Euromédecine",               3.827723650000000, 43.638953590000000 },
                             { "FR_MTP_OCCI",  "Occitanie",                  3.848597960000000, 43.634562320000001 },
                             { "FR_CAS_CDGA",  "Charles de Gaulle",          3.897762100000000, 43.628542119999999 },
                             { "FR_MTP_ARCE",  "Arceaux",                    3.867490670000000, 43.611716469999998 },
                             { "FR_MTP_POLY",  "Polygone",                   3.884765390000000, 43.608370960000002 },
                             { "FR_MTP_GA109", "Multiplexe (est)",           3.918980000000000, 43.605060000000000 },
                             { "FR_MTP_GA250", "Multiplexe (ouest)",         3.914030000000000, 43.604000000000000 },
                             // Les parkings ci-après n'ont pas de données temps réel sur le site de Montpellier 3M
                             { 0,              "Peyrou",                     3.870383780000000, 43.611297000000000 },
                             { 0,              "Hôtel de ville",             3.895853270000000, 43.599231000000003 },
                             { 0,              "Jacou",                      3.912884750000000, 43.654598700000001 },
                             { 0,              "Georges Pompidou",           3.921084190000000, 43.649339200000000 },
                             { 0,              "Via Domitia",                3.929538080000000, 43.646658010000003 },
                             { 0,              "Juvignac",                   3.809621860000000, 43.617403740000000 },
                             { 0,              "Saint-Jean-de-Védas Centre", 3.830585520000000, 43.574962790000001 },
                             { 0,              "Lattes",                     3.904817620000000, 43.570809879999999 },
                             { 0,              "Parc expo",                  3.945678520000000, 43.572910210000003 },
                             { 0,              "Pérols centre",              3.957355560000000, 43.565378570000000 },
                             { 0,              "Décathlon",                  3.923800380000000, 43.606185590000003 },
                             { 0,              "Ikéa",                       3.925582560000000, 43.604609619999998 },
                             { 0,              "Géant Casino",               3.922104130000000, 43.603155600000001 },
                             { 0,              "Mare Nostrum",               3.919015140000000, 43.602370800000003 },
                             { 0,              "Végapolis",                  3.914773710000000, 43.602896510000001 },
                             { 0,              "Multiplexe",                 3.914110760000000, 43.604152429999999 },
                             { 0,              "La Mantilla",                3.902399940000000, 43.598772959999998 },
                             { 0, 0, 0, 0 }
};

parking_data_t* available_parkings;

#define MONTPELLIER3M_BASE_URL "https://data.montpellier3m.fr/"
#define MONTPELLIER3M_API_PATH_PREFIX "sites/default/files/ressources/"
#define MONTPELLIER3M_API_PATH_SUFFIX ".xml"

yxml_ret_t r;
yxml_t x[1];
char stack[32];

String _buildURL(const char *id) {
  String res = MONTPELLIER3M_BASE_URL;
  res += MONTPELLIER3M_API_PATH_PREFIX;
  res += id;
  res += MONTPELLIER3M_API_PATH_SUFFIX;
  return res;
}

int getAvailableSpaces(String response) {
  yxml_init(x, stack, sizeof(stack));

  const char* xml = response.c_str();
  char sizebuf[1024], *sizecur = NULL, *tmp;
  bool isFree = false, isOpen = false;

  while (*xml) {
    r = yxml_parse(x, *xml);

    switch(r) {
      case YXML_ELEMSTART:
        sizecur = (strcmp(x->elem, "Status") == 0 || strcmp(x->elem, "Free") == 0) ? sizebuf : NULL;
        isFree = (strcmp(x->elem, "Free") == 0);
        if (yxml_symlen(x, x->elem) != strlen(x->elem))
          Serial.println("assertfail: elem lengths don't match");
        break;
      case YXML_CONTENT:
        if(!sizecur) /* Are we in the "Status" or "Free" element? */
          break;
        /* Append x->data to sizecur while there is space */
        tmp = x->data;
        while(*tmp && sizecur < sizebuf+sizeof(sizebuf))
          *(sizecur++) = *(tmp++);
        if(sizecur == sizebuf+sizeof(sizebuf))
          return -1; /* Too long element content, handle error */
        *sizecur = 0;
        break;
      case YXML_ELEMEND:
        if(sizecur) {
          /* Now we have the value of the "Status" or "Free" element in sizebuf */
          if(isFree) {
            return (isOpen) ? atoi(sizebuf) : -1;
          } else {
            isOpen = (strcmp(sizebuf, "Open") == 0);
          }
          sizecur = NULL;
        }
        break;
    }
    
    xml++;
  }
  
  return -1;
}

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(wifi_name, wifi_password);
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    //client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    const parking_t *ptr = parkings;

    while(ptr->id) {
      String url = _buildURL(ptr->id);

      Serial.println(url);
      
      if (https.begin(*client, url)) {  // HTTPS

        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            
            parking_data_t data;
            int spaces = getAvailableSpaces(payload);

            if(spaces > 0) {
              data.id = ptr->id;
              data.free = spaces;

              Serial.printf("Le pkg %s a %d places disponibles\n", data.id, data.free);
            }
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      
      ++ptr;
    }
  }

  Serial.println("Wait 10s before next round...");
  delay(10000);
}
