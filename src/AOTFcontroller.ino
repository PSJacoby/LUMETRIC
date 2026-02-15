/*
 * This goal of the application is to set the digital output on pins 8-13 
 * This can be accomplished in three ways.  First, a serial command can directly set
 * the digital output pattern.  Second, a series of patterns can be stored in the 
 * Arduino and TTLs coming in on pin 2 will then trigger to the consecutive pattern (trigger mode).
 * Third, intervals between consecutive patterns can be specified and paterns will be 
 * generated at these specified time points (timed trigger mode).
 *
 * Interface specifications:
 * digital pattern specification: single byte, bit 0 corresponds to pin 8, 
 *   bit 1 to pin 9, etc..  Bits 7 and 8 will not be used (and should stay 0).
 *
 * Set digital output command: 1p
 *   Where p is the desired digital pattern.  Controller will return 1 to 
 *   indicate succesfull execution.
 *
 * Get digital output command: 2
 *   Controller will return 2p.  Where p is the current digital output pattern
 *
 * Set Analogue output command: 3xvv
 *   Where x is the output channel (either 1 or 2), and vv is the output in a 
 *   12-bit significant number.
 *   Controller will return 3xvv:
 *
 * Get Analogue output:  4
 *
 *
 * Set digital patten for triggered mode: 5xd 
 *   Where x is the number of the pattern (Quesry the max number of patterns with 32, default is 12, but number can be changed in the firmware).
 *   and d is the digital pattern to be stored at that position.  Note that x should
 *   be the real number (i.e., not  ASCI encoded)
 *   Controller will return 5xd 
 *
 * Set the Number of digital patterns to be used: 6x
 *   Where x indicates how many digital patterns will be used (currently, up to 12
 *   patterns maximum).  In triggered mode, after reaching this many triggers, 
 *   the controller will re-start the sequence with the first pattern.
 *   Controller will return 6x
 *
 * Skip trigger: 7x
 *   Where x indicates how many digital change events on the trigger input pin
 *   will be ignored.
 *   Controller will respond with 7x
 *
 * Start trigger mode: 8
 *   Controller will return 8 to indicate start of triggered mode
 *   Stop triggered a 9. Trigger mode will  supersede (but not stop) 
 *   blanking mode (if it was active)
 * 
 * Stop Trigger mode: 9
 *   Controller will return 9x where x is the number of triggers received during the last
 *   trigger mode run
 *
 * Set time interval for timed trigger mode: 10xtt
 *   Where x is the number of the interval (currently, 12 intervals can be stored)
 *   and tt is the interval (in ms) in Arduino unsigned int format.  
 *   Controller will return 10x
 *
  * Sets how often the timed pattern will be repeated: 11x
 *   This value will be used in timed-trigger mode and sets how often the output
 *   pattern will be repeated. 
 *   Controller will return 11x
 *  
 * Starts timed trigger mode: 12
 *   In timed trigger mode, digital patterns as set with function 5 will appear on the 
 *   output pins with intervals (in ms) as set with function 10.  After the number of 
 *   patterns set with function 6, the pattern will be repeated for the number of times
 *   set with function 11.  Any input character (which will be processed) will stop 
 *   the pattern generation.
 *   Controller will retun 12.
 * 
 * Start blanking Mode: 20
 *   In blanking mode, zeroes will be written on the output pins when the trigger pin
 *   is low, when the trigger pin is high, the pattern set with command #1 will be 
 *   applied to the output pins. 
 *   Controller will return 20
 *
 * Stop blanking Mode: 21
 *   Stops blanking mode.  Controller returns 21
 *
 * Blanking mode trigger direction: 22x
 *   Sets whether to blank on trigger high or trigger low.  x=0: blank on trigger high,
 *   x=1: blank on trigger low.  x=0 is the default
 *   Controller returns 22
 *
 * 
 * Get Identification: 30
 *   Returns (asci!) MM-Ard\r\n
 *
 * Get Version: 31
 *   Returns: version number (as ASCI string) \r\n
 *
 * Get Max number of patterns that can be uploaded: 32
 *   Returns: Max number of patterns as an unsigned int, 2 bytes, highbyte first
 *   Available as of version 3
 *
 * Fast version to upload a digital sequence: 33
 *   first 2 bytes indicate the number of bytes to be uploaded (and length of the sequence)
 *   followed by the indicated number of bytes
 *   Available as of version 4
 *
 * Get DA channel count: 34
 *   Returns: 34 followed by 1 byte with the number of DA channels
 *   Available as of version 5
 *
 * Get digital pin count: 35
 *   Returns: 35 followed by 1 byte with the number of digital output pins
 *   Available as of version 5
 *
 *
 * Read digital state of analogue input pins 0-5: 40
 *   Returns raw value of PINC (two high bits are not used)
 *
 * Read analogue state of pint pins 0-5: 41x
 *   x=0-5.  Returns analogue value as a 10-bit number (0-1023)
 *
 *
 * 
 * Possible extensions:
 *   Set and Get Mode (low, change, rising, falling) for trigger mode
 *   Get digital patterm
 *   Get Number of digital patterns
 */
 
   unsigned int version_ = 5;

