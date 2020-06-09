
#include "BOARD.h"
#include "FreeRunningTimer.h"
#include "MessageIDs.h"
#include "Protocol.h"
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RotaryEncoder.h"
#include "NonvolatileMemory.h"
#include "ADCFilter.h"

static signed short filters[4][FILTERLENGTH];
static signed short samples[4][FILTERLENGTH];

static int position;
static int pin;

//#define PARTFOUR
#ifdef PARTFOUR

int main(void)
{
    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    RotaryEncoder_Init(ENCODER_IC_MODE);
    RotaryEncoder_Settings();
    RCServo_Init();
    NonVolatileMemory_Init();
    ADCFilter_Init();

    char str[MAXPAYLOADLENGTH];
    sprintf(str, "ADCFilter compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);

    unsigned int tPlus = 0;
    unsigned int tMinus = 0;

            
    while (1) {
        tPlus = FreeRunningTimer_GetMilliSeconds();
        if ((tPlus - tMinus) >= 10) {
            tMinus = tPlus;
            short raw = ADCFilter_RawReading(pin);
            raw = Protocol_ShortEndednessConversion(raw);
            short filt = ADCFilter_FilteredReading(pin);
            filt = Protocol_ShortEndednessConversion(filt);
            int payload = ((filt << 16) & 0xFFFF0000) | (raw & 0x0000FFFF);
            Protocol_SendMessage(4, ID_ADC_READING, &payload);
        }
        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_ADC_SELECT_CHANNEL) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                pin = data[1];
                short channel = ADCFilter_RawReading(pin);
                channel = Protocol_ShortEndednessConversion(channel);
                Protocol_SendMessage(2, ID_ADC_SELECT_CHANNEL_RESP, &channel);
            }
            if (Protocol_ReadNextID() == ID_ADC_FILTER_VALUES){
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                short newFilt[FILTERLENGTH];
                int i = 1;
                int j = 0;
                while (i < 65){
                    newFilt[j] = data[i] & 0xFF;
                    newFilt[j] |= (data[i+1] << 8) & 0xFF00;
                    j++;
                    i+= 2;
                }
                ADCFilter_SetWeights(pin, newFilt);
                short filtRead = ADCFilter_FilteredReading(pin);
                filtRead = Protocol_ShortEndednessConversion(filtRead);
                Protocol_SendMessage(2, ID_ADC_FILTER_VALUES_RESP, &filtRead);
            }
        }
    }
}

#endif

/**
 * @Function ADCFilter_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief initializes ADC system along with naive filters */
int ADCFilter_Init(void)
{
    AD1CON1bits.ON = 0;

    AD1PCFGbits.PCFG10 = 1; //A3
    AD1PCFGbits.PCFG8 = 1; //A2
    AD1PCFGbits.PCFG4 = 1; //A1
    AD1PCFGbits.PCFG2 = 1; //A0

    AD1CSSLbits.CSSL10 = 1; //A3
    AD1CSSLbits.CSSL8 = 1; //A2
    AD1CSSLbits.CSSL4 = 1; //A1
    AD1CSSLbits.CSSL2 = 1; //A0


    AD1CON1bits.FORM = 0b000; //signed 16bit int
    AD1CON1bits.SSRC = 0b111; //autoconvert
    AD1CON1bits.ASAM = 1; //auto sample

    AD1CON2bits.CSCNA = 1; //scanning
    AD1CON2bits.VCFG = 0; //internal voltage ref
    AD1CON2bits.SMPI = 0b100; //sample after each set, not sure what a set is
    AD1CON2bits.BUFM = 0;
    AD1CON3bits.ADRC = 0;
    AD1CON3bits.ADCS = 173;
    AD1CON3bits.SAMC = 16;

    AD1CON1bits.ON = 1;

    IPC6bits.AD1IP = 4;
    IPC6bits.AD1IS = 0;

    IFS1bits.AD1IF = 0;
    IEC1bits.AD1IE = 1;



}

void __ISR(_ADC_VECTOR) ADCIntHandler(void)
{
    IFS1bits.AD1IF = 0;
    samples[0][position] = ADC1BUF0;
    samples[1][position] = ADC1BUF1;
    samples[2][position] = ADC1BUF2;
    samples[3][position] = ADC1BUF3;
    position = (position + 1) % FILTERLENGTH;
}

/**
 * @Function ADCFilter_RawReading(short pin)
 * @param pin, which channel to return
 * @return un-filtered AD Value
 * @brief returns current reading for desired channel */
short ADCFilter_RawReading(short pin)
{
    return samples[pin][position];
}

/**
 * @Function ADCFilter_FilteredReading(short pin)
 * @param pin, which channel to return
 * @return Filtered AD Value
 * @brief returns filtered signal using weights loaded for that channel */
short ADCFilter_FilteredReading(short pin)
{
    return ADCFilter_ApplyFilter(filters[pin], samples[pin], position);
}

/**
 * @Function short ADCFilter_ApplyFilter(short filter[], short values[], short startIndex)
 * @param filter, pointer to filter weights
 * @param values, pointer to circular buffer of values
 * @param startIndex, location of first sample so filter can be applied correctly
 * @return Filtered and Scaled Value
 * @brief returns final signal given the input arguments
 * @warning returns a short but internally calculated value should be an int */
short ADCFilter_ApplyFilter(short filter[], short values[], short startIndex)
{
    int dataind = startIndex;
    int filterind = 0;
    int result = 0;
    do {
        result += filter[filterind] * values[dataind];
        filterind++;
        dataind = (dataind + 1) % FILTERLENGTH;
    } while (dataind != startIndex);

    short done = result / 32768;

    return done;
}

/**
 * @Function ADCFilter_SetWeights(short pin, short weights[])
 * @param pin, which channel to return
 * @param pin, array of shorts to load into the filter for the channel
 * @return SUCCESS or ERROR
 * @brief loads new filter weights for selected channel */
int ADCFilter_SetWeights(short pin, short weights[])
{
    int i;
    for (i = 0; i < FILTERLENGTH; i++) {
        filters[pin][i] = weights[i];
    }
}