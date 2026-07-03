#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define DEMO_MODE 1


#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define LIGHT_PIN 33     // digital LDR module (HIGH/LOW via onboard comparator)

DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


const int SOIL_DRY_RAW = 4000;  
const int SOIL_WET_RAW = 1450;  

const float SOIL_SATURATED_PCT = 85.0;  
const float SOIL_WILT_PCT      = 15.0;  
const float SOIL_WATER_JUMP_PCT = 20.0; 


#if DEMO_MODE
  const unsigned long LIGHT_LOG_INTERVAL = 5000UL;      
  const int LIGHT_HISTORY_SIZE = 60;                    
  const unsigned long EXPECTED_DRYDOWN_MS = 2UL*60UL*1000UL; 
  const unsigned long OVERWATER_HOLD_MS   = 1UL*60UL*1000UL; 
#else
  const unsigned long LIGHT_LOG_INTERVAL = 5UL*60UL*1000UL;    
  const int LIGHT_HISTORY_SIZE = 288;                          
  const unsigned long EXPECTED_DRYDOWN_MS = 5UL*24UL*60UL*60UL*1000UL; 
  const unsigned long OVERWATER_HOLD_MS   = 2UL*24UL*60UL*60UL*1000UL; 
#endif

uint8_t lightHistory[LIGHT_HISTORY_SIZE] = {0};
int lightHistIndex = 0;
int lightHistCount = 0;
unsigned long lastLightLog = 0;
bool hasLightData = false;

.
const float LIGHT_IDEAL_MIN = 25.0, LIGHT_IDEAL_MAX = 60.0;
const float LIGHT_ACC_MIN = 12.0,  LIGHT_ACC_MAX = 80.0;


const float TEMP_LOW = 15.0, TEMP_HIGH = 35.0;
const float HUM_LOW  = 30.0, HUM_HIGH  = 80.0;

const float TEMP_DECAY_RATE = 0.20;
const float HUM_DECAY_RATE  = 0.15;
const float WILT_DECAY_RATE = 0.30;
const float OVERWATER_DECAY_RATE = 0.02;
const float RECOVERY_RATE = 0.10;


const float SOIL_IDEAL_MIN = 35.0, SOIL_IDEAL_MAX = 75.0;
const float TEMP_IDEAL_MIN = 18.0, TEMP_IDEAL_MAX = 28.0;
const float HUM_IDEAL_MIN  = 40.0, HUM_IDEAL_MAX  = 60.0;


const unsigned long SCREEN_CYCLE_MS = 5000UL;   
const float GUIDANCE_TRIGGER_SCORE = 80.0;      


int lightSmoothBuf[5] = {0};
int lightSmoothIdx = 0;

float lightScore = 100, soilScore = 100, tempScore = 100, humScore = 100;

unsigned long tempBadStart = 0, humBadStart = 0;
unsigned long tempGoodStart = 0, humGoodStart = 0;

float prevSoilPct = -1;
unsigned long lastWaterTime = 0;
unsigned long wiltStart = 0;
unsigned long wetStart = 0;
unsigned long overwaterBadStart = 0;
unsigned long soilRecoverStart = 0;


float safeTemp() { float t = dht.readTemperature(); return isnan(t) ? 25 : t; }
float safeHum()  { float h = dht.readHumidity();    return isnan(h) ? 50 : h; }

float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

float decay(float score, unsigned long start, float ratePerSec) {
  if (start == 0) return score;
  float sec = (millis() - start) / 1000.0;
  return constrain(100.0 - (sec * ratePerSec), 0.0, 100.0);
}

float recover(float score, unsigned long start, float ratePerSec, float cap) {
  if (start == 0) return score;
  float sec = (millis() - start) / 1000.0;
  return constrain(score + (sec * ratePerSec), 0.0, cap);
}

float scoreFromRange(float value, float idealMin, float idealMax, float accMin, float accMax) {
  if (value >= idealMin && value <= idealMax) return 100.0;
  if (value < idealMin) {
    if (value <= accMin) return 0.0;
    return 100.0 * (value - accMin) / (idealMin - accMin);
  } else {
    if (value >= accMax) return 0.0;
    return 100.0 * (accMax - value) / (accMax - idealMax);
  }
}

// 6-tier status (simplified from the original 10-tier version)
String getPlantStatus(float score) {
  if (score >= 90) return "EXCELLENT";
  else if (score >= 75) return "HEALTHY";
  else if (score >= 60) return "GOOD";
  else if (score >= 40) return "STRESS";
  else if (score >= 20) return "CRITICAL";
  else return "DYING";
}


