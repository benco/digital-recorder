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
//#include <SD.h>
//#include <configMS.h>

// LED connection pin numbers: digital output
const int FileLED = 13;
const int RecordLED = 12;
const int PlayLED = 11;
// Speaker: analog data written as PWM output 
const int SpeakerPin = 10;
// Button pins: digital input, debounced by ISR
const int PlayButton = 7;
const int RecordButton = 6;
const int DeleteButton = 5;
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
<<<<<<< HEAD

int DERP = 0;
=======
bool quitRecord = false;
bool quitPlaying = false;
>>>>>>> a189d2387fa9482aaae03f27d9f4e4a2f9a5d257

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
  DERP = latestButton;
}


// Initializing...
void setup() {
  // Serial setup for debugging statements
  Serial.begin(9600); 
  // LED controls & turn them all off (assume active high)
  pinMode(FileLED, OUTPUT); 
  pinMode(RecordLED, OUTPUT); 
  digitalWrite(FileLED, LOW);
  digitalWrite(RecordLED, LOW);

  // Initialize SD shield
  // while (don't have SD card)
  ///  wait
  ///  blink a light quickly
  // Get file system access
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
      while (isPlaying) {
<<<<<<< HEAD
        // play a little
        isPlaying = !isPressed(PLAY);
        Serial.println("playing...");
=======
        if (quitPlaying) {
          Serial.print("Press again to quit!");
          isPlaying = !isPressed(PLAY); //stays true until released
        }else{
          Serial.print("playing...");
          quitPlaying = isPressed(PLAY); //stays true until pressed
        }
>>>>>>> a189d2387fa9482aaae03f27d9f4e4a2f9a5d257
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
      digitalWrite(RecordLED, LOW); // RecordLED on (to Ben: Shouldn't be HIGHT?)
      isRecording = true;
      quitRecord = false;
      while (isRecording) {
<<<<<<< HEAD
        // record a little (MESHAL)
        isRecording = !isPressed(RECORD);
        Serial.println("recording...");
=======
        if (quitRecord) {
          Serial.print("Press again to quit!");
          isRecording = !isPressed(RECORD); //stays true until pressed
        } else {
        Serial.print("recording...");
        quitRecord = isPressed(RECORD);
        }
>>>>>>> a189d2387fa9482aaae03f27d9f4e4a2f9a5d257
      }
      digitalWrite(RecordLED, LOW); // RecordLED off
      haveFile = true;
      digitalWrite(FileLED, HIGH); // FileLED on
      // create file...?
      // CONTINUE
    } else {
<<<<<<< HEAD
      // Blink angrily (MESHAL)
      Serial.println("WE ALREADY HAVE A FILE");
=======
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
      digitalWrite(FileLED, LOW); //End of blinking
      Serial.print("WE ALREADY HAVE A FILE");
>>>>>>> a189d2387fa9482aaae03f27d9f4e4a2f9a5d257
      // CONTINUE
    }
  } else if (isPressed(DELETE)) {
    if (haveFile) {
      // delete the file somehow
      haveFile = false;
      digitalWrite(FileLED, LOW); // FileLED off
    } else {
<<<<<<< HEAD
      Serial.println("NO FILE TO DELETE");
=======
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
      digitalWrite(FileLED, LOW); //End of blinking
      Serial.print("NO FILE TO DELETE");
>>>>>>> a189d2387fa9482aaae03f27d9f4e4a2f9a5d257
      // CONTINUE
    }
  } else {
    // No buttons were pressed. GREAT!
    Serial.print(DERP);
    Serial.print(BufferArray);
    BufferArray = 0;
    Serial.println("No buttons pressed...");
    // CONTINUE
  }
  //delay(100);
}
