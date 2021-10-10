/*

relay_timer_wemos

Timer example:
https://www.switchdoc.com/2015/10/iot-esp8266-timer-tutorial-arduino-ide/

© octobre 2021, ouilogique.com

*/

#include <Arduino.h>
extern "C"
{
#include "user_interface.h"
}
#include <FS.h> // À enlever
#include "LittleFS.h"

const char *FILENAME = "/wait_time_ms.txt";

uint32_t WAIT_TIME_MS = 1000;
os_timer_t RELAY_TIMER;
volatile bool TIMER_HAS_RUN_OUT = false;
volatile bool SERIAL_DATA_AVALAIBLE = false;

static const uint8_t RELAI_PIN = D1;    // GPIO 5;
static const uint8_t BUTTON_PIN = D2;   // GPIO 4;
static const uint8_t BLUE_LED_PIN = D4; // GPIO 2;

static const uint8_t LED_HIGH = 0;
static const uint8_t LED_LOW = 1;

static const uint8_t RELAY_HIGH = 1;
static const uint8_t RELAY_LOW = 0;

/**
 *
 */
void timer_callback(void *pArg)
{
    TIMER_HAS_RUN_OUT = true;
}

/**
 *
 */
void configure_timer(void)
{
    os_timer_setfn(&RELAY_TIMER, timer_callback, NULL);
}

/**
 *
 */
void configure_serial()
{
    Serial.begin(BAUD_RATE);
    Serial.println(F("\n\n\n# RELAY TIMER\n"));
    Serial.print(F("FILE NAME:        "));
    Serial.println(__FILE__);
    Serial.print(F("PATH:             "));
    Serial.println(PATH);
    Serial.print(F("COMPILATION DATE: "));
    Serial.println(COMPILATION_DATE);
    Serial.print(F("COMPILATION TIME: "));
    Serial.println(COMPILATION_TIME);
    Serial.println("");
}

/**
 *
 */
void configure_gpio()
{
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(RELAI_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(BLUE_LED_PIN, LED_LOW);
    digitalWrite(RELAI_PIN, RELAY_LOW);
}

/**
 *
 */
void switch_on_relay()
{
    digitalWrite(BLUE_LED_PIN, LED_HIGH);
    digitalWrite(RELAI_PIN, RELAY_HIGH);
    Serial.print("WAIT TIME (ms) ");
    Serial.println(WAIT_TIME_MS);
    Serial.print("RELAY ON       ");
    Serial.println(millis());
}

/**
 *
 */
void switch_off_relay()
{
    digitalWrite(BLUE_LED_PIN, LED_LOW);
    digitalWrite(RELAI_PIN, RELAY_LOW);
    Serial.print("RELAY OFF      ");
    Serial.println(millis());
}

/**
 *
 */
void serialEvent()
{
    SERIAL_DATA_AVALAIBLE = true;
}

/**
 * Source: http://tripsintech.com/arduino-isnumeric-function/
 */
boolean isNumeric(String str)
{
    unsigned int stringLength = str.length();
    if (stringLength == 0)
    {
        return false;
    }
    boolean seenDecimal = false;
    for (unsigned int i = 0; i < stringLength; ++i)
    {
        if (isDigit(str.charAt(i)))
        {
            continue;
        }
        if (str.charAt(i) == '.')
        {
            if (seenDecimal)
            {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}

/**
 *
 */
void write_WAIT_TIME_MS_to_file()
{
    File _f = LittleFS.open(FILENAME, "w");
    if (!_f)
    {
        Serial.println("Error opening or creating file.");
        return;
    }
    _f.print(WAIT_TIME_MS);
    _f.close();
}

/**
 *
 */
void read_WAIT_TIME_MS_from_file()
{
    File _f = LittleFS.open(FILENAME, "r");
    if (!_f)
    {
        Serial.println("Error opening file.");
        return;
    }
    char _buffer[100];
    uint8_t _i = 0;
    while (_f.available())
    {
        _buffer[_i] = char(_f.read());
        _i++;
    }
    WAIT_TIME_MS = atoi(_buffer);
    Serial.print("WAIT TIME (ms) ");
    Serial.println(WAIT_TIME_MS);
    _f.close();
}

/**
 * The value of WAIT_TIME_MS will be stored
 * in a file on the LittleFS file system.
 */
void configure_littlefs()
{
    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS.");
        return;
    }

    // If the file must be deleted, the command is:
    // LittleFS.remove(FILENAME);

    // Create the file if it doesn’t exists.
    if (!LittleFS.exists(FILENAME))
    {
        Serial.println("Create file.");
        write_WAIT_TIME_MS_to_file();
    }

    // Read the file.
    read_WAIT_TIME_MS_from_file();
}

/**
 *
 */
void setup()
{
    delay(1000);
    configure_serial();
    configure_gpio();
    configure_timer();
    configure_littlefs();
}

/**
 *
 */
void loop()
{
    yield();

    static uint8_t _state = 0;

    // Read the button state.
    if (_state == 0)
    {
        if (!digitalRead(BUTTON_PIN))
            _state = 1;
        else if (SERIAL_DATA_AVALAIBLE)
        {
            SERIAL_DATA_AVALAIBLE = false;
            _state = 4;
        }
    }

    // If the button was pressed, open the relay.
    else if (_state == 1)
    {
        os_timer_arm(&RELAY_TIMER, WAIT_TIME_MS, true);
        switch_on_relay();
        _state = 2;
    }

    // Wait for the timer to run out.
    else if (_state == 2)
    {
        if (TIMER_HAS_RUN_OUT)
            _state = 3;
    }

    // If the timer has run out, switch off the relay
    // And wait until the button is released.
    else if (_state == 3)
    {
        switch_off_relay();
        os_timer_disarm(&RELAY_TIMER);
        TIMER_HAS_RUN_OUT = false;
        while (!digitalRead(BUTTON_PIN))
        {
            delay(100);
        }
        _state = 0;
    }

    // Read the serial port.
    // There are three possible commands:
    // - Integer (0..2^32-1) to set the wait time
    // - ON to open the relay
    // - OFF to close the relay
    else if (_state == 4)
    {
        _state = 0;

        if (!Serial.available())
            return;

        String command = Serial.readStringUntil('\n');

        if (command.length() == 0)
            return;

        if (isNumeric(command))
        {
            WAIT_TIME_MS = command.toInt();
            write_WAIT_TIME_MS_to_file();
            Serial.print("WAIT TIME (ms) ");
            Serial.println(WAIT_TIME_MS);
        }
        else if (command.equals("ON"))
            switch_on_relay();
        else if (command.equals("OFF"))
            switch_off_relay();
        else
            Serial.println("Invalid command");
    }
}