// If you have one of these DA chips attached, uncomment the appropriate define
// #define TLV5618
// #define TLV56x8


  // const uint8_t numDAChannels_ = 0;  // Set to appropriate number depending on attached DA chip
  #if defined TLV5618
  const uint8_t numDAChannels_ = 2;
  #elif defined TLV56x8
  const uint8_t numDAChannels_ = 4;
  #else
  const uint8_t numDAChannels_ = 0;
  #endif

  const uint8_t numDigitalPins_ = 6;
   
   // pin on which to receive the trigger (2 and 3 can be used with interrupts, although this code does not use interrupts)
   int inPin_ = 2;
   // to read out the state of inPin_ faster, use 
   int inPinBit_ = 1 << inPin_;  // bit mask 
   
   // pin connected to DIN of TLV5618
   int dataPin = 3;
   // pin connected to SCLK of TLV5618
   int clockPin = 4;
   // pin connected to CS of TLV5618
   #ifdef TLV5618
   int latchPin = 5;
   #endif

   #ifdef TLV56x8
   int CS1 = 5;  // Used for TLV56X8
   int CS2 = 6;  // Used for TLV56X8
   #endif

   const uint16_t SEQUENCELENGTH = 256;  // Can be increased, but pay attention that there is significant memory left for local variables
   byte triggerPattern_[SEQUENCELENGTH]; 
   unsigned int triggerDelay_[SEQUENCELENGTH]; 
   int patternLength_ = 0;
   byte repeatPattern_ = 0;
   volatile long triggerNr_; // total # of triggers in this run (0-based)
   volatile long sequenceNr_; // # of trigger in sequence (0-based)
   int skipTriggers_ = 0;  // # of triggers to skip before starting to generate patterns
   byte currentPattern_ = 0;
   const unsigned long timeOut_ = 1000;
   bool blanking_ = false;
   bool blankOnHigh_ = false;
   bool triggerMode_ = false;
   boolean triggerState_ = false;
 
 void setup() {
   // Higher speeds do not appear to be reliable
   Serial.begin(57600);
   
   // Debug serial port on Serial1 (pins 18=TX, 19=RX)
   Serial1.begin(115200);
   Serial1.println("\n=== Arduino AOTF Controller Debug ===\n");
  
   pinMode(inPin_, INPUT);
   pinMode (dataPin, OUTPUT);
   pinMode (clockPin, OUTPUT);
   #ifdef TLV5618
   pinMode (latchPin, OUTPUT);
   #endif
   #ifdef TLV56x8
   pinMode (CS1, OUTPUT);
   pinMode (CS2, OUTPUT);
   #endif
   // Arduino Giga R1 - Using Port J pins (PJ0-PJ5 = D25,27,29,31,33,35)
   pinMode(25, OUTPUT);
   pinMode(27, OUTPUT);
   pinMode(29, OUTPUT);
   pinMode(31, OUTPUT);
   pinMode(33, OUTPUT);
   pinMode(35, OUTPUT);
   
   // Note: DDRC and PORTC (Arduino Uno) are not used on Arduino Giga R1
   
   #ifdef TLV5618
   digitalWrite(latchPin, HIGH);   
   #endif
   #ifdef TLV56x8
   digitalWrite(CS1, HIGH);
   digitalWrite(CS2, HIGH);
   #endif

   for (unsigned int i = 0; i < SEQUENCELENGTH; i++) {
      triggerPattern_[i] = 0;
      triggerDelay_[i] = 0;
   }
 }

