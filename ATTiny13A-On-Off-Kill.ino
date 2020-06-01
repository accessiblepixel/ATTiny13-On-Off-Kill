/*   FILENAME: ATTiny13A-On-Off-Kill.ino
 *   ORIGINAL FILENAME: ATTiny85-On-Off.cpp
 *   VERSION: 3.5
 *   AUTHOR(s): 
 *   ATTiny13A VERSION adapted by Jessica https://accessiblepixel.com
 *   ORIGINAL VERSION by Ralph Bacon https://github.com/RalphBacon/173-ATTiny85-Push-Button-On-Off-control
 *   CONTACT: media@accessiblepixel.com
 *   LICENSE: 
 *   
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *   
 *   -------------------------------------------------------------------------------------------------------
 *  
 *   Pinout for ATMEL ATTINY 13/13A
 *   Pin 1 is RESET.
 *
 *                                     +-\/-+
 *                               PB5  1|o   |8  Vcc
 * [KILL (to/from external µC)]  PB3  2|    |7  PB2  [Green 'Power' LED]
 *      [N-Channel MOSFET Gate]  PB4  3|    |6  PB1  [Button (Interrupt)]
 *                               GND  4|    |5  PB0  [Red 'HALT' LED]
 *                                     +----+
 */

#include "Arduino.h"
#include <avr/interrupt.h>

// Interrupt pin PB1 aka PCINT1 aka physicl pin 6. is
// the only pin we can configure as an interrupt on the ATTiny13

#define INT_PIN PB1

// RED LED on PB0 to indicate when shutdown requests are not accepted 
// and to show when KILL is waiting for you to release the button.

#define HALT_LED PB0

// Output pin to N-Channel MOSfET
#define PWR_PIN PB4

// Power ON LED pin (will flash on shutdown) pin 5
#define PWR_LED PB2

// KILL interrupt for external µC
#define KILL PB3

// ISR handled variables
volatile bool togglePowerRequest = false;
bool shutDownAllowed = false;

// State of output ON/OFF
bool powerIsOn = false;
bool shutDownInProgress = false;

// When did we switch ON (so we can delay switching off)
unsigned long millisOnTime = 0;

// How long to hold down button to force turn off (ms)
#define KILL_TIME 5000

// Minimum time before shutdown requests accepted (ms)
#define MIN_ON_TIME 5000

// Define time between state changes of flashing LED notifications (ms)
word timeToFlashLED = 125; 

// When did we press and hold the OFF button
unsigned long millisOffTime = 0;

// Forward declaration(s)
void powerDownLedFlash();
void shutDownPower();
void waitingToKill();

//===================================
// SETUP    SETUP     SETUP     SETUP
//===================================

void setup(){
  // Keep the KILL command a WEAK HIGH until we need it
  pinMode(KILL, INPUT_PULLUP);

  // Keep the MOSFET switched OFF initially
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);

  // Enable LED pins as outputs.
  pinMode(PWR_LED, OUTPUT);
  pinMode(HALT_LED, OUTPUT);

  // Power ON Led
  digitalWrite(PWR_LED, LOW);
  
  // Interrupt pin when we press power pin again
  pinMode(INT_PIN, INPUT_PULLUP);
  
  // None of the 'original' Arduino way of setting an intterupt works here.
  // and using the MCUCR and GIMISK commands specific for the ATTiny85 didn't translate
  // across to the ATTiny13A correctly. I also changed where the interrupt is defined, as
  // it would trigger the interrupt if you kept the power switch pressed on startup.

  // Time to power up
  
  togglePowerRequest = true;
}

//=============================
// LOOP    LOOP    LOOP    LOOP
//=============================

