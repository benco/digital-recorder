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
 | - Read into a circular AUDIO_BUFFER from the SD card      |
 | - Maybe a structure for the circular AUDIO_BUFFER?        |
 | - Reave the chaff!                                        |
 |                                                           |
 *-----------------------------------------------------------*/
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <ILI_SdSpi.h>
#include <ILI_SdFatConfig.h>
SdFat SD;




// B U T T O N S ///////////////////////////////////////////////////////

// Button pins: used analog to free PORTD, if DAC output used
#define PlayButton 8
#define RecordButton 7
#define DeleteButton 6
// Debouncing variables/conditions
volatile int ButtonBuffer = 0;
#define PLAY 0xFFFF		// 0b11 (3)
#define RECORD 0xAAAA	// 0b10 (2)
#define DELETE 0x5555	// 0b01 (1)
#define NONE 0			// 0b00 (0)

void initButtons() {
	pinMode(PlayButton, INPUT);
	pinMode(RecordButton, INPUT);
	pinMode(DeleteButton, INPUT);
}

// Debounces by shifting the button # into a volatile
//  array, which is checked later.
void readButtons() {
	int latestButton = 0;
	if (!digitalRead(PlayButton)) latestButton = 3; 
	else if (!digitalRead(RecordButton)) latestButton = 2;
	else if (!digitalRead(DeleteButton)) latestButton = 1;
	// Shift in 2-bit binary value for latest button
	ButtonBuffer = (ButtonBuffer << 2) | latestButton;
}
// We check if the button was pressed by checking the ButtonBuffer
//  NOTE: & conditions w/0xFFFC to trigger when button released!
bool wasPressed(int button) {
	if (ButtonBuffer == (button & 0xFFFC)) { 
		ButtonBuffer = 0; 
		return true; 
	} 
	return false;
}

// Blocking function! Waits for button press and returns result
int waitForButton() {
	while (true) {
		readButtons();
		if (wasPressed(PLAY)) return PLAY;
		else if (wasPressed(RECORD)) return RECORD;
		else if (wasPressed(DELETE)) return DELETE;
	}
}

////////////////////////////////////////////////////////////////////////




// B U F F E R /////////////////////////////////////////////////////////

// Arduino has 2k SRAM available; (using only 1/8 of that)
#define AudioBufferSize 128
unsigned char AUDIO_BUFFER[AudioBufferSize]; 
// 1-bit loop track, 7-bit addr in buffer
unsigned char head = 0;  
unsigned char tail = 0;

// Put sample in cyclical buffer, return success
bool writeBuffer(char sample) { 
	if ((tail ^ head) != 0x80) {
		AUDIO_BUFFER[(head & 0x7F)] = sample;
		head++;
		return true;
	} 
	return false; 
}

// Get sample from buffer and store at pointer, returns success
bool readBuffer(unsigned char *sample) { 
	if ((tail ^ head) != 0x00) {
		*sample = AUDIO_BUFFER[(tail & 0x7F)];
		tail++;
		return true;
	}
	return false; 
}

////////////////////////////////////////////////////////////////////////




// M E M O R Y /////////////////////////////////////////////////////////

