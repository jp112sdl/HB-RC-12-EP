//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2019-04-03 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//#define NDISPLAY

#define DISPLAY_COLORED   1       // 1 = Farbdisplay; 0 = schwarz/weiß Display

//////////////////// DISPLAY DEFINITIONS /////////////////////////////////////
#include <GxEPD.h>
#if DISPLAY_COLORED == 1
#include <GxGDEW029Z10/GxGDEW029Z10.h>  // 2.9" b/w/r
//#include <GxGDEW026Z39/GxGDEW026Z39.h>    // 2.6" b/w/r
#else
#include <GxGDEW029T5/GxGDEW029T5.h>      // 2.9" b/w
#endif
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include "U8G2_FONTS_GFX.h"
#include "Icons.h"

#define GxRST_PIN  14
#define GxBUSY_PIN 11
#define GxDC_PIN   12
#define GxCS_PIN   17
#define DISPLAY_ROTATE     0 // 0 = 0° , 1 = 90°, 2 = 180°, 3 = 270°

GxIO_Class io(SPI, GxCS_PIN, GxDC_PIN, GxRST_PIN);
GxEPD_Class display(io, GxRST_PIN, GxBUSY_PIN);

U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
U8G2_FONTS_GFX u8g2Fonts(display);
//////////////////////////////////////////////////////////////////////////////

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>
#include <MultiChannelDevice.h>
#include <Remote.h>

#define CC1101_CS_PIN       4   // PB4
#define CC1101_GDO0_PIN     2   // PB2
//#define CC1101_SCK_PIN      7   // PB7
//#define CC1101_MOSI_PIN     5   // PB5
//#define CC1101_MISO_PIN     6   // PB6
#define CONFIG_BUTTON_PIN  15   // PD7
#define LED_PIN_1          10   // PD2
#define LED_PIN_2          13   // PD5
#define BTN01_PIN          A0 //A0   // PA0
#define BTN02_PIN          20 //A1   // PA1
#define BTN03_PIN          A1 //A2   // PA2
#define BTN04_PIN          21 //A3   // PA3
#define BTN05_PIN          A2 //A4   // PA4
#define BTN06_PIN          22 //A5   // PA5
#define BTN07_PIN          A3 //A6   // PA6
#define BTN08_PIN          23 //A7   // PA7
#define BTN09_PIN          A4 //23   // PC7
#define BTN10_PIN          A6 //22   // PC6
#define BTN11_PIN          A5 //21   // PC5
#define BTN12_PIN          A7 //20   // PC4

#define TEXT_LENGTH        10
#define CHANNEL_COUNT      12
#define PEERS_PER_CHANNEL   8

using namespace as;

struct {
  bool Inverted = false;
  uint16_t clFG = GxEPD_BLACK;
  uint16_t clBG = GxEPD_WHITE;
} DisplayDeviceConfig;

typedef struct {
  uint8_t IconNumber  = 0x00;
#if DISPLAY_COLORED == 1
  bool    IconColored = false;
  bool    TextColored = false;
#endif
  bool    TextBold    = false;
  String  TextContent        = "";
  bool ShowLine       = false;
} DisplayLineConfig;
DisplayLineConfig DisplayLines[CHANNEL_COUNT];

const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x46 - DISPLAY_COLORED, 0x00},   // Device ID
  "JPRCEP0001",                           // Device Serial
  {0xf3, 0x46 - DISPLAY_COLORED},         // Device Model
  0x10,                                   // Firmware Version
  as::DeviceType::Remote,                 // Device Type
  {0x00, 0x00}                            // Info Bytes
};

bool first              = false;
bool mustRefreshDisplay = false;

/**
   Configure the used hardware
*/
typedef LibSPI<CC1101_CS_PIN> SPIType;
typedef Radio<SPIType, CC1101_GDO0_PIN> RadioType;
typedef DualStatusLed<LED_PIN_1, LED_PIN_2> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;

class Hal: public BaseHal {
    AlarmClock btncounter;
  public:
    void init(const HMID& id) {
      BaseHal::init(id);
      battery.init(20, btncounter);
      battery.low(24);
      battery.critical(22);
    }

