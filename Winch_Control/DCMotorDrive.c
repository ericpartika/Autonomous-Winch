


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
#include "DCMotorDrive.h"
#include "FeedbackControl.h"



static int pulse = 0;
static int IN = 0;



//#define DC
#ifdef DC

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    Protocol_Init();
    FreeRunningTimer_Init();
    NOP_delay_5ms();
    char str[MAXPAYLOADLENGTH];
    sprintf(str, "DCMotorDrive compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);
    DCMotorDrive_Init();
    RotaryEncoder_Init(ENCODER_SPI_MODE);
    //RotaryEncoder_Settings();

    unsigned int tPlus = 0;
    unsigned int tMinus = 0;
    static int rate;

    static int counter;

    while (1) {
        
        LATEbits.LATE0 = IN;
        LATEbits.LATE1 = ~IN;
        tPlus = FreeRunningTimer_GetMilliSeconds();
        if ((tPlus - tMinus) >= 100) {
            tMinus = tPlus;
            int rate = RotaryEncoder_getRate();
            //rate = Protocol_ShortEndednessConversion(rate);
            rate = Protocol_IntEndednessConversion(rate);
            Protocol_SendMessage(4, ID_REPORT_RATE, &rate);
            
            }
        
        
        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_COMMAND_OPEN_MOTOR_SPEED) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                signed int motor = (data[1] << 24) & 0xFF000000;
                motor |= (data[2] << 16) & 0x00FF0000;
                motor |= (data[3] << 8) & 0x0000FF00;
                motor |= (data[4]) & 0x000000FF;
                DCMotorDrive_SetMotorSpeed(motor);
                motor = Protocol_IntEndednessConversion(motor);
                Protocol_SendMessage(4, ID_COMMAND_OPEN_MOTOR_SPEED_RESP, &motor);
            }
        }
    }
}

#endif

//void __ISR(_TIMER_3_VECTOR, ipl3auto) Timer3IntHandler(void) {
//
//    IFS0bits.T3IF = 0x0;
//    //            LATDbits.LATD8 ^= 1;
//
//}

void __ISR(_OUTPUT_COMPARE_3_VECTOR) __OC3Interrupt(void) {
    //IFS0bits.T3IF = 0;
    IFS0bits.OC3IF = 0;
    if (pulse == 0){
        pulse = 1;
    }
    OC3RS = pulse;
}

int DCMotorDrive_Init(void) {
    //timer set up
    T3CONbits.ON = 0;
    T3CONbits.TCKPS = 1; //prescaler tick = about .1 microsec
    TMR3 = 0; //timer register
    PR3 = 0x8000; //period register
    IFS0bits.T3IF = 0;
    IPC3bits.T3IP = 6;
    IPC3bits.T3IS = 0;
    IEC0bits.T3IE = 0;
    T3CONbits.ON = 1;

    //OC setup
    OC3CONbits.ON = 0;
    OC3CONbits.OC32 = 0; //16 bit mode
    OC3CONbits.OCM = 0x5; //set to pwm mode
    OC3CONbits.OCTSEL = 1; //select timer3
    IPC3bits.OC3IP = 5;
    IPC3bits.OC3IS = 0;
    OC3RS = 0;
    IFS0bits.OC3IF = 0;
    IEC0bits.OC3IE = 1;
    OC3CONbits.ON = 1;

    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;

}

/**
 * @Function DCMotorDrive_SetMotorSpeed(int newMotorSpeed)
 * @param newMotorSpeed, in units of Duty Cycle (+/- 1000)
 * @return SUCCESS or ERROR
 * @brief Sets the new duty cycle for the motor, 0%->0, 100%->1000 */
int DCMotorDrive_SetMotorSpeed(int newMotorSpeed) {
    //pulse = newMotorSpeed;
    if (newMotorSpeed < 0) {
        newMotorSpeed *= -1;
        IN = 1;
        LATEbits.LATE0 = IN;
        LATEbits.LATE1 = ~IN;
    }else {
        IN = 0;
        newMotorSpeed *= .5;
        LATEbits.LATE0 = IN;
        LATEbits.LATE1 = ~IN;
    }
    //((int64_t)feedbackOutput* 1000) >> FEEDBACK_MAXOUTPUT_POWER;
    //pulse = ((int64_t)newMotorSpeed* 1000) >> FEEDBACK_MAXOUTPUT_POWER;
    pulse = (newMotorSpeed << 15) / 1000;

}


/**
 * @Function DCMotorControl_GetMotorSpeed(void);
 * @param None
 * @return duty cycle of motor 
 * @brief returns speed in units of Duty Cycle (+/- 1000) */
int DCMotorControl_GetMotorSpeed(void) {
    return (pulse * 1000) >> 15;
}