#define SD_SPI_SPEED SPI_HALF_SPEED
SdFile myFile;
// Intermediate AUDIO_BUFFER, mitigating SD and Timer disparity
#define MiddleBufferSize 32
unsigned char MIDDLE_BUFFER[MiddleBufferSize]; 
unsigned char msample = 5;	// pass ot cyclical buffer read
int i = 0;					// used for counts
const char wavHeader[] = { 	
	0x52,0x49,0x46,0x46,0xa8,0xeb,0xc2,0x06,
	0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,
	0x12,0x7a,0x00,0x00,0x12,0x7a,0x00,0x00,
	0x01,0x00,0x08,0x00,0x4c,0x49,0x53,0x54,
	0x40,0x00,0x00,0x00,0x49,0x4e,0x46,0x4f,
	0x49,0x4e,0x41,0x4d,0x06,0x00,0x00,0x00,
	0x56,0x6f,0x69,0x63,0x65,0x00,0x49,0x41,
	0x52,0x54,0x18,0x00,0x00,0x00,0x44,0x69,
	0x67,0x69,0x74,0x61,0x6c,0x20,0x52,0x65,
	0x63,0x6f,0x72,0x64,0x65,0x72,0x20,0x47,
	0x72,0x6f,0x75,0x70,0x00,0x00,0x49,0x43,
	0x52,0x44,0x06,0x00,0x00,0x00,0x32,0x30,
	0x31,0x35,0x00,0x00,0x64,0x61,0x74,0x61,
	0x3c,0xeb,0xc2,0x06,0x80,0x80,0x80,0x80,
	0x7f,0x80,0x7f,0x80,0x7f,0x80,0x80,0x7f,
	0x80,0x7f,0x80,0x80,0x7f,0x80,0x80,0x80,
	0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,
	0x7f,0x80,0x7f,0x80,0x7f,0x80,0x80,0x7f,
	0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x80,
	0x7f,0x80,0x7f,0x7f,0x80,0x7f,0x80,0x80,
	0x7f,0x80,0x7f,0x80,0x80,0x7f,0x80,0x7f,
	0x80,0x80,0x80,0x80,0x80,0x7f,0x80,0x7f,
	0x80,0x7f,0x80,0x80,0x7f,0x80,0x7f,0x80
};

// Initialize SdFatLib card via shield
bool initMem() { 
	pinMode(4, OUTPUT);
	bool success = SD.begin(4, SD_SPI_SPEED);
	if (success) Serial.println("SD INIT: success");
	else Serial.println("SD INIT: failed");
	return success;
}

// Open a file on the SD card, returns success
bool openFile(char fn[], bool create) {
	// if creating, only want to create if no file exists
	uint8_t flags = O_RDWR | (create ? O_CREAT | O_EXCL : 0);
	if (!myFile.open(fn, flags)) {
		Serial.println("OPEN FILE: failed");
		return false;
	}
	return true;
}

// Close current file
void closeFile(char fn[]) { myFile.close(); }

// Delete the file!!!
void deleteFile(char fn[]) { myFile.remove(); }

// Load AUDIO_BUFFER half-way! 4 x 16 = 128 / 2
void playPrep() { for (int i=0; i<4; i++) playALittle(); }

// SD card -> MIDDLE_BUFFER, loop MIDDLE_BUFFER -> AUDIO_BUFFER
void playALittle() {
	myFile.read(&MIDDLE_BUFFER, MiddleBufferSize);	// 2436c
	for (i=0; i<MiddleBufferSize; i++) {
		if (!writeBuffer(MIDDLE_BUFFER[i])) {
			myFile.seekCur(i - MiddleBufferSize);
			return;
		}
	}
}

// Create WAV header/format/data begin chunks
void recordPrep() { myFile.write(&wavHeader, 184); }

// loop AUDIO_BUFFER -> MIDDLE_BUFFER, MIDDLE_BUFFER -> SD card
void recordALittle() {
	for (i=0; i<MiddleBufferSize; i++) {
		if (!readBuffer(&msample)) break;
		MIDDLE_BUFFER[i] = msample;
	}
	myFile.write(&MIDDLE_BUFFER, i);
	//myFile.sync();
}

////////////////////////////////////////////////////////////////////////




// A U D I O ///////////////////////////////////////////////////////////

#define SpeakerPin 5
#define MicrophonePin A5
unsigned char sample = 0;	// used for readBuffer
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))

// Set up playing/recording timers 2/1
//  Use compare match registers for 40khz increments
void initAudio() {
	pinMode(SpeakerPin, OUTPUT);
	TCCR0B = TCCR0B & 0b11111000 | 0x01; // set pin5 pwm 62.5k
	cli(); // disables interrupts

	// PLAYING: AUDIO OUTPUT ISR USING TIMER2
	TIMSK2 = 0x0;	// Timer2 off
	TCCR2A = 0;		// set entire TCCR2A register to 0
	TCCR2B = 0;		// same for TCCR2B
	TCNT2  = 0;		//initialize counter value to 0
	//OCR2A = 49;		// = (16*10^6) / (40000*8) - 1 (must be <256)
	//OCR2A = 180;	// = (16*10^6) / (11025*8) - 1 (must be <256)
	//OCR2A = 47;     // works perfectly for 40k sampled files
	OCR2A = 63;
	TCCR2A |= (1 << WGM21);	// turn on CTC mode
	TCCR2B |= (1 << CS21);	// Set CS21 bit for 8 prescaler

	// RECORDING: AUDIO INPUT ISR USING TIMER1
	TIMSK1 = 0x0;	// Timer1 off
	TCCR1A = 0;		// set entire TCCR1A register to 0
	TCCR1B = 0;		// same for TCCR1B
	TCNT1  = 0;		//initialize counter value to 0
	//OCR1A = 44;		// = (16*10^6) / (44100*8) - 1 (must be <256)
	//OCR1A = 49;		// = (16*10^6) / (40000*8) - 1 (must be <256)
	//OCR1A = 180;	// = (16*10^6) / (11025*8) - 1 (must be <256)
	OCR1A = 47;	
	TCCR1B |= (1 << WGM12);	// turn on CTC mode
	TCCR1B |= (1 << CS11);	// Set CS11 bit for 8 prescaler

	// RECORDING: ADC SETTINGS
	sbi(ADCSRA, ADPS2);	// these 3 bit instructions set prescale to 16
	cbi(ADCSRA, ADPS1);	// ADC clock is now 1MHz, max recommended...
	cbi(ADCSRA, ADPS0);
	ADMUX = 0;
	ADMUX |= (1 << REFS0) | (1 << REFS1);	// Use internal ref of 1.1V
	ADMUX |= (MicrophonePin & 0x07); 		// Select ADC input!
	
	sei();//enable interrupts
}

// Starts playing timer! enable timer2 compare interrupt
//void startPlay() { TIMSK2 |= (1 << OCIE2A); }
void startPlay() { 
	TIMSK2 |= (1 << OCIE2A); 
	analogWrite(SpeakerPin, 128);
}

// Stops playing timer!
//void stopPlay() { TIMSK2 = 0x0; }
void stopPlay() { 
	TIMSK2 = 0x0; 
	digitalWrite(SpeakerPin, LOW);
}

// Starts recording timer! enable timer1 compare interrupt
void startRecord() { 
	TIMSK1 |= (1 << OCIE1A); 
}

// Stops recording timer!
void stopRecord() { TIMSK1 = 0x0; }

bool DERP = true;
// PLAYING: AUDIO OUTPUT ISR
ISR(TIMER2_COMPA_vect) {
	if (tail != head) {
		//analogWrite(SpeakerPin, sample); // SLOW FUNCTION
		sbi(TCCR0A, COM0B1);
		OCR0B = sample;
		sample = AUDIO_BUFFER[(tail & 0x7F)];
		tail++;
	}
}

int isampl = 0;
// RECORDING: AUDIO INPUT ISR
ISR(TIMER1_COMPA_vect) {
	if ((tail ^ head) != 0x80) {
		digitalWrite(3, HIGH); //DERPDERPDERPEDEPR
		while (bit_is_set(ADCSRA, ADSC)); // get result of last conv
		sample = (ADCL >> 2);
		sample |= (ADCH << 6);
		AUDIO_BUFFER[(head & 0x7F)] = sample;
		head++;
		//isampl = analogRead(MicrophonePin); // SLOW FUNCTION!
		//myFile.write(isampl >> 2); // SLOW FUNCTION
		sbi(ADCSRA, ADSC); // start conversion
		digitalWrite(3, LOW); //DERPDERPDERPEDEPR
	} 
}

////////////////////////////////////////////////////////////////////////



// S T A T E S /////////////////////////////////////////////////////////

// LED connection pin numbers: digital output
#define FileLED 10
#define ActiveLED 9

void initLEDs() {
	pinMode(FileLED, OUTPUT);
	digitalWrite(FileLED, LOW);
	pinMode(ActiveLED, OUTPUT);
	digitalWrite(ActiveLED, LOW);
}

// Tell user there was an error
void angryBlink(int pin, int num_blinks, bool end_state) {
	for (int i=0; i<(2*num_blinks); i++) {
		end_state = !end_state;
		digitalWrite(pin, end_state);
		delay(30000);
	}
}

////////////////////////////////////////////////////////////////////////




char fileName[] = "voice.wav";
//char fileName[] = "88mph.wav";
//char fileName[] = "cda.wav";
//char fileName[] = "koko40.wav";

void setup() {
	Serial.begin(9600);
	Serial.println("SERIAL CONNECTION ESTABLISHED");
	initAudio();
	while (!initMem()) angryBlink(FileLED, 3, LOW);
	pinMode(3, OUTPUT); // DERPDERPDERPDEPREDPRPEDPRER
	pinMode(4, OUTPUT); // DERPDERPDERPDEPREDPRPEDPRER
	initButtons();
	initLEDs();
	if (openFile(fileName, false)) {
		closeFile(fileName);
		digitalWrite(FileLED, HIGH);
	}
}




// O P E R A T I O N ///////////////////////////////////////////////////

void loop() {
	switch (waitForButton()) {
	case PLAY:
		if(!openFile(fileName, false)) {
			Serial.println("FILE NOT FOUND");
			angryBlink(FileLED, 3, LOW);
			break;
		}
		digitalWrite(ActiveLED, HIGH);
		playPrep();						// fills buffer half-way
		startPlay();
		while(!wasPressed(PLAY)) {
			//Serial.println(sample);
			playALittle();
			readButtons();
		}
		stopPlay();
		closeFile(fileName);
		digitalWrite(ActiveLED, LOW);
		break;

	case RECORD:
		if(!openFile(fileName, true)) {
			closeFile(fileName);
			Serial.println("ALREADY HAVE FILE");
			angryBlink(FileLED, 3, HIGH);
			break;
		}
		digitalWrite(ActiveLED, HIGH);
		recordPrep();
		startRecord();
		//delay(2); 					// wait for buffer to fill half-way!
		while(!wasPressed(RECORD)) {
			recordALittle();
			//Serial.println(sample); // DEPREDEPRPDEPREP
			readButtons();
		}
		stopRecord();
		closeFile(fileName);
		digitalWrite(ActiveLED, LOW);
		digitalWrite(FileLED, HIGH);
		break;

	case DELETE:
		if (openFile(fileName, false)) {
			deleteFile(fileName);
			digitalWrite(FileLED, LOW);
			Serial.println("FILE DELETED");
			break;
		}
		Serial.println("NOTHING TO DELETE");
		angryBlink(FileLED, 3, LOW);
		break;

	default:
		Serial.println("MAIN LOOP: reached default case?");

	}
}

////////////////////////////////////////////////////////////////////////




// T E S T I N G ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/*
void testNewFatLib() {
	digitalWrite(9, HIGH);
	digitalWrite(9, LOW);
        if (myFile.available()) {
      	digitalWrite(9, HIGH);
      	digitalWrite(9, LOW);
      	myFile.read(index, 2);
      //Serial.println("FUCK");
        }
        else {
        myFile.close();
        myFile.open("lorem.txt", O_READ);
        }
}
*/
////////////////////////////////////////////////////////////////////////
