#include <Arduino.h>
/* 
 * Must use IRremote library version 4.0.0 or newer.
 * Install via Library Manager: "IRremote" by Shirriff
 */
#define EXCLUDE_UNIVERSAL_PROTOCOLS // Saves memory, we only need NEC
#define EXCLUDE_EXOTIC_PROTOCOLS
#include <IRremote.hpp>

// --- Hardware Pins ---
constexpr uint8_t IR_RECEIVE_PIN = 2; // Connect TSOP data here
constexpr uint8_t IR_SEND_PIN    = 3; // Must be Pin 3 on ATmega328P (Timer2 PWM)

// --- Known Addresses (Placeholders - you will need to dump yours) ---
constexpr uint16_t SNDBAR_ADDRESS = 0x78; 
constexpr uint16_t AVR_ADDRESS    = 0x7A;

// --- Soundbar Commands (Incoming) ---
constexpr uint8_t SNDBAR_CMD_POWER    = 0x10;
constexpr uint8_t SNDBAR_CMD_VOL_UP   = 0x1E;
constexpr uint8_t SNDBAR_CMD_VOL_DOWN = 0x1F;
constexpr uint8_t SNDBAR_CMD_MUTE     = 0x9C; // Added: Soundbar Mute

// --- AV Receiver Commands (Outgoing) ---
constexpr uint8_t AVR_CMD_POWER       = 0x20;
constexpr uint8_t AVR_CMD_VOL_UP      = 0x1A;
constexpr uint8_t AVR_CMD_VOL_DOWN    = 0x1B;
constexpr uint8_t AVR_CMD_MUTE        = 0x23; // Added: AVR Mute

// State tracking for the repeat mechanism
uint8_t last_avr_command = 0x00;

unsigned long previousMillis = 0;
const long blinkInterval = 500; // 500ms ON, 500ms OFF = 1 blink per second
bool ledIsOn = false;
uint8_t colorState = 0; // 0=Red, 1=Green, 2=Blue

void setup() {
    Serial.begin(115200);
    //while (!Serial);
    uint32_t t = millis();
    while (!Serial && (millis() - t < 3000)); 

    // Setup RGB Pins
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);

    // Start the receiver
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    
    // Start the sender on Pin 3
    IrSender.begin(IR_SEND_PIN);

    pinMode(D4, OUTPUT);
    
    unsigned long previousMillis = 0;
    const long blinkInterval = 500; // 500ms ON, 500ms OFF = 1 blink per second
    bool ledIsOn = false;
    uint8_t colorState = 0; // 0=Red, 1=Green, 2=Blue
    
    Serial.println(F("Yamaha IR Translator Ready."));
}

void loop() {


    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
        previousMillis = currentMillis;
        ledIsOn = !ledIsOn; // Toggle state

        if (ledIsOn) {
            // Turn on the current color, turn off the others
            digitalWrite(LEDR,   colorState == 0 ? LOW : HIGH);
            digitalWrite(LEDG, colorState == 1 ?   LOW : HIGH);
            digitalWrite(LEDB,  colorState == 2 ?  LOW : HIGH);
            
            // Move to the next color for the next blink
            colorState = (colorState + 1) % 3; 
        } else {
            // Turn all off during the OFF phase of the blink
            digitalWrite(LEDR,  HIGH);
            digitalWrite(LEDG,  HIGH);
            digitalWrite(LEDB,  HIGH);
        }
    }

    if (IrReceiver.decode()) {
        Serial.println("Received something");
        
        // We only care about NEC protocol for Yamaha
        if (IrReceiver.decodedIRData.protocol == NEC) {
            Serial.print(F("Received address 0x"));
            Serial.print(IrReceiver.decodedIRData.address, HEX);
            Serial.print(F(", cmd 0x"));
            Serial.println(IrReceiver.decodedIRData.command, HEX);
            
            // 2. Handle the "Repeat" code (Button is being held down)
            if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
                if (last_avr_command != 0x00) {
                  //  Serial.println(F("Sending AVR Repeat"));
                    // Send a generic NEC repeat pulse
                  //  IrSender.sendNECRepeat();
                }
            } 
            // 3. Handle a new button press
            else if (IrReceiver.decodedIRData.address == SNDBAR_ADDRESS) {
                
                uint8_t incoming_cmd = IrReceiver.decodedIRData.command;
                uint8_t outgoing_cmd = 0x00;

                // Map Soundbar to AVR
                switch (incoming_cmd) {
                    case SNDBAR_CMD_POWER:    outgoing_cmd = AVR_CMD_POWER; break;
                    case SNDBAR_CMD_VOL_UP:   outgoing_cmd = AVR_CMD_VOL_UP; break;
                    case SNDBAR_CMD_VOL_DOWN: outgoing_cmd = AVR_CMD_VOL_DOWN; break;
                    case SNDBAR_CMD_MUTE:     outgoing_cmd = AVR_CMD_MUTE; break;     // Added: Mute mapping
                    default: break;
                }

                if (outgoing_cmd != 0x00) {
                    Serial.print(F("Translating Cmd: 0x"));
                    Serial.print(incoming_cmd, HEX);
                    Serial.print(F(" -> 0x"));
                    Serial.println(outgoing_cmd, HEX);

                    delay(55); // wait until after potential first repeat
                    
                    // Transmit the mapped AVR code (0 repeats, we handle repeats manually)
                    IrSender.sendNEC(AVR_ADDRESS, outgoing_cmd, 0);
                    
                    // Cache it so we know what to do if a repeat pulse follows
                    last_avr_command = outgoing_cmd;
                } else {
                    // Reset cache if we got an unmapped button
                    last_avr_command = 0x00; 
                }
            }
        }
        
        // 4. Critical Step: Transmitting disables the receiver to prevent a feedback loop.
        // We must re-enable it immediately to catch the next pulse in the ~40ms gap.
        IrReceiver.resume(); // Resets state and resumes listening
    }
}