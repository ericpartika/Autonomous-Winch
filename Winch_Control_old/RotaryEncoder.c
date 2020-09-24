

#include "BOARD.h"
#include "FreeRunningTimer.h"
#include "MessageIDs.h"
#include "Protocol.h"
#include "RotaryEncoder.h"
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CS LATFbits.LATF1
int RotaryEncoder_CalcParity(int in);
void RotaryEncoder_ReadAddress(int pac);
void RotaryEncoder_Settings(void);
int build(int pack, int rw);
void NOP_delay_5ms();
char str[MAXPAYLOADLENGTH];
unsigned short upgoing = 0;
unsigned short downgoing = 0;
unsigned short elapsed = 0;
short recieve;

#define TICK_RATE 2
#define ENCODER_ROLLOVER (1 << 14)
#define MAX_RATE 7000

//#define part2
#ifdef part2

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    NOP_delay_5ms();
    sprintf(str, "%cRotary encoder compiled on %s at %s", ID_DEBUG, __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);
    RotaryEncoder_Init(ENCODER_IC_MODE);
    RotaryEncoder_Settings();
    NOP_delay_5ms();
    unsigned int tPlus = 0;
    unsigned int tMinus = 0;

    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();
        if ((tPlus - tMinus) >= 20) {
            //LATFbits.LATF1 ^= 1;
            tMinus = tPlus;
            //            sprintf(str, "%c Angle:%u", ID_DEBUG, elapsed);
            //            Protocol_SendDebugMessage(str);
            //            unsigned short ang = Protocol_ShortEndednessConversion(elapsed);
            unsigned short ang = elapsed;
            Protocol_SendMessage(3, ID_ROTARY_ANGLE, &ang);


        }

    }



    //    unsigned short packet = 0x00CC;
    //    packet |= (RotaryEncoder_CalcParity(packet) << 15);
    //    CS = 0x0;
    //    SPI2BUF = packet;
    //    while(SPI2STATbits.SPIBUSY);
    //    CS = 0x1;
    //    NOP_delay_5ms();
    //    CS = 0;
    //    sprintf(str, "%c%X", ID_DEBUG, SPI2BUF);
    //    Protocol_SendDebugMessage(str);
    //    CS = 1;


}
#endif

//#define SPITEST
#ifdef SPITEST

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    NOP_delay_5ms();
    sprintf(str, "Rotary encoder compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);
    RotaryEncoder_Init(ENCODER_SPI_MODE);
    //RotaryEncoder_Settings();
    NOP_delay_5ms();
    unsigned int tPlus = 0;
    unsigned int tMinus = 0;
    TRISDbits.TRISD1 = 0;
    LATDbits.LATD1 = 0;

    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();
        if ((tPlus - tMinus) >= 10) {
            tMinus = tPlus;
            //            unsigned short ang = elapsed;
            recieve = Protocol_ShortEndednessConversion(recieve);
            Protocol_SendMessage(2, ID_ROTARY_ANGLE, &recieve);


        }

    }
}

#endif