    void sendPeer () {
      --btncounter;
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

void updateDisplay(bool doit) {
  if (doit) {
    u8g2Fonts.begin(display);
    mustRefreshDisplay = false;
#ifndef NDISPLAY
    display.drawPaged(updateDisplay);
#else
    DPRINTLN(F("display.drawPaged(updateDisplay);"));
#endif
  }
}

class RefreshDisplayAlarm : public Alarm {
public:
  RefreshDisplayAlarm () :  Alarm(0)  {}
  virtual ~RefreshDisplayAlarm () {}
  void cancel (AlarmClock& clock) {
    clock.cancel(*this);
  }
  void set (uint32_t t,AlarmClock& clock) {
    clock.cancel(*this);
    Alarm::set(t);
    clock.add(*this);
  }
  virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
    updateDisplay(mustRefreshDisplay);
  }
};

DEFREGISTER(Reg0, MASTERID_REGS, DREG_LEDMODE, DREG_LOWBATLIMIT, 0x06)
class RCEPList0 : public RegList0<Reg0> {
  public:
    RCEPList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    bool displayInvertingHb(bool v) const { return this->writeRegister(0x06, 0x01,0,v); }
    bool displayInvertingHb() const { return this->readRegister(0x06, 0x01,0,false); }

    void defaults () {
      clear();
      displayInvertingHb(false);
      ledMode(1);
      lowBatLimit(24);
    }
};

DEFREGISTER(Reg1, CREG_LONGPRESSTIME, CREG_AES_ACTIVE, CREG_DOUBLEPRESSTIME, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x90, 0x92)
class RCEPList1 : public RegList1<Reg1> {
  public:
    RCEPList1 (uint16_t addr) : RegList1<Reg1>(addr) {}

    bool showLine (uint8_t value) const { return this->writeRegister(0x90, 0x01, 0, value & 0xff); }
    bool showLine () const { return this->readRegister(0x90, 0x01, 0, false); }

#if DISPLAY_COLORED == 1
    bool textColored (uint8_t value) const { return this->writeRegister(0x90, 0x02, 0, value & 0xff); }
    bool textColored () const { return this->readRegister(0x90, 0x02, 0, false); }

    bool iconColored (uint8_t value) const { return this->writeRegister(0x90, 0x04, 0, value & 0xff); }
    bool iconColored () const { return this->readRegister(0x90, 0x04, 0, false); }
#endif

    bool textBold (uint8_t value) const { return this->writeRegister(0x90, 0x08, 0, value & 0xff); }
    bool textBold () const { return this->readRegister(0x90, 0x08, 0, false); }

    bool iconNumber (uint8_t value) const { return this->writeRegister(0x92, value & 0xff); }
    uint8_t iconNumber () const { return this->readRegister(0x92, 0); }

    bool textContent (uint8_t value[TEXT_LENGTH]) const { for (int i = 0; i < TEXT_LENGTH; i++) { this->writeRegister(0x36 + i, value[i] & 0xff); } return true; }
    String textContent () const { String a = ""; for (int i = 0; i < TEXT_LENGTH; i++) { byte b = this->readRegister(0x36 + i, 0x20); if (b == 0x00) b = 0x20; a += char(b); } return a; }

    void defaults () {
      clear();
      //aesActive(false);
      uint8_t initValues[TEXT_LENGTH];
      memset(initValues, 0x20, TEXT_LENGTH);
      textContent(initValues);
      showLine(false);
      textBold(false);
      iconNumber(0);
#if DISPLAY_COLORED == 1
      textColored(false);
      iconColored(false);
#endif
    }
};

class ConfigChannel : public RemoteChannel<Hal,PEERS_PER_CHANNEL,RCEPList0, RCEPList1>  {
public:
  ConfigChannel () : RemoteChannel()  {}
    virtual ~ConfigChannel () {}

