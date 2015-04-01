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
 | [3/30/15] Rearranged stuff, buttons read through analog   |
 |           pins, Audio ISR works                           |
 |                                                           |
 *-----------------------------------------------------------*
 | Todo:                                                     |
 | - Read into a big-ass circular AUDIO_BUFFER from the SD card    |
 | - Maybe a structure for the circular AUDIO_BUFFER?              |
 | - Reave the chaff!                                        |
 |                                                           |
 *-----------------------------------------------------------*/
#include <SD.h>
//#include <configMS.h> //what is this??

// Representing button conditions
#define PLAY 0xFFFF
#define RECORD 0xAAAA
#define DELETE 0x5555

// LED connection pin numbers: digital output
#define FileLED 10
#define ActiveLED 9
// Speaker: analog data written as PWM output 
//          NEW: configure digital port!
//          SUPER NEW: WE'RE USING PORTD DOWG
const int APORT[] = {3, 3, 3, 3, 3, 3, 3, 3}; 
const int SpeakerPin = 5;
const int MicrophonePin = A0;
// Button pins: digital input, debounced by ISR
#define PlayButton A0
#define RecordButton A1
#define DeleteButton A2
#define InterruptPin 2  // Triggers ISR for all buttons
// Debouncing array
volatile int ButtonBuffer = 0;
// State variables
bool haveFile = true; // SHOULD START OUT FALSE? (yes, but nice for test)
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
int latestButton = 0;
#define ButtonLow 90
void BUTTON_ISR() {
  //int latestButton = 0;
  // Determine which button pressed, then
  //  get lowest two bits of the associated
  //  32-bit (or however long ints are) condition. 
  if (analogRead(PlayButton)<ButtonLow) latestButton = PLAY & 3; 
  else if (analogRead(RecordButton)<ButtonLow) latestButton = RECORD & 3;
  else if (analogRead(DeleteButton)<ButtonLow) latestButton = DELETE & 3;
  else latestButton = 0;
  // Shift in that binary value
  ButtonBuffer = (ButtonBuffer << 2) | latestButton;
}


