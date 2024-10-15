M5AtomS3 GPS Logger with Temperature Logging

This project is a GPS logger built on the M5AtomS3 (ESP32-S3-based microcontroller) that logs GPS data (location, altitude, speed) and temperature readings from a DS18B20 temperature sensor. The data is logged in GPX format on an SD card, displayed on the AtomS3's LCD, and served through a web interface. The device also supports deep sleep mode to conserve power when not in use.
Features

    GPS Data Logging: Logs latitude, longitude, altitude, and speed from a connected GPS module.
    Temperature Logging: Reads temperature from a DS18B20 sensor and logs it alongside GPS data.
    Data Storage: Saves GPS and temperature data in GPX format on an SD card.
    Display: Shows GPS data and temperature on the M5AtomS3 LCD display.
    Web Server: Provides a simple web interface to view files stored on the SD card.
    Deep Sleep Mode: Supports a button-based deep sleep mode to conserve battery power.

Hardware Requirements

    M5AtomS3 - ESP32-S3 based microcontroller with a built-in display.
    GPS Module - Configured for communication via pins RX (22) and TX (23).
    DS18B20 Temperature Sensor - Connected via GPIO 26.
    SD Card Module - For logging GPS and temperature data.
    Button (optional) - For toggling GPS activation and entering deep sleep mode.

Pin Configuration

    GPS Module:
        RX (GPS to ESP32): Pin 22
        TX (ESP32 to GPS): Pin 23
    DS18B20 Temperature Sensor: Pin 26
    Button for Deep Sleep: Pin 41

Software Requirements

    Arduino IDE (1.8.13 or later)
    ESP32 Board Support: Install from the Arduino IDE Boards Manager.
    Required Libraries:
        M5AtomS3
        TinyGPSPlus
        DallasTemperature
        OneWire
        SD
        SPIFFS
        WiFi
        WebServer

Setup Instructions
Step 1: Install Required Libraries

In the Arduino IDE, install the following libraries via Sketch > Include Library > Manage Libraries:

    M5AtomS3
    TinyGPSPlus
    DallasTemperature
    OneWire
    SD
    SPIFFS
    WiFi
    WebServer

Step 2: Wiring

Connect the hardware components as follows:
Component	Pin Connection
GPS Module (RX)	GPIO 22
GPS Module (TX)	GPIO 23
DS18B20 Sensor	GPIO 26
Button	GPIO 41
SD Card Module	SPI Interface
Step 3: Flash the Code

    Clone this repository or download the code.
    Open the code in the Arduino IDE.
    Select the correct board (M5AtomS3) from Tools > Board.
    Connect the M5AtomS3 to your computer via USB.
    Compile and upload the code to the device.

Step 4: Run the Device

    Once powered on, the device will display GPS data (latitude, longitude, altitude, speed) on the M5AtomS3's screen.
    The device will also log this data, along with temperature readings from the DS18B20 sensor, into a GPX file on the SD card.
    Press the button to toggle GPS activation and enter deep sleep mode to conserve power.
    The web interface can be accessed by entering the IP address of the M5AtomS3 (after it connects to Wi-Fi) into your browser.

Usage
Display Output

The M5AtomS3’s LCD displays the following information:

    Latitude, Longitude, Altitude
    Speed (in km/h)
    Number of GPS satellites
    Temperature (in °C) if the DS18B20 sensor is connected

If no GPS fix is available, the device will display "No Fix" in red, and if the temperature sensor is not connected, it will display "Temp: N/A".
SD Card Logging

GPS and temperature data are logged in GPX format on the SD card. If the SD card is inserted, the GPX file will be stored as /gpslog.gpx. Each entry includes latitude, longitude, altitude, time, and temperature data.
Web Interface

The device hosts a simple web server that lists the files on the SD card. To access the interface:

    Connect to the same network as the M5AtomS3.
    Enter the IP address of the device in your browser.
    You’ll see a list of logged files from the SD card.

Deep Sleep Mode

    Pressing the button (GPIO 41) toggles GPS activation. When the GPS is disabled, the device enters deep sleep mode to conserve power.
    The device wakes up from deep sleep when the button is pressed again.

Future Improvements

    Add support for storing logs in multiple files (one per day).
    Add time synchronization using an external real-time clock (RTC) or NTP server.
    Implement more advanced web-based features for controlling logging options.

License

This project is licensed under the MIT License - see the LICENSE file for details.
