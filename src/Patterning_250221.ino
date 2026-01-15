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
 *   Where x is the number of the pattern (currently, 12 patterns can be stored).
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
 
   unsigned int version_ = 2;
   
   // pin on which to receive the trigger (2 and 3 can be used with interrupts, although this code does not use interrupts)
   int inPin_ = 2;
   // to read out the state of inPin_ faster, use 
   int inPinBit_ = 1 << inPin_;  // bit mask 
   
   // pin connected to DIN of TLV5618
   int dataPin = 3;
   // pin connected to SCLK of TLV5618
   int clockPin = 4;
   // pin connected to CS of TLV5618
   int latchPin = 5;

   const int SEQUENCELENGTH = 12;  // this should be good enough for everybody;)
   byte triggerPattern_[SEQUENCELENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0};
   unsigned int triggerDelay_[SEQUENCELENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0};
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
   unsigned int FirstframeRate = 0;
   unsigned int FirsthighByte = 0;
   unsigned int FirstlowByte = 0;
   unsigned int FirstExposure = 0;
   unsigned int FirstframeNumber = 0;
   byte Firstpinassignment;
   unsigned int SecframeRate = 0;
   unsigned int SechighByte = 0;
   unsigned int SeclowByte = 0;
   unsigned int SecExposure = 0;
   unsigned int SecframeNumber = 0;
   byte Secpinassignment;
   unsigned int ThirdframeRate = 0;
   unsigned int ThirdhighByte = 0;
   unsigned int ThirdlowByte = 0;
   unsigned int ThirdExposure = 0;
   unsigned int ThirdframeNumber = 0;
   byte Thirdpinassignment;
   byte FirstConstantpinassignment;
   byte SecondConstantpinassignment;
   byte ThirdConstantpinassignment;
   bool PatvarSet = true;
   bool PatgoOn = true;
   unsigned int FFC = 0; 
   unsigned int SFC = 0; 
   unsigned int TFC = 0; 
   unsigned long PatpreviousMillis = 0;
   unsigned long PatExposingMillis = 0; 
   unsigned long Exposure = 10000; 
   bool PatExposing = true;
   
 void setup() {
   // Higher speeds do not appear to be reliable
   Serial.begin(57600);
  
   pinMode(inPin_, INPUT);
   pinMode (dataPin, OUTPUT);
   pinMode (clockPin, OUTPUT);
   pinMode (latchPin, OUTPUT);
   pinMode(8, OUTPUT);
   pinMode(9, OUTPUT);
   pinMode(10, OUTPUT);
   pinMode(11, OUTPUT);
   pinMode(12, OUTPUT);
   pinMode(13, OUTPUT);
   
   // Set analogue pins as input:
   DDRC = DDRC & B11000000;
   // Turn on build-in pull-up resistors
   PORTC = PORTC | B00111111;
   
   digitalWrite(latchPin, HIGH);   
 }
 
 void loop() {
   if (Serial.available() > 0) {
     int inByte = Serial.read();
     switch (inByte) {
       
       // Set digital output
       case 1 :
          if (waitForSerial(timeOut_)){
            currentPattern_ = Serial.read();
            // Do not set bits 6 and 7 (not sure if this is needed..)
            currentPattern_ = currentPattern_ & B00111111;
            if (!blanking_)
              PORTB = currentPattern_;
            Serial.write( byte(1));
          }
          break;
          
       // Get digital output
       case 2:
          Serial.write( byte(2));
          Serial.write( PORTB);
          break;
          
       // Set Analogue output (TODO: save for 'Get Analogue output')
       case 3:
         if (waitForSerial(timeOut_)){
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
          if (waitForSerial(timeOut_)){
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
           if ( (pL >= 0) && (pL <= 12) ) {
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
           sequenceNr_ = 0;
           triggerNr_ = -skipTriggers_;
           triggerState_ = digitalRead(inPin_) == HIGH;
           PORTB = B00000000;
           Serial.write( byte(8));
           triggerMode_ = true;           
         }
         break;
         
         // return result from last triggermode
       case 9:
          triggerMode_ = false;
          PORTB = B00000000;
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
           PORTB = B00000000;
           Serial.write( byte(12));
           for (byte i = 0; i < repeatPattern_ && (Serial.available() == 0); i++) {
             for (int j = 0; j < patternLength_ && (Serial.available() == 0); j++) {
               PORTB = triggerPattern_[j];
               delay(triggerDelay_[j]);
             }
           }
           PORTB = B00000000;
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
         Serial.println("MM-Ard");
         break;
         
       // Returns version string
       case 31:
         Serial.println(version_);
         break;

       case 40:
         Serial.write( byte(40));
         Serial.write( PINC);
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

       case 47:{ //Data Eater
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               Firstpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               Secpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               Thirdpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               FirsthighByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               FirstlowByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         FirsthighByte = FirsthighByte << 8;
         FirstframeRate = (unsigned long)(FirsthighByte | FirstlowByte); // everything from 1ms to 60001ms 
         if (FirstframeRate == 60001){
           FirstframeRate = (unsigned long)40;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               SechighByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               SeclowByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         SechighByte = SechighByte << 8;
         SecframeRate = (unsigned long)(SechighByte | SeclowByte); // everything from 1ms to 60001ms 
         if (SecframeRate == 60001){
           SecframeRate = (unsigned long)40;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               ThirdhighByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               ThirdlowByte = status;
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         ThirdhighByte = ThirdhighByte << 8;
         ThirdframeRate = (unsigned long)(ThirdhighByte | ThirdlowByte); // everything from 1ms to 60001ms 
         if (ThirdframeRate == 60001){
           ThirdframeRate = (unsigned long)40;
         }         
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               FirstExposure = (unsigned long)status;
               if (FirstExposure == 251){
                 FirstExposure = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               SecExposure = (unsigned long)status;
               if (SecExposure == 251){
                 SecExposure = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               ThirdExposure = (unsigned long)status;
               if (ThirdExposure == 251){
                 ThirdExposure = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               FirstframeNumber = (unsigned long)status;
               if (FirstframeNumber == 251){
                 FirstframeNumber = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               SecframeNumber = (unsigned long)status;
               if (SecframeNumber == 251){
                 SecframeNumber = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if(waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){ 
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               ThirdframeNumber = (unsigned long)status;
               if (ThirdframeNumber == 251){
                 ThirdframeNumber = (unsigned long)40;
               }
               PatvarSet = false;
             }
           } 
           PatvarSet = true; 
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               FirstConstantpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               SecondConstantpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
         if (waitForSerial(timeOut_)){
           while (PatvarSet) {
             int status = Serial.read();
             if (status == 40){
               Serial.write( byte(40)); //send keep alive
               Serial.write( PINC);
             }
             else {
               ThirdConstantpinassignment = lowByte(status);
               PatvarSet = false;              
             }
           } 
           PatvarSet = true;
         }
        break;
       }


       case 48:{ // Aquisition Patterning 
         PORTB = 0;       
         PatpreviousMillis = millis();
         PatExposingMillis = micros(); 
         PatgoOn = true;
         while (PatgoOn) {
           while (FFC<FirstframeNumber){
             if (millis() - PatpreviousMillis > (FirstframeRate)) {
             FFC = FFC+1;
             PatpreviousMillis = millis();
             PORTB = Firstpinassignment + FirstConstantpinassignment;
             PatExposingMillis = micros();                    
             PatExposing = true;
             while (PatExposing) {
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){                    
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                    PatgoOn = false;
                    PORTB = 0; 
                    break;
                  }
                }
               if (micros() - PatExposingMillis > (FirstExposure * 1000)) {
                 PORTB = FirstConstantpinassignment;
                 PatExposing = false;
                 }
               }   
               
             }              
           }              
           FFC = 0;              
           while (SFC<SecframeNumber){
             if (millis() - PatpreviousMillis > (SecframeRate)) {
             SFC = SFC+1;
             PatpreviousMillis = millis();
             PORTB = Secpinassignment + SecondConstantpinassignment;
             PatExposingMillis = micros();                    
             PatExposing = true;
             while (PatExposing) {
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){                    
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                    PatgoOn = false;
                    PORTB = 0; 
                    break;
                  }
                }
               if (micros() - PatExposingMillis > (SecExposure * 1000)) {
                 PORTB = SecondConstantpinassignment;
                 PatExposing = false;
                 }
               }   
             }              
           }              
           SFC = 0;  
           while (TFC<ThirdframeNumber){
             if (millis() - PatpreviousMillis > (ThirdframeRate)) {
             TFC = TFC+1;
             PatpreviousMillis = millis();
             PORTB = Thirdpinassignment + ThirdConstantpinassignment;
             PatExposingMillis = micros();                    
             PatExposing = true;
             while (PatExposing) {
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){                    
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                    PatgoOn = false;
                    PORTB = 0; 
                    break;
                  }
                }
               if (micros() - PatExposingMillis > (ThirdExposure * 1000)) {
                 PORTB = ThirdConstantpinassignment;
                 PatExposing = false;
                 }
               }   
             }              
           }              
           TFC = 0;                
         }
         PORTB = 0;   
         break;
         }

       case 46:{ // Hard coded Pat
         PORTB = 0;       
         PatpreviousMillis = micros();
         PatExposingMillis = micros(); 
         while (PatgoOn) {
           while (FFC<1){
             if (micros() - PatpreviousMillis > 25000) {
             FFC = FFC+1;
             PatpreviousMillis = micros();
             PORTB = 17;
             PatExposingMillis = micros();                    
             PatExposing = true;
             while (PatExposing) {
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){                    
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                    PatgoOn = false;
                    PORTB = 0; 
                    break;
                  }
                }
               if (micros() - PatExposingMillis > Exposure) {
                 PORTB = 0;
                 PatExposing = false;
                 }
               }   
             }              
           }              
           FFC = 0;              
           while (SFC<1){
             if (micros() - PatpreviousMillis > 25000) {
             SFC = SFC+1;
             PatpreviousMillis = micros();
             PORTB = 3;
             PatExposingMillis = micros();                    
             PatExposing = true;
             while (PatExposing) {
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){                    
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                    PatgoOn = false;
                  }
                }
                if (micros() - PatExposingMillis > Exposure) {
                 PORTB = 0;
                 PatExposing = false;
                 }
               }   
             }              
           }              
           SFC = 0;   
           PORTB = 0;         
          }
         }
         break;

       case 45:{ // the trick is to build the serial comm. in a way that it does not disrupt the keep alive
          unsigned int frameRate = 0;
          unsigned int highByte = 0;
          unsigned int lowByte = 0;
          unsigned int Exposure = 0;
          byte pinassignment;
          bool varSet = true;
          if (waitForSerial(timeOut_)){
            while (varSet) {
              int status = Serial.read();
              if (status == 40){
                //Serial.write(0); 
                Serial.write( byte(40)); //send keep alive
                Serial.write( PINC);
              }
              else {
                pinassignment = lowByte(status);
                varSet = false;                
              }
            } 
            varSet = true;
          }
          if (waitForSerial(timeOut_)){
            while (varSet) {
              int status = Serial.read();
              if (status == 40){
                //Serial.write(1); 
                Serial.write( byte(40)); //send keep alive
                Serial.write( PINC);
              }
              else {
                highByte = status;
                varSet = false;
              }
            } 
            varSet = true;
          }
          if (waitForSerial(timeOut_)){
            while (varSet) {
              int status = Serial.read();
              if (status == 40){
                //Serial.write(2); 
                Serial.write( byte(40)); //send keep alive
                Serial.write( PINC);
              }
              else {
                lowByte = status;
                varSet = false;
              }
            } 
            varSet = true; 
          }
          highByte = highByte << 8;
          frameRate = highByte | lowByte; // everything from 1ms to 60001ms 
          if (frameRate == 60001){
            frameRate = 40;
          }
          if(waitForSerial(timeOut_)){
            while (varSet) {
              int status = Serial.read();
              if (status ==40){
                //Serial.write(3); 
                Serial.write( byte(40)); //send keep alive
                Serial.write( PINC);
              }
              else {
                Exposure = status;
                if (Exposure == 251){
                  Exposure = 40;
                }
                varSet = false;
              }
            } 
          }
          unsigned long previousMillis = millis();
          bool goOn = true;
          while (goOn) {//TODO check out Switch state pin belegung
            if (Serial.available() !=0){
              int status = Serial.read();
              if (status == 40){
                //Serial.write(5); //test where it dies
                Serial.write( byte(40)); //send keep alive
                Serial.write( PINC);
              }
              if (status == 66) {
                  goOn = false;
              }
            }
            if (millis() - previousMillis > frameRate) {
             previousMillis = millis();
             //PORTB = 1;
             //delay(5);
              PORTB = pinassignment;
              //digitalWrite(LED_BUILTIN, HIGH);
              unsigned long ExposingMillis = micros();                    
              //Serial.write("PIN ON"); //FOR TESTING erstmal Pin 13 = ARDUINO LED                 
              bool Exposing = true;
              while (Exposing) {//TODO check out Switch state pin belegung
                if (Serial.available() !=0){
                  int status = Serial.read();
                  if (status == 40){
                    //Serial.write(7);                     
                    Serial.write( byte(40)); //send keep alive
                    Serial.write( PINC);
                  }
                  if (status == 66) {
                  goOn = false;
                  }
                }
                if (micros() - ExposingMillis > (unsigned long)(Exposure * 1000)) {
                  PORTB = (pinassignment -1);
                  Exposing = false;
                  //Serial.write("PIN OFF"); 
                }   
              }              
            }
          }
          PORTB = 0;
          break;
       }


 
       case 53:
         Serial.write( byte(2));
         Serial.write( PORTB);
         break; 
       }
    }
    
    // In trigger mode, we will blank even if blanking is not on..
    if (triggerMode_) {
      boolean tmp = PIND & inPinBit_;
      if (tmp != triggerState_) {
        if (blankOnHigh_ && tmp ) {
          PORTB = 0;
        }
        else if (!blankOnHigh_ && !tmp ) {
          PORTB = 0;
        }
        else { 
          if (triggerNr_ >=0) {
            PORTB = triggerPattern_[sequenceNr_];
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
        if (! (PIND & inPinBit_))
          PORTB = currentPattern_;
        else
          PORTB = 0;
      } else {
        if (! (PIND & inPinBit_))
          PORTB = 0;
        else  
          PORTB = currentPattern_;
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
 