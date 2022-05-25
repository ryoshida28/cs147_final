#include <Arduino.h>
#include <TFT_eSPI.h>
#include <IridiumSBD.h>
// #include <SoftwareSerial.h>

// === Defines ====================================

#define DIAGNOSTICS false
#define RXD2    32
#define TXD2    26 // 
#define SLEEP   25 // grey

#define IridiumSerial Serial2

// === End Defines ================================



// === Globals ====================================
TFT_eSPI tft;
// SoftwareSerial IridiumSerial;
IridiumSBD modem = IridiumSBD(IridiumSerial, SLEEP);

int count;
// === End Globals ================================

void setup() {

    // = Initialize Serial
    Serial.begin(115200);
    while (!Serial);

    IridiumSerial.begin(19200, SERIAL_8N1, RXD2, TXD2);

    // = Initialize OLED
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(8);
    tft.setTextDatum(CC_DATUM);

    delay(2000);

    // = Initialize IridiumSBD
    int signalQuality = -1;
    int err;
    delay(2000);

    Serial.println("Starting modem...");
    err = modem.begin();
    if (err != ISBD_SUCCESS) {
        Serial.print("Begin faild: error ");
        Serial.println("err");
        if (err == ISBD_NO_MODEM_DETECTED) Serial.println("No modem detected: check wiring.");
        return;
    }
    char version[12];
    err = modem.getFirmwareVersion(version, sizeof(version));
    if (err != ISBD_SUCCESS) {
        Serial.print("FirmwareVersion failed: error ");
        Serial.println(err);
        return;
    }
    err = modem.getSignalQuality(signalQuality);
    if (err != ISBD_SUCCESS) {
        Serial.print("SignalQuality failed: error ");
        Serial.print(err);
        return;
    }
    Serial.print("On a scale of 0 to 5, signal quality is currently ");
    Serial.print(signalQuality);
//
//    // Example: Test the signal quality.
//    // This returns a number between 0 and 5.
//    // 2 or better is preferred.
//    err = modem.getSignalQuality(signalQuality);
//    if (err != ISBD_SUCCESS)
//    {
//    Serial.print("SignalQuality failed: error ");
//    Serial.println(err);
//    return;
//    }
//
//    Serial.print("On a scale of 0 to 5, signal quality is currently ");
//    Serial.print(signalQuality);
//    Serial.println(".");
//
//    // Send the message
//    Serial.print("Trying to send the message.  This might take several minutes.\r\n");
//    err = modem.sendSBDText("Hello, world!");
//    if (err != ISBD_SUCCESS)
//    {
//    Serial.print("sendSBDText failed: error ");
//    Serial.println(err);
//    if (err == ISBD_SENDRECEIVE_TIMEOUT)
//      Serial.println("Try again with a better view of the sky.");
//    }
//
//    else
//    {
//    Serial.println("Hey, it worked!");
//    }   

    // = Initialize Globals
    count = 0;
}

void loop() {
    tft.fillScreen(TFT_BLACK);
    tft.drawNumber(count, 120, 67);

    ++count;

    delay(1000);
}


#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c) { Serial.write(c); }
void ISBDDiagsCallback(IridiumSBD *device, char c) { Serial.write(c); }
#else
void ISBDConsoleCallback(IridiumSBD *device, char c) { return; }
void ISBDDiagsCallback(IridiumSBD *device, char c) { return; }
#endif
