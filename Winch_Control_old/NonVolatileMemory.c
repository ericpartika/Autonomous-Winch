

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

void NOP_delay_6ms();
#define EEPROM_ADDRESS 0b1010000

//#define PARTFOUR
#ifdef PARTFOUR

int main(void) {
    BOARD_Init();
    LEDS_INIT();
    FreeRunningTimer_Init();
    Protocol_Init();
    RotaryEncoder_Init(ENCODER_IC_MODE);
    RotaryEncoder_Settings();
    NonVolatileMemory_Init();


    char str[MAXPAYLOADLENGTH];
    sprintf(str, "NonVolatileMemory Compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);


    while (1) {
        if (Protocol_IsMessageAvailable()) {
            if (Protocol_ReadNextID() == ID_NVM_WRITE_BYTE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                unsigned char dat = data[5];
                int addy = 0;
                addy |= data[1] & 0xFF;
                addy |= (data[2] << 8) & 0xFF00;
                NonVolatileMemory_WriteByte(addy, dat);
                Protocol_SendMessage(0, ID_NVM_WRITE_BYTE_ACK, &data);
            }
            if (Protocol_ReadNextID() == ID_NVM_READ_BYTE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                int addy = 0;
                addy |= data[1] & 0xFF;
                addy |= (data[2] << 8) & 0xFF00;
                unsigned char out = NonVolatileMemory_ReadByte(addy);
                Protocol_SendMessage(1, ID_NVM_READ_BYTE_RESP, &out);
            }
            if (Protocol_ReadNextID() == ID_NVM_WRITE_PAGE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                //unsigned char dat = data[5];
                int addy = data[4] & 0xFF;

                //                addy |= data[1] & 0xFF;
                //                addy |= (data[2] << 8) & 0xFF00;
                int size = strlen(data) - 5;
                unsigned char hold[64];
                int i;
                for (i = 0; i < 64; i++) {
                    hold[i] = data[i + 5];
                }
                char len = strlen(hold);
                if (NonVolatileMemory_WritePage(addy, 64, hold) == SUCCESS) {
                    Protocol_SendMessage(0, ID_NVM_WRITE_PAGE_ACK, &data);
                } else {
                    Protocol_SendDebugMessage("error");
                }
            }
            if (Protocol_ReadNextID() == ID_NVM_READ_PAGE) {
                char data[MAXPAYLOADLENGTH];
                Protocol_GetPayload(data);
                int addy = data[1] & 0xFF;
                //                addy |= data[1] & 0xFF;
                //                addy |= (data[2] << 8) & 0xFF00;
                unsigned char out[64];
                if (NonVolatileMemory_ReadPage(addy, 64, out) == SUCCESS) {
                    Protocol_SendMessage(64, ID_NVM_READ_PAGE_RESP, &out);
                } else {
                    Protocol_SendDebugMessage("error read page");
                }
            }
        }
    }
}

#endif

/**
 * @Function NonVolatileMemory_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief initializes I2C for usage */
int NonVolatileMemory_Init(void) {

    I2C1CONbits.ON = 0;
    //ackdt is to set ack bit (ack = 0)
    //acken is to send to ack bit
    //pen is stop bit 
    //sen is start bit
    //rsen is restart bit
    I2C1ADD = 0b1010000; //last three bits might need to be A2 A1 A0
    I2C1BRG = 0x00C5;
    I2C1CONbits.ON = 1;
}

void NOP_delay_6ms() {
    int i;
    for (i = 0; i < 5000; i++) {
        asm(" nop ");
    }
}

/**
 * @Function char NonVolatileMemory_WriteByte(int address, unsigned char data)
 * @param address, device address to write to
 * @param data, value to write at said address
 * @return SUCCESS or ERROR
 * @brief writes one byte to device */
char NonVolatileMemory_WriteByte(int address, unsigned char data) {
    int ackerror = 0;
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN == 1);

    I2C1TRN = (I2C1ADD << 1) & 0xFFFE;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address >> 8) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = data;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1CONbits.PEN = 1;
    while(I2C1CONbits.PEN == 1);
    NOP_delay_6ms();
    return SUCCESS;
}

/**
 * @Function NonVolatileMemory_ReadByte(int address)
 * @param address, device address to read from
 * @return value at said address
 * @brief reads one byte from device
 * @warning Default value for this EEPROM is 0xFF */
unsigned char NonVolatileMemory_ReadByte(int address) {
    int ackerror = 0;
    unsigned char data;
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN == 1);

    I2C1TRN = (I2C1ADD << 1) & 0xFFFE;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address >> 8) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1CONbits.RSEN = 1;
    while (I2C1CONbits.RSEN == 1);

    I2C1TRN = (I2C1ADD << 1) | 0x1;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1CONbits.RCEN = 1;
    //while(I2C1CONbits.RCEN == 1);
    //while(I2C1STATbits.RBF == 0);

    data = I2C1RCV;

    I2C1CONbits.ACKDT = 1;
    I2C1CONbits.ACKEN = 1;
    while (I2C1STATbits.TRSTAT == 1);

    I2C1CONbits.PEN = 1;
    NOP_delay_6ms();
    return data;
}

/**
 * @Function char int NonVolatileMemory_WritePage(int page, char length, unsigned char data[])
 * @param address, device address to write to
 * @param data, value to write at said address
 * @return SUCCESS or ERROR
 * @brief writes one byte to device */
