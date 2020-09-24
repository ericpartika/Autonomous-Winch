#include "BOARD.h"
#include "MessageIDs.h"
#include "Protocol.h"

#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

short LEDS = 0xF0;
static uint8_t current, last;    
    
//can use change notify system

int main(void){
    LEDS_INIT();
    
    
    
    //set pin 34 to input
    TRISDbits.TRISD5 = 1;
    
    TRISFbits.TRISF5 = 0;
    LATFbits.LATF5 = 1;
    
    //timer set up
    T3CONbits.ON = 0;
    T3CONbits.TCKPS = 1; //prescaler tick = about .1 microsec
    TMR3 = 0; //timer register
    PR3 = 0x8000; //period register
    IFS0bits.T3IF = 0;
    IPC3bits.T3IP = 6;
    IPC3bits.T3IS = 0;
    IEC0bits.T3IE = 1;
    T3CONbits.ON = 1;
   
    
    while(1);
    return 0;
}


void __ISR(_TIMER_3_VECTOR, ipl3auto) Timer3IntHandler(void) {

    IFS0bits.T3IF = 0;
    LEDS_SET(LEDS);
    current = PORTDbits.RD5;
    
    if(current == 0 && last == 1){
        LEDS_SET(LEDS);
    }
    
    last = current;

}