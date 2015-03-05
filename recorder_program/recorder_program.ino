/*-----------------------------------------------------------*
 |                                                           |
 | Senior Design - ECE 493                       Spring 2015 |
 | Digital Sound Recorder                                    |
 |                                                           |
 *-----------------------------------------------------------*
 | File: recorder_code.ino                                   |
 | Description: Contains code for the control operation and  |
 |              recording on the Arduino Uno microcontroller |
 |                                                           |
 *-----------------------------------------------------------*
 | Changelog:                                                |
 | [2/11/15] Implemented the block diagram on the drive      |
 |                                                           |
 *-----------------------------------------------------------*
 | Todo:                                                     |
 | - Figure out how to access SD card files                  |
 | - Make a recording routine to be called by a timer int    |
 | - Have serial messages to debug stuff                     |
 |                                                           |
 *-----------------------------------------------------------*/
#include <SD.h>
//#include <configMS.h>

// LED connection pin numbers: digital output
const int FileLED = 5;
const int RecordLED = 6; //9 works fine
const int PlayLED = 7;
// Speaker: analog data written as PWM output 
const int SpeakerPin = 3;
// Button pins: digital input, debounced by ISR
const int PlayButton = 9;
const int RecordButton = 8;
const int DeleteButton = 0;
const int InterruptPin = 2;  // Triggers ISR for all buttons
// Debouncing array
volatile int BufferArray = 0;
// Representing button conditions
const int PLAY = 0xFFFF;
const int RECORD = 0xAAAA;
const int DELETE = 0x5555;
// State variables
bool haveFile = true; 
bool isPlaying = false;
bool isRecording = false;
bool quitRecord = false;
bool quitPlaying = false;
// File for reading
File myFile;

// When any button is pressed, 2 go low and this
//  method will continually run until all buttons
//  are released.
// Debounces by shifting the button # into a volatile
//  array, which is checked later.
void buttonISR() {
  int latestButton = 0;
  // Determine which button pressed, then
  //  get lowest two bits of the associated
  //  32-bit (or however long ints are) condition. 
  if (!digitalRead(PlayButton)) latestButton = PLAY & 3; 
  else if (!digitalRead(RecordButton)) latestButton = RECORD & 3;
  else if (!digitalRead(DeleteButton)) latestButton = DELETE & 3;
  else latestButton = 0;
  // Shift in that binary value
  BufferArray = (BufferArray << 2) | latestButton;
}


// Initializing...
void setup() {
  // Serial setup for debugging statements
  Serial.begin(9600); 
  // LED controls & turn them all off (assume active high)
  pinMode(FileLED, OUTPUT); 
  pinMode(RecordLED, OUTPUT); 
  pinMode(PlayLED, OUTPUT);
  digitalWrite(PlayLED, LOW);
  digitalWrite(FileLED, LOW);
  digitalWrite(RecordLED, LOW);
  // Initialize SD shield
  if (!SD.begin(4)) {
    Serial.println("SD init failed!!!");
    return;
  }
  Serial.println("SD init done");
  
  // TESTING THE SD CARD ACCESS
  myFile = SD.open("test.txt", FILE_READ);
  if (myFile) {
    Serial.println("Output of test.txt:");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("error opening file!");
  }
  
  // if (we have "recording.mp3")
  ///  haveFile = true
  ///  FileLED on
  
  // Button inputs
  pinMode(PlayButton, INPUT);
  pinMode(RecordButton, INPUT);
  pinMode(DeleteButton, INPUT);
  pinMode(InterruptPin, INPUT);
  
  // Activate button-sensing ISR
  /// When pin 2 goes low, the button ISR
  /// will run and check when button we have
  attachInterrupt(0, buttonISR, LOW);
  
}

// We check if the button was pressed by
//  seeing if the whole buffer if full of
//  that button's 2-bit number.
bool isPressed(int button) {
  if ((BufferArray & 0xFF00) == (button & 0xFF00)) {
    BufferArray = 0;
    return true;
  } else {
    return false;
  }
}

