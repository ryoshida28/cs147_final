#include <Arduino.h>
#include <TFT_eSPI.h>
#include <IridiumSBD.h>

// === Defines ====================================

#define DIAGNOSTICS false
#define RXD2    32 // yellow
#define TXD2    26 // orange
#define SLEEP   25 // grey
#define INTPIN  36

#define IridiumSerial Serial2

// === End Defines ================================



// === Globals ====================================
TFT_eSPI tft;
IridiumSBD modem = IridiumSBD(IridiumSerial, SLEEP);
TaskHandle_t task_snd; // task_snd = message sending task, task_cmp = speed calculation task

int count;
bool is_init;
long debounce;
// === End Globals ================================

// === Interrupts =================================
void detect() {
    long nxt = millis();
    if (nxt - debounce > 40) {
        Serial.println("Interrupt running on " + String(xPortGetCoreID()));
        debounce = nxt;
    }
}
// === End Interrupts =============================

// === Tasks ======================================

void send_data_task(void *param) {
    int err;
    Serial.println("Task running on " + String(xPortGetCoreID()));
    Serial.println("Starting modem...");
    err = modem.begin();
    if (err != ISBD_SUCCESS) {
        Serial.print("Begin failed: error");
        Serial.println(err);
        if (err == ISBD_NO_MODEM_DETECTED) Serial.println("No modem detected: check wiring");
        vTaskDelete(nullptr);
        return;
    }
    Serial.println("modem started");

    Serial.println("Sending");
    delay(5000);
    Serial.println("Sent");
    modem.sleep();
    vTaskDelete(nullptr);
}

// === End Tasks ==================================

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

    // Initialize Pins
    pinMode(INTPIN, INPUT);


    // = Initialize IridiumSBD
    int signalQuality = -1;
    int err;

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
    Serial.println(signalQuality);

    modem.sleep();
    count = 0;
    is_init = true;
    debounce = 0;
}

void loop() {
    if (is_init) { // ensure pin runs on correct core
        attachInterrupt(digitalPinToInterrupt(INTPIN), detect, FALLING);
        is_init = false;
    }

    Serial.println("Loop running on " + String(xPortGetCoreID()));
    tft.fillScreen(TFT_BLACK);
    tft.drawNumber(count, 120, 67);

    if (++count >= 30) {
        count = 0;
        xTaskCreate(send_data_task, "TaskSend", 10000, nullptr, 2, nullptr);
    }

    delay(1000);
}


#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c) { Serial.write(c); }
void ISBDDiagsCallback(IridiumSBD *device, char c) { Serial.write(c); }
#else
void ISBDConsoleCallback(IridiumSBD *device, char c) { return; }
void ISBDDiagsCallback(IridiumSBD *device, char c) { return; }
#endif
