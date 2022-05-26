// Shortcut key for Mute using M5ATOM (StickC)
//  press btnB : mode chang
//  press btnA : mute ON/OFF

// mode (color / key)
// mode=0 : green (110, 190,  75) : WebEx    : Ctrl+M
// mode=1 : blue  ( 49, 198, 237) : Zoom     : Alt+A
// mode=2 : purple( 70,  71, 117) : MS Teams : Ctrl+Shift+M
// mode=3 : red   (110,   0,   0) : Google Hangout : Ctrl+D

// ref:
//   https://qiita.com/poruruba/items/6e4e29068a28f5ee1711
//   https://qiita.com/poruruba/items/eff3fedb1d4a63cbe08d
//   http://okiraku-camera.tokyo/blog/?p=8333
//   https://github.com/akita11/ATOMmute/blob/master/ATOMmute.ino

#include "M5StickC.h"
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
//#include "esp_wifi.h"

// BLE
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

uint8_t mode = 0;
bool connected = false;

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    connected = true;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
  }

  void onDisconnect(BLEServer* pServer){
    connected = false;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(false);
  }
};

// BLE
void taskServer(void*){
  BLEDevice::init("M5StickMute");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1); // <-- input REPORTID from report map
  output = hid->outputReport(1); // <-- output REPORTID from report map

  std::string name = "akita11";
  hid->manufacturer()->setValue(name);

  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00,0x02);

  uint32_t pass_key = 123456;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &pass_key, sizeof(uint32_t));

    const uint8_t report[] = {
      USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
      USAGE(1),           0x06,       // Keyboard
      COLLECTION(1),      0x01,       // Application
      REPORT_ID(1),       0x01,        //   Report ID (1)
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
      REPORT_COUNT(1),    0x08,
      HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
      REPORT_SIZE(1),     0x08,
      HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
      REPORT_SIZE(1),     0x08,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
      REPORT_SIZE(1),     0x01,
      USAGE_PAGE(1),      0x08,       //   LEDs
      USAGE_MINIMUM(1),   0x01,       //   Num Lock
      USAGE_MAXIMUM(1),   0x05,       //   Kana
      HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
      REPORT_SIZE(1),     0x03,
      HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      END_COLLECTION(0)
    };

  hid->reportMap((uint8_t*)report, sizeof(report));
  hid->startServices();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
  hid->setBatteryLevel(7);

  Serial.println("Advertising started");
  delay(portMAX_DELAY);
};

enum KEY_MODIFIER_MASK {
  KEY_MASK_CTRL = 0x01,
  KEY_MASK_SHIFT = 0x02,
  KEY_MASK_ALT = 0x04,
  KEY_MASK_WIN = 0x08
};

void setup() {
  M5.begin(true, false, true); // enable serial & display, disable I2C

  Serial.begin(9600);
  Serial.println("Start");
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillScreen(RED);
  M5.Lcd.drawString("Google Meet",0,0);
  mode=0;
  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);
//  esp_wifi_stop(); // hangs up?
}

void loop() {
  M5.update();

  if (M5.BtnB.wasPressed()){
    mode = (mode + 1) % 4;
    Serial.print("mode is switched to "); Serial.println(mode);
    // mode=0 : green (110, 190,  75) : WebEx    : Ctrl+M
    // mode=1 : blue  ( 49, 198, 237) : Zoom     : Alt+A
    // mode=2 : purple( 70,  71, 117) : MS Teams : Ctrl+Shift+M
    // mode=3 : red   (110,   0,   0) : Google Hangout : Ctrl+D
    if (mode == 0) {
      M5.Lcd.fillScreen(GREEN);
      M5.Lcd.drawString("WebEx",0,0);
    }
    if (mode == 1) {
      M5.Lcd.fillScreen(BLUE);
      M5.Lcd.drawString("Zoom",0,0);
    }
    if (mode == 2) {
      M5.Lcd.fillScreen(PURPLE);
      M5.Lcd.drawString("MS Teams",0,0);
    }
    if (mode == 3) {
      M5.Lcd.fillScreen(RED);
      M5.Lcd.drawString("Meet",0,0);
    }
    while(M5.BtnB.read() == 1) delay(10);
  }
  else if( M5.BtnA.wasPressed() ){
    Serial.println("Btn pressed");
    if(connected){
      // BL
      Serial.println("KBD");
      // Keycode (USB HID Usage)
      // http://hp.vector.co.jp/authors/VA003720/lpproj/others/kbdjpn.htm
      // 1st arg: modifier; KEY_MASK_CTRL etc.

      Serial.println("press");
      uint8_t msg[] = {0x00, 0x00, 0x00, 0x0,0x0,0x0,0x0,0x0};
      if (mode == 0){msg[0] = KEY_MASK_CTRL; msg[2] = 0x10; }// Ctrl+M for WebEx
      else if (mode == 1){msg[0] = KEY_MASK_ALT; msg[2] = 0x04; }// Alt+A for Zoom
      else if (mode == 2){msg[0] = KEY_MASK_CTRL | KEY_MASK_SHIFT; msg[2] = 0x10; }// Ctrl+Shift+M for MS Teams
      else if (mode == 3){msg[0] = KEY_MASK_CTRL; msg[2] = 0x07; }// Ctrl+D for GoogleHangout
      input->setValue(msg, sizeof(msg));
      input->notify();
      msg[2] = 0; input->setValue(msg, sizeof(msg)); input->notify();
      msg[0] = 0; input->setValue(msg, sizeof(msg)); input->notify();
    }
  }
}