// Helper functions for Arduino Giga GPIO Port J
// Pins D25,27,29,31,33,35 map to PJ0-PJ5
// Pattern bits 0-5 map directly to PJ0-PJ5 (ODR bits 0-5)
void setPortJ(byte pattern) {
   GPIOJ->ODR = (pattern & 0x3F);
   
   // Debug output
   Serial1.print("Set Port J: 0b");
   Serial1.print(pattern, BIN);
   Serial1.print(" Pins: ");
   for (int i = 0; i < 6; i++) {
     Serial1.print("D");
     Serial1.print(25 + i*2);
     Serial1.print("=");
     Serial1.print((pattern & (1 << i)) ? "HIGH " : "LOW ");
   }
   Serial1.println();
}

byte getPortJ() {
   return (byte)(GPIOJ->ODR & 0x3F);
}
 
 void loop() {
   if (Serial.available() > 0) {
     int inByte = Serial.read();
     
     // Debug: Log received command
     Serial1.print("Command received: ");
     Serial1.print(inByte);
     Serial1.print(" (0x");
     Serial1.print(inByte, HEX);
     Serial1.println(")");
     
     switch (inByte) {
       
       // Set digital output
       case 1 :
          if (waitForSerial(timeOut_)) {
            currentPattern_ = Serial.read();
            Serial1.print("  Pattern data: 0b");
            Serial1.println(currentPattern_, BIN);
            // Do not set bits 6 and 7 (not sure if this is needed..)
            currentPattern_ = currentPattern_ & B00111111;
            if (!blanking_)
              setPortJ(currentPattern_);
            Serial.write( byte(1));
          }
          break;
          
       // Get digital output
       case 2:
          {
            byte currentPort = getPortJ();
            Serial1.print("  Reading Port J: 0b");
            Serial1.println(currentPort, BIN);
            Serial.write( byte(2));
            Serial.write( currentPort);
          }
          break;
          
       // Set Analogue output (TODO: save for 'Get Analogue output')
       case 3:
         if (waitForSerial(timeOut_)) {
           int channel = Serial.read();
           if (waitForSerial(timeOut_)) {
              byte msb = Serial.read();
              msb &= B00001111;
              if (waitForSerial(timeOut_)) {
                byte lsb = Serial.read();
                analogueOut(channel, msb, lsb);
                Serial.write( byte(3));
                Serial.write( channel);
                Serial.write(msb);
                Serial.write(lsb);
              }
           }
         }
         break;
         
       // Sets the specified digital pattern
       case 5:
          if (waitForSerial(timeOut_)) {
            int patternNumber = Serial.read();
            if ( (patternNumber >= 0) && (patternNumber < SEQUENCELENGTH) ) {
              if (waitForSerial(timeOut_)) {
                triggerPattern_[patternNumber] = Serial.read();
                triggerPattern_[patternNumber] = triggerPattern_[patternNumber] & B00111111;
                Serial.write( byte(5));
                Serial.write( patternNumber);
                Serial.write( triggerPattern_[patternNumber]);
                break;
              }
            }
          }
          Serial.write( "n:");//Serial.print("n:");
          break;
          
       // Sets the number of digital patterns that will be used
       case 6:
         if (waitForSerial(timeOut_)) {
           int pL = Serial.read();
           if ( (pL >= 0) && (pL <= SEQUENCELENGTH) ) {
             patternLength_ = pL;
             Serial.write( byte(6));
             Serial.write( patternLength_);
           }
         }
         break;
         
       // Skip triggers
       case 7:
         if (waitForSerial(timeOut_)) {
           skipTriggers_ = Serial.read();
           Serial.write( byte(7));
           Serial.write( skipTriggers_);
         }
         break;
         
       //  starts trigger mode
       case 8: 
         if (patternLength_ > 0) {
           Serial1.println("  Starting trigger mode");
           sequenceNr_ = 0;
           triggerNr_ = -skipTriggers_;
           triggerState_ = digitalRead(inPin_) == HIGH;
           setPortJ(0);
           Serial.write( byte(8));
           triggerMode_ = true;           
         }
         break;
         
         // return result from last triggermode
       case 9:
          Serial1.print("  Stopping trigger mode. Triggers received: ");
          Serial1.println(triggerNr_);
          triggerMode_ = false;
          setPortJ(0);
          Serial.write( byte(9));
          Serial.write( triggerNr_);
          break;
          
       // Sets time interval for timed trigger mode
       // Tricky part is that we are getting an unsigned int as two bytes
       case 10:
          if (waitForSerial(timeOut_)) {
            int patternNumber = Serial.read();
            if ( (patternNumber >= 0) && (patternNumber < SEQUENCELENGTH) ) {
              if (waitForSerial(timeOut_)) {
                unsigned int highByte = 0;
                unsigned int lowByte = 0;
                highByte = Serial.read();
                if (waitForSerial(timeOut_))
                  lowByte = Serial.read();
                highByte = highByte << 8;
                triggerDelay_[patternNumber] = highByte | lowByte;
                Serial.write( byte(10));
                Serial.write(patternNumber);
                break;
              }
            }
          }
          break;

       // Sets the number of times the patterns is repeated in timed trigger mode
       case 11:
         if (waitForSerial(timeOut_)) {
           repeatPattern_ = Serial.read();
           Serial.write( byte(11));
           Serial.write( repeatPattern_);
         }
         break;

       //  starts timed trigger mode
       case 12: 
         if (patternLength_ > 0) {
           setPortJ(0);
           Serial.write( byte(12));
           for (byte i = 0; i < repeatPattern_ && (Serial.available() == 0); i++) {
             for (int j = 0; j < patternLength_ && (Serial.available() == 0); j++) {
               setPortJ(triggerPattern_[j]);
               delay(triggerDelay_[j]);
             }
           }
           setPortJ(0);
         }
         break;

       // Blanks output based on TTL input
       case 20:
         blanking_ = true;
         Serial.write( byte(20));
         break;
         
       // Stops blanking mode
       case 21:
         blanking_ = false;
         Serial.write( byte(21));
         break;
         
       // Sets 'polarity' of input TTL for blanking mode
       case 22: 
         if (waitForSerial(timeOut_)) {
           int mode = Serial.read();
           if (mode==0)
             blankOnHigh_= true;
           else
             blankOnHigh_= false;
         }
         Serial.write( byte(22));
         break;
         
       // Gives identification of the device
       case 30:
         Serial1.println("  Sending identification: MM-Ard");
         Serial.println("MM-Ard");
         break;
         
       // Returns version string
       case 31:
         Serial1.print("  Sending version: ");
         Serial1.println(version_);
         Serial.println(version_);
         break;

        // returns Maximum number of patterns for sequencing
       case 32:
         Serial.write( byte(32));
         Serial.write(highByte(SEQUENCELENGTH));
         Serial.write(lowByte(SEQUENCELENGTH));
         break;

       // Faster way of uploading sequence:
       case 33:
         {
           unsigned int highByte = 0;
           unsigned int lowByte = 0;
           unsigned int count = 0;
           if (waitForSerial(timeOut_)) {
             highByte = Serial.read();
             if (waitForSerial(timeOut_)) {
               lowByte = Serial.read();
               highByte = highByte << 8;
               unsigned int expectedNumPatterns = highByte | lowByte;
               if ((expectedNumPatterns >= 0) && (expectedNumPatterns < SEQUENCELENGTH)) {
                 while (count < expectedNumPatterns && waitForSerial(timeOut_)) {
                   triggerPattern_[count] = Serial.read();
                   triggerPattern_[count] = triggerPattern_[count] & B00111111;
                   count++;
                 }
               }
             }
           }
           patternLength_ = count;
           Serial.write(byte(33));
           Serial.write(highByte(count));
           Serial.write(lowByte(count));
         }
         break;

       // Returns the number of DA channels
       case 34:
         Serial.write(byte(34));
         Serial.write(byte(numDAChannels_));
         break;

       // Returns the number of digital output pins
       case 35:
         Serial.write(byte(35));
         Serial.write(byte(numDigitalPins_));
         break;

       case 40:
         // Read digital state of analog pins A0-A5 (equivalent to PINC on Uno)
         {
           byte pinStates = 0;
           for (int i = 0; i < 6; i++) {
             if (digitalRead(A0 + i)) {
               pinStates |= (1 << i);
             }
           }
           Serial.write( byte(40));
           Serial.write( pinStates);
         }
         break;
         
       case 41:
         if (waitForSerial(timeOut_)) {
           int pin = Serial.read();  
           if (pin >= 0 && pin <=5) {
              int val = analogRead(pin);
              Serial.write( byte(41));
              Serial.write( pin);
              Serial.write( highByte(val));
              Serial.write( lowByte(val));
           }
         }
         break;
         
       case 42:
         if (waitForSerial(timeOut_)) {
           int pin = Serial.read();
           if (waitForSerial(timeOut_)) {
             int state = Serial.read();
             Serial.write( byte(42));
             Serial.write( pin);
             if (state == 0) {
                digitalWrite(14+pin, LOW);
                Serial.write( byte(0));
             }
             if (state == 1) {
                digitalWrite(14+pin, HIGH);
                Serial.write( byte(1));
             }
           }
         }
         break;

       }
    }
    
    // In trigger mode, we will blank even if blanking is not on..
    if (triggerMode_) {
      boolean tmp = digitalRead(inPin_);
      if (tmp != triggerState_) {
        if (blankOnHigh_ && tmp ) {
          setPortJ(0);
        }
        else if (!blankOnHigh_ && !tmp ) {
          setPortJ(0);
        }
        else { 
          if (triggerNr_ >=0) {
            setPortJ(triggerPattern_[sequenceNr_]);
            sequenceNr_++;
            if (sequenceNr_ >= patternLength_)
              sequenceNr_ = 0;
          }
          triggerNr_++;
        }
        
        triggerState_ = tmp;       
      }  
    } else if (blanking_) {
      if (blankOnHigh_) {
        if (! digitalRead(inPin_))
          setPortJ(currentPattern_);
        else
          setPortJ(0);
      }  else {
        if (! digitalRead(inPin_))
          setPortJ(0);
        else  
          setPortJ(currentPattern_);
      }
    }
}

 
bool waitForSerial(unsigned long timeOut)
{
    unsigned long startTime = millis();
    while (Serial.available() == 0 && (millis() - startTime < timeOut) ) {}
    if (Serial.available() > 0)
       return true;
    return false;
 }