String getGuidance(float soilPct, float temp, float hum, float litPercent, float lScore, bool lightAvail) {
  float soilSeverity  = scoreFromRange(soilPct, SOIL_IDEAL_MIN, SOIL_IDEAL_MAX, SOIL_WILT_PCT, SOIL_SATURATED_PCT);
  float tempSeverity  = scoreFromRange(temp, TEMP_IDEAL_MIN, TEMP_IDEAL_MAX, TEMP_LOW, TEMP_HIGH);
  float humSeverity   = scoreFromRange(hum, HUM_IDEAL_MIN, HUM_IDEAL_MAX, HUM_LOW, HUM_HIGH);
  float lightSeverity = lightAvail ? lScore : 999; 

  float severities[4] = {soilSeverity, tempSeverity, humSeverity, lightSeverity};
  int worstIdx = 0;
  for (int i = 1; i < 4; i++) {
    if (severities[i] < severities[worstIdx]) worstIdx = i;
  }

  if (severities[worstIdx] >= GUIDANCE_TRIGGER_SCORE) {
    return "All good!\nNo action needed";
  }

  switch (worstIdx) {
    case 0: // soil
      if (soilPct <= SOIL_WILT_PCT) return "Water needed!\nSoil is too dry";
      else return "Too wet - hold\noff watering";
    case 1: // temp
      if (temp > TEMP_HIGH) return "Too hot - move\nto cooler spot";
      else return "Too cold - move\nto warmer spot";
    case 2: // humidity
      if (hum < HUM_LOW) return "Air too dry -\nmist the plant";
      else return "Too humid -\nimprove airflow";
    case 3: // light
      if (litPercent < LIGHT_IDEAL_MIN) return "Needs more light\nmove to window";
      else return "Too much light\nadd some shade";
  }
  return "Monitoring...";
}


void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  pinMode(LIGHT_PIN, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}