int RotaryEncoder_Init(char interfaceMode) {

    if (interfaceMode == ENCODER_IC_MODE) {
        SPI2CONbits.ON = 0x0;
        SPI2CONbits.MODE32 = 0;
        SPI2CONbits.MODE16 = 1; // might need to be 0x01
        SPI2CONbits.CKE = 0; //falling edge
        SPI2CONbits.CKP = 0; //clock idle low
        SPI2CONbits.MSTEN = 1;
        SPI2CONbits.SMP = 1;
        SPI2CONbits.SSEN = 0;
        IEC1bits.SPI2EIE = 0;
        IEC1bits.SPI2RXIE = 0;
        IEC1bits.SPI2TXIE = 0;
        SPI2BRG = 3; //from FRM
        //SPI2BRG = (BOARD_GetPBClock() / (16 * 5000000)) - 1; // from protocol and lecture
        TRISFbits.TRISF1 = 0; //CS
        uint16_t clr = SPI2BUF; //clear buffer
        CS = 1;
        SPI2CONbits.ON = 1;


        TRISDbits.TRISD1 = 0;
        LATDbits.LATD1 = 0;

        T2CONbits.ON = 0x0;
        T2CONbits.TCKPS = 0x1; //prescaler each tick is .1 microsecs
        TMR2 = 0x0; //timer register
        PR2 = 0x8F40; //period register
        IFS0bits.T2IF = 0x0;
        IPC2bits.T2IP = 0x1;
        IPC2bits.T2IS = 0x0;
        IEC0bits.T2IE = 0;



        IC3CONbits.ON = 0;
        IC3CONbits.FEDGE = 1;
        IC3CONbits.C32 = 0;
        IC3CONbits.ICTMR = 1;
        IC3CONbits.ICI = 0x0;
        IC3CONbits.ICM = 0x1;

        IEC0bits.IC3IE = 0;
        IFS0bits.IC3IF = 0;
        IPC3bits.IC3IP = 3;
        IPC3bits.IC3IS = 0;
        IEC0bits.IC3IE = 1;
        IC3CONbits.ON = 1;

        T2CONbits.ON = 0x1;
    }





    if (interfaceMode == ENCODER_SPI_MODE) {
        SPI1CONbits.ON = 0;
        SPI2CONbits.ON = 0x0;
        SPI2CONbits.MODE32 = 0;
        SPI2CONbits.MODE16 = 1; // might need to be 0x01
        SPI2CONbits.CKE = 0; //falling edge
        SPI2CONbits.CKP = 0; //clock idle low
        SPI2CONbits.MSTEN = 1;
        SPI2CONbits.SMP = 1;
        SPI2CONbits.SSEN = 0;


        IFS1bits.SPI2RXIF = 0;
        IPC7bits.SPI2IP = 4;
        IPC7bits.SPI2IS = 0;
        IEC1bits.SPI2EIE = 1;
        IEC1bits.SPI2RXIE = 1;
        IEC1bits.SPI2TXIE = 0;


        SPI2BRG = 3; //from FRM
        //SPI2BRG = (BOARD_GetPBClock() / (16 * 5000000)) - 1; // from protocol and lecture
        TRISFbits.TRISF1 = 0; //CS
        //uint16_t clr = SPI2BUF; //clear buffer
        CS = 1;
        SPI2CONbits.ON = 1;


        T2CONbits.ON = 0x0;
        T2CONbits.TCKPS = 0x2; //prescaler each tick is .1 microsecs
        TMR2 = 0x0; //timer register
        PR2 = 0x4E20; //period register
        IFS0bits.T2IF = 0x0;
        IPC2bits.T2IP = 0x4;
        IPC2bits.T2IS = 0x0;
        IEC0bits.T2IE = 1;

        T2CONbits.ON = 0x1;



        CS = 0;
        //NOP_delay_5ms();
        SPI2BUF = 0xFFFF;
        //    SPI2BUF = build(packet, 1);
        while (SPI2STATbits.SPIBUSY);
        CS = 1;
        //        NOP_delay_5ms();
        //        //int trash = SPI2BUF;
        //        CS = 0;
        //        NOP_delay_5ms();
        //        SPI2BUF = 0xC000;
        //        //    SPI2BUF = build(0x0000, 1);
        //        while (SPI2STATbits.SPIBUSY);
        //        CS = 1;
        //        uint16_t angle = SPI2BUF;
    }


}

void __ISR(_TIMER_2_VECTOR) Timer2IntHandler(void) {
    IFS0bits.T2IF = 0x0;
    CS = 0;

    //NOP_delay_5ms();
    SPI2BUF = 0xFFFF;
    //    SPI2BUF = build(packet, 1);
    while (SPI2STATbits.SPIBUSY);
    CS = 1;


//            NOP_delay_5ms();
    //        //int trash = SPI2BUF;
    //        CS = 0;
    //        NOP_delay_5ms();
    //        SPI2BUF = 0xC000;
    //        //    SPI2BUF = build(0x0000, 1);
    //        while (SPI2STATbits.SPIBUSY);
    //        CS = 1;
    //        uint16_t angle = SPI2BUF;
}

void __ISR(_SPI_2_VECTOR) __SPI2Interrupt(void) {
    IFS1bits.SPI2RXIF = 0;
    IFS1bits.SPI2TXIF = 0;

    recieve = SPI2BUF & 0x3FFF;
}

