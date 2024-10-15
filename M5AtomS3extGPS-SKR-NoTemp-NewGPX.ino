#include <Arduino.h>
#include "M5Atom.h"        // M5Atom library
#include <SPI.h>           // SPI communication
#include "FS.h"            // Filesystem (used for SD card)
#include <SD.h>            // SD card handling
#include <TinyGPSPlus.h>   // GPS library
#include <OneWire.h>       // OneWire protocol (for temperature sensor)
#include <DallasTemperature.h> // Dallas temperature sensor library

#define TIME_ZONE 0             // Time compensation for UTC
#define RECORD_FREQUENCY 5000   // Record frequency in milliseconds
#define ACTIVATE_DEEP_SLEEP_ONCLICK true
#define GPS_TIMEOUT 5000        // GPS acquisition timeout in milliseconds
#define DISPLAY_GPS_DATA true   // Flag to display GPS message

#define SYDNEY_TIMEZONE_OFFSET 11  // Sydney time during daylight saving (adjust for standard time if needed)
#define MIDNIGHT_HOUR 0
#define MIDNIGHT_MINUTE 0
#define MIDNIGHT_SECOND 0
#define DEBOUNCE_DELAY 50  // Debounce delay for button clicks

unsigned long lastButtonPressTime = 0;  // To track button press timing
int buttonPressCount = 0;               // To count button presses
bool fileClosedForMidnight = false;     // To track if the file was closed at midnight

/* Function Prototypes */
static void readGPS(unsigned long ms);
static void printGPSData();
static void createDataFile();
static void addGPXPoint();
static void closeGPXFile();
static void checkMidnightCloseout();
static void handleDoubleClick();
static void ledCloseoutSequence();
static void blink_led_orange();
static void blink_led_red();
static void blink_led_blue();
static void blink_led_green();
static void updateStatFile();
static void printWakeupReason();

// OneWire & DallasTemperature sensor setup
const int oneWireBus = 26;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// TinyGPS++ object for GPS data
TinyGPSPlus gps;
HardwareSerial gps_uart(1);

bool firstStart = true;          // Flag for first startup
char filepath_gpx[25];           // GPX file path
char filepath_stats[25];         // Stats file path
char today_folder[15];           // Folder for today's data
char point_date[30];
float prev_lat = 0.0;            // Previous latitude
float prev_long;  
bool ENABLE_GPS = true;
bool DESACTIVATE_GPS = false;
#define FORCE_UPDATE_STATS true
#define SPEED_BUFFER_SIZE 10

typedef struct TODAYSTATS {
  float dist;
  float speed_max;
  float speed_mean; 
  double speedbuffer[SPEED_BUFFER_SIZE];
  bool speedbufferfull = false;
  int speedbufferpos = 0;
} TODAYSTATS_t;

TODAYSTATS_t today_stats;

void external_button_pressed() {
  if (!ENABLE_GPS) {
    // GPS is disabled, do nothing
  } else {
    blink_led_green();
  }
}

void setup() {
  sensors.begin(); // Initialize DS18B20 sensor
  M5.begin(true, false, true); // Initialize M5Atom (Serial, I2C, Display enabled)
  
  // Initialize SPI and SD card
  SPI.begin(23, 33, 19, -1);
  if (!SD.begin(-1, SPI, 40000000)) {
    Serial.println("SD card initialization failed!");
  } else {
    sdcard_type_t Type = SD.cardType();
    Serial.printf("SDCard Type = %d \r\n", Type);
    Serial.printf("SDCard Size = %d MB\r\n", (int)(SD.cardSize() / 1024 / 1024));
  }

  // Open GPS UART port
  gps_uart.begin(9600, SERIAL_8N1, 22, -1);
  delay(250);

  // Clear M5Atom display
  M5.dis.drawpix(0, 0, 0);

  // Attach interrupt for button (GPIO39) for handling double-click events
  pinMode(39, INPUT);
  attachInterrupt(39, [] {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPressTime < DEBOUNCE_DELAY) return;  // Debounce
    lastButtonPressTime = currentTime;

    buttonPressCount++;  // Increment button press count
    if (buttonPressCount == 2) {  // Detect double-click
      handleDoubleClick();  // Handle GPX file closeout on double-click
      buttonPressCount = 0;  // Reset count after double-click
    }
  }, RISING);

  printWakeupReason();
}

