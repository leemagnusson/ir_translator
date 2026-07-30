#include <IRremote.h>

// Define the pin where the IR receiver is connected
const int IR_RECEIVE_PIN = 2;

void setup() {
  // Start the serial connection to print to the console
  Serial.begin(9600);
  
  // Start the IR receiver
  // The second parameter enables the built-in LED to blink when a signal is received
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.println("IR Receiver is ready. Press a button on your remote.");
}

void loop() {
  // Check if a signal has been received and successfully decoded
  if (IrReceiver.decode()) {
    
    // Print the received command as a hexadecimal number
    Serial.print("Command received: 0x");
    Serial.println(IrReceiver.decodedIRData.command, HEX);
    
    // Resume listening for the next signal
    IrReceiver.resume();
  }
}