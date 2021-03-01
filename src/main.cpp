#include <M5Stack.h>

#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

#define HORIZONTAL_RESOLUTION 320
#define VERTICAL_RESOLUTION   240
#define POSITION_OFFSET_Y      20
#define POSITION_OFFSET_X      30
#define SIGNAL_LENGTH (HORIZONTAL_RESOLUTION - POSITION_OFFSET_X)

#define Y_SCALE_MIN_MA 0


float oldSignal[SIGNAL_LENGTH];
float adcBuffer[SIGNAL_LENGTH];
uint16_t bufferIndex = 0;
float oldAvg = 0;

#define M5STACKFIRE_SPEAKER_PIN 25
uint16_t yScaleMaxMa = 5;
uint16_t oldYScaleMaxMa = 5;
float oldDisplayedValue = 0;
float maxCurrentValue = 0;

void setup(void)
{
  M5.begin(true, false, true, true);

  dacWrite(M5STACKFIRE_SPEAKER_PIN, 0); // make sure that the speaker is quite

  Serial.begin(115200);
  while (!Serial) {
    // will pause Zero, Leonardo, etc until serial console opens
    delay(1);
  }

  uint32_t currentFrequency;

  Serial.println("Hello!");

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();

  Serial.println("Measuring voltage and current with INA219 ...");



  dacWrite(M5STACKFIRE_SPEAKER_PIN, 0); // make sure that the speaker is quite
  M5.Lcd.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.fillRect(10, 1, 150, 160, BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(GREEN);  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextSize(1);


  for(int i = 0; i< SIGNAL_LENGTH; i++) {
    oldSignal[i] = 200;
  }

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Current : ");

  M5.Lcd.setCursor(100, 0);
  M5.Lcd.printf("Max : ");

  M5.Lcd.setCursor(200, 0);
  M5.Lcd.printf("Average : ");

  M5.Lcd.setCursor(0, POSITION_OFFSET_Y);
  M5.Lcd.printf("%2dmA", yScaleMaxMa);

  M5.Lcd.setCursor(0, VERTICAL_RESOLUTION - POSITION_OFFSET_Y);
  M5.Lcd.printf("%2dmA", Y_SCALE_MIN_MA);
}


void loop(void)
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float current_raw = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  current_mA = ina219.getCurrent_mA();


  if(current_mA != oldDisplayedValue) {
    //M5.Lcd.fillRect(0, 0, 320, POSITION_OFFSET_Y, BLACK);

    M5.Lcd.setCursor(50, 0);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.printf("%2.2f", oldDisplayedValue);
    M5.Lcd.print(" mA");

    M5.Lcd.setCursor(50, 0);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.printf("%2.2f", current_mA);
    M5.Lcd.print(" mA");

    oldDisplayedValue = current_mA;
  }

  if(oldYScaleMaxMa != yScaleMaxMa) {
    M5.Lcd.setCursor(0, POSITION_OFFSET_Y);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.printf("%2dmA", oldYScaleMaxMa);

    M5.Lcd.setCursor(0, POSITION_OFFSET_Y);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.printf("%2dmA", yScaleMaxMa);
    oldYScaleMaxMa = yScaleMaxMa;
  }

  adcBuffer[bufferIndex] =  current_mA;
  uint16_t x = bufferIndex;

  m5.Lcd.drawLine(x + POSITION_OFFSET_X, POSITION_OFFSET_Y, x + POSITION_OFFSET_X, 240, BLACK);
  m5.Lcd.drawPixel(x + POSITION_OFFSET_X, VERTICAL_RESOLUTION - POSITION_OFFSET_Y, LIGHTGREY);
  m5.Lcd.drawLine(x+1 + POSITION_OFFSET_X, POSITION_OFFSET_Y, x+1 + POSITION_OFFSET_X, VERTICAL_RESOLUTION - POSITION_OFFSET_Y, YELLOW);


  int16_t y = map(adcBuffer[bufferIndex], Y_SCALE_MIN_MA, yScaleMaxMa, VERTICAL_RESOLUTION - (POSITION_OFFSET_Y), (POSITION_OFFSET_Y));
  if(y < POSITION_OFFSET_Y)
    y = POSITION_OFFSET_Y;

  auto max = *std::max_element(adcBuffer, adcBuffer+SIGNAL_LENGTH);
  auto avg = std::accumulate(adcBuffer, adcBuffer+SIGNAL_LENGTH, 0.0f);
  avg /= SIGNAL_LENGTH;
  if(max != maxCurrentValue) {
    M5.Lcd.setCursor(135, 0);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.printf("%2.2f", maxCurrentValue);

    M5.Lcd.setCursor(135, 0);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.printf("%2.2f", max);
    maxCurrentValue = max;
  }

  if(oldAvg != avg) {
    M5.Lcd.setCursor(255, 0);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.printf("%2.2f", oldAvg);

    M5.Lcd.setCursor(255, 0);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.printf("%2.2f", avg);
    oldAvg = avg;
  }

  auto prevValue = ((x > 0) ? oldSignal[x-1] : oldSignal[SIGNAL_LENGTH-1]);
  m5.Lcd.drawLine(x + POSITION_OFFSET_X, prevValue, x + POSITION_OFFSET_X, y, RED);



  //oldSignal[x] = y;




  bufferIndex++;
  if(bufferIndex == HORIZONTAL_RESOLUTION - POSITION_OFFSET_X)
    bufferIndex = 0;

  M5.update();
  if (M5.BtnA.wasReleased() && yScaleMaxMa > 0)
    yScaleMaxMa--;

  if (M5.BtnC.wasReleased() && yScaleMaxMa < 400)
    yScaleMaxMa++;



  //delay(0);
}