void loop() {
  if (DESACTIVATE_GPS) {
    DESACTIVATE_GPS = false;
    blink_led_orange();
    if (ACTIVATE_DEEP_SLEEP_ONCLICK) {
      closeGPXFile();  // Close file when entering deep sleep
      Serial.println("Wakeup on button activated, entering deep sleep");
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
      esp_deep_sleep_start();
    }
  }

  if (ENABLE_GPS) {
    readGPS(500);

    if (gps.location.isValid() && gps.date.isValid()) {
      if (firstStart) {
        createDataFile();
        delay(250);
        firstStart = false;
      }
      addGPXPoint();
      checkMidnightCloseout();  // Check if it's midnight to close the GPX file
    }

    if (millis() > GPS_TIMEOUT && gps.charsProcessed() < 10) {
      Serial.println("No GPS data received: Check wiring or position");
    }
  }

  delay(RECORD_FREQUENCY);
}

static void createDataFile() {
  sprintf(today_folder, "/%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
  if (!SD.exists(today_folder)) {
    SD.mkdir(today_folder);
    Serial.printf("Created today folder: %s\n", today_folder);
  } else {
    Serial.printf("Today folder already exists: %s\n", today_folder);
  }

  sprintf(filepath_gpx, "%s/track.gpx", today_folder);

  if (!SD.exists(filepath_gpx)) {
    Serial.printf("Creating new GPX file: %s\n", filepath_gpx);
    File gpxFile = SD.open(filepath_gpx, FILE_WRITE);
    if (gpxFile) {
      gpxFile.print(F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"));
      gpxFile.print(F("<gpx version=\"1.1\" creator=\"YourAppName\" xmlns=\"http://www.topografix.com/GPX/1/1\" "));
      gpxFile.print(F("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "));
      gpxFile.print(F("xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "));
      gpxFile.print(F("http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n"));
      gpxFile.print(F("<trk>\r\n<trkseg>\r\n")); // Open track and track segment
      gpxFile.close();
    } else {
      Serial.printf("Unable to open GPX file: %s\n", filepath_gpx);
      blink_led_red();
    }
  }
}

static void addGPXPoint() {
  if (gps.speed.kmph() > 5) {
    double current_speed = gps.speed.kmph();
    today_stats.speedbuffer[today_stats.speedbufferpos] = current_speed;
    today_stats.speedbufferpos++;

    if (current_speed > today_stats.speed_max) today_stats.speed_max = current_speed;

    if (today_stats.speedbufferpos >= SPEED_BUFFER_SIZE) {
      today_stats.speedbufferpos = 0;
      today_stats.speedbufferfull = true;
    }

    if (today_stats.speedbufferfull) {
      float speed_mean = 0;
      for (int i = 0; i < SPEED_BUFFER_SIZE; i++) {
        speed_mean += today_stats.speedbuffer[i];
      }
      today_stats.speed_mean = speed_mean / SPEED_BUFFER_SIZE;
    }

    if (prev_lat != 0.0) {
      double _distance = gps.distanceBetween(gps.location.lat(), gps.location.lng(), prev_lat, prev_long);
      if (_distance > 0) {
        today_stats.dist += _distance;
        updateStatFile();
      }
      prev_lat = gps.location.lat();
      prev_long = gps.location.lng();
    } else {
      prev_lat = gps.location.lat();
      prev_long = gps.location.lng();
    }
  }

  if (FORCE_UPDATE_STATS) updateStatFile();

  sprintf(point_date, "%04d-%02d-%02dT%02d:%02d:%02dZ", gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour() + TIME_ZONE, gps.time.minute(), gps.time.second());

  File gpxFile = SD.open(filepath_gpx, FILE_APPEND);  // Use FILE_APPEND to add new points
  if (!gpxFile) {
    Serial.printf("Unable to open GPX file: %s\n", filepath_gpx);
    blink_led_red();
  } else {
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);

    gpxFile.printf("<trkpt lat=\"%f\" lon=\"%f\">\r\n<time>%s</time>\r\n<ele>%f</ele>\r\n<sat>%d</sat>\r\n", 
                   gps.location.lat(), gps.location.lng(), point_date, gps.altitude.meters(), gps.satellites.value());
    gpxFile.printf("<extensions>\r\n<gpxtpx:TrackPointExtension>\r\n<gpxtpx:atemp>%f</gpxtpx:atemp>\r\n</gpxtpx:TrackPointExtension>\r\n</extensions>\r\n</trkpt>\r\n", 
                   temperatureC);
    gpxFile.close();

    blink_led_blue();
  }
}

static void checkMidnightCloseout() {
  // Check if GPS date and time is valid
  if (gps.time.isValid() && gps.date.isValid()) {
    // Calculate Sydney time based on GPS UTC time and Sydney time zone offset
    int localHour = gps.time.hour() + SYDNEY_TIMEZONE_OFFSET;
    if (localHour >= 24) localHour -= 24;  // Adjust for overflow if time exceeds 24 hours

    if (localHour == MIDNIGHT_HOUR && gps.time.minute() == MIDNIGHT_MINUTE && gps.time.second() == MIDNIGHT_SECOND) {
      if (!fileClosedForMidnight) {
        closeGPXFile();  // Close GPX file at midnight
        fileClosedForMidnight = true;  // Ensure file is only closed once at midnight
        ledCloseoutSequence();  // Indicate file closeout with LED sequence
      }
    } else {
      fileClosedForMidnight = false;  // Reset for the next midnight event
    }
  }
}

static void handleDoubleClick() {
  Serial.println("Double-click detected, closing GPX file");
  closeGPXFile();  // Close the GPX file on double-click
  ledCloseoutSequence();  // Indicate file closeout with LED sequence
}

static void ledCloseoutSequence() {
  // Red -> Amber -> Green over 3 seconds
  M5.dis.drawpix(0, 0, 0xff3300);  // Red
  delay(1000);
  M5.dis.drawpix(0, 0, 0xe68a00);  // Amber
  delay(1000);
  M5.dis.drawpix(0, 0, 0x00ff00);  // Green
  delay(1000);
  M5.dis.drawpix(0, 0, 0);  // Turn off
}

static void closeGPXFile() {
  File gpxFile = SD.open(filepath_gpx, FILE_APPEND);
  if (gpxFile) {
    gpxFile.print(F("</trkseg>\r\n</trk>\r\n</gpx>\r\n"));  // Close track segment and GPX
    gpxFile.close();
    Serial.println("GPX file successfully closed");
  } else {
    Serial.printf("Unable to open GPX file: %s to close it\n", filepath_gpx);
  }
}

static void updateStatFile() {
  File statsFile = SD.open(filepath_stats, FILE_WRITE);
  if (!statsFile) {
    Serial.printf("Unable to open stats file: %s\n", filepath_stats);
  } else {
    Serial.println("Writing stats data to file");
    statsFile.printf("key,value,unit\r\n");
    statsFile.printf("distance,%f,m\r\n", today_stats.dist);
    statsFile.printf("distance,%f,km\r\n", today_stats.dist / 1000.0);
    statsFile.printf("speed_max,%f,km/h\r\n", today_stats.speed_max);
    statsFile.printf("speed_mean,%f,km/h\r\n", today_stats.speed_mean);
    statsFile.close();
  }
}

static void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_GPIO: Serial.println("Wakeup caused by GPIO"); break;
    case ESP_SLEEP_WAKEUP_UART: Serial.println("Wakeup caused by UART"); break;
    default: Serial.printf("Wakeup not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

static void blink_led_orange() {
  M5.dis.drawpix(0, 0, 0xe68a00);
  delay(200);
  M5.dis.drawpix(0, 0, 0);
  delay(200);
}

static void blink_led_red() {
  M5.dis.drawpix(0, 0, 0xff3300);
  delay(200);
  M5.dis.drawpix(0, 0, 0);
  delay(200);
}

static void blink_led_blue() {
  M5.dis.drawpix(0, 0, 0x0000cc);
  delay(200);
  M5.dis.drawpix(0, 0, 0);
  delay(200);
}

static void blink_led_green() {
  M5.dis.drawpix(0, 0, 0x00ff00);
  delay(200);
  M5.dis.drawpix(0, 0, 0);
  delay(200);
}
