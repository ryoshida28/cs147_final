#include <Arduino.h>
#include <TFT_eSPI.h>
#include <IridiumSBD.h>
#include <string.h>
#include <ESP32_New_TimerInterrupt.h>

// === Defines ====================================

#define SENDMODE false
#define DIAGNOSTICS false
#define RXD2    32 // yellow
#define TXD2    26 // orange
#define SLEEP   25 // grey
#define INTPIN  36 // reed switch
#define SPDCNST 4997.988312529217
#define BUFSIZE 200
#define PAYLOADSIZE 50
#define TIMERINT 6000000L

#define IridiumSerial Serial2

// === End Defines ================================



// === Globals ====================================
TFT_eSPI tft;
IridiumSBD modem = IridiumSBD(IridiumSerial, SLEEP);
TaskHandle_t task_snd; // task_snd = message sending task, task_cmp = speed calculation task
ESP32Timer timer(0);

int count;
bool is_init;
long last_interrupt;
volatile float currspd;

char buf[BUFSIZE]; // circular queue buffer
int qhead,qtail,qlen;
bool qclear;
SemaphoreHandle_t xmutex;
// === End Globals ================================

// === Interrupts =================================
void detect() {
    long nxt = millis();
    long dif = nxt - last_interrupt;
    if (dif > 100) {
        Serial.print("cycle: "); Serial.println(dif);
        currspd = (SPDCNST) / (dif);
        last_interrupt = nxt;
    }
}

bool timer_interrupt(void *param) {
    xSemaphoreTake(xmutex, portMAX_DELAY);
    if (qlen < BUFSIZE) {
        buf[qhead] = (char) ((int) (currspd+0.5));
        qhead = (qhead+1)%BUFSIZE;
        ++qlen;
    }
    xSemaphoreGive(xmutex);
    return true;
}
// === End Interrupts =============================

// === Tasks ======================================

void send_data_task(void *param) {
    // Read buf into locbuf
    char locbuf[BUFSIZE];
    int chead, ctail, clen; // will copy queue pointers to these variables
    xSemaphoreTake(xmutex, portMAX_DELAY);
    memcpy(locbuf, buf, BUFSIZE);
    chead = qhead; ctail = qtail; clen = qlen;
    qtail = qhead;
    qlen = 0;
    qclear = true;
    xSemaphoreGive(xmutex);

#if SENDMODE
    
    uint8_t payload[PAYLOADSIZE];
    for (int i = 0; i < clen && i < PAYLOADSIZE; ++i) {
        payload[i] = (uint8_t) locbuf[(chead-50+i+BUFSIZE)%BUFSIZE];
    }

    int err;
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

    err = modem.sendSBDBinary(payload, PAYLOADSIZE * sizeof(uint8_t));
    if (err != ISBD_SUCCESS) {
      Serial.print("sendSBDText failed: error ");
      Serial.println(err);
      if (err == ISBD_SENDRECEIVE_TIMEOUT) {
        Serial.println("Try again with a better view of the sky.");
      }
    } else {
      Serial.println("Hey, it worked");
    }
    modem.sleep();
#else
    Serial.println("Sending");
    for (int i = 0; i < clen; ++i) {
        Serial.print((int) locbuf[(ctail+i)%BUFSIZE]);
        Serial.print(" ");
    } Serial.println();
    delay(5000);
    Serial.println("Sent");
#endif

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
    is_init = true;
    qhead = qtail = qlen = 0;
    qclear = true;
    xmutex = xSemaphoreCreateMutex();
    count = 0;
}

void loop() {
    if (is_init) { // ensure pin runs on correct core
        attachInterrupt(digitalPinToInterrupt(INTPIN), detect, FALLING);
        timer.attachInterruptInterval(TIMERINT, timer_interrupt);
        is_init = false;
    }

    tft.fillScreen(TFT_BLACK);
    tft.drawNumber((int) currspd, 120, 67);


    int clen;
    xSemaphoreTake(xmutex, portMAX_DELAY);
    clen = qlen;

    if (qclear && clen >= PAYLOADSIZE) {
        qclear = false;
        xSemaphoreGive(xmutex);
        xTaskCreate(send_data_task, "TaskSend", 10000, nullptr, 2, nullptr);
    } else {
        xSemaphoreGive(xmutex);
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