#if defined TLV5618
// Sets analogue output in the TLV5618
// channel is either 0 ('A') or 1 ('B')
// value should be between 0 and 4095 (12 bit max)
// pins should be connected as described above
void analogueOut(int channel, byte msb, byte lsb) 
{
  digitalWrite(latchPin, LOW);
  msb &= B00001111;
  if (channel == 0)
     msb |= B10000000;
  // Note that in all other cases, the data will be written to DAC B and BUFFER
  shiftOut(dataPin, clockPin, MSBFIRST, msb);
  shiftOut(dataPin, clockPin, MSBFIRST, lsb);
  // The TLV5618 needs one more toggle of the clockPin:
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  digitalWrite(latchPin, HIGH);
}

#elif defined TLV56x8
// Sets analogue output in the TLV5618
// channel is either 0 ('A') or 1 ('B')
// value should be between 0 and 4095 (12 bit max)
// pins should be connected as described above
void analogueOut(int channel, byte msb, byte lsb) 
{
   // Select DAC

    // Configure Channel
    msb &= B00001111;
    if (channel == 0){
        digitalWrite(CS1, LOW);  // Activate DAC 1
        digitalWrite(CS2, HIGH); // 
        msb |= B10000000; // 
        }
    else if (channel == 1){
        digitalWrite(CS1, LOW);  // Activate DAC 2
        digitalWrite(CS2, HIGH); // 
        msb &= B00001111; // 
        }
    // Alternative Channel Selection Logic
    else if (channel == 2){
        digitalWrite(CS1, HIGH);  // Activate DAC 3
        digitalWrite(CS2, LOW); // 
        msb |= B10000000; // 
        }
    else if (channel == 3){
        digitalWrite(CS1, HIGH);  // Activate DAC 4
        digitalWrite(CS2, LOW); //
        msb &= B00001111; // 
    }
    // send data
    shiftOut(dataPin, clockPin, MSBFIRST, msb);
    shiftOut(dataPin, clockPin, MSBFIRST, lsb);

    // End Transmission
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);

    // Deactivate all DACs
    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
}
#else

void analogueOut(int channel, byte msb, byte lsb) {}; // noop

#endif





/* 
 // This function is called through an interrupt   
void triggerMode() 
{
  if (triggerNr_ >=0) {
    PORTB = triggerPattern_[sequenceNr_];
    sequenceNr_++;
    if (sequenceNr_ >= patternLength_)
      sequenceNr_ = 0;
  }
  triggerNr_++;
}


void blankNormal() 
{
    if (DDRD & B00000100) {
      PORTB = currentPattern_;
    } else
      PORTB = 0;
}

void blankInverted()
{
   if (DDRD & B00000100) {
     PORTB = 0;
   } else {     
     PORTB = currentPattern_;  
   }
}   

*/
  