void loop() {
  unsigned long now = millis();

  float temp = safeTemp();
  float hum = safeHum();
  int soilRaw = analogRead(SOIL_PIN);
  float soilPct = constrain(mapFloat(soilRaw, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100), 0.0, 100.0);

 
  lightSmoothBuf[lightSmoothIdx] = digitalRead(LIGHT_PIN);
  lightSmoothIdx = (lightSmoothIdx + 1) % 5;
  int litCount = 0;
  for (int i = 0; i < 5; i++) litCount += lightSmoothBuf[i];
  int inverted = 5 - litCount;             
  int brightNow = (inverted >= 3) ? 1 : 0;

  if (lastLightLog == 0 || now - lastLightLog >= LIGHT_LOG_INTERVAL) {
    lastLightLog = now;
    lightHistory[lightHistIndex] = brightNow;
    lightHistIndex = (lightHistIndex + 1) % LIGHT_HISTORY_SIZE;
    if (lightHistCount < LIGHT_HISTORY_SIZE) lightHistCount++;
    hasLightData = true;
  }

  float litPercent = 0;
  if (hasLightData) {
    int sum = 0;
    for (int i = 0; i < lightHistCount; i++) sum += lightHistory[i];
    litPercent = (100.0 * sum) / lightHistCount;
    lightScore = scoreFromRange(litPercent, LIGHT_IDEAL_MIN, LIGHT_IDEAL_MAX, LIGHT_ACC_MIN, LIGHT_ACC_MAX);
  }


  bool soilProblem = false;
  float soilProblemRate = 0.1;

  if (prevSoilPct >= 0) {
    float delta = soilPct - prevSoilPct;
    if (delta > SOIL_WATER_JUMP_PCT) {
      lastWaterTime = now;
      wetStart = now;
      wiltStart = 0;
      overwaterBadStart = 0;
    }
  }
  prevSoilPct = soilPct;

  if (soilPct <= SOIL_WILT_PCT) {
    if (wiltStart == 0) wiltStart = now;
    soilProblem = true;
    soilProblemRate = WILT_DECAY_RATE;
    wetStart = 0;
  } else if (soilPct >= SOIL_SATURATED_PCT) {
    wiltStart = 0;
    if (wetStart == 0) wetStart = now;
    if (now - wetStart > OVERWATER_HOLD_MS) {
      if (overwaterBadStart == 0) overwaterBadStart = wetStart + OVERWATER_HOLD_MS;
      soilProblem = true;
      soilProblemRate = OVERWATER_DECAY_RATE;
    }
  } else {
    wiltStart = 0;
    wetStart = 0;
    overwaterBadStart = 0;
    if (lastWaterTime != 0 && (now - lastWaterTime) > EXPECTED_DRYDOWN_MS) {
      soilProblem = true;
      soilProblemRate = 0.03;
    }
  }

  if (soilProblem) {
    unsigned long problemStart = (wiltStart != 0) ? wiltStart :
                                  (overwaterBadStart != 0) ? overwaterBadStart : lastWaterTime;
    soilScore = decay(soilScore, problemStart, soilProblemRate);
    soilRecoverStart = 0;
  } else {
    if (soilRecoverStart == 0) soilRecoverStart = now;
    soilScore = recover(soilScore, soilRecoverStart, RECOVERY_RATE, 100.0);
  }

 
  if (temp > TEMP_HIGH || temp < TEMP_LOW) {
    if (tempBadStart == 0) tempBadStart = now;
    tempGoodStart = 0;
    tempScore = decay(tempScore, tempBadStart, TEMP_DECAY_RATE);
  } else {
    tempBadStart = 0;
    if (tempGoodStart == 0) tempGoodStart = now;
    tempScore = recover(tempScore, tempGoodStart, RECOVERY_RATE, 100.0);
  }

 
  if (hum < HUM_LOW || hum > HUM_HIGH) {
    if (humBadStart == 0) humBadStart = now;
    humGoodStart = 0;
    humScore = decay(humScore, humBadStart, HUM_DECAY_RATE);
  } else {
    humBadStart = 0;
    if (humGoodStart == 0) humGoodStart = now;
    humScore = recover(humScore, humGoodStart, RECOVERY_RATE, 100.0);
  }

 
  float total;
  if (hasLightData) {
    total = (lightScore + soilScore + tempScore + humScore) / 4.0;
  } else {
    total = (soilScore + tempScore + humScore) / 3.0;
  }
  total = constrain(total, 0.0, 100.0);
  String status = getPlantStatus(total);

 
  Serial.print("T:"); Serial.print(temp);
  Serial.print(" H:"); Serial.print(hum);
  Serial.print(" SOIL_RAW:"); Serial.print(soilRaw);
  Serial.print(" SOIL%:"); Serial.print(soilPct, 0);
  Serial.print(" SUN%:"); Serial.print(hasLightData ? String(litPercent, 0) : "--");
  Serial.print(" | scores L:"); Serial.print(hasLightData ? String(lightScore, 0) : "--");
  Serial.print(" S:"); Serial.print(soilScore, 0);
  Serial.print(" T:"); Serial.print(tempScore, 0);
  Serial.print(" H:"); Serial.print(humScore, 0);
  Serial.print(" TOTAL:"); Serial.print(total, 0);
  Serial.print(" "); Serial.println(status);

  // ---------------- OLED (clean, presentation-ready, 2 screens) ----------------
  // Screen alternates automatically every SCREEN_CYCLE_MS based on elapsed time -
  // no extra state variable needed, and it stays in sync even across loop delays.
  int screenIndex = (now / SCREEN_CYCLE_MS) % 2;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (screenIndex == 0) {
    // ---- Screen 1: live readings (unchanged from before) ----
    display.setCursor(0, 0);
    display.println("PLANT MONITOR");
    display.drawLine(0, 9, 128, 9, WHITE);

    display.setCursor(0, 14);
    display.print("TEMP : "); display.print(temp, 0); display.println("C");
    display.print("HUM  : "); display.print(hum, 0); display.println("%");
    display.print("SOIL : "); display.print(soilPct, 0); display.println("%");
    display.print("SUN  : "); if (hasLightData) { display.print(litPercent, 0); display.println("%"); } else display.println("--");

    display.setCursor(0, 46);
    display.drawLine(0, 44, 128, 44, WHITE);
    display.print("SCORE: "); display.println(total, 0);
    display.println(status);

  } else {
    // ---- Screen 2: recovery guidance ----
    String guidance = getGuidance(soilPct, temp, hum, litPercent, lightScore, hasLightData);

    display.setCursor(0, 0);
    display.println("PLANT CARE");
    display.drawLine(0, 9, 128, 9, WHITE);

    display.setCursor(0, 20);
    display.setTextSize(1);
    display.println(guidance);  
  }

  display.display();

  delay(2000);
}
