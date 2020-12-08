


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

#define ADSK LATDbits.LATD11

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
    unsigned int tPlusu = 0;
    unsigned int tMinusu = 0;
    static int r;
    static int u;
    static uint32_t distance;
    static int current;
    static uint8_t currentDOUT, lastDOUT;
    TRISFbits.TRISF2 = 0;
    LATFbits.LATF2 = 1;
    //for hx711
    //set pin 34 to input
    TRISDbits.TRISD5 = 1;
    //set pin 35 to output for clock
    TRISDbits.TRISD11 = 0;
    LATDbits.LATD11 = 0;
    //reading for hx711
    int reading = 0;

    static int counter;
    FeedbackControl_SetProportionalGain(50000);
    FeedbackControl_SetIntegralGain(250);
    FeedbackControl_SetDerivativeGain(75000);
    current = RotaryEncoder_getRate();

    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();
        tPlusu = FreeRunningTimer_GetMicroSeconds();

        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_COMMAND_OPEN_MOTOR_SPEED) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                signed int motor = (data[1] << 24) & 0xFF000000;
                motor |= (data[2] << 16) & 0x00FF0000;
                motor |= (data[3] << 8) & 0x0000FF00;
                motor |= (data[4]) & 0x000000FF;
                distance = motor * 100;
                DCMotorDrive_SetMotorSpeed(motor);
                //motor = Protocol_IntEndednessConversion(motor);
                Protocol_SendMessage(4, ID_COMMAND_OPEN_MOTOR_SPEED_RESP, &motor);
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

        //hx711 stuff
        if ((tPlusu - tMinusu) >= 0) {
            tMinusu = tPlusu;

            currentDOUT = PORTDbits.RD5;

            if (currentDOUT == 0 && lastDOUT == 1) {
                reading = 0;
                int i = 0;
                for (i = 0; i < 24; i++) {
                    ADSK = 1;
                    reading = reading << 1;
                    ADSK = 0;
                    if (PORTDbits.RD5) reading++;
                }
                ADSK = 1;
                reading = reading^0x8000;
                ADSK = 0;
                int hold = Protocol_IntEndednessConversion(reading);
                Protocol_SendMessage(4, ID_SERVO_RESPONSE, &hold);
            }

            lastDOUT = currentDOUT;
        }

        if ((tPlus - tMinus) >= 10) {
            tMinus = tPlus;
            counter++;

            int pos = RotaryEncoder_getRate();
            current = Accumulated_angle(-pos)/3;
            //current = Accumulated_angle(pos)*(-3);
            int hold = Protocol_IntEndednessConversion(current);
            Protocol_SendMessage(4, ID_REPORT_RATE, &hold);
            int log[2];
            log[0] = current/1000;
            log[1] = reading/10000;
            Protocol_SendMessage(8, ID_LOG_INT_TWO, &log);
            u = FeedbackControl_Update(distance, 1 * current);
            u = ((int64_t) u * 1000) >> FEEDBACK_MAXOUTPUT_POWER;

            if ((reading/10000) < 20) {
                DCMotorDrive_SetMotorSpeed(-600);
            } else if ((reading/10000) > 50) {
                DCMotorDrive_SetMotorSpeed(600);
            } else {
                DCMotorDrive_SetMotorSpeed(u);
            }
            //DCMotorDrive_SetMotorSpeed(u);

        }

    }

}


#endif