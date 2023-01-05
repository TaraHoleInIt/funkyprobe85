### About
This is an attempt at making a quick-and-dirty 16-bit logic probe.  
Maybe someone will find this useful somehow for some reason.  

### Requirements
- ATTinyCore Arduino framework
- ATtiny85 with fuses set at 8MHz
- 128x32 I2C SSD1306 OLED Display
- 0.1uF Capacitor
- 2x 4.7k Resistors
- 3x 10k Resistors
- 2x 100k Resistors
- 3x Pushbuttons
- Wire

### Using
The probe has a painfully basic user interface consisting of 4 lines of text:  
- The first line is a hexadecimal representation of the binary values you read in,
- The second and third lines, going right to left, are used to display numbers 0 to 15 for each bit, and
- The fourth line shows the current binary representation of the binary values you have read in.
  
The currently selected bit starts at 0, and it is displayed as inverted text for the bit numbers as well as the value currently seen by the probe.  
When the probe is not connected, the letter 'Z' will appear in place of the binary value and you will be unable to "click" to save the state of the current bit. Note that the hex value will not change until after you have saved the state of the current bit.
  
There are a total of 3 buttons, including the reset button. The "click" button saves the state read by the probe into the currently selected bit and advances to the next bit.  
The "back" button simply selects the previous bit; it can be used to undo a mistake or you can cycle around until you select your desired bit.  
Of course, the reset button is great for just erasing everything and starting from scratch.  
