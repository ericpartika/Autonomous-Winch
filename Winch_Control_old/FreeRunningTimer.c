


#include "BOARD.h"
#include "FreeRunningTimer.h"
#include "MessageIDs.h"
#include "Protocol.h"
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned int millisecs = 0;
unsigned int microsecs = 0;
unsigned int count = 0;

//#define part1
#ifdef part1

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    TRISFbits.TRISF1 = 0;
    LATFbits.LATF1 = 1;

    char str[MAXPAYLOADLENGTH];
    sprintf(str, "%cFreeRunningTimer compiled on %s at %s", ID_DEBUG, __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);

    unsigned int tPlus = 0;
    unsigned int tMinus = 0;
    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();
        if ((tPlus - tMinus) >= 2000) {
            LATFbits.LATF1 ^= 1;
            LEDS_SET(~LEDS_GET());
            tMinus = tPlus;
            sprintf(str, "%cMilliseconds: %u, Microseconds: %u", ID_DEBUG, FreeRunningTimer_GetMilliSeconds(), FreeRunningTimer_GetMicroSeconds());
            Protocol_SendDebugMessage(str);
            
        }

    }
}
#endif

void __ISR(_TIMER_5_VECTOR, ipl3auto) Timer5IntHandler(void) {
    //    IFS0CLR = ~(_IFS0_T5IF_MASK);
    IFS0bits.T5IF = 0x0;
    microsecs += 1000;
    millisecs++;
    
}

void FreeRunningTimer_Init(void) {
    T5CON = 0;
    T5CONbits.ON = 0x0;
    T5CONbits.TCKPS = 0x1; //prescaler each tick is .1 microsecs
    TMR5 = 0x0; //timer register
    PR5 = 0x4E20; //period register
    IFS0bits.T5IF = 0x0;
    IPC5bits.T5IP = 0x1;
    IPC5bits.T5IS = 0x0;
    IEC0bits.T5IE = 0x1;
    T5CONbits.ON = 0x1;
}

unsigned int FreeRunningTimer_GetMilliSeconds(void) {

    return millisecs;
}

unsigned int FreeRunningTimer_GetMicroSeconds(void) {
    return microsecs + (TMR5/10);
}