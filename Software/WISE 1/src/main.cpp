#/*

SMART DOOR LOCK
Arduino Mega 2560
RC522 RFID
OLED SSD1306 128x64
Servo SG90
Buzzer
Push Button
===========

*/

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ====================================================
OLED CONFIG
==================================================== */

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1);

/* ====================================================
PIN CONFIG
==================================================== */

#define RFID_SS 53
#define RFID_RST 49

#define SERVO_PIN 9

#define BUZZER_PIN 8

#define LED_LOCK 7
#define LED_OPEN 6

#define BTN_UNLOCK 3
#define BTN_LOCK 2

/* ====================================================
RFID
==================================================== */

MFRC522 rfid(RFID_SS, RFID_RST);

const byte MASTER_UID[4] =
    {
        0x01,
        0x02,
        0x03,
        0x04};

/* ====================================================
SERVO
==================================================== */

Servo doorServo;

bool lockState = true;

/* ====================================================
OLED ICON
==================================================== */

void OLED_ShowLocked()
{
    display.clearDisplay();

    display.drawCircle(
        64,
        14,
        8,
        SSD1306_WHITE);

    display.drawRoundRect(
        48,
        22,
        32,
        22,
        4,
        SSD1306_WHITE);

    display.fillCircle(
        64,
        32,
        2,
        SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(36, 52);
    display.print("TERKUNCI");

    display.display();
    
}

void OLED_ShowUnlocked()
{
    display.clearDisplay();

    display.drawCircle(
        60,
        14,
        8,
        SSD1306_WHITE);

    display.drawLine(
        68,
        6,
        68,
        20,
        SSD1306_BLACK);

    display.drawRoundRect(
        48,
        22,
        32,
        22,
        4,
        SSD1306_WHITE);

    display.fillCircle(
        64,
        32,
        2,
        SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(40, 52);
    display.print("TERBUKA");

    display.display();

}

void OLED_ShowScanning()
{
    display.clearDisplay();

    display.drawCircle(
        64,
        18,
        16,
        SSD1306_WHITE);

    display.drawCircle(
        64,
        18,
        10,
        SSD1306_WHITE);

    display.fillCircle(
        64,
        18,
        2,
        SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(22, 50);
    display.print("Membaca tag...");

    display.display();

}
/* ====================================================
BUZZER
==================================================== */

void beep(uint8_t times)
{
    for (uint8_t t = 0; t < times; t++)
    {
        tone(BUZZER_PIN, 2000, 100);
        delay(150);
    }
}

/* ====================================================
RFID UID CHECK
==================================================== */

bool authorizedCard(MFRC522::Uid *uid)
{
    if (uid->size != 4)
        return false;

for (uint8_t i = 0; i < 4; i++)
    {
        if (uid->uidByte[i] != MASTER_UID[i])
            return false;
    }

    return true;

}

/* ====================================================
SERIAL UID PRINT
==================================================== */

void printUID()
{
    Serial.print("UID: ");

for (byte i = 0; i < rfid.uid.size; i++)
    {
        if (rfid.uid.uidByte[i] < 0x10)
            Serial.print("0");

        Serial.print(
            rfid.uid.uidByte[i],
            HEX);

        Serial.print(" ");
    }

    Serial.println();
    
}

/* ====================================================
DOOR CONTROL
==================================================== */

void doorLock()
{
    lockState = true;

    digitalWrite(
        LED_LOCK,
        HIGH);

    digitalWrite(
        LED_OPEN,
        LOW);

    doorServo.write(0);

    OLED_ShowLocked();

    Serial.println(
        "PINTU TERKUNCI");

    beep(1);

}

void doorUnlock()
{
    lockState = false;

digitalWrite(
        LED_LOCK,
        LOW);

    digitalWrite(
        LED_OPEN,
        HIGH);

    doorServo.write(90);

    OLED_ShowUnlocked();

    Serial.println(
        "PINTU TERBUKA");

    beep(2);
    
}

/* ====================================================
RFID PROCESS
==================================================== */

void processRFID()
{
    if (!rfid.PICC_IsNewCardPresent())
        return;

    OLED_ShowScanning();

    delay(300);

    if (!rfid.PICC_ReadCardSerial())
        return;

    printUID();

    if (authorizedCard(&rfid.uid))
    {
        Serial.println(
            "AKSES DITERIMA");

        if (lockState)
        {
            doorUnlock();
        }
    }
    else
    {
        Serial.println(
            "AKSES DITOLAK");

        beep(3);

        delay(1000);

        if (lockState)
            OLED_ShowLocked();
        else
            OLED_ShowUnlocked();
    }

    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();

    delay(1000);
    
}

/* ====================================================
BUTTON PROCESS
==================================================== */

void processButtons()
{
    /* Tombol Buka */

    if (!digitalRead(BTN_UNLOCK))
    {
        delay(20);

        if (!digitalRead(BTN_UNLOCK))
        {
            if (lockState)
            {
                doorUnlock();
            }

            while (!digitalRead(BTN_UNLOCK))
                ;
        }
    }

    /* Tombol Kunci */

    if (!digitalRead(BTN_LOCK))
    {
        delay(20);

        if (!digitalRead(BTN_LOCK))
        {
            if (!lockState)
            {
                doorLock();
            }

            while (!digitalRead(BTN_LOCK))
                ;
        }
    }

}
/* ====================================================
SETUP
==================================================== */

void setup()
{
    Serial.begin(9600);

    Serial.println();
    Serial.println("====================");
    Serial.println(" SMART DOOR LOCK ");
    Serial.println("====================");

    /* LED */

    pinMode(
        LED_LOCK,
        OUTPUT);

    pinMode(
        LED_OPEN,
        OUTPUT);

    /* BUZZER */

    pinMode(
        BUZZER_PIN,
        OUTPUT);

    /* BUTTON */

    pinMode(
        BTN_UNLOCK,
        INPUT_PULLUP);

    pinMode(
        BTN_LOCK,
        INPUT_PULLUP);

    /* SERVO */

    doorServo.attach(
        SERVO_PIN);

    /* OLED */

    if (!display.begin(
            SSD1306_SWITCHCAPVCC,
            0x3C))
    {
        Serial.println(
            "OLED FAIL");

        while (true)
            ;
    }

    display.clearDisplay();
    display.display();

    /* RFID */

    SPI.begin();

    rfid.PCD_Init();

    Serial.println(
        "RC522 READY");

    /* STATUS AWAL */

    doorLock();

    Serial.println(
        "SYSTEM READY");

}

/* ====================================================
LOOP
==================================================== */

void loop()
{
    processButtons();

    processRFID();
    
}