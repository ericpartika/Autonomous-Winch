

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

#define MAX_CONTROL_OUTPUT (1 << FEEDBACK_MAXOUTPUT_POWER)

static int A;
static int Yminus;
static int P;
static int I;
static int D;


//#define FEED
#ifdef FEED

int main(void) {

    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    NOP_delay_5ms();
    char str[MAXPAYLOADLENGTH];
    sprintf(str, "FeedbackControl compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);
    RotaryEncoder_Init(ENCODER_SPI_MODE);
    DCMotorDrive_Init();
    //RotaryEncoder_Settings();
    NOP_delay_5ms();
    unsigned int tPlus = 0;
    unsigned int tMinus = 0;




    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();


        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_FEEDBACK_SET_GAINS) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                P = (data[1] << 24) & 0xFF000000;
                P |= (data[2] << 16) & 0x00FF0000;
                P |= (data[3] << 8) & 0x0000FF00;
                P |= (data[4]) & 0x000000FF;
                FeedbackControl_SetProportionalGain(P);
                
                I = (data[5] << 24) & 0xFF000000;
                I |= (data[6] << 16) & 0x00FF0000;
                I |= (data[7] << 8) & 0x0000FF00;
                I |= (data[8]) & 0x000000FF;
                FeedbackControl_SetIntegralGain(I);
                
                D = (data[9] << 24) & 0xFF000000;
                D |= (data[10] << 16) & 0x00FF0000;
                D |= (data[11] << 8) & 0x0000FF00;
                D |= (data[12]) & 0x000000FF;
                FeedbackControl_SetDerivativeGain(D);
                
                Protocol_SendMessage(0, ID_FEEDBACK_SET_GAINS_RESP, NULL);
                
            }
            if (Protocol_ReadNextID() == ID_FEEDBACK_RESET_CONTROLLER) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                A = 0;
                Yminus = 0;
                Protocol_SendMessage(0, ID_FEEDBACK_RESET_CONTROLLER_RESP, NULL);
            }
            if (Protocol_ReadNextID() == ID_FEEDBACK_UPDATE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                int r = (data[1] << 24) & 0xFF000000;
                r |= (data[2] << 16) & 0x00FF0000;
                r |= (data[3] << 8) & 0x0000FF00;
                r |= (data[4]) & 0x000000FF;
                
                int s = (data[5] << 24) & 0xFF000000;
                s |= (data[6] << 16) & 0x00FF0000;
                s |= (data[7] << 8) & 0x0000FF00;
                s |= (data[8]) & 0x000000FF;
                
                int u = FeedbackControl_Update(r, s);
                u = Protocol_IntEndednessConversion(u);
                Protocol_SendMessage(4, ID_FEEDBACK_UPDATE_OUTPUT, &u);
            }
        }
        //
        //        
        //        if ((tPlus - tMinus) >= 10) {
        //            tMinus = tPlus;
        //            counter++;
        //            //            unsigned short ang = elapsed;
        //            rate = RotaryEncoder_ThreshCheck(tPlus);
        //            rate = Protocol_IntEndednessConversion(rate);
        //            Protocol_SendMessage(4, ID_REPORT_RATE, &rate);
        //        }

    }

}


#endif

/**
 * @Function FeedbackControl_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief initializes the controller to the default values and (P,I,D)->(1, 0, 0)*/
int FeedbackControl_Init(void) {
    P = 1;
    I = 0;
    D = 0;
}

/**
 * @Function FeedbackControl_SetProportionalGain(int newGain);
 * @param newGain, integer proportional gain
 * @return SUCCESS or ERROR
 * @brief sets the new P gain for controller */
int FeedbackControl_SetProportionalGain(int newGain) {
    P = newGain;
}

/**
 * @Function FeedbackControl_SetIntegralGain(int newGain);
 * @param newGain, integer integral gain
 * @return SUCCESS or ERROR
 * @brief sets the new I gain for controller */
int FeedbackControl_SetIntegralGain(int newGain) {
    I = newGain;
}

/**
 * @Function FeedbackControl_SetDerivativeGain(int newGain);
 * @param newGain, integer derivative gain
 * @return SUCCESS or ERROR
 * @brief sets the new D gain for controller */
int FeedbackControl_SetDerivativeGain(int newGain) {
    D = newGain;
}

/**
 * @Function FeedbackControl_GetPorportionalGain(void)
 * @param None
 * @return Proportional Gain
 * @brief retrieves requested gain */
int FeedbackControl_GetProportionalGain(void) {
    return P;
}

/**
 * @Function FeedbackControl_GetIntegralGain(void)
 * @param None
 * @return Integral Gain
 * @brief retrieves requested gain */
int FeedbackControl_GetIntegralGain(void) {
    return I;
}

/**
 * @Function FeedbackControl_GetDerivativeGain(void)
 * @param None
 * @return Derivative Gain
 * @brief retrieves requested gain */
int FeedbackControl_GetDerivativeGain(void) {
    return D;
}

/**
 * @Function FeedbackControl_Update(int referenceValue, int sensorValue)
 * @param referenceValue, wanted reference
 * @param sensorValue, current sensor value
 * @brief performs feedback step according to algorithm in lab manual */
int FeedbackControl_Update(int referenceValue, int sensorValue) {
    int e = referenceValue - sensorValue;
    A += e;
    int deriv = -1 * (sensorValue - Yminus);
    Yminus = sensorValue;
    int u = (P * e) + (I * A) + (D * deriv);
    if (u > MAX_CONTROL_OUTPUT) {
        u = MAX_CONTROL_OUTPUT;
        A -= e;
    } else if (u < -MAX_CONTROL_OUTPUT) {
        u = -MAX_CONTROL_OUTPUT;
        A -= e;
    }
    return u;
}

/**
 * @Function FeedbackControl_ResetController(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief resets integrator and last sensor value to zero */
int FeedbackControl_ResetController(void) {
    A = 0;
    Yminus = 0;
}