void loop() {
  
  // Only after a few seconds do we allow shutdown requests
  
  if (millis() >= millisOnTime + MIN_ON_TIME && powerIsOn)
    if (!shutDownAllowed) {
      shutDownAllowed = true;
      
      // Enable interrupts here (ATTiny13A Specific) after MIN_ON_TIME has been
      // exceeded to avoid false tripping of the ISR.
      MCUCR = (MCUCR & ~(_BV(ISC00) | (ISC01))) | (FALLING << ISC00);
      GIMSK |= _BV(INT0);

      // Turn off the RED LED so that visually you can see when shutdown 
      // requests are accepted.
      digitalWrite(HALT_LED,LOW);
    }

  // Button was pressed and no shut down already running
  if (togglePowerRequest && !shutDownInProgress) {
    // Is the power currently ON?
    if (powerIsOn) {
      
      // And we can switch off after initial delay period
      if (shutDownAllowed) {
        shutDownInProgress = true;

        // Send the 100mS KILL command to external processor
        digitalWrite(KILL, LOW);
        pinMode(KILL, OUTPUT);
        delay(100);

        // Now we wait for confirmation from ext. µC
        pinMode(KILL, INPUT_PULLUP);
        digitalWrite(KILL, HIGH);

        // Kill INT0 ext interrupt as we don't need it again
        // until shutdown completed (when we can power up again)
        GIMSK &= ~_BV(INT0);
      }
    }
    else {
      
      // Set Power pin to MOSFET gate to high to turn on N-Channel MOSFET
      digitalWrite(PWR_PIN, HIGH);

      // Turn on Power LED pin to indicate we are running
      digitalWrite(PWR_LED, HIGH);

      // Power is ON

      // Illuminate RED LED to show that no shutdown requests until after MIN_ON_TIME milliseconds
      digitalWrite(HALT_LED,HIGH);
      millisOnTime = millis();

      // Set the flag that power is now ON
      powerIsOn = true;
    }

    // Clear the button press state
    togglePowerRequest = false;
  }

  // IF we are shutting down do not accept further presses
  // and wait for confirmation from external µC before killing power
  
  if (shutDownInProgress) {

    // Flash the power LED to indicate shutdown request
    powerDownLedFlash();

    // If we get the KILL command from external µC shutdown power
    if (digitalRead(KILL) == LOW) {
      
      // External µC has confirmed KILL power
      // we can now shut off the power
      shutDownPower();
    }

    // If the power switch is held down long enough KILL anyway
    if (digitalRead(INT_PIN) == HIGH) {
      
      // Button released so reset start time
      millisOffTime = 0;
      
    } else {

      // If we haven't captured start of long press do it now
      if (millisOffTime == 0) {
        millisOffTime = millis();
      }

      // Has the button been pressed long enough to KILL power?
      if (millis() > millisOffTime + KILL_TIME) {

        // Turn off the power (Instant Off)
        shutDownPower();
        
      }
    }
  }
}

// Interupt Service Routine (ATTiny13 Specific)

ISR(INT0_vect) {
  
  // Keep track of when we entered this routine
  
  static volatile unsigned long oldMillis = 0;

  // Switch has closed. 
  // Check for debounce ~10ms should be adequate.
  
  if (millis() > oldMillis + 10) {
    togglePowerRequest = true;
    oldMillis = millis();
  }
}

// Flash the POWER ON LED to indicate shutting down. this just toggles the LED
// on and off every timeToFlashLED milliseconds. A nice indication that we are shutting down.

void powerDownLedFlash() {
  static unsigned long flashMillis = millis();
  if (powerIsOn) {
    if (millis() > flashMillis + timeToFlashLED) {
      digitalWrite(PWR_LED, !digitalRead(PWR_LED));
      flashMillis = millis();
    }
  }
}

// Flash the Status LED to indicate shutdown is complete, and that you should release the 
// button to disconnect power. This also uses timeToFlashLED milliseconds.

void waitingToKill() {
  static unsigned long flashMillis = millis();
    if (millis() > flashMillis + timeToFlashLED) {
      digitalWrite(HALT_LED, !digitalRead(HALT_LED));
      flashMillis = millis();
    }
}

// Common power shutdown routine.

void shutDownPower() {
  
  // Reset all flags to initial state
  powerIsOn = false;
  shutDownAllowed = false;
  shutDownInProgress = false;
  millisOffTime = 0;

  // Wait for button to be released or we will start up again!
  while (digitalRead(INT_PIN) == LOW) {
    waitingToKill();
    delay(10);
  }

  // Turn off indicator LEDs and MOSFET
  digitalWrite(PWR_PIN, LOW);
  digitalWrite(HALT_LED, LOW);
  digitalWrite(PWR_LED, LOW);
}