void loop() {
  if (isPressed(PLAY)) {
    while (isPressed(PLAY)) {} //Will start playing when button released (Hanger)
    if (haveFile) {
      digitalWrite(PlayLED, HIGH); // PlayLED on
      isPlaying = true;
      quitPlaying = false;
      
      byte data = 0;
      myFile = SD.open("knight_rider_access.wav", FILE_READ);
      if (myFile) {
        for (int i=0; i<64; i++) {
          Serial.print(myFile.read());
        }
        Serial.println("");
        delay(10000);
      } else {
        Serial.println("WTFFFF MANNNN");
        delay(1000);
      }
       
      while (isPlaying) {
        
        //data = myFile.read();
        
        if (quitPlaying) {
          Serial.print("Press again to quit!");
          isPlaying = !isPressed(PLAY); //stays true until released
        }else{
          Serial.print("playing...");
          quitPlaying = isPressed(PLAY); //stays true until pressed
        }
      }
      digitalWrite(PlayLED, LOW); // PlayLED off
    } else {
      digitalWrite(PlayLED, HIGH); //Blink angirly here as well to let the user know
      delay(300);                  //that there is no file to play, we could remove it
      digitalWrite(PlayLED, LOW); //if we have time to use LCD
      delay(300);
      digitalWrite(PlayLED, HIGH);
      delay(300);
      digitalWrite(PlayLED, LOW);
      delay(300);
      digitalWrite(PlayLED, HIGH);
      delay(300);
      digitalWrite(PlayLED, LOW); //End of blinking
      
      
      Serial.print("NO FILE TO PLAY");
      // CONTINUE
    }
  } else if (isPressed(RECORD)) {
    while (isPressed(RECORD)) {} //Will start recodring when button is released
    if (!haveFile) {
      digitalWrite(RecordLED, HIGH); // RecordLED on (to Ben: Shouldn't be HIGHT?)
      isRecording = true;
      quitRecord = false;
      while (isRecording) {
        if (quitRecord) {
          Serial.print("Press again to quit!");
          isRecording = !isPressed(RECORD); //stays true until pressed
        } else {
        Serial.print("recording...");
        quitRecord = isPressed(RECORD);
        }
      }
      digitalWrite(RecordLED, LOW); // RecordLED off
      haveFile = true;
      digitalWrite(FileLED, HIGH); // FileLED on
      // create file...?
      // CONTINUE
    } else {
      digitalWrite(FileLED, HIGH); //Blink angirly
      delay(300);                  
      digitalWrite(FileLED, LOW); 
      delay(300);
      digitalWrite(FileLED, HIGH);
      delay(300);
      digitalWrite(FileLED, LOW);
      delay(300);
      digitalWrite(FileLED, HIGH);
      delay(300);
      digitalWrite(FileLED, LOW); 
      delay(300);
      digitalWrite(FileLED, HIGH);//End of blinking
      
      Serial.print("WE ALREADY HAVE A FILE");
      // CONTINUE
    }
  } else if (isPressed(DELETE)) {
    if (haveFile) {
      // delete the file somehow
      haveFile = false;
      digitalWrite(FileLED, LOW); // FileLED off
    } else {
      digitalWrite(FileLED, HIGH); //Blink angirly
      delay(300);                  
      digitalWrite(FileLED, LOW); 
      delay(300);
      digitalWrite(FileLED, HIGH);
      delay(300);
      digitalWrite(FileLED, LOW);
      delay(300);
      digitalWrite(FileLED, HIGH);
      delay(300);
      digitalWrite(FileLED, LOW);  //End of blinking
      
      Serial.print("NO FILE TO DELETE");
      // CONTINUE
    }
  } else {
    // No buttons were pressed. GREAT!
    //Serial.println(BufferArray);
    BufferArray = 0;
    Serial.println("No buttons pressed...");
    // CONTINUE
  }
  //delay(100);
}
