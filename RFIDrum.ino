//**************************************************************//
//  Name    : shiftOutCode, Hello World                                
//  Author  : Carlyn Maw,Tom Igoe, David A. Mellis 
//  Date    : 25 Oct, 2006    
//  Modified: 23 Mar 2010                                 
//  Version : 2.0                                             
//  Notes   : Code for using a 74HC595 Shift Register           //
//          : to count from 0 to 255                           
//****************************************************************

// Controls a StrongLink SL018 or SL030 RFID reader by I2C
// Arduino to SL018/SL030 wiring:
// A3/TAG     1      5
// A4/SDA     2      3
// A5/SCL     3      4
// 5V         4      -
// GND        5      6
// 3V3        -      1
 
// We'll read an ID something like "AD93E24B" from an RFID card and use it to fire off
// some solenoids

#include <Wire.h>
#include <SL018.h>

SL018 rfid;

//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;

// Which bits in the latched driver chip apply to which solenoid
const int kApplauseSolenoid = 4;
const int kExtraSolenoidA = 1;
const int kExtraSolenoidB = 2;
const int kExtraSolenoidC = 128;
// Maximum value any of applause
const int kApplauseClippingVal = 20;
// Maximum value s1-3 will have
const int kExtrasClippingVal = 10;

// Number of beats in a bar
const int kPatternSize = 4;

// Number of milliseconds that the solenoids should be energised
// for each tap
#define SOLENOID_FIRING_TIME  20
// Buffer to hold a single command line sent from the computer
// 1-20 1-20 1-20 1-20 => applause, solenoid 1, solenoid 2, solenoid 3
char commandString[15];
int commandLen = 0;

void setup() {
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
  
  Serial.println("RFIDrum starting");
  Serial.println();
}

void loop() {

  // Look for an RFID tag
  Serial.println("Looking for RFID tag...");
  rfid.seekTag();
  while(!rfid.available())
  {
    delay(50);
  }
  
  // We've found a tag, read its ID
  rfid.led(true);
  Serial.print("Found a ");
  Serial.print(rfid.getTagName());
  Serial.println(" card");
  Serial.print("ID: ");
  Serial.println(rfid.getTagString());
  processTag(rfid.getTagNumber(), rfid.getTagLength());
  rfid.led(false);
  Serial.println();
}

void playBeat(byte aBeatPattern, int aBPM)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, aBeatPattern);
  digitalWrite(latchPin, HIGH);
      
  delay(SOLENOID_FIRING_TIME);
          
  // Then pull the solenoid away
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
  digitalWrite(latchPin, HIGH);

  // Now wait for the BPM time
  unsigned long bpmDelay = 30UL * 1000UL / (unsigned long)aBPM;
  delay(bpmDelay - SOLENOID_FIRING_TIME);
}

void playPattern(const byte* aPattern, int aPatternLen, int aBPM)
{
  for (int i =0; i < aPatternLen; i++)
  {
    playBeat(aPattern[i], aBPM);
  }
}

void printPattern(const char* aPatternDesc, const byte* aPattern, int aPatternLen)
{
  Serial.print(aPatternDesc);
  Serial.print(", hex: ");
  for (int i =0; i < aPatternLen; i++)
  {
    Serial.print(aPattern[i], HEX);
  }
  Serial.print(" binary: ");
  for (int i =0; i < aPatternLen; i++)
  {
    Serial.print(aPattern[i], BIN);
  }
  Serial.println();
}