// Initializing...
void setup() {
  // Serial setup for debugging statements
  Serial.begin(9600); 
	Serial.println("WHAT THE FUCK!");
  // LED controls & turn them all off (assume active high)
  pinMode(FileLED, OUTPUT); 
  pinMode(ActiveLED, OUTPUT); 
  digitalWrite(FileLED, LOW);
  digitalWrite(ActiveLED, LOW);

	//// SPEAKER SETUP ///////////////////////////////////////////////////
	
  // Audio OUTPUT ISR using Timer2
	cli(); // disables interrupts
	TIMSK2 = 0x0;
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
  //TIMSK2 |= (1 << OCIE2A);
	TIMSK2 = 0x0;
  sei();//enable interrupts
  
  // Audio INPUT ISR using Timer2
	cli(); // disables interrupts
	TIMSK1 = 0x0;
	//set timer1 interrupt at 8kHz
	TCCR1A = 0;// set entire TCCR2A register to 0
  TCCR1B = 0;// same for TCCR2B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 40khz increments
  OCR1A = 49;// = (16*10^6) / (40000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR1A |= (1 << WGM11);
  // Set CS21 bit for 8 prescaler
  TCCR1B |= (1 << CS11);   
  // enable timer compare interrupt
  //TIMSK1 |= (1 << OCIE1A);
	TIMSK1 = 0x0;
  sei();//enable interrupts
	
	//////////////////////////////////////////////////////////////////////

	// Initialize SD shield
	pinMode(8, OUTPUT);
  if (!SD.begin(8)) {
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
  // if (we have "recording.mp3")
  ///  haveFile = true
  ///  FileLED on
  
	// set PORTD as output, except interrupt pin 2
	DDRD = 0xFB;
  
  // Activate button-sensing ISR
  /// When pin 2 goes low, the button ISR
  /// will run and check when button we have
  attachInterrupt(0, BUTTON_ISR, LOW);
  
}

/* TEST OUTPUT OF 40k/256 Hz SINE WAVE ****************** /
volatile unsigned int AUDIO_BUFFER[] = {
128, 131, 134, 137, 141, 144, 147, 150, 153, 156,
159, 162, 165, 168, 171, 174, 177, 180, 183, 186,
189, 191, 194, 197, 199, 202, 205, 207, 209, 212,
214, 217, 219, 221, 223, 225, 227, 229, 231, 233,
235, 236, 238, 240, 241, 243, 244, 245, 246, 248,
249, 250, 251, 252, 252, 253, 254, 254, 255, 255,
255, 256, 256, 256, 256, 256, 256, 256, 255, 255,
254, 254, 253, 253, 252, 251, 250, 249, 248, 247,
246, 245, 243, 242, 240, 239, 237, 236, 234, 232,
230, 228, 226, 224, 222, 220, 218, 215, 213, 211,
208, 206, 203, 201, 198, 195, 193, 190, 187, 184,
181, 179, 176, 173, 170, 167, 164, 161, 158, 155,
152, 148, 145, 142, 139, 136, 133, 130, 126, 123,
120, 117, 114, 111, 108, 104, 101, 98, 95, 92, 
89, 86, 83, 80, 77, 75, 72, 69, 66, 63, 
61, 58, 55, 53, 50, 48, 45, 43, 41, 38, 
36, 34, 32, 30, 28, 26, 24, 22, 20, 19, 
17, 16, 14, 13, 11, 10, 9, 8, 7, 6,
5, 4, 3, 3, 2, 2, 1, 1, 0, 0,
0, 0, 0, 0, 0, 1, 1, 1, 2, 2,
3, 4, 4, 5, 6, 7, 8, 10, 11, 12, 
13, 15, 16, 18, 20, 21, 23, 25, 27, 29, 
31, 33, 35, 37, 39, 42, 44, 47, 49, 51, 
54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 
82, 85, 88, 91, 94, 97, 100, 103, 106, 109,
112, 115, 119, 122, 125, 128  
};
byte INDEX_R = 0;
/ ********************************************************/

// B U F F E R /////////////////////////////////////////////////////////

// Arduino has 2k SRAM for buffer! So use 1k, or 2x10
#define AudioBufferSize 512
unsigned char AUDIO_BUFFER[AudioBufferSize]; // Is malloc better? Maybe not
unsigned int INDEX_W;
bool LOOP_W;
unsigned int INDEX_R;
bool LOOP_R;
unsigned int sample = 0x0;

void AB_reset() {
	INDEX_W = 0;
	LOOP_W = false;
	INDEX_R = 0;
	LOOP_R = false;
}

int AB_size() {
	if (LOOP_W == LOOP_R && INDEX_W >= INDEX_R)  {
		return INDEX_W - INDEX_R;
	} else if (LOOP_W != LOOP_R && INDEX_W <= INDEX_R) {
		return INDEX_W + (AudioBufferSize - INDEX_R); 
	} else {
		return -1;
	}
}

int AB_canWrite() {
	int size = AB_size();
	return (size >= 0) ? AudioBufferSize - size : 0;
}
	
// Write n bytes from File into AudioBuffer
int AB_writeFrom(File f, int n) {
	if (n < AB_canWrite()) return -1;
	if ((INDEX_W + n) >= AudioBufferSize) {
		int r = AudioBufferSize - INDEX_W;
		f.readBytes((char *)&AUDIO_BUFFER+INDEX_W, r);
		INDEX_W = 0;
		LOOP_W = !LOOP_W;
		n = n - r;
	}
	f.readBytes((char *)&AUDIO_BUFFER+INDEX_W, n);
	INDEX_W = INDEX_W + n;
	return 0;
}

int AB_canRead() {
	int size = AB_size();
	return (size >= 0) ? size : 0;
}

// Read n bytes out to File from AudioBuffer
int AB_readTo(File f, int n) {
	if (n < AB_canRead()) return -1;
	if ((INDEX_R + n) >= AudioBufferSize) {
		int r = AudioBufferSize - INDEX_R;
		f.write((char *)&AUDIO_BUFFER+INDEX_R, r);
		INDEX_R = 0;
		LOOP_R = !LOOP_R;
		n = n - r;
	}
	f.write((char *)&AUDIO_BUFFER+INDEX_R, n);
	INDEX_R = INDEX_R + n;
	return 0;
}

////////////////////////////////////////////////////////////////////////

// AUDIO OUTPUT ISR, Runs every 25 us to put a digital sample on
// the digital audio output port.
ISR(TIMER2_COMPA_vect) {
  PORTD &= 0x04;
  PORTD |= (AUDIO_BUFFER[INDEX_R]&0xFB);
  INDEX_R = (INDEX_R + 1) % AudioBufferSize;
}

// AUDIO INPUT ISR, Runs every 25 us to put a digital sample on
// the digital audio output port.
ISR(TIMER1_COMPA_vect) {
  AUDIO_BUFFER[INDEX_W] = analogRead(MicrophonePin);
  INDEX_W = (INDEX_W + 1) % AudioBufferSize;
}

// We check if the button was pressed by
//  seeing if the whole AUDIO_BUFFER if full of
//  that button's 2-bit number.
bool isPressed(int button) {
  if ((ButtonBuffer & 0xFF00) == (button & 0xFF00)) {
    ButtonBuffer = 0;
    return true;
  } else {
		if (ButtonBuffer != 0) Serial.println(ButtonBuffer);
    return false;
  }
}

// ASUMPTION: b has length of >= n!
long readWavSample(int n, File f) {
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

void angryBlink(int pin, int num_blinks, bool end_state) {
	for (int i=0; i<(2*num_blinks); i++) {
		end_state = !end_state;
		digitalWrite(pin, end_state);
		delay(300);
	}
}


// PLAY:
// State Variables 
#define BUF_INIT 0x0
#define BUF_PLAY 0x1
#define RE_BUFFR 0x2
#define PLY_ONLY 0x3
unsigned char play_state = 0;
// Buffer Parameters
#define prefAmt 64
#define lenInit 512
#define lenMinB 128

void loop() {

	// P L A Y /////////////////////////////////////////////////////////////
  if (isPressed(PLAY)) {
    while (isPressed(PLAY)) {} //Will start playing when button released (Hanger)
		myFile = SD.open("timemachine.wav", FILE_READ);
    if (myFile && haveFile) {
      digitalWrite(ActiveLED, HIGH); // PlayLED on
      quitPlaying = false;
			INDEX_W;
			INDEX_R;

			while (!quitPlaying) {
				switch (play_state) {
					case BUF_INIT:
						if (AB_size() < lenInit && myFile.available() > 0) {
							AB_writeFrom(myFile, prefAmt);
						} else {
							play_state = BUF_PLAY;
  						TIMSK2 |= (1 << OCIE2A); // Start output
							isPlaying = true;
						}
						break;
					case BUF_PLAY:
						if (AB_size() > 0 && myFile.available() > 0) {
							AB_writeFrom(myFile, prefAmt);
						} else if (myFile.available() > 0) {
							play_state = RE_BUFFR;
							TIMSK2 = 0; // Stop output
							isPlaying = false;
						} else {
							play_state = PLY_ONLY;
						}
						break;
					case RE_BUFFR:
						if (AB_size() < lenMinB && myFile.available() > 0) {
							AB_writeFrom(myFile, prefAmt);
						} else {
							play_state = BUF_PLAY;
  						TIMSK2 |= (1 << OCIE2A); // Start output
							isPlaying = true;
						}
						break;
					case PLY_ONLY:
						if (AB_size() <= 0) {
							TIMSK2 = 0; // Stop output
							isPlaying = false;
							quitPlaying = true;
						}
						break;
				}
				// Polling the play button
        if (quitPlaying) {
          Serial.print("Press again to quit!");
          isPlaying = !isPressed(PLAY); //stays true until released
        }else{
          Serial.print("playing...");
          quitPlaying = isPressed(PLAY); //stays true until pressed
        }
			}
					
			TIMSK2 = 0x0; //Timer2.stop();
      digitalWrite(ActiveLED, LOW); // PlayLED off

    } else {
			angryBlink(ActiveLED, 3, LOW); // User error: nothing to play!
      Serial.print("NO FILE TO PLAY");
      // CONTINUE
    }
  } 

	// R E C O R D /////////////////////////////////////////////////////////
	else if (isPressed(RECORD)) {
    while (isPressed(RECORD)) {} //Will start recodring when button is released
    if (!haveFile) {
      digitalWrite(ActiveLED, HIGH); // RecordLED on 
      isRecording = true;
      quitRecord = false;

      //Timer2.start();
  		TIMSK2 |= (1 << OCIE2A);

      while (isRecording) {

				// Writing to SD card from AUDIO_BUFFER! <<< !!!?

        if (quitRecord) {
          Serial.print("Press again to quit!");
          isRecording = !isPressed(RECORD); //stays true until pressed
        } else {
        Serial.print("recording...");
        quitRecord = isPressed(RECORD);
        }
      }

      //Timer2.stop();
  		TIMSK2 = 0;

      digitalWrite(ActiveLED, LOW); // RecordLED off
      haveFile = true;
      digitalWrite(FileLED, HIGH); // FileLED on
      // create file...? <<< !!!??
      // CONTINUE

    } else {
			angryBlink(FileLED, 3, HIGH); // User error: already have file!
      Serial.print("WE ALREADY HAVE A FILE");
      // CONTINUE
    }
  } 

	// D E L E T E /////////////////////////////////////////////////////////
	else if (isPressed(DELETE)) {
    if (haveFile) {
      // delete the file somehow <<< !!!
      haveFile = false;
      digitalWrite(FileLED, LOW); // FileLED off

    } else {
			angryBlink(FileLED, 3, LOW); // User error: nothing to delete!
      Serial.print("NO FILE TO DELETE");
      // CONTINUE
    }

	// n o t h i n g . . . /////////////////////////////////////////////////
  } else {
    // No buttons were pressed. GREAT!
    ButtonBuffer = 0;
    // CONTINUE
  }
}




