
#include <Arduino.h>
#include "PIControl.h"


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID  "d8c38db4-e40f-48e6-aac4-57e40b14d3c1"
#define CHARACTERISTIC_UUID_RX  "4e56b8c1-096c-4a58-97b8-0e262462b219"
#define CHARACTERISTIC_UUID_TX  "2f6c1ff8-258c-4454-9269-25f1d9cb1309"
#define DEVICE_NAME "HomeControl0.1"
#define LED_COMM "L"


//pin di controllo led: 25 --> GPIO25
#define LED 25          
#define PHOTORES 32
#define OUT_MAX 255 //in uscita ho 8 bit, quindi valori da 0 a 255
#define MIN_VALUE_READ 1900 //valore minimo letto dal ADC sul photoresistor
#define SAMPLE_TIME 250


BLECharacteristic *characteristicTX; 
bool deviceConnected = false; 
bool refreshServer = false;           //false:spento, true:acceso

enum LedMode {Duccio, Chill, Off, Manual}; //
std::map<LedMode, uint8_t> lightMap = { //in caso per risparmiare memoria posso usare direttamente un array
  {Duccio, 255},
  {Chill, 125},
  {Off, 0}
};
//LedMode led_state = Off;
double lightIn;
double lightOut = 0;
double lightDesired = 0; 

//cose da portare in scope con "using" per comodità
using std::string;

//definisco il tipo pfunc che è un puntatore a una funzione di tipo void senza parametri
using pfunc = void (*)();


//TO DO: controllo luminosità led (con fotoresistore o manuale), lettura temperatura

void powerOffLed() {
  lightDesired = lightMap[Off];
  //led_state = Off;
};

void disconnection() {
  refreshServer = true;
}

void apriTutto() {
  lightDesired = lightMap[Duccio];
  //led_state = Duccio;
}

void ledChill() {
  lightDesired = lightMap[Chill];
  //led_state = Chill;
}

//Dizionario delle funzioni e comandi da chiamare tramite comunicazione ble
std::map<string, pfunc> commandDictionary= {
  {"L_DUCCIO", apriTutto},
  {"L_OFF", powerOffLed},
  {"L_CHILL", ledChill},
  {"EXIT", disconnection}
};

//Dizionario dei valori settabili
std::map<string, double*> settableValues= {
  {LED_COMM, &lightDesired}
};


//Callbacks utilizzate dalle caratteristiche

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    string rxValue = characteristic->getValue();
    
    //si suppongono stringhe di almeno 2 caratteri come comandi
    if (rxValue.length() > 1) {
      //std::size_t pos = rxValue.find("?");
      int pos = -1;
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
        if (rxValue[i] == '?') pos = i;
      }
      Serial.println();
      //rxValue = rxValue + "\0";
      //se contiene '?' è una funzione con parametro: quello che c'è prima indica quale e quello che c'è dopo come settarlo
      if (pos != -1){
        int manValue = std::stoi(rxValue.substr(pos+1));
        string comm = rxValue.substr(0, pos);
        *(settableValues[comm]) = manValue + 0.0;
        //led_state = Manual;
      }
      else {
        pfunc f = commandDictionary[rxValue];
        (*f)();   
      }
      /*
      characteristicTX->setValue( "state:" + 0);
      characteristicTX->notify();
      */

    }
    characteristic->setValue("");
  }//onWrite
};

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
 
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      refreshServer = true;
    }
};

void serverInitialization() {
  BLEDevice::init(DEVICE_NAME);
  BLEServer *server = BLEDevice::createServer();

  // CallBack del server personalizzate 
  server->setCallbacks(new ServerCallbacks()); 

  // Creazione del servizio BLE
  BLEService *service = server->createService(SERVICE_UUID);

  characteristicTX = service->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  characteristicTX->addDescriptor(new BLE2902());
  BLECharacteristic *characteristic = service->createCharacteristic(
                                        CHARACTERISTIC_UUID_RX,
                                        BLECharacteristic::PROPERTY_WRITE
                                      );

  characteristic->setCallbacks(new CharacteristicCallbacks());
  
  // Start del sevizio
  service->start();
  server->getAdvertising()->start();

}


PIControl controller(&lightIn, &lightOut, &lightDesired, 0, 10, SAMPLE_TIME);

void setup() {
 
  serverInitialization();
  
  // Di default il led è spento
  pinMode(LED, OUTPUT);
  powerOffLed();
  Serial.begin(9600);
}

void loop() {
  
  if (refreshServer) {
    BLEDevice::deinit();
    serverInitialization();
    refreshServer = false;
  }
  delay(SAMPLE_TIME);
  lightIn = (analogRead(PHOTORES) - 1900)/7.0;
  if (lightIn < 0)
    lightIn = 0;
  else if (lightIn > OUT_MAX)
    lightIn = OUT_MAX;
  
  controller.compute();
  
  analogWrite(LED, round(lightOut));
}