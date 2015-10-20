/****************************************
/			DUBSTEP CRICKET
/				  by
/			   Emil Maid
/				   &
/			   Uros Tesic
/****************************************/

//Buttons B6-0, C6-0, D6-0 play the corresponding C-major scale tone (6-C, 5-D, 4-E, 3-F, 2-G, 1-A, 0-B(H))
//A0 plays the tune stored in the buffer, A1 stops the tune, A4 clears the buffer, A2 turns recording mode on, A3 turns it off
//While recording A5 LED is switched on as an indicator

//Set all switches on ports to pull-down
//Buzzer is connected to PORT E

//PORT A: |x| x|**|Cl|Of|On|St|Pl|
//PORT B: |x|C4|D4|E4|F4|G4|A4|B4|
//PORT C: |x|C5|D5|E5|F5|G5|A5|B5|
//PORT D: |x|C6|D6|E6|F6|G6|A6|B6|


//The table of tone frequencies
//C-major scale from C4 to B6(H6)
const unsigned frequencies[] = {
262, 294, 330, 349, 392, 440, 494, 
523, 587, 659, 698, 784, 880, 988,
1046, 1174, 1318,1397, 1568, 1760, 1976};

//The length of note buffer
const unsigned bufferLength = 128;
//Frequency buffer and duration buffer
unsigned freq[bufferLength], dur[bufferLength];
//The buffer for active lights
unsigned char portbs[bufferLength], portcs[bufferLength], portds[bufferLength];
//Circular buffer elements
unsigned first, last;
//Current frequency and duration
unsigned duration, frequency;
//Current active lights
unsigned maskB, maskC, maskD;
//Current element
unsigned iter;
//Button state flag
bit buttonPressed;


//No buttons are pressed at first
void keyboardInit() {
     maskB = maskC = maskD = 0;
}
//Buffer is empty
void bufferInit() {
     PORTA.b5 = 0;
     frequency = duration = 0;
     first = last = 0;
}

//Function to add frequency, duration and status of ports to the buffer
//Frequency and duration are used to play the corresponding tone, and port status is used to turn on the right LED
void bufferAdd(unsigned f, unsigned d, unsigned char portbb, unsigned char portcc, unsigned char portdd) {
     //if buffer full, then override first element
     if((last + 1)%bufferLength == first%bufferLength) {
          freq[last] = f;
          dur[last] = d;
          portbs[last] = portbb;
          portcs[last] = portcc;
          portds[last] = portdd;
          last = (last+1)%bufferLength;
          first = (first+1)%bufferLength;
          return;
     }
     //If buffer not full, just write it in the next free location
     freq[last] = f;
     dur[last] = d;
     portbs[last] = portbb;
     portcs[last] = portcc;
     portds[last] = portdd;
     last = (last+1)%bufferLength;
}

//Empty the buffer
void bufferErase() {
     last = first;
}

//Play all the notes from the buffer
void bufferPlay() {
	//If the buffer is empty, do nothing
     if(first == last)
              return;
     //Skip silent intro
     if(freq[first] == 0)
                    dur[first] = 1;
     //Set ports as output
     TRISB = TRISC = TRISD = 0x00;
     //Iterate over the notes
     for(iter = first;iter != last; iter = (iter+1)%bufferLength) {
     	//If the button is pressed
             if(Button(&PORTA, 1, 1, 0)){

                Sound_Play(freq[iter],dur[iter]);//Play the note
                PORTB = portbs[iter];//Turn the LED on
                PORTC = portcs[iter];
                PORTD = portds[iter];
             }
             else
                 break;
     }
     PORTB = PORTC = PORTD = 0;//Turn LEDs off
     TRISB = TRISC = TRISD = 0x7F;//Set ports as input again, for the glory of president Putin and the motherland
}

//Maps pressed button to the index in frequency array
unsigned offset(unsigned char port) {
     int i = 0;
     while((port & 1) == 0) {
         port >>=1;
         i++;
     }
     return i;
}

//Timer0
//Prescaler 1:1; TMR0 Preload = 25536; Actual Interrupt Time : 20 ms

void InitTimer0(){
  T0CON  = 0x88;
  TMR0H  = 0x63;
  TMR0L  = 0xC0;
  GIE_bit  = 1;//Global interrupt enable
  TMR0IE_bit  = 1;//Timer0 interrupt enable
}

void Interrupt(){
	//Check if Timer0 caused the interrupt
  if (TMR0IF_bit){
    TMR0IF_bit = 0;//Set Timer0 interrupt flag to 0
    TMR0H  = 0x63;//Reset timer
    TMR0L  = 0xC0;

    //If we are still playing the same tone, increase its duration
    if(maskB == PORTB && maskC == PORTC && maskD == PORTD) {
        duration++;
        return;
    }
    //If the status of ports has changed, and we are recording, add the note to the buffer
    if(PORTA.b5)
                bufferAdd(frequency, duration*20, PORTB, PORTC, PORTD);
    //Remember the new note as the current one
    duration = 0;
    maskB = PORTB;
    maskC = PORTC;
    maskD = PORTD;
    //Map pressed button to the frequency
    if(PORTB) {
         frequency = frequencies[6-offset(PORTB)];
         return;
    }
    if(PORTC) {
         frequency = frequencies[13 - offset(PORTC)];
         return;
    }
    if(PORTD) {
         frequency = frequencies[20 - offset(PORTD)];
         return;
    }
    //If no button is pressed, remember musical break. Human ear cannot hear low pitches
    frequency = 0;
  }
}

void main() {
     ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = 0;//Set ports as digital
     TRISA = 0x1F;//Set ports as input
     TRISB = TRISC = TRISD = 0x7F;//Set ports as input
     TRISE = 0;//Set port as output (buzzer)
     PORTA = PORTB = PORTC = PORTD = 0x00;
     PORTE = 0;
     InitTimer0();//Initialize Timer0
     Sound_Init(&PORTE, 1);//Initialize buzzer
     bufferInit();//Initialize buffer
     keyboardInit();//Initialize keyboard
     while(1) {
          if(Button(&PORTA, 0, 1, 1))
               bufferPlay(), buttonPressed = 1;//Play buffer
          if(Button(&PORTA, 4, 1, 1))
               bufferErase(), buttonPressed = 1;//Erase buffer
          if(Button(&PORTA, 2, 1, 1) && !buttonPressed)//Start recording
               PORTA.b5 = 1, buttonPressed = 1;
          if(Button(&PORTA, 3, 1, 1) && !buttonPressed)//Stop recording
               PORTA.b5 = 0, buttonPressed = 1;
               
          //If a button is released, update the flag
          if(Button(&PORTA, 0, 1, 0))
               buttonPressed = 0;
          if(Button(&PORTA, 4, 1, 0))
               buttonPressed = 0;
          if(Button(&PORTA, 2, 1, 0))
               buttonPressed = 0;
          if(Button(&PORTA, 3, 1, 0))
               buttonPressed = 0;

          Sound_Play(frequency, 5);//Play a sound when a button is pressed
     }
}
