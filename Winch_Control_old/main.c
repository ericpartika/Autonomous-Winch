


#include "BOARD.h"
#include "FreeRunningTimer.h"
#include "MessageIDs.h"
#include "Protocol.h"
#include "RotaryEncoder.h"
#include "DCMotorDrive.h"
#include "FeedbackControl.h"
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define DELTA 50

#define MAIN
#ifdef MAIN

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    NOP_delay_5ms();
    char str[MAXPAYLOADLENGTH];
    sprintf(str, "Rotary encoder compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);
    RotaryEncoder_Init(ENCODER_SPI_MODE);
    DCMotorDrive_Init();
    //RotaryEncoder_Settings();
    NOP_delay_5ms();
    unsigned int tPlus = 0;
    unsigned int tMinus = 0;
    static int r;
    static int u;
    static int distance;
    static int current;
    TRISFbits.TRISF2 = 0;
    LATFbits.LATF2 = 1;
    static int counter;
    FeedbackControl_SetProportionalGain(50000);
    FeedbackControl_SetIntegralGain(25);
    FeedbackControl_SetDerivativeGain(75000);
    current = RotaryEncoder_getRate();

    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();


        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_COMMAND_OPEN_MOTOR_SPEED) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                signed int motor = (data[1] << 24) & 0xFF000000;
                motor |= (data[2] << 16) & 0x00FF0000;
                motor |= (data[3] << 8) & 0x0000FF00;
                motor |= (data[4]) & 0x000000FF;
                distance = motor * 100;
                //DCMotorDrive_SetMotorSpeed(motor);
                motor = Protocol_IntEndednessConversion(motor);
                Protocol_SendMessage(4, ID_COMMAND_OPEN_MOTOR_SPEED_RESP, &motor);
            }
            if (Protocol_ReadNextID() == ID_FEEDBACK_SET_GAINS) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                int P = (data[1] << 24) & 0xFF000000;
                P |= (data[2] << 16) & 0x00FF0000;
                P |= (data[3] << 8) & 0x0000FF00;
                P |= (data[4]) & 0x000000FF;
                FeedbackControl_SetProportionalGain(P);

                int I = (data[5] << 24) & 0xFF000000;
                I |= (data[6] << 16) & 0x00FF0000;
                I |= (data[7] << 8) & 0x0000FF00;
                I |= (data[8]) & 0x000000FF;
                FeedbackControl_SetIntegralGain(I);

                int D = (data[9] << 24) & 0xFF000000;
                D |= (data[10] << 16) & 0x00FF0000;
                D |= (data[11] << 8) & 0x0000FF00;
                D |= (data[12]) & 0x000000FF;
                FeedbackControl_SetDerivativeGain(D);

                Protocol_SendMessage(0, ID_FEEDBACK_SET_GAINS_RESP, NULL);

            }
            if (Protocol_ReadNextID() == ID_FEEDBACK_RESET_CONTROLLER) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                FeedbackControl_ResetController();
                Protocol_SendMessage(0, ID_FEEDBACK_RESET_CONTROLLER_RESP, NULL);
            }
            if (Protocol_ReadNextID() == ID_COMMANDED_RATE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                r = (data[1] << 24) & 0xFF000000;
                r |= (data[2] << 16) & 0x00FF0000;
                r |= (data[3] << 8) & 0x0000FF00;
                r |= (data[4]) & 0x000000FF;
                Protocol_SendMessage(4, ID_FEEDBACK_UPDATE_OUTPUT, &u);
            }
        }


        if ((tPlus - tMinus) >= 10) {
            tMinus = tPlus;
            counter++;
            
            int pos = RotaryEncoder_getRate();
            current = Accumulated_angle(pos);
            int hold = Protocol_IntEndednessConversion(current);
            Protocol_SendMessage(4, ID_REPORT_RATE, &hold);
            u = FeedbackControl_Update(distance, -1*current);
            u = ((int64_t) u * 1000) >> FEEDBACK_MAXOUTPUT_POWER;
            DCMotorDrive_SetMotorSpeed(u);

        }

    }

}


#endif