void __ISR(_INPUT_CAPTURE_3_VECTOR) __IC3Interrupt(void) {
    IFS0bits.IC3IF = 0;
    static int state = 0;
    //LATDbits.LATD1 ^= 1;
    if (state == 0) {
        if (PORTDbits.RD10) {
            upgoing = 0xFFFF & IC3BUF;
            state = 1;
        } else {
            if (IC3CONbits.ICBNE) {
                int trash = IC3BUF;
            }
            state = 0;
        }

    } else if (state == 1) {
        if (!PORTDbits.RD10) {
            downgoing = 0xFFFF & IC3BUF;
            if (((downgoing - upgoing) > 1000) && ((downgoing - upgoing) < 36000)) {
                elapsed = downgoing - upgoing;
            }
            state = 0;
        } else {
            if (IC3CONbits.ICBNE) {
                int trash = IC3BUF;
            }
            state = 1;
        }
    }
}

int RotaryEncoder_CalcParity(int in) {

    int p = 0;
    int i = 1;
    while (i <= 0x8000) {
        if (i & in) {
            p++;
        }
        i = i << 1;
    }
    p = p % 2;
    return p;
}

void NOP_delay_5ms() {
    int i;
    for (i = 0; i < 100; i++) {
        asm(" nop ");
    }
}

void RotaryEncoder_Settings(void) {
    CS = 0;
    SPI2BUF = 0x0018;
    while (SPI2STATbits.SPIBUSY == 1);
    int i = 0;
    //    while (i < 10) {
    //        i++;
    //    }
    CS = 1;
    //    while (i < 10) {
    //        i++;
    //    }
    //int trash = SPI2BUF;
    CS = 0;
    SPI2BUF = 0x0081;
    while (SPI2STATbits.SPIBUSY == 1);
    //    while (i < 10) {
    //        i++;
    //    }
    CS = 1;
}

//void RotaryEncoder_ReadAddress(int pac) {
//    int packet = pac;
//    CS = 0;
//    //    NOP_delay_5ms();
//    SPI2BUF = 0xFFFC;
//    //    SPI2BUF = build(packet, 1);
//    while (SPI2STATbits.SPIBUSY);
//    CS = 1;
//
//
//    //    int trash = SPI2BUF;
//    //    CS = 0;
//    //    NOP_delay_5ms();
//    //    SPI2BUF = 0xC000;
//    //    //    SPI2BUF = build(0x0000, 1);
//    //    while (SPI2STATbits.SPIBUSY);
//    //    CS = 1;
//    //    uint16_t angle = SPI2BUF;
//
//    Protocol_SendMessage(sizeof (angle), ID_ROTARY_ANGLE, &angle);
//}

int build(int pack, int rw) {
    int result = pack;
    int pair = RotaryEncoder_CalcParity(result);
    if (pair) {
        result = result | 0x8000;
    }
    if (rw) {
        result = result | 0x4000;
    }

    pair = RotaryEncoder_CalcParity(result);
    if (pair) {
        result = result | 0x8000;
    } else {
        result = result & 0x7FFF;
    }

    return result;
}

unsigned short RotaryEncoder_getElapsed(void) {
    return elapsed;
}

int RotaryEncoder_getRate(void) {
    static short last;
    
    return  (int)  recieve;
}

int RotaryEncoder_ThreshCheck(int Tcurrent) {
    static int Tprev;
    static int AngPrev;

    if ((Tcurrent - Tprev) >= TICK_RATE) {
        Tprev = Tcurrent;
        short AngCurrent = RotaryEncoder_getRate();
        int w = AngCurrent - AngPrev;
        AngPrev = AngCurrent;
        if (w > MAX_RATE) {
            w = w - ENCODER_ROLLOVER;
        }
        if (w < -MAX_RATE) {
            w = w + ENCODER_ROLLOVER;
        }
        return w;
    }
}

int Accumulated_angle(int current) {
    static int previous;
    static int n;

    int w = current - previous;
    previous = current;
    if (w > MAX_RATE) {
        w = w - ENCODER_ROLLOVER;
        n -= ENCODER_ROLLOVER;
    }
    if (w < -MAX_RATE) {
        w = w + ENCODER_ROLLOVER;
        n += ENCODER_ROLLOVER;
    }
    return current + n;
}