void processTag(const byte* aTagNumber, int aTagLength)
{  
  int completeStart;
  int phaseStart;
  int phaseEnd;

  Serial.println();
  Serial.println("Processing tag...");
  
  // Use the ID as the seed for the random number generator so we get a unique (and
  // repeatable) sequence for any particular card
  unsigned int seed =0;
  for (int i =0; i < aTagLength; i+=sizeof(seed))
  {
    // Cast the ID to a uint so we use the full size that randomSeed takes
    seed += *((unsigned int*)&aTagNumber[i]);
  }
  // Cope with a trailing byte in the ID
  if (aTagLength % sizeof(seed))
  {
    // This /almost/ copes with unsigned int being 4 bytes rather than two, but this
    // bit doesn't, as it only adds the last byte, whereas there could be up to three
    // extra bytes if unsigned int is 32 bits rather than 16.
    seed += aTagNumber[aTagLength-1];
  }
  randomSeed(seed);
  
  // Work out how fast we should play the rhythm
  int bpm = 110 + random(40);
  // We'll play a series of 4/4 bars, three different beat patterns, in an A-B-C-B-C
  // sequence.  We support up to 8 solenoids, so each beat will take
  // a byte of info, and therefore a bar will be 4 bytes
  byte patternA[kPatternSize];
  byte patternB[kPatternSize];
  byte patternC[kPatternSize];

  for (int i = 0; i < kPatternSize; i++)
  {
    patternA[i] = random(256);
    patternB[i] = random(256);
    patternC[i] = random(256);
  }
  // Show our workings
  Serial.print("BPM: ");
  Serial.println(bpm);
  printPattern("Pattern A", patternA, kPatternSize);
  printPattern("Pattern B", patternB, kPatternSize);
  printPattern("Pattern C", patternC, kPatternSize);
  
  // play patternA 1-4 times
  for (int i = 0; i < 1+random(3); i++)
  {
    Serial.print("A");
    playPattern(patternA, kPatternSize, bpm);
  }
  Serial.println();
  // then patternB a few times
  for (int i = 0; i < 3+random(5); i++)
  {
    Serial.print("B");
    playPattern(patternB, kPatternSize, bpm);
  }
  Serial.println();  
  // patternC 2-4 times
  for (int i = 0; i < 2+random(2); i++)
  {
    Serial.print("C");
    playPattern(patternC, kPatternSize, bpm);
  }
  Serial.println();
#if 0
  // patternB another 0-4 times
  for (int i = 0; i < random(4); i++)
  {
    Serial.print("B");
    playPattern(patternB, kPatternSize, bpm);
  }
  Serial.println();
  // finally patternC again 0-5 times
  for (int i = 0; i < random(5); i++)
  {
    Serial.print("C");
    playPattern(patternC, kPatternSize, bpm);
  }
  Serial.println();
#endif
  
#if 0
  // Wait for a keypress
  if (Serial.available() > 0)
  {
    commandString[commandLen++] = Serial.read();

    if ( (commandString[commandLen-1] == '\n') || (commandString[commandLen-1] == 's') )
    {
      // We've got a full command
      char* parseChar = commandString;
      // Parse it
      int applause = 0;
      int s1 = 0;
      int s2 = 0;
      int s3 = 0;
      while ( (*parseChar >= '0') && (*parseChar <= '9') )
      {
        applause = applause*10;
        applause += *parseChar - '0';
        parseChar++;
      }
      // skip over the space separator
      parseChar++;
      while ( (*parseChar >= '0') && (*parseChar <= '9') )
      {
        s1 = s1*10;
        s1 += *parseChar - '0';
        parseChar++;
      }
      // skip over the space separator
      parseChar++;
      while ( (*parseChar >= '0') && (*parseChar <= '9') )
      {
        s2 = s2*10;
        s2 += *parseChar - '0';
        parseChar++;
      }
      // skip over the space separator
      parseChar++;
      while ( (*parseChar >= '0') && (*parseChar <= '9') )
      {
        s3 = s3*10;
        s3 += *parseChar - '0';
        parseChar++;
      }
      
      // We've got some applause to play
      Serial.print("applause: ");
      Serial.print(applause);
      Serial.print(" s1: ");
      Serial.print(s1);
      Serial.print(" s2: ");
      Serial.print(s2);
      Serial.print(" s3: ");
      Serial.println(s3);

      // Clip applause
      applause = min(applause, kApplauseClippingVal);
    
      completeStart = millis();
      phaseStart = completeStart;
    
      // Attack
      int d;
      // Array for which solenoids to fire
      char solenoids;
      for (d = 100+(5*applause); d > (30 + (20-applause)); d -= applause)
      {
        digitalWrite(latchPin, LOW);
        solenoids = kApplauseSolenoid;
        if (random(kExtrasClippingVal) < s1) solenoids |= kExtraSolenoidA;
        if (random(kExtrasClippingVal) < s2) solenoids |= kExtraSolenoidB;
        if (random(kExtrasClippingVal) < s3) solenoids |= kExtraSolenoidC;
        shiftOut(dataPin, clockPin, MSBFIRST, solenoids);
        digitalWrite(latchPin, HIGH);
      
        delay(SOLENOID_FIRING_TIME);
          
        // Then pull the solenoid away
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, 0);
        digitalWrite(latchPin, HIGH);

        delay(d);
      }
      d = 30+(20 - applause);

      phaseEnd = millis();
      Serial.print("Attack time: ");
      Serial.println(phaseEnd - phaseStart);
      phaseStart = phaseEnd;

      // Sustain    
      for (int i = 0; i < applause*8; i++)
      {
        digitalWrite(latchPin, LOW);
        solenoids = kApplauseSolenoid;
        if (random(kExtrasClippingVal) < s1) solenoids |= kExtraSolenoidA;
        if (random(kExtrasClippingVal) < s2) solenoids |= kExtraSolenoidB;
        if (random(kExtrasClippingVal) < s3) solenoids |= kExtraSolenoidC;
        shiftOut(dataPin, clockPin, MSBFIRST, solenoids);
        digitalWrite(latchPin, HIGH);
      
        delay(SOLENOID_FIRING_TIME);
          
        // Then pull the solenoid away
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, 0);
        digitalWrite(latchPin, HIGH);

        delay(d);
      }

      phaseEnd = millis();
      Serial.print("Sustain time: ");
      Serial.println(phaseEnd - phaseStart);
      phaseStart = phaseEnd;

      // Decay
      int i = 1;
      for (; d < 600; d += (i++*(24-applause)))
      {
        digitalWrite(latchPin, LOW);
        solenoids = kApplauseSolenoid;
        if (random(kExtrasClippingVal) < s1) solenoids |= kExtraSolenoidA;
        if (random(kExtrasClippingVal) < s2) solenoids |= kExtraSolenoidB;
        if (random(kExtrasClippingVal) < s3) solenoids |= kExtraSolenoidC;
        shiftOut(dataPin, clockPin, MSBFIRST, solenoids);
        digitalWrite(latchPin, HIGH);
      
        delay(SOLENOID_FIRING_TIME);
          
        // Then pull the solenoid away
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, 0);
        digitalWrite(latchPin, HIGH);

        delay(d);
      }

      phaseEnd = millis();
      Serial.print("Decay time: ");
      Serial.println(phaseEnd - phaseStart);
      phaseStart = phaseEnd;
      Serial.print("Total time: ");
      Serial.println(phaseEnd - completeStart);

      // Reset ready for the next command
      commandLen = 0;
    }
  }
#endif
}