    void configChanged() {
      RemoteChannel::configChanged();
      if (first == false) {
        if (number() < 10) DPRINT("0"); DDEC(number());
        DPRINT(" SHOWLINE: "); DDEC(this->getList1().showLine());
        DPRINT(" ICON NUMBER: "); DDEC(this->getList1().iconNumber());
        DPRINT(" TEXT BOLD: "); DDEC(this->getList1().textBold());
        DPRINT(" TEXT CONTENT: "); DPRINT(this->getList1().textContent());
#if DISPLAY_COLORED == 1
        DPRINT(" TEXT COLORED: "); DDEC(this->getList1().textColored());
        DPRINT(" ICON COLORED: "); DDEC(this->getList1().iconColored());
#endif
        DPRINTLN("");
        mustRefreshDisplay = true;
      }
      uint8_t d = number() - 1;
      DisplayLines[d].ShowLine      = this->getList1().showLine();
      DisplayLines[d].IconNumber    = this->getList1().iconNumber();
      DisplayLines[d].TextBold      = this->getList1().textBold();
      DisplayLines[d].TextContent   = this->getList1().textContent();
#if DISPLAY_COLORED == 1
      DisplayLines[d].TextColored   = this->getList1().textColored();
      DisplayLines[d].IconColored   = this->getList1().iconColored();
#endif
    }
};

class RemoteType : public ChannelDevice<Hal, ConfigChannel, CHANNEL_COUNT, RCEPList0> {
  public:
    ConfigChannel        cdata[CHANNEL_COUNT];
    RefreshDisplayAlarm  rda;
  public:
    typedef ChannelDevice<Hal, ConfigChannel, CHANNEL_COUNT, RCEPList0> DeviceType;
    RemoteType (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) ,rda() {
      for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) DeviceType::registerChannel(cdata[i], i + 1);
    }
    virtual ~RemoteType () {}

    bool process(Message& msg) {
      HMID devid;
      this->getDeviceID(devid);
      if (msg.to() == devid)
        rda.set(millis2ticks(3000), sysclock);
      return ChannelDevice::process(msg);
    }

    virtual void configChanged () {
      uint8_t lowbat = getList0().lowBatLimit();
      if( lowbat > 0 ) {
        battery().low(lowbat);
      }
      bool inv = this->getList0().displayInvertingHb();
      if (inv == true) {
        DisplayDeviceConfig.clFG = GxEPD_WHITE;
        DisplayDeviceConfig.clBG = GxEPD_BLACK;
      } else {
        DisplayDeviceConfig.clFG = GxEPD_BLACK;
        DisplayDeviceConfig.clBG = GxEPD_WHITE;
      }

      if (first == false && DisplayDeviceConfig.Inverted != inv) mustRefreshDisplay = true;

      DPRINT("DISP INV: "); DDECLN(this->getList0().displayInvertingHb());
      DPRINT("LED MODE: "); DDECLN(this->getList0().ledMode());
      DPRINT("LOWBAT  : "); DDECLN(this->getList0().lowBatLimit());
      DisplayDeviceConfig.Inverted = inv;

    }
    ConfigChannel& btnChannel (uint8_t c) {
      return cdata[c - 1];
    }
};

RemoteType sdev(devinfo, 0x20);

class ConfBtn : public ConfigButton<RemoteType>  {
public:
  RefreshDisplayAlarm  rda;
  ConfBtn (RemoteType& i) : ConfigButton(i), rda()  {}
  virtual ~ConfBtn () {}

  virtual void state (uint8_t s) {
    if( s == ButtonType::longreleased ) {
      mustRefreshDisplay = true;
      rda.set(millis2ticks(20), sysclock);
    }
    ConfigButton::state(s);
  }
};
ConfBtn cfgBtn(sdev);


void setup() {
  first = true;

  initIcons();
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);

  sdev.init(hal);

  remoteChannelISR(sdev.btnChannel(1), BTN01_PIN);
  remoteChannelISR(sdev.btnChannel(2), BTN02_PIN);
  remoteChannelISR(sdev.btnChannel(3), BTN03_PIN);
  remoteChannelISR(sdev.btnChannel(4), BTN04_PIN);
  remoteChannelISR(sdev.btnChannel(5), BTN05_PIN);
  remoteChannelISR(sdev.btnChannel(6), BTN06_PIN);
  remoteChannelISR(sdev.btnChannel(7), BTN07_PIN);
  remoteChannelISR(sdev.btnChannel(8), BTN08_PIN);
  remoteChannelISR(sdev.btnChannel(9), BTN09_PIN);
  remoteChannelISR(sdev.btnChannel(10), BTN10_PIN);
  remoteChannelISR(sdev.btnChannel(11), BTN11_PIN);
  remoteChannelISR(sdev.btnChannel(12), BTN12_PIN);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();

  display.init(57600);
  first = false;
  DPRINTLN("setup done.");
}

void loop() {
  yield();
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    if (hal.battery.critical()) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep<>>(hal);
  }
}

uint16_t centerPosition(const char * text) {
  return (display.width() / 2) - (u8g2Fonts.getUTF8Width(text) / 2);
}

void updateDisplay() {
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setBackgroundColor(DisplayDeviceConfig.clBG);
  display.fillScreen(DisplayDeviceConfig.clBG);
  u8g2Fonts.setFont(u8g2_font_helvB18_tf);
  for (uint16_t i = 0; i < CHANNEL_COUNT; i++) {
    //Text Font festlegen (B)old oder (R)egular
    u8g2Fonts.setFont(DisplayLines[i].TextBold == true ?  u8g2_font_helvB14_tf : u8g2_font_helvR14_tf );
    //Text Farbe festlegen
#if DISPLAY_COLORED == 1
    u8g2Fonts.setForegroundColor(DisplayLines[i].TextColored == true ? GxEPD_RED : DisplayDeviceConfig.clFG);
#else
    u8g2Fonts.setForegroundColor(DisplayDeviceConfig.clFG);
#endif
    //Text Zeichenersetzungen
    String viewText = DisplayLines[i].TextContent;
    viewText.trim();
    viewText.replace("{", "ä");
    viewText.replace("|", "ö");
    viewText.replace("}", "ü");
    viewText.replace("[", "Ä");
    viewText.replace("#", "Ö");
    viewText.replace("$", "Ü");
    viewText.replace("~", "ß");
    viewText.replace("'", "=");

    if (DisplayLines[i].ShowLine == true)
      display.drawLine(0, ((i / 2) + 1) * 49, display.width(), ((i / 2) + 1) * 49, DisplayDeviceConfig.clFG);

    uint8_t icon_width = 0;
    if (DisplayLines[i].IconNumber > 1) {
     uint8_t  icon_number = DisplayLines[i].IconNumber - 2;
     icon_width = Icons[icon_number].width;
     uint16_t icon_top = (Icons[icon_number].height - ( Icons[icon_number].height / 2)) + ((i / 2) * 49);
     uint16_t icon_left = (i % 2 == 0) ? 0 : display.width() - Icons[icon_number].width;
#if DISPLAY_COLORED == 1
     display.drawBitmap(Icons[icon_number].Icon, icon_left , icon_top, icon_width, Icons[icon_number].height, DisplayLines[i].IconColored == true ? GxEPD_RED : DisplayDeviceConfig.clFG, DisplayDeviceConfig.Inverted ? GxEPD::bm_normal : GxEPD::bm_default );
#else
     display.drawBitmap(Icons[icon_number].Icon, icon_left , icon_top, icon_width, Icons[icon_number].height, DisplayDeviceConfig.clFG, DisplayDeviceConfig.Inverted ? GxEPD::bm_normal : GxEPD::bm_default );
#endif
    } else {
      if (DisplayLines[i].IconNumber == 1) {
       uint16_t top =  (i / 2) * 49;
       if (i % 2 == 0) {
         icon_width = u8g2Fonts.getUTF8Width("<");
         u8g2Fonts.setCursor(0, top + 18); u8g2Fonts.print("<");
       } else {
         icon_width = u8g2Fonts.getUTF8Width(">");
         u8g2Fonts.setCursor(display.width() - icon_width, top + 42); u8g2Fonts.print(">");
       }
      }
    }

    uint16_t leftTextPos = 0;
    leftTextPos = centerPosition(viewText.c_str());
    u8g2Fonts.setCursor(leftTextPos, ((i / 2) * 49) + ((i % 2 == 0) ? 21 : 43));
    u8g2Fonts.print(viewText);
  }
}