int NonVolatileMemory_WritePage(int page, char length, unsigned char data[]) {

    int ackerror = 0;
    int address = page << 6;
    int i;
    unsigned char hold[64];
    for (i = 0; i < 64; i++) {
        hold[i] = data[i];
    }

    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN == 1);

    I2C1TRN = (I2C1ADD << 1) & 0xFFFE;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address >> 8) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }

    I2C1TRN = (address) & 0xFF;
    while (I2C1STATbits.TRSTAT == 1);
    if (I2C1STATbits.ACKSTAT == 1) {
        ackerror = 1;
        return ERROR;
    }


    for (i = 0; i < length; i++) {
        I2C1TRN = data[i];
        //        while(I2C1STATbits.TBF == 1);
        while (I2C1STATbits.TRSTAT == 1);
        if (I2C1STATbits.ACKSTAT == 1) {
            ackerror = 1;
            return ERROR;
        }

    }
    
    NOP_delay_6ms();
    
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN == 1);
    return SUCCESS;

}

/**
 * @Function int NonVolatileMemory_ReadPage(int page, char length, unsigned char data[])
 * @param page, page value to read from
 * @param length, value between 1 and 64 bytes to read
 * @param data, array to store values into
 * @return SUCCESS or ERROR
 * @brief reads bytes in page mode, up to 64 at once
 * @warning Default value for this EEPROM is 0xFF */
int NonVolatileMemory_ReadPage(int page, char length, unsigned char data[]) {

    unsigned char dataIndex = 0;

    page <<= 6;

    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN == 1);

    I2C1TRN = EEPROM_ADDRESS << 1;
    while (I2C1STATbits.TRSTAT != 0);

    if (I2C1STATbits.ACKSTAT == 1) {
        return ERROR;
    }

    I2C1TRN = (page >> 8) & 0xFF;

    while (I2C1STATbits.TRSTAT != 0);

    if (I2C1STATbits.ACKSTAT == 1) {
        return ERROR;
    }

    I2C1TRN = page & 0xFF;

    while (I2C1STATbits.TRSTAT != 0);

    if (I2C1STATbits.ACKSTAT == 1) {
        return ERROR;
    }

    I2C1CONbits.RSEN = 1;
    while (I2C1CONbits.RSEN == 1);

    I2C1TRN = (EEPROM_ADDRESS << 1) + 1;

    while (I2C1STATbits.TRSTAT != 0);

    if (I2C1STATbits.ACKSTAT == 1) {
        return ERROR;
    }

    I2C1CONbits.RCEN = 1;
    for (dataIndex = 0; dataIndex < length - 1; dataIndex++) {
        while (I2C1STATbits.RBF != 1);
        data[dataIndex] = I2C1RCV;
        I2C1CONbits.ACKDT = 0;
        I2C1CONbits.ACKEN = 1;
        while (I2C1CONbits.ACKEN == 1);
        I2C1CONbits.RCEN = 1;
    }

    while (I2C1STATbits.RBF != 1);
    data[dataIndex] = I2C1RCV;

    I2C1CONbits.ACKDT = 1;
    I2C1CONbits.ACKEN = 1;
    while (I2C1CONbits.ACKEN == 1);
    
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN == 1);

    return SUCCESS;


















    //    int ackerror = 0;
    //    int address = page << 6;
    //    //unsigned char data;
    //    I2C1CONbits.SEN = 1;
    //    while (I2C1CONbits.SEN == 1);
    //
    //    I2C1TRN = (I2C1ADD << 1) & 0xFFFE;
    //    while (I2C1STATbits.TRSTAT == 1);
    //    if (I2C1STATbits.ACKSTAT == 1) {
    //        ackerror = 1;
    //        return ERROR;
    //    }
    //
    //    I2C1TRN = (address >> 8) & 0xFF;
    //    while (I2C1STATbits.TRSTAT == 1);
    //    if (I2C1STATbits.ACKSTAT == 1) {
    //        ackerror = 1;
    //        return ERROR;
    //    }
    //
    //    I2C1TRN = (address) & 0xFF;
    //    while (I2C1STATbits.TRSTAT == 1);
    //    if (I2C1STATbits.ACKSTAT == 1) {
    //        ackerror = 1;
    //        return ERROR;
    //    }
    //
    //    I2C1CONbits.RSEN = 1;
    //    while (I2C1CONbits.RSEN == 1);
    //
    //    I2C1TRN = ((I2C1ADD << 1) & 0xFFFF) | 1;
    //    while (I2C1STATbits.TRSTAT == 1);
    //    if (I2C1STATbits.ACKSTAT == 1) {
    //        ackerror = 1;
    //        return ERROR;
    //    }
    //
    //    int i;
    //    for (i = 0; i < length; i++) {
    //        I2C1CONbits.RCEN = 1;
    //        while (I2C1CONbits.RCEN == 1);
    //        data[i] = I2C1RCV;
    //        NOP_delay_6ms();
    //        I2C1CONbits.ACKDT = 0;
    //        I2C1CONbits.ACKEN = 1;
    //        while (I2C1STATbits.TRSTAT == 1);
    //    }
    //
    //
    //    I2C1CONbits.ACKDT = 1;
    //    I2C1CONbits.ACKEN = 1;
    //    //while (I2C1STATbits.TRSTAT == 1);
    //
    //    I2C1CONbits.PEN = 1;
    //    NOP_delay_6ms();
    //    return SUCCESS;
}