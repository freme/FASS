#include "LedControl.h"                 // to drive the matrix

#define RED 0                           // The address of the MAX7221 for the red leds == false
#define GREEN 1                         // The address of the MAX7221 for the green leds == true
#define LCC 4                           // number of leds to control LedControlCount
LedControl lc[LCC] = {LedControl(4,3,2,2), LedControl(10,9,8,2), LedControl(13,12,11,2), LedControl(7,6,5,2)};

boolean lcd[LCC][128];                  // data, for each matrix, 64bit red and 64bit green
boolean toggle=RED;                     // tells which MAX7219 is currently off (in shutdown mode)
boolean newData=false;                 // set if new Data arrived

void setup() {
  Serial.begin(57600);
  int intensity=12;                      // 0 = dim, 15 = full brightness
  for (int i=0; i<LCC; i++) {          // put all in power safe mode = default
    lc[i].shutdown(GREEN,true);          
    lc[i].shutdown(RED,false);
    lc[i].setIntensity(GREEN,intensity);
    lc[i].setIntensity(RED,intensity-5); // red is brighter than green
    lc[i].clearDisplay(GREEN);
    lc[i].clearDisplay(RED);
    for (int j=0; j<128; j++) {
      lcd[i][j]=false;
    }
  }
  // set timer interrupt
  cli();//stop interrupts

  //set timer1 interrupt at 200Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 200hz increments
  //156 for 100hz
  //77 for 200hz
  OCR1A = 156;// = (16*10^6) / (200Hz*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // Set CS11 bit for 8 prescaler
  //TCCR1B |= (1 << CS11);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts

  //test all Leds
  for (int i=0; i<LCC; i++) {    

  setNoneForAll(i);
  setRedForAll(i);
  clearAllLeds(i);
  setLeds(i);
  
  delay(1000);
  
  setNoneForAll(i);
  setGreenForAll(i); 
  clearAllLeds(i);
  setLeds(i);

  delay(1000); 
  
  setNoneForAll(i);
  setGreenForAll(i); 
  setRedForAll(i);
  clearAllLeds(i);
  setLeds(i);
 
  delay(1000); 
  clearAllLeds(i);
  }

}

void loop() { 
  if(Serial.available() >= 18) {
    // data is of format DisplayNum:DATA(16Byte)
    int displaynum = Serial.parseInt();
    Serial.read(); //consum the ':'
    Serial.println(displaynum, DEC);
    char data[16];
    Serial.readBytes(data, 16);
    for (int i=0; i<16; i++) {
      for (int j=0; j<8; j++) {
        lcd[displaynum][i*8+j] = bitRead(data[i], j);
      }
    }
//  clearAllLeds(displaynum);
  setLeds(displaynum);
//  delay(1000);
  }

}

ISR(TIMER1_COMPA_vect){
  TIMSK1 &= ~(1 << OCIE1A);  // disable this interrupt
  sei();  //Now enable global interupts before this interrupt is finished
  if(toggle){                     // true means GREEN          
    for (int id=0; id<LCC; id++) {
      lc[id].shutdown(RED,true);    // first switch red off             
      lc[id].shutdown(GREEN,false); // only then switch red on
    }
    toggle=RED;
  }
  else {                         // means false .. RED is off 
    for (int id=0; id<LCC; id++) {
      lc[id].shutdown(GREEN,true); // first switch green off         
      lc[id].shutdown(RED,false);  // only then switch red on
    }
    toggle=GREEN;
  }
/*
  if(Serial.available() >= 18) {
    // data is of format DisplayNum:DATA(16Byte)
    int displaynum = Serial.parseInt();
    Serial.read(); //consum the ':'
    Serial.println(displaynum, DEC);
    char data[16];
    Serial.readBytes(data, 16);
    for (int i=0; i<16; i++) {
      for (int j=0; j<8; j++) {
        lcd[displaynum][i*8+j] = bitRead(data[i], j);
      }
    }
    newData=true;
  }
*/
  TIMSK1 |= (1 << OCIE1A); // enable this interrupt
}

inline void startISR() {
  TCNT1 = 0;               //initialize counter value to 0    
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
}

inline void stopISR(){
  TIMSK1 &= ~(1 << OCIE1A);  // disable interrupts
}


void setLeds(int id) { // set all leds to display picture
  stopISR();
  int cnt  = 0;
  for(int row=0;row<8;row++) {
    for(int col=0;col<8;col++) {
      lc[id].setLed(RED, row, col, lcd[id][cnt]);
      lc[id].setLed(GREEN, row, col, lcd[id][cnt+64]);
      cnt++;
    }
  }
  startISR();
}

void setNoneForAll(int id) {   //empty picture
  for(int cnt=0;cnt<128;cnt++) lcd[id][cnt]=false;
}

void setGreenForAll(int id) { // set picture to all green 
  for(int cnt=64;cnt<128;cnt++) lcd[id][cnt]=true;
}

void setRedForAll(int id) { // set picture to all red
  for(int cnt=0;cnt<64;cnt++) lcd[id][cnt]=true;
}

void clearAllLeds(int id) { //switch off all leds
  stopISR();
  lc[id].clearDisplay(GREEN);
  lc[id].clearDisplay(RED);
  startISR();
}
