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
 | [3/??/15] Added test signal output code in play block     |
 |                                                           |
 *-----------------------------------------------------------*
 | Todo:                                                     |
 | - Figure out how to access SD card files                  |
 | - Make a recording routine to be called by a timer int    |
 | - Have serial messages to debug stuff                     |
 |                                                           |
 *-----------------------------------------------------------*/
#include <SD.h>
//#include <configMS.h> //what is this??

// LED connection pin numbers: digital output
const int FileLED = 10;
const int RecordLED = 6; //9 works fine
const int PlayLED = 7;
// Speaker: analog data written as PWM output 
//          NEW: configure digital port!
const int APORT[] = {3, 3, 3, 3, 3, 3, 3, 3}; 
const int SpeakerPin = 5;
const int MicrophonePin = A0;
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
bool haveFile = true; // SHOULD START OUT FALSE? 
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

// Table for sine-output tester!!!
int sine[] = { 127, 134, 142, 150, 158, 166, 173, 181, 188, 195,
201, 207, 213, 219, 224, 229, 234, 238, 241, 245,
247, 250, 251, 252, 253, 254, 253, 252, 251, 250,
247, 245, 241, 238, 234, 229, 224, 219, 213, 207,
201, 195, 188, 181, 173, 166, 158, 150, 142, 134,
127, 119, 111, 103, 95, 87, 80, 72, 65, 58,
 52, 46, 40, 34, 29, 24, 19, 15, 12, 8,
  6, 3, 2, 1, 0, 0, 0, 1, 2, 3,
  6, 8, 12, 15, 19, 24, 29, 34, 40, 46,
 52, 58, 65, 72, 80, 87, 95, 103, 111, 119,
127 };


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
	pinMode(4, OUTPUT);
  if (!SD.begin(4)) {
    Serial.println("SD init failed!!!");
  } else {
  	Serial.println("SD init done");
	}
  
  // TESTING THE SD CARD ACCESS
  myFile = SD.open("test.txt", FILE_READ);
  //myFile = SD.open("timemachine.wav", FILE_READ);
  if (myFile) {
    Serial.println("Output of test.txt:");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("error opening file!");
  }
  
	//// SPEAKER SETUP ///////////////////////////////////////////////////
	
  // OLD Setup speaker PWM output for pin 5 to 62500Hz
  TCCR0B = TCCR0B & 0b11111000 | 0x01;
  
	// Setup pins for DIGITAL audio port
	for (int i=0; i<sizeof(APORT); i++) {
		pinMode(APORT[i], OUTPUT);
	}
	digitalWrite(APORT[0], LOW);

  // NEW audio handler using built-in support
	cli(); // disables interrupts
	//set timer2 interrupt at 8kHz
	TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 40khz increments
  OCR2A = 49;// = (16*10^6) / (40000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei();//enable interrupts
  
	//////////////////////////////////////////////////////////////////////

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

volatile unsigned int buffer[256];
byte index = 0;
unsigned int sample = 0x0;

// AUDIO ISR, Runs every 25 us to put a digital sample on
// the digital audio output port.
ISR(TIMER2_COMPA_vect) {
  if (isPlaying) {
    sample = buffer[index];
    index = index + 1;
    digitalWrite(APORT[0], (sample & 0x1));
    digitalWrite(APORT[1], (sample & 0x2));
    digitalWrite(APORT[2], (sample & 0x4));
    digitalWrite(APORT[3], (sample & 0x8));
    digitalWrite(APORT[4], (sample & 0x10));
    digitalWrite(APORT[5], (sample & 0x20));
    digitalWrite(APORT[6], (sample & 0x40));
    digitalWrite(APORT[7], (sample & 0x80));
  } else {
    sample = analogRead(MicrophonePin);
    buffer[index] = sample;
    index = index + 1;
  }
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

// ASUMPTION: b has length of >= n!
long readSample(int n, File f) {
  unsigned long sample = 0;
  if (f.available() >= n) {
    for (int i=0; i<n; i++) {
      Serial.print(((unsigned)f.read()) << (8*i));
      Serial.print("\t");
      sample = sample + (((unsigned)f.read()) << (8*i));
    }
  }
  Serial.print(sample);
  Serial.print("\t");
  return sample;
}

void loop() {
	Serial.print("I HAVE ENTERED THE LOOP!!!!");
  if (isPressed(PLAY)) {
    while (isPressed(PLAY)) {} //Will start playing when button released (Hanger)
    if (haveFile) {
      digitalWrite(PlayLED, HIGH); // PlayLED on
      isPlaying = false;
      quitPlaying = false;
      //Timer1.start();

/*
      byte data = 0;
      myFile = SD.open("timemachine.wav", FILE_READ);
      //myFile = SD.open("sine/20000hz.wav", FILE_READ);
      //myFile = SD.open("krider.wav", FILE_READ);
      //myFile = SD.open("nocomp.wav", FILE_READ);
      //myFile = SD.open("ateam.wav", FILE_READ);
      //myFile = SD.open("test.txt", FILE_READ);
*/
      if (myFile) {
        // print out the header
        for (int i=0; i<44; i++) {
          Serial.print(myFile.read());
        }
        // play the audio, assuming 24-bit
        unsigned long sample = 0;
        
        int count = 0;
        boolean derp = true;
        
        while (myFile.available()) {
         //sample = (readSample(3, myFile) * 255) / 65536;
/*
				 sample = myFile.read();
         //Serial.println(sample);
				 buffer[count] = sample;
				 if (count == 255) isPlaying = true;
				 count = (count + 1)%256;
         //analogWrite(SpeakerPin, sample);
*/
         
         // SINE
         //analogWrite(SpeakerPin, sine[count]);
				 buffer[count] = sine[count];
				 if (count == 100) isPlaying = true;
         count = (count+1)%101;
         
         // SQUARE
         //if (derp) analogWrite(SpeakerPin, 200);
         //else analogWrite(SpeakerPin, 0);
         //derp = !derp;
        }
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
      //Timer1.stop();
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




