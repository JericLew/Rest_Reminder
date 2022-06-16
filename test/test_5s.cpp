#include <Arduino.h>
#include <Adafruit_MLX90614.h>
#include <LiquidCrystal.h>


//initialise mlx object, pins for LCD and for buzzer
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int LED_PIN = 8;
const int BUZZ_PIN = 13;


void setup() {  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //Serial setup
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Adafruit MLX90614 test");

  //mlx setup
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };
  
  Serial.print("Emissivity = "); Serial.println(mlx.readEmissivity());
  Serial.println("================================================");

  //alert setup
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

//constants
String BLANK_LINE = "                  ";
const unsigned long WAIT_TIME = 5000;
const unsigned long ALERT_INTERVAL = 1000;
const unsigned long ALERT_TRIGGER = 5000; 

//variables
unsigned long starttime = 0; //initial starttime to prevent waiting at epoch
unsigned long cuttime = 0;
unsigned long prev_alert_millis = 0;
int buzz_state = LOW;
int led_state = LOW;

void serialTemp() {
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC());
  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
  Serial.println();
}

void serialDebug() {
   Serial.println((String)"Time: " + millis());
   Serial.println((String)"Start Time: " + starttime);
   Serial.println((String)"Cut Time: " + cuttime);
}

void lcdTemp() {
  lcd.setCursor(0, 0);
  lcd.print("A:"); lcd.print(mlx.readAmbientTempC()); lcd.print("/O:"); lcd.print(mlx.readObjectTempC()); 
}

bool thermalCheck() {
  if (mlx.readObjectTempC() - mlx.readAmbientTempC() >0.75) {
    return true;
  }
  return false;
}

bool trackingCheck(bool thermal_status) {
  if (thermal_status == true) {
    if (starttime == 0) {
      starttime = millis();
    }
    cuttime = 0;
    return true;
  }
  else {
    if (cuttime == 0) {
      cuttime = millis();
    }
    if (starttime != 0 & millis() - cuttime <= WAIT_TIME) {
      return true;
    }
    starttime = 0;
    return false;
  }
}

bool alertCheck(bool tracking_status, bool thermal_status) {
  if (tracking_status & thermal_status & millis() - starttime >= ALERT_TRIGGER) {
    return true;
  }
  return false;
}

void alert(bool alert_status) {
  if (alert_status == true) {
    unsigned long curr_alert_millis = millis();
    if (curr_alert_millis - prev_alert_millis >= ALERT_INTERVAL) {
      prev_alert_millis = curr_alert_millis;
      if (buzz_state == LOW) {
        buzz_state = HIGH;
        led_state = HIGH;
      }
      else {
        buzz_state = LOW;
        led_state = LOW;
      }
    }
  }
  else {
    buzz_state = LOW;
    led_state = LOW;
   }
  digitalWrite(BUZZ_PIN, buzz_state);
  digitalWrite(LED_PIN, led_state);
}


void lcdPrintStatus(bool thermal_status, bool tracking_status, bool alert_status){
  lcd.setCursor(0,1); lcd.print(BLANK_LINE);
  if (thermal_status & tracking_status & not alert_status) {
    lcd.setCursor(0,1); lcd.print("Detected t:"); lcd.print( (millis()-starttime)/1000);
  }
  if (thermal_status & tracking_status & alert_status) {
    lcd.setCursor(0,1); lcd.print("Alert!   t:"); lcd.print( (millis()-starttime)/1000);
  }

  if (not thermal_status & tracking_status) {
    lcd.setCursor(0, 1); lcd.print("Waiting  t:"); lcd.print( (millis()-starttime)/1000);
  }
  if (not thermal_status & not tracking_status & not alert_status) {
    lcd.setCursor(0, 1); lcd.print("Not Detected");
  }
}

void loop() {
  serialTemp();
  lcdTemp();
  serialDebug();
  bool thermal_status = thermalCheck();
  bool tracking_status = trackingCheck(thermal_status);
  bool alert_status = alertCheck(tracking_status,thermal_status);
  lcdPrintStatus(thermal_status,tracking_status,alert_status);
  alert(alert_status);
  delay(500);
}