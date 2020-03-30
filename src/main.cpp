/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/
//#define SCAN_TIME 10     // seconds
//#define WAIT_WIFI_LOOP 5 // around 4 seconds for 1 loop
//#define SLEEP_TIME 30000   // seconds
//#define RSSILIMIT -60


//https://www.mikroshop.ch/Mikrokontroller.html?gruppe=3&artikel=1820

#define SCAN_TIME 2     // seconds
#define WAIT_WIFI_LOOP 5 // around 4 seconds for 1 loop
#define SLEEP_TIME 10   // seconds
#define RSSILIMIT -60

#include <Arduino.h>
#include <sstream>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

std::stringstream ss;
bool data_sent = false;
int wait_wifi_counter = 0;

int proximity_count = 0;
int total_proximity_count = 0;
int loopcount = 0;


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
   // log_e("Advertised Device: %s \n", advertisedDevice.toString().c_str());
  }
};

void setup()
{
  log_e("ESP32 BLE Scanner");

  // disable brownout detector to maximize battery life
  log_e("disable brownout detector");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  log_e("BLEDevice::init()");
  BLEDevice::init("PROXIMITYALERT");
  BLEDevice::setPower(ESP_PWR_LVL_N12); 

  
  log_e("Start server\n", SCAN_TIME);

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Hello World says Neil");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();



}

void loop()
{

  // put your main code here, to run repeatedly:
  BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false); //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);

  log_e("Start BLE scan for %d seconds...\n", SCAN_TIME);

  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();

  bool noalert = true;
  loopcount+=1;
  for (int i = 0; i < count; i++)
  {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);

    if (d.haveName())
    {
      if(d.getName()=="PROXIMITYALERT"){
        if(d.getRSSI()> RSSILIMIT){
          noalert = false;
          proximity_count += 1;
          log_e("***RSSI: %d***", d.getRSSI());
          if(proximity_count >= 5){
            log_e("ALERT ALERT ALERT");
          }
        }
        else{
          log_e("RSSI: %d", d.getRSSI());
        }
      }
    }
  }
  if(noalert){
    proximity_count = 0;
  }
  else{
    total_proximity_count += 1;
  }

  log_e("Scan done!");
  log_e("FOUND DEVICE FOR %d CONCURRENT CYCLES", proximity_count);
  log_e("TOTAL %d CYCLES OF MEASURED %d CYCLES TOO CLOSE", total_proximity_count, loopcount);




  delay(SLEEP_TIME);


/*
    esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000); // translate second to micro second

    log_e("Enter deep sleep for %d seconds...\n", SLEEP_TIME);
    
    esp_deep_sleep_start();
    */
}