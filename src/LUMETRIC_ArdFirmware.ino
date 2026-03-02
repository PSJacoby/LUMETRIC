/*
 * LUMETRIC_ArdFirmware.ino
 * Based on AOTFcontroller.ino (Arduino Giga R1 version).
 *
 * Additions for LUMETRIC acquisition management (cases 60, 61, 62):
 *
 * Load acquisition table: 60
 *   [60] [N]
 *   Then N rows; every field is sent as exactly 2 bytes via the encodeU16
 *   scheme (see readUint16_keepAlive for the full sentinel table).
 *   All values are capped at 60000; max real hi byte = 234 (0xEA), leaving
 *   sentinel values 235-239 permanently free (none equal 40).
 *   Row layout (16 bytes per row):
 *     [pinPattern:2]     PORTJ bit pattern  (bit0=camera trig, bits1-5=LEDs; hi always 0)
 *     [exposureMs:2]     exposure time ms, 0-60000
 *     [intervalMs:2]     frame interval ms, 0-60000 (must be >= exposure)
 *     [frames:2]         frames to capture, 0-60000 (0 = infinite until switch/quit)
 *     [loopGoto:2]       1-based row to jump to after frames (0 = advance); hi always 0
 *     [loopTimes:2]      times to execute loopGoto, 0-60000 (0 = infinite)
 *     [constIllum:2]     PORTJ pattern during gap (no camera bit; 0 = off); hi always 0
 *     [loopSwitchGoto:2] 1-based row to jump to on switch command (0 = none); hi always 0
 *   Reply: [60] [N]
 *
 * Run acquisition: 61
 *   [61] - starts acquisition from the first row of the table (= row 1 in the GUI).
 *   Internally the firmware counts rows from 0, but all values you send
 *   (loopGoto, loopSwitchGoto) use the same 1-based numbering as the GUI.
 *   During run, the following single bytes are recognised at any point:
 *     40 -> keep-alive (reply [40][pinStates])
 *     66 -> quit (stop acquisition immediately)
 *     67 -> switch (jump to loopSwitchGoto of the currently active row)
 *   Reply when finished or quit: [61] [totalFramesHi] [totalFramesLo]
 *   (For a single frame, load a 1-row table via case 60 first.)
 *
 * -----------------------------------------------------------------------
 * Original AOTFcontroller interface (unchanged):
 *
 * Set digital output: 1p
 * Get digital output: 2
 * Set Analogue output: 3xvv
 * Get Analogue output: 4
 * Set digital pattern for trigger mode: 5xd
 * Set number of patterns: 6x
 * Skip trigger: 7x
 * Start trigger mode: 8
 * Stop trigger mode: 9
 * Set time interval for timed trigger mode: 10xtt
 * Set repeat count: 11x
 * Start timed trigger mode: 12
 * Start blanking mode: 20
 * Stop blanking mode: 21
 * Blanking polarity: 22x
 * Get identification: 30
 * Get version: 31
 * Get max patterns: 32
 * Fast sequence upload: 33
 * Get DA channel count: 34
 * Get digital pin count: 35
 * Read digital analogue inputs: 40
 * Read analogue input: 41x
 * Set analogue pin: 42
 */

   unsigned int version_ = 5;

// If you have one of these DA chips attached, uncomment the appropriate define
// #define TLV5618
// #define TLV56x8

  #if defined TLV5618
  const uint8_t numDAChannels_ = 2;
  #elif defined TLV56x8
  const uint8_t numDAChannels_ = 4;
  #else
  const uint8_t numDAChannels_ = 0;
  #endif

  const uint8_t numDigitalPins_ = 6;

   // pin on which to receive the trigger
   int inPin_ = 2;
   int inPinBit_ = 1 << inPin_;

   // DA chip SPI pins
   int dataPin  = 3;
   int clockPin = 4;
   #ifdef TLV5618
   int latchPin = 5;
   #endif
   #ifdef TLV56x8
   int CS1 = 5;
   int CS2 = 6;
   #endif

   // Original trigger-mode sequence table
   const uint16_t SEQUENCELENGTH = 256;
   byte         triggerPattern_[SEQUENCELENGTH];
   unsigned int triggerDelay_[SEQUENCELENGTH];
   int          patternLength_ = 0;
   byte         repeatPattern_ = 0;
   volatile long triggerNr_;
   volatile long sequenceNr_;
   int          skipTriggers_ = 0;
   byte         currentPattern_ = 0;
   const unsigned long timeOut_ = 1000;
   bool blanking_    = false;
   bool blankOnHigh_ = false;
   bool triggerMode_ = false;
   boolean triggerState_ = false;

// ============================================================
//  LUMETRIC acquisition table
// ============================================================

#define MAX_ACQ_ROWS 32

struct AcqRow {
    byte     pinPattern;     // PORTJ bit pattern (bit0=camera trig, bit1-5=LEDs)
    uint16_t exposureMs;     // LED + trigger on-time (ms)
    uint16_t intervalMs;     // full frame period including exposure (ms)
    uint16_t frames;         // frames to capture (0 = infinite until switch/quit)
    uint8_t  loopGoto;       // GUI row number to jump to after frames (1 = first row, 0 = advance to next)
    uint16_t loopTimes;      // times to execute loopGoto before advancing (0 = infinite)
    byte     constIllum;     // PORTJ pattern held during inter-frame gap (no camera bit; 0 = all off)
    uint8_t  loopSwitchGoto; // 1-based row to jump to on switch command (0 = no switch)
};

AcqRow acqTable_[MAX_ACQ_ROWS];
int    acqRowCount_ = 0;

// Input pin wired to camera's trigger-out (HIGH = shutter open / camera exposing).
// Full pinPattern LEDs are only fired once this pin goes HIGH (shutter-gating).
// constIllum LEDs fire immediately with the camera trigger and persist through the gap.
#define CAM_FEEDBACK_PIN 8

// Uncomment the line below to skip the camera feedback wait entirely (use when
// the feedback wire is not connected — LEDs fire immediately after the trigger pulse).
// When commented out (feedback active): J0 fires first; LEDs turn on only once the
// camera confirms it is integrating (FIRE/TriggerOut HIGH on CAM_FEEDBACK_PIN).
// This ensures the LED pulse exactly matches confirmed sensor integration time,
// eliminating any latency between J0↑ and actual sensor-open.
// Max wait for FIRE HIGH is CAM_FEEDBACK_TIMEOUT_US (abort frame if exceeded).
// #define CAM_FEEDBACK_BYPASS

#define CAM_FEEDBACK_TIMEOUT_US 50000UL  // 50 ms max wait for FIRE signal

// ============================================================
//  Setup
// ============================================================

void setup() {
   Serial.begin(57600);

   pinMode(inPin_, INPUT);
   pinMode(dataPin,  OUTPUT);
   pinMode(clockPin, OUTPUT);
   #ifdef TLV5618
   pinMode(latchPin, OUTPUT);
   #endif
   #ifdef TLV56x8
   pinMode(CS1, OUTPUT);
   pinMode(CS2, OUTPUT);
   #endif

   // Camera feedback input: HIGH when camera shutter is open.
   // Must use INPUT_PULLDOWN so the pin reads LOW (not floating) when
   // the camera trigger-out wire is absent — otherwise the shutter-gate
   // wait loop exits immediately on a floating-HIGH pin, or never on a
   // floating-LOW pin. Wire the camera's FIRE/TriggerOut to this pin.
   pinMode(CAM_FEEDBACK_PIN, INPUT_PULLDOWN);

   // Arduino Giga R1 – Port J outputs (D25, D27, D29, D31, D33, D35 = PJ0-PJ5)
   pinMode(25, OUTPUT);
   pinMode(27, OUTPUT);
   pinMode(29, OUTPUT);
   pinMode(31, OUTPUT);
   pinMode(33, OUTPUT);
   pinMode(35, OUTPUT);

   #ifdef TLV5618
   digitalWrite(latchPin, HIGH);
   #endif
   #ifdef TLV56x8
   digitalWrite(CS1, HIGH);
   digitalWrite(CS2, HIGH);
   #endif

   for (uint16_t i = 0; i < SEQUENCELENGTH; i++) {
      triggerPattern_[i] = 0;
      triggerDelay_[i]   = 0;
   }
}

// ============================================================
//  Port J helpers (Arduino Giga R1)
// ============================================================

// Write 6-bit pattern to GPIOJ (PJ0-PJ5 = D25,D27,D29,D31,D33,D35)
void setPortJ(byte pattern) {
   GPIOJ->ODR = (GPIOJ->ODR & ~0x3F) | (pattern & 0x3F);
}

byte getPortJ() {
   return (byte)(GPIOJ->ODR & 0x3F);
}

// Read digital state of analogue input pins A0-A5 as a 6-bit byte
byte getAnalogPinStates() {
   byte s = 0;
   for (int i = 0; i < 6; i++) {
      if (digitalRead(A0 + i)) s |= (1 << i);
   }
   return s;
}

// ============================================================
//  LUMETRIC helpers
// ============================================================

// Read one data byte, transparently responding to keep-alive byte (40).
// Returns -1 on timeout.
int readByte_keepAlive() {
   // Returns the next raw data byte, transparently handling MM keep-alive (byte 40).
   // NOTE: 251 is no longer an escape byte — it may appear as legitimate payload.
   while (true) {
      if (!waitForSerial(timeOut_)) return -1;
      int b = Serial.read();
      if (b == 40) {
         // Keep-alive from MM: reply and wait for the real byte
         Serial.write(byte(40));
         Serial.write(getAnalogPinStates());
      } else {
         return b;
      }
   }
}

// Read a uint16 value encoded by BeanShell's encodeU16() scheme.
// All fields (both uint16 and uint8) are sent as 2 bytes.
// Hi byte sentinels encode problematic lo bytes (40 or 251) without
// using 40 on the wire (which MM would consume as keep-alive).
// Sentinel values 235-239 are safe: max real hi = 234 (60000 >> 8).
//   normal :  sent=[hi][lo]          -> (hi<<8)|lo
//   235    :  sent=[235][lo]         -> (40<<8)|lo          (real hi was 40)
//   236    :  sent=[236][hi]         -> (hi<<8)|40          (real lo is 40)
//   237    :  sent=[237][hi]         -> (hi<<8)|251         (real lo is 251)
//   238    :  sent=[238][0]          -> (40<<8)|40
//   239    :  sent=[239][0]          -> (40<<8)|251
// Returns -1 on timeout.
int32_t readUint16_keepAlive() {
   int hi = readByte_keepAlive(); if (hi < 0) return -1;
   int lo = readByte_keepAlive(); if (lo < 0) return -1;
   switch (hi) {
      case 235: return (40  << 8) | lo;   // real hi=40,  lo=lo
      case 236: return (lo  << 8) | 40;   // real hi=lo,  lo=40
      case 237: return (lo  << 8) | 251;  // real hi=lo,  lo=251
      case 238: return (40  << 8) | 40;   // real hi=40,  lo=40
      case 239: return (40  << 8) | 251;  // real hi=40,  lo=251
      default:  return ((uint16_t)hi << 8) | (uint8_t)lo;
   }
}

// Check serial port during an active acquisition frame.
// Handles: 40=keep-alive, 66=quit, 67=switch.
void checkSerial_acq(bool &running, bool &switchReq) {
   while (Serial.available() > 0) {
      int b = Serial.read();
      if (b == 40) {
         Serial.write(byte(40));
         Serial.write(getAnalogPinStates());
      } else if (b == 66) {
         running = false;
      } else if (b == 67) {
         switchReq = true;
      }
      // other bytes ignored during acquisition
   }
}

// Fire one frame:
//   If exposureMs > 0 (normal imaging), two modes depending on CAM_FEEDBACK_BYPASS:
//
//   BYPASS (no feedback wire):
//     1. Fire J0 + full LED pattern simultaneously
//     2. Hold for exposureMs (µs resolution)
//     3. Drop J0 + LEDs → constIllum
//
//   FEEDBACK active (OUTPUT TRIGGER wired to CAM_FEEDBACK_PIN):
//     1. Fire J0 only (no LEDs) — triggers camera global reset
//     2. Busy-wait for CAM_FEEDBACK_PIN HIGH (FIRE/TriggerOut = camera confirmed integrating)
//        with 50ms timeout (fires LEDs anyway on timeout so frame isn't skipped)
//     3. Fire full LED pattern
//     4. Hold LEDs for exposureMs (µs resolution from LED-on moment)
//     5. Drop J0 + LEDs → constIllum
//
//   If exposureMs == 0 (photoswitching / dark row):
//     Apply constIllum only — no camera trigger, no shutter wait
//   Both: wait until intervalMs has elapsed from frame start, then return

void doExposure(byte pinPattern, uint16_t exposureMs, byte constIllum, uint16_t intervalMs,
                bool &running, bool &switchReq) {

   // Use micros() throughout — DWT cycle counter on Giga R1 (480 MHz), ~2 ns resolution.
   unsigned long frameStartUs = micros();
   unsigned long intervalUs   = (unsigned long)intervalMs * 1000UL;

   if (exposureMs > 0) {
      unsigned long exposureUs  = (unsigned long)exposureMs * 1000UL;

#ifdef CAM_FEEDBACK_BYPASS
      // No feedback wire: fire J0 + LEDs simultaneously.
      setPortJ(pinPattern | 0x01);

      // Check for quit byte immediately after asserting trigger
      if (Serial.available() > 0) {
         int b = Serial.read();
         if (b == 66) { running = false; }
      }
      if (!running) { setPortJ(0); return; }

      // Busy-wait for exposureMs with µs precision.
      unsigned long expStartUs = micros();
      while ((unsigned long)(micros() - expStartUs) < exposureUs) {
         checkSerial_acq(running, switchReq);
         if (!running) { setPortJ(0); return; }
      }
#else
      // Feedback active: fire J0 trigger only (no LEDs yet), then wait for
      // the camera's FIRE/TriggerOut (CAM_FEEDBACK_PIN) to go HIGH before
      // turning on LEDs. This synchronises the LED pulse to confirmed sensor
      // integration, eliminating J0→sensor latency (~1-2 ms global reset time).
      setPortJ(0x01);  // J0 HIGH, LEDs off

      // Wait for FIRE HIGH with timeout
      unsigned long fireWaitStart = micros();
      while (digitalRead(CAM_FEEDBACK_PIN) == LOW) {
         checkSerial_acq(running, switchReq);
         if (!running) { setPortJ(0); return; }
         if ((unsigned long)(micros() - fireWaitStart) > CAM_FEEDBACK_TIMEOUT_US) {
            // Camera never confirmed ready — fire LEDs anyway so we don't miss the frame
            break;
         }
      }

      // LEDs ON — camera is confirmed integrating (or timeout fired)
      setPortJ(pinPattern | 0x01);

      // Busy-wait for exposureMs from when LEDs turned on.
      unsigned long expStartUs = micros();
      while ((unsigned long)(micros() - expStartUs) < exposureUs) {
         checkSerial_acq(running, switchReq);
         if (!running) { setPortJ(0); return; }
      }
#endif

      // Drop J0 LOW and LEDs off — constIllum must NOT have bit0 set.
      setPortJ(constIllum);

   } else {
      // exposureMs == 0: photoswitching / dark row — apply constIllum, no camera trigger
      setPortJ(constIllum);
   }

   // Wait remainder of interval (µs-timed from frameStartUs), checking serial while spinning.
   while ((unsigned long)(micros() - frameStartUs) < intervalUs) {
      checkSerial_acq(running, switchReq);
      if (!running) { setPortJ(0); return; }
   }
}

// ============================================================
//  Main loop
// ============================================================

void loop() {
   if (Serial.available() > 0) {
      int inByte = Serial.read();

      switch (inByte) {

       // Set digital output
       case 1:
          if (waitForSerial(timeOut_)) {
            currentPattern_ = Serial.read() & B00111111;
            if (!blanking_) setPortJ(currentPattern_);
            Serial.write(byte(1));
          }
          break;

       // Get digital output
       case 2:
          Serial.write(byte(2));
          Serial.write(getPortJ());
          break;

       // Set Analogue output
       case 3:
         if (waitForSerial(timeOut_)) {
           int channel = Serial.read();
           if (waitForSerial(timeOut_)) {
              byte msb = Serial.read() & B00001111;
              if (waitForSerial(timeOut_)) {
                byte lsb = Serial.read();
                analogueOut(channel, msb, lsb);
                Serial.write(byte(3));
                Serial.write(channel);
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
            if (patternNumber >= 0 && patternNumber < (int)SEQUENCELENGTH) {
              if (waitForSerial(timeOut_)) {
                triggerPattern_[patternNumber] = Serial.read() & B00111111;
                Serial.write(byte(5));
                Serial.write(patternNumber);
                Serial.write(triggerPattern_[patternNumber]);
                break;
              }
            }
          }
          Serial.write("n:");
          break;

       // Sets number of digital patterns
       case 6:
         if (waitForSerial(timeOut_)) {
           int pL = Serial.read();
           if (pL >= 0 && pL <= (int)SEQUENCELENGTH) {
             patternLength_ = pL;
             Serial.write(byte(6));
             Serial.write(patternLength_);
           }
         }
         break;

       // Skip triggers
       case 7:
         if (waitForSerial(timeOut_)) {
           skipTriggers_ = Serial.read();
           Serial.write(byte(7));
           Serial.write(skipTriggers_);
         }
         break;

       // Start trigger mode
       case 8:
         if (patternLength_ > 0) {
           sequenceNr_   = 0;
           triggerNr_    = -skipTriggers_;
           triggerState_ = digitalRead(inPin_) == HIGH;
           setPortJ(0);
           Serial.write(byte(8));
           triggerMode_ = true;
         }
         break;

       // Stop trigger mode
       case 9:
          triggerMode_ = false;
          setPortJ(0);
          Serial.write(byte(9));
          Serial.write((byte)triggerNr_);
          break;

       // Set time interval for timed trigger mode
       case 10:
          if (waitForSerial(timeOut_)) {
            int patternNumber = Serial.read();
            if (patternNumber >= 0 && patternNumber < (int)SEQUENCELENGTH) {
              if (waitForSerial(timeOut_)) {
                unsigned int hi = Serial.read();
                unsigned int lo = 0;
                if (waitForSerial(timeOut_)) lo = Serial.read();
                triggerDelay_[patternNumber] = (hi << 8) | lo;
                Serial.write(byte(10));
                Serial.write(patternNumber);
                break;
              }
            }
          }
          break;

       // Set repeat count for timed trigger
       case 11:
         if (waitForSerial(timeOut_)) {
           repeatPattern_ = Serial.read();
           Serial.write(byte(11));
           Serial.write(repeatPattern_);
         }
         break;

       // Start timed trigger mode
       case 12:
         if (patternLength_ > 0) {
           setPortJ(0);
           Serial.write(byte(12));
           for (byte i = 0; i < repeatPattern_ && Serial.available() == 0; i++) {
             for (int j = 0; j < patternLength_ && Serial.available() == 0; j++) {
               setPortJ(triggerPattern_[j]);
               delay(triggerDelay_[j]);
             }
           }
           setPortJ(0);
         }
         break;

       // Start blanking mode
       case 20:
         blanking_ = true;
         Serial.write(byte(20));
         break;

       // Stop blanking mode
       case 21:
         blanking_ = false;
         Serial.write(byte(21));
         break;

       // Blanking polarity
       case 22:
         if (waitForSerial(timeOut_)) {
           int mode = Serial.read();
           blankOnHigh_ = (mode == 0);
         }
         Serial.write(byte(22));
         break;

       // Identification
       case 30:
         Serial.println("MM-Ard");
         break;

       // Version
       case 31:
         Serial.println(version_);
         break;

       // Max pattern count
       case 32:
         Serial.write(byte(32));
         Serial.write(highByte(SEQUENCELENGTH));
         Serial.write(lowByte(SEQUENCELENGTH));
         break;

       // Fast sequence upload
       case 33: {
           unsigned int hi = 0, lo = 0, count = 0;
           if (waitForSerial(timeOut_)) {
             hi = Serial.read();
             if (waitForSerial(timeOut_)) {
               lo = Serial.read();
               uint16_t expectedNum = (hi << 8) | lo;
               if (expectedNum < SEQUENCELENGTH) {
                 while (count < expectedNum && waitForSerial(timeOut_)) {
                   triggerPattern_[count++] = Serial.read() & B00111111;
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

       // DA channel count
       case 34:
         Serial.write(byte(34));
         Serial.write(byte(numDAChannels_));
         break;

       // Digital pin count
       case 35:
         Serial.write(byte(35));
         Serial.write(byte(numDigitalPins_));
         break;

       // Read digital state of analogue input pins
       case 40:
         Serial.write(byte(40));
         Serial.write(getAnalogPinStates());
         break;

       // Read analogue value of one input pin
       case 41:
         if (waitForSerial(timeOut_)) {
           int pin = Serial.read();
           if (pin >= 0 && pin <= 5) {
              int val = analogRead(pin);
              Serial.write(byte(41));
              Serial.write(pin);
              Serial.write(highByte(val));
              Serial.write(lowByte(val));
           }
         }
         break;

       // Set analogue pin state
       case 42:
         if (waitForSerial(timeOut_)) {
           int pin = Serial.read();
           if (waitForSerial(timeOut_)) {
             int state = Serial.read();
             Serial.write(byte(42));
             Serial.write(pin);
             if (state == 0) { digitalWrite(14 + pin, LOW);  Serial.write(byte(0)); }
             if (state == 1) { digitalWrite(14 + pin, HIGH); Serial.write(byte(1)); }
           }
         }
         break;

       // ============================================================
       //  LUMETRIC: Load acquisition table
       //  Protocol: [60] [N]  then N rows × 16 bytes (8 fields × 2 bytes each)
       //  All fields sent via encodeU16() — see readUint16_keepAlive() for scheme.
       //  Row layout: pinPattern(2) expMs(2) intMs(2) frames(2)
       //              loopGoto(2) loopTimes(2) constIllum(2) loopSwitchGoto(2)
       // ============================================================
       case 60: {
         int nRows = readByte_keepAlive();
         if (nRows < 1 || nRows > MAX_ACQ_ROWS) break;

         // All fields are sent as 2 bytes via encodeU16().
         // Single-byte fields (pinPattern, loopGoto, constIllum, lsGoto) always
         // have real hi=0, so we decode with readUint16_keepAlive() and cast to uint8_t.
         bool ok = true;
         for (int r = 0; r < nRows && ok; r++) {
            int32_t v;

            // pinPattern (uint8, sent as uint16 with hi=0)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].pinPattern = (byte)(v & 0xFF);

            // exposureMs (uint16, capped at 60000)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].exposureMs = (uint16_t)v;

            // intervalMs (uint16, capped at 60000)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].intervalMs = (uint16_t)v;

            // frames (uint16, capped at 60000; 0 = infinite)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].frames = (uint16_t)v;

            // loopGoto (uint8, sent as uint16 with hi=0)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].loopGoto = (uint8_t)(v & 0xFF);

            // loopTimes (uint16, capped at 60000; 0 = infinite)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].loopTimes = (uint16_t)v;

            // constIllum (uint8, sent as uint16 with hi=0)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].constIllum = (byte)(v & 0xFF);

            // loopSwitchGoto (uint8, sent as uint16 with hi=0)
            v = readUint16_keepAlive(); if (v < 0) { ok = false; break; }
            acqTable_[r].loopSwitchGoto = (uint8_t)(v & 0xFF);

         }

         if (ok) {
            acqRowCount_ = nRows;
         }
         break;
       }

       // ============================================================
       //  LUMETRIC: Run acquisition
       //  Protocol: [61]
       //  During run: byte 66 = quit, byte 67 = switch
       //  Reply when done/quit: [61] [totalFramesHi] [totalFramesLo]
       // ============================================================
       case 61: {
         if (acqRowCount_ == 0) { break; }

         // Per-row loop counters: how many times we have already jumped back via loopGoto
         uint16_t loopCounters[MAX_ACQ_ROWS];
         for (int i = 0; i < MAX_ACQ_ROWS; i++) loopCounters[i] = 0;

         int      currentRow  = 0;
         bool     running     = true;
         bool     switchReq   = false;
         uint16_t totalFrames = 0;

         //Serial.write(byte(61));  // ACK: acquisition started

         while (running && currentRow >= 0 && currentRow < acqRowCount_) {
            AcqRow &row = acqTable_[currentRow];
            switchReq = false;

            uint16_t framesDoneInRow = 0;

            // ---- inner frame loop for this row ----
            while (running) {
               doExposure(row.pinPattern, row.exposureMs, row.constIllum,
                          row.intervalMs, running, switchReq);
               if (!running) break;

               totalFrames++;
               framesDoneInRow++;

               // Switch command received: jump to loopSwitchGoto
               if (switchReq) {
                  switchReq = false;
                  if (row.loopSwitchGoto > 0 && row.loopSwitchGoto <= acqRowCount_) {
                     currentRow = row.loopSwitchGoto - 1;
                     loopCounters[currentRow] = 0;  // reset loop counter for new row
                  }
                  goto nextRow;  // break inner loop, go to outer loop without further logic
               }

               // Finite frames: check if this row's frame quota is done
               if (row.frames > 0 && framesDoneInRow >= row.frames) break;
               // if row.frames == 0: infinite, only switch/quit can exit
            }

            if (!running) break;

            // ---- row done: handle loopGoto logic ----
            if (row.loopGoto > 0 && row.loopGoto <= (uint8_t)acqRowCount_) {
               // This row loops back somewhere
               loopCounters[currentRow]++;
               bool loopAgain = (row.loopTimes == 0) ||
                                (loopCounters[currentRow] < row.loopTimes);
               if (loopAgain) {
                  currentRow = row.loopGoto - 1;  // jump back (1-based -> 0-based)
               } else {
                  loopCounters[currentRow] = 0;   // reset for future use
                  currentRow++;                   // advance to next row
               }
            } else {
               // No loop, advance to next row
               currentRow++;
            }

            nextRow: ;  // label for switch-command jump
         }

         setPortJ(0);
         break;
       }

       // Load hardcoded test table — use when "Use hardcoded test table" is ticked in BeanShell.
       // pin=0b10001 (bit0=cam trig, bit4=LED), exp=100ms, interval=1000ms, 5 frames then stop.
       case 69: {
         acqTable_[0].pinPattern      = 0x11;  // bit0=camera trigger, bit4=one LED
         acqTable_[0].exposureMs      = 100;
         acqTable_[0].intervalMs      = 1000;
         acqTable_[0].frames          = 5;     // 5 frames then stop (change to 0 for infinite)
         acqTable_[0].loopGoto        = 0;
         acqTable_[0].loopTimes       = 0;
         acqTable_[0].constIllum      = 0;
         acqTable_[0].loopSwitchGoto  = 0;
         acqRowCount_ = 1;
         break;
       }

      } // end switch
   } // end if Serial.available

   // Trigger mode (original): edge-detect on inPin_ drives pattern sequence
   if (triggerMode_) {
      boolean tmp = digitalRead(inPin_);
      if (tmp != triggerState_) {
         if (blankOnHigh_ && tmp) {
            setPortJ(0);
         } else if (!blankOnHigh_ && !tmp) {
            setPortJ(0);
         } else {
            if (triggerNr_ >= 0) {
               setPortJ(triggerPattern_[sequenceNr_]);
               sequenceNr_++;
               if (sequenceNr_ >= patternLength_) sequenceNr_ = 0;
            }
            triggerNr_++;
         }
         triggerState_ = tmp;
      }
   } else if (blanking_) {
      if (blankOnHigh_) {
         setPortJ(!digitalRead(inPin_) ? currentPattern_ : 0);
      } else {
         setPortJ(!digitalRead(inPin_) ? 0 : currentPattern_);
      }
   }
}

// ============================================================
//  Utility
// ============================================================

bool waitForSerial(unsigned long timeOut) {
   unsigned long startTime = millis();
   while (Serial.available() == 0 && (millis() - startTime < timeOut)) {}
   return (Serial.available() > 0);
}

// ============================================================
//  DA chip output (conditional compilation)
// ============================================================

#if defined TLV5618
void analogueOut(int channel, byte msb, byte lsb) {
   digitalWrite(latchPin, LOW);
   msb &= B00001111;
   if (channel == 0) msb |= B10000000;
   shiftOut(dataPin, clockPin, MSBFIRST, msb);
   shiftOut(dataPin, clockPin, MSBFIRST, lsb);
   digitalWrite(clockPin, HIGH);
   digitalWrite(clockPin, LOW);
   digitalWrite(latchPin, HIGH);
}

#elif defined TLV56x8
void analogueOut(int channel, byte msb, byte lsb) {
   msb &= B00001111;
   if      (channel == 0) { digitalWrite(CS1, LOW);  digitalWrite(CS2, HIGH); msb |= B10000000; }
   else if (channel == 1) { digitalWrite(CS1, LOW);  digitalWrite(CS2, HIGH); }
   else if (channel == 2) { digitalWrite(CS1, HIGH); digitalWrite(CS2, LOW);  msb |= B10000000; }
   else if (channel == 3) { digitalWrite(CS1, HIGH); digitalWrite(CS2, LOW);  }
   shiftOut(dataPin, clockPin, MSBFIRST, msb);
   shiftOut(dataPin, clockPin, MSBFIRST, lsb);
   digitalWrite(clockPin, HIGH);
   digitalWrite(clockPin, LOW);
   digitalWrite(CS1, HIGH);
   digitalWrite(CS2, HIGH);
}

#else
void analogueOut(int channel, byte msb, byte lsb) {}  // no-op
#endif
