// Hardware Setup
// ESP32 Board
// 26 leds - pin 25 - DATA
// LED Strip 26 marks (1 leds per mark)
// SDA	GPIO 21, SCL	GPIO 22

#include "Adafruit_NeoPixel.h"
#include "Simpletimer.h"
#include <LiquidCrystal_I2C_ESP32.h>
#include "BluetoothSerial.h"

#define SERIAL_PORT_SPEED 115200

typedef struct {
  int _NumLeds;
  int _NumMarks;
  int _Lane;
  int _Mode;  // not running = 0, running = 1, waiting = 2
  int _Speed;
  int _RestDistance;
  int _RestTime;
  int _Brightness;
  String _ColorName;
  int _Color;
  int CurrentMark;
  boolean MarkDirectionUp;
  Adafruit_NeoPixel SwimStrip;
  Simpletimer timerMarker{};
} SwimLane;

SwimLane Pool[2] = {
  {26, 26, 1, 0, 90, 400, 15, 50, "Red", 0xFF0000, 1, true, Adafruit_NeoPixel(26, 25, NEO_GRB + NEO_KHZ800)},
  {26, 26, 1, 0, 90, 400, 15, 50, "Blue", 0x0000FF, 1, true, Adafruit_NeoPixel(26, 26, NEO_GRB + NEO_KHZ800)}
};
String MsgBT = "";
String strTemp;
int i;

LiquidCrystal_I2C_ESP32 lcd(0x3F, 16, 2);
BluetoothSerial SerialBT;

void TurnOnMark(int Lane, int mark) { 
  // Lane = 0..n, mark = 1 to NUM_MARKS
  Pool[Lane].SwimStrip.fill(Pool[Lane]._Color, (mark - 1) * (Pool[Lane]._NumLeds / Pool[Lane]._NumMarks), Pool[Lane]._NumLeds / Pool[Lane]._NumMarks);
  Pool[Lane].SwimStrip.show();
}

void TurnOffMark(int Lane, int mark) {
  // Lane = 0..n, mark = 1 to NUM_MARKS
  Pool[Lane].SwimStrip.fill(0, (mark - 1) * (Pool[Lane]._NumLeds / Pool[Lane]._NumMarks), Pool[Lane]._NumLeds / Pool[Lane]._NumMarks);
  Pool[Lane].SwimStrip.show();
}

String SecsToMMSS(int Seconds) {
  String MMSS = "";
  if (Seconds / 60 < 10) MMSS += "0";
  MMSS += (String)(Seconds / 60);
  MMSS += ":";
  if (Seconds % 60 < 10) MMSS += "0";
  MMSS += (String)(Seconds % 60);
  return MMSS;
}

void setup() {
  delay(2000);
  Serial.begin(SERIAL_PORT_SPEED);
  Serial.println("Start");

  lcd.init();
  lcd.backlight();  //enables backlight
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Start");

  Pool[0].SwimStrip.begin();
  Pool[0].SwimStrip.setBrightness(Pool[0]._Brightness);
  Pool[0].SwimStrip.clear();  // trun off all the leds
  Pool[0].SwimStrip.setPixelColor(4,0xFF);
  Pool[0].SwimStrip.show();
  Pool[1].SwimStrip.begin();
  Pool[1].SwimStrip.setBrightness(Pool[1]._Brightness);
  Pool[1].SwimStrip.clear();  // trun off all the leds
  Pool[1].SwimStrip.show();

  SerialBT.begin("SwimLine");
  MsgBT.reserve(150);
  //MsgBT = "U1,90,100,15,40,Red";  // Lane,Speed,Distance,RestTime,Brightness,ColorName
  //MsgParse(MsgBT);

  delay(2000);
}

void loop() {
  if (SerialBT.available()) {
    MsgBT = SerialBT.readStringUntil('\n');
    Serial.println("Bluetooth message: " + MsgBT);
    i = MsgBT[1] - '1';
    switch (MsgBT[0]) {
      case 'C':  // Clear / Restart
        Pool[i]._Mode = 0;
        Pool[i].CurrentMark = 1;
        Pool[i].MarkDirectionUp = true;
        Pool[i].SwimStrip.clear();
        Pool[i].SwimStrip.show();
        break;
      case 'T':  // Start / Continue
        Pool[i]._Mode = 1;
        break;
      case 'S':  // Stop / Pause
        Pool[i]._Mode = 0;
        break;
      case 'U':  // Update in possible while running
        MsgParse(MsgBT);
        Pool[i].SwimStrip.setBrightness(Pool[i]._Brightness);
        Pool[i].SwimStrip.show();
        break;
    }
  }
  if (Pool[0]._Mode == 1) {
    if (Pool[0].timerMarker.timer(Pool[0]._Speed / (4.0 * (Pool[0]._NumMarks - 1)) * 1000.0)) {
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("1: " + SecsToMMSS(Pool[0]._Speed) + " " + (String)Pool[0].CurrentMark);
      SwimTimer(0);
    }
  }
  if (Pool[1]._Mode == 1) {
    if (Pool[1].timerMarker.timer(Pool[1]._Speed / (4.0 * (Pool[1]._NumMarks - 1)) * 1000.0)) {
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("2: " + SecsToMMSS(Pool[1]._Speed) + " " + (String)Pool[1].CurrentMark);
      SwimTimer(1);
    }
  }
}

void SwimTimer(int Lane) { // Lane = 0..n
  Serial.println((String)Lane + "- Marker: " + (String)Pool[Lane].CurrentMark);
  TurnOnMark(Lane, Pool[Lane].CurrentMark);
  if (Pool[Lane].MarkDirectionUp) {
    TurnOffMark(Lane, Pool[Lane].CurrentMark - 1);
    Pool[Lane].CurrentMark++;
  } else {
    TurnOffMark(Lane, Pool[Lane].CurrentMark + 1);
    Pool[Lane].CurrentMark--;
  }
  Pool[Lane].SwimStrip.show();

  if ((Pool[Lane].CurrentMark == Pool[Lane]._NumMarks) || (Pool[Lane].CurrentMark == 1)) {
    Pool[Lane].MarkDirectionUp = not(Pool[Lane].MarkDirectionUp);
  }
}

void MsgParse(String Msg) { // // Lane(1-2),Speed,Distance,RestTime,Brightness,ColorName
  int Lane; //
  Lane = (MsgBT.substring(1, MsgBT.indexOf(","))).toInt()-1;
  MsgBT = MsgBT.substring(MsgBT.indexOf(",") + 1);
  Pool[Lane]._Speed = (MsgBT.substring(0, MsgBT.indexOf(","))).toInt();
  MsgBT = MsgBT.substring(MsgBT.indexOf(",") + 1);
  Pool[Lane]._RestDistance = (MsgBT.substring(0, MsgBT.indexOf(","))).toInt();
  MsgBT = MsgBT.substring(MsgBT.indexOf(",") + 1);
  Pool[Lane]._RestTime = (MsgBT.substring(0, MsgBT.indexOf(","))).toInt();
  MsgBT = MsgBT.substring(MsgBT.indexOf(",") + 1);
  Pool[Lane]._Brightness = (MsgBT.substring(0, MsgBT.indexOf(","))).toInt();
  MsgBT = MsgBT.substring(MsgBT.indexOf(",") + 1);
  Pool[Lane]._ColorName = (MsgBT.substring(0, MsgBT.indexOf(",")));
  Pool[Lane]._Color = ConvertColor(Pool[Lane]._ColorName);

  Serial.print("Msg Update" + (String)Lane + ": " + (String)Pool[Lane]._Speed + "," + (String)Pool[Lane]._RestDistance + ",");
  Serial.println((String)Pool[Lane]._RestTime + "," + (String)Pool[Lane]._Brightness + "," + Pool[Lane]._ColorName);
}


int ConvertColor(String StrColor) {
  switch (StrColor[0]) {
    case 'R':
      return 0xFF0000;
      break;
    case 'G':
      return 0x00FF00;
      break;
    case 'B':
      return 0x0000FF;
      break;
    case 'Y':
      return 0xFFFF00;
      break;
    case 'P':
      return 0x800080;
      break;
    default:
      return 0xF5F5F5; // whitesmoke
  }
}
