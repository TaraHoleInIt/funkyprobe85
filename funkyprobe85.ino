// Copyright (c) 2023 Tara Keeling
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include <Wire.h>

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include <stdio.h>
#include <string.h>

#if defined __AVR_ATtiny85__
    #define Config_Pin_ClickButton PIN_B1
    #define Config_Pin_BackButton PIN_B3
    #define Config_Pin_Probe A2
#else
    #error TODO: Port me to another platform!
#endif

// Because this is what I've been testing with
#ifndef ATTINY_CORE
    #error Requires ATTinyCore
#endif

// TODO:
// I was going somewhere with this?
#define Config_VIH 2000
#define Config_VIL 800

#define DisplayWidth 128
#define FontWidth 8
#define CharsWide ( DisplayWidth / FontWidth )

// TODO:
// This is a ***damn mess
class FilteredButton {
public:
    FilteredButton( int gpio ) {
        this->gpio = gpio;
        this->rawInput = 0;
        this->lastState = false;
        this->state = false;
    }

    void Begin( void ) {
        pinMode( this->gpio, INPUT );
    }

    void Update( void ) {
        uint8_t temp = 0;

        temp = rawInput;

        rawInput <<= 1;
        rawInput |= ( digitalRead( gpio ) == HIGH ) ? 1 : 0;

        if ( rawInput == 0xFF && temp == 0x7F ) {
            down = true;
        }
    }

    // TODO:
    // Gross :(
    bool IsDown( void ) {
        bool temp = down;
        down = false;
        return temp;
        //return ( lastState == false && state == true );
    }

    bool IsUp( void ) {
        return lastState == true && state == false;
    }

private:
    uint8_t rawInput;
    int gpio;

    bool lastState;
    bool state;

    bool down;
};

void selectBit( int newBit );
void drawChar( char c, int x, int y );
void setCurrentBit( int newValue );
int measureVCC( void );
char getProbeState( void );
bool isColumnSelected( int col );
void drawProbeValue( void );
void drawHexValue( void );
int measureProbeVoltage( void );

static const char hexTable[ 16 ] PROGMEM = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'A', 'B',
    'C', 'D', 'E', 'F'
};

SSD1306AsciiWire lcd;

uint16_t binaryValue = 0;
char probeValue = 0;
int selectedBit = 0;

FilteredButton clickButton( Config_Pin_ClickButton );
FilteredButton backButton( Config_Pin_BackButton );

int measureVCC( void ) {
    int result = 0;
 
    result = analogRead( 0x0C );
    delay( 2 );
    result = analogRead( 0x0C );
    
    return ( 1100UL * 1023UL ) / result;
}

// probe voltage:
// x = (probe_adc/1023) * vcc
// is this correct? idk
int measureProbeVoltage( void ) {
    uint32_t vcc = measureVCC( );
    uint32_t result = 0;

    result = analogRead( Config_Pin_Probe ) * 1000UL;
    result = ( result / 1023UL ) * vcc;
    result = result / 1000UL;

    return ( int ) result;
}

void setup( void ) {
    clickButton.Begin( );
    backButton.Begin( );

    Wire.begin( );
    Wire.setClock( 400000 );

    lcd.begin( &Adafruit128x32, 0x3C );
    lcd.setScrollMode( SCROLL_MODE_OFF );
    lcd.clear( );

    lcd.setFont( cp437font8x8 );
}

// Source:
// https://forum.arduino.cc/t/detecting-all-possible-states-of-an-input-pin/696838/15
// TODO:
// Proper calculations for VHI, and such...
#define HIGH_Z_DETECT_PCT 0.05
#define HIGH_Z_LOW ( ( int ) ( 512.0 * ( 1.0 - HIGH_Z_DETECT_PCT ) ) )
#define HIGH_Z_HIGH ( ( int ) ( 512.0 * ( 1.0 + HIGH_Z_DETECT_PCT ) ) )

char getProbeState( void ) {
    int adcValue = analogRead( Config_Pin_Probe );
    char result = 'Z';

    // i have absolutely no idea if this is correct
    if ( adcValue < HIGH_Z_LOW || adcValue > HIGH_Z_HIGH ) {
        adcValue = measureProbeVoltage( );

        if ( adcValue < Config_VIH ) {
            result = '0';
        } else {
            result = '1';
        }
    }

    return result;
}

bool isColumnSelected( int col ) {
    return ( 15 - col ) == selectedBit;
}

void drawHexValue( void ) {
    uint16_t value = binaryValue;

    lcd.setCursor( 0, 0 );
    lcd.setInvertMode( false );

    for ( int i = 0; i < 4; i++ ) {
        lcd.write( pgm_read_byte( &hexTable[ ( ( value >> 12 ) & 0x0F ) ] ) );
        value <<= 4;
    }

    lcd.write( 'h' );
}

void drawChar( char c, int x, int y ) {
    lcd.setCursor( x * FontWidth, y );
    lcd.write( c );
}

void drawBitValues( void ) {
    uint16_t value = binaryValue;
    bool isSelected = false;
    char c = 0;

    for ( int i = 15; i >= 0; i-- ) {
        isSelected = isColumnSelected( i );
        c = ( value & 0x01 ) ? '1' : '0';

        if ( isSelected ) {
            c = getProbeState( );
        }

        lcd.setInvertMode( isSelected );
        drawChar( c, i, 3 );

        value >>= 1;
    }
}

void drawBitNumbers( void ) {
    int whole = 0;
    int frac = 0;
    char upper = 0;
    char lower = 0;

    for ( int i = 15, x = 0; i >= 0; i-- ) {
        whole = i / 10;
        frac = i % 10;

        upper = ( whole ) ? '0' + whole : ' ';
        lower = '0' + frac;

        lcd.setInvertMode( isColumnSelected( 15 - i ) );
        drawChar( upper, x, 1 );
        drawChar( lower, x, 2 );

        x++;
    }
}

void drawProbeValue( void ) {
    lcd.setInvertMode( true );
    drawChar( probeValue, 15 - selectedBit, 3 );
}

void selectBit( int newBit ) {
    newBit = ( newBit < 0 ) ? 15 : newBit;
    newBit = ( newBit > 15 ) ? 0 : newBit;

    selectedBit = newBit;
    
    drawBitNumbers( );
    drawBitValues( );
}

void setCurrentBit( int newValue ) {
    newValue &= 0x01;

    if ( newValue ) {
        binaryValue |= bit( selectedBit );
    } else {
        binaryValue &= ~bit( selectedBit );
    }

    selectBit( selectedBit + 1 );
    drawHexValue( );
}

void loop( void ) {
    uint32_t nextProbeUpdate = 0;
    uint32_t timeNow = 0;

    drawHexValue( );
    drawBitNumbers( );
    drawBitValues( );
    drawProbeValue( );

    while ( true ) {
        timeNow = millis( );

        clickButton.Update( );
        backButton.Update( );

        if ( timeNow >= nextProbeUpdate ) {
            nextProbeUpdate = timeNow + 100;
            probeValue = getProbeState( );

            drawProbeValue( );
        }

        if ( backButton.IsDown( ) ) {
            selectBit( selectedBit - 1 );
        }

        if ( clickButton.IsDown( ) ) {
            if ( probeValue != 'Z' ) {
                setCurrentBit( ( probeValue == '1' ) ? 1 : 0 );
            }
        }

        delay( 1 );
    }
}
