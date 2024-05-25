#define PIN 26
#define NUM_MARKS 26
#define NUM_LEDS 26
#define SERIAL_PORT_SPEED 115200

#include "Adafruit_NeoPixel.h"
#include <LiquidCrystal_I2C_ESP32.h>

#define MARK_COLOR SwimStrip.Color(0, 0, 255)  // Blue

int delayMark = 400;
int CurrentMark = 1;
boolean MarkDirectionUp = true;

Adafruit_NeoPixel SwimStrip = Adafruit_NeoPixel(NUM_MARKS, PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C_ESP32 lcd(0x3F, 16, 2);

void TurnOnMark(int mark, uint32_t markColor = MARK_COLOR) {
  // mark = 1 to NUM_MARKS
  SwimStrip.fill(markColor, (mark - 1) * (NUM_LEDS / NUM_MARKS), NUM_LEDS / NUM_MARKS);
  SwimStrip.show();
}

void TurnOffMark(int mark) {
  SwimStrip.fill(SwimStrip.Color(0, 0, 0), (mark - 1) * (NUM_LEDS / NUM_MARKS), NUM_LEDS / NUM_MARKS);
  SwimStrip.show();
}

void SwimTimer() {
  Serial.println("SwimTimer Marker: " + (String)CurrentMark);
  TurnOnMark(CurrentMark);
  if (MarkDirectionUp) {
    TurnOffMark(CurrentMark - 1);
    CurrentMark++;
  } else {
    TurnOffMark(CurrentMark + 1);
    CurrentMark--;
  }
  SwimStrip.show();

  if ((CurrentMark == NUM_MARKS) || (CurrentMark == 1)) {
    MarkDirectionUp = not(MarkDirectionUp);
  }
}

void setup() {
  delay(2000);
  Serial.begin(SERIAL_PORT_SPEED);
  Serial.println("Start");

  lcd.init();
  lcd.backlight();  //enables backlight
  lcd.clear();

  SwimStrip.begin();
  SwimStrip.setBrightness(50);
  SwimStrip.clear();
  SwimStrip.show();
  delay(1000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Marker: " + (String)CurrentMark);
  SwimTimer();
  delay(delayMark);

}
