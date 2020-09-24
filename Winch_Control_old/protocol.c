




#include "BOARD.h"
#include "MessageIDs.h"
#include "Protocol.h"
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define IF_TESTING
//#define PROTO

//defines
int counter = 0;
int adding = 0;
int pulling = 0;
int block = 0;

#define MAX_BUFFER_LENGTH 100

//structs

typedef struct circBuffer {
    uint8_t head;
    uint8_t tail;
    unsigned char data[MAXPAYLOADLENGTH];
} circBuffer;

typedef struct messageData {
    unsigned int len;
    unsigned char payload[MAXPAYLOADLENGTH];
} messageData;

typedef struct payBuffer {
    uint8_t head;
    uint8_t tail;
    messageData data[PACKETBUFFERSIZE];
} payBuffer;

typedef enum {
    WAITING, //0
    RHEAD, //1
    RLEN, //2
    RID, //3
    RPAYLOAD, //4
    RTAIL, //5
    RCHECKSUM, //6
} stateMachine;

circBuffer myCirc = {.head = 0, .tail = 0};

payBuffer myPay = {.head = 0, .tail = 0};

payBuffer inPay = {.head = 0, .tail = 0};

int state = WAITING;

void add(char c);
char pull(void);
char echo;


#ifdef PROTO

int main(void) {
    BOARD_Init();
    LEDS_INIT();

    //enable uart
    U1MODEbits.ON = 0x1;
    U1MODEbits.PDSEL = 0x0;
    //enable transmit and receive bits
    U1STAbits.URXEN = 0x1;
    U1STAbits.UTXEN = 0x1;
    //set baud rate
    U1BRG = (BOARD_GetPBClock() / (16 * 115200)) - 1;


    //clear interrupt flags
    IFS0bits.U1TXIF = 0x0;
    IFS0bits.U1RXIF = 0x0;
    //generate interrupt when transmit buffer contains empty space
    U1STAbits.UTXISEL = 0x01;
    IEC0bits.U1TXIE = 0x1;

    IPC6bits.U1IP = 0x6;
    IPC6bits.U1IS = 0;
    //generate interrupt for receive
    U1STAbits.URXISEL = 0x0;
    IEC0bits.U1RXIE = 0x1;
    char str[MAXPAYLOADLENGTH];
    //    sprintf(str, "%cOCT: 1234556574587376345", ID_DEBUG);
    //    str[0] = ID_DEBUG;
    //    str[1] = 0x1;
    //    str[2] = 0x2;
    //    str[3] = 0x3;
    sprintf(str, "Compiled on %s at %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(str);

}
#endif

void __ISR(_UART1_VECTOR) IntUart1Handler(void) {

    if (IFS0bits.U1TXIF) {
        IFS0bits.U1TXIF = 0;
        if (myCirc.head != myCirc.tail && !block) {
            unsigned char c = myCirc.data[myCirc.head];
            myCirc.head = (myCirc.head + 1) % MAXPAYLOADLENGTH;
            U1TXREG = c;
        }


    }
    if (IFS0bits.U1RXIF) {
        IFS0bits.U1RXIF = 0;
        if (Protocol_IsQueueFull() == ERROR) {
            Protocol_RunReceiveStateMachine(U1RXREG);
        }
    }

}

/**
 * @Function Protocol_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief 
 * @author mdunne */
int Protocol_Init(void) {
    U1MODEbits.ON = 0x1;
    U1MODEbits.PDSEL = 0x0;
    //enable transmit and receive bits
    U1STAbits.URXEN = 0x1;
    U1STAbits.UTXEN = 0x1;
    //set baud rate
    U1BRG = (BOARD_GetPBClock() / (16 * 115200)) - 1;


    //clear interrupt flags
    IFS0bits.U1TXIF = 0x0;
    IFS0bits.U1RXIF = 0x0;
    //generate interrupt when transmit buffer contains empty space
    U1STAbits.UTXISEL = 0x01;
    IEC0bits.U1TXIE = 0x1;

    IPC6bits.U1IP = 0x6;
    IPC6bits.U1IS = 0;
    //generate interrupt for receive
    U1STAbits.URXISEL = 0x0;
    IEC0bits.U1RXIE = 0x1;
}

/**
 * @Function char PutChar(char ch)
 * @param ch, new char to add to the circular buffer
 * @return SUCCESS or ERROR
 * @brief adds to circular buffer if space exists, if not returns ERROR
 * @author mdunne */
int PutChar(char ch) {
    block = 1;
    if ((myCirc.head == ((myCirc.tail % MAXPAYLOADLENGTH) + 1)) || (myCirc.head == 0 && myCirc.tail == MAXPAYLOADLENGTH - 1)) {


    } else {
        myCirc.data[myCirc.tail] = ch;
        myCirc.tail = (myCirc.tail + 1) % MAXPAYLOADLENGTH;

    }

    //force interrupt
    block = 0;
    if (U1STAbits.TRMT) {
        IFS0bits.U1TXIF = 0x1;
    }

    return SUCCESS;
}

/**
 * @Function int Protocol_SendMessage(unsigned char len, void *Payload)
 * @param len, length of full <b>Payload</b> variable
 * @param Payload, pointer to data, will be copied in during the function
 * @return SUCCESS or ERROR
 * @brief 
 * @author mdunne */
int Protocol_SendMessage(unsigned char len, unsigned char ID, void *Payload) {

    unsigned char checksum;
    unsigned char eyedee = ID;
    unsigned char lenf = len;
    unsigned char* c;
    c = Payload;
    PutChar(HEAD);
    PutChar(lenf + 0x01);
    PutChar(eyedee);
    checksum = Protocol_CalcIterativeChecksum(eyedee, 0x00);
    int i;
    for (i = 0; i < lenf; i++) {
        checksum = Protocol_CalcIterativeChecksum(c[i], checksum);
        PutChar(c[i]);
    }
    PutChar(TAIL);
    PutChar(checksum);
    PutChar('\r');
    PutChar('\n');

}

/**
 * @Function char Protocol_CalcIterativeChecksum(unsigned char charIn, unsigned char curChecksum)
 * @param charIn, new char to add to the checksum
 * @param curChecksum, current checksum, most likely the last return of this function, can use 0 to reset
 * @return the new checksum value
 * @brief Returns the BSD checksum of the char stream given the curChecksum and the new char
 * @author mdunne */
unsigned char Protocol_CalcIterativeChecksum(unsigned char charIn, unsigned char curChecksum) {
    curChecksum = (curChecksum >> 1) + (curChecksum << 7);
    curChecksum += charIn;
    return curChecksum;
}

/**
 * @Function void Protocol_runReceiveStateMachine(unsigned char charIn)
 * @param charIn, next character to process
 * @return None
 * @brief Runs the protocol state machine for receiving characters, it should be called from 
 * within the interrupt and process the current character
 * @author mdunne */
void Protocol_RunReceiveStateMachine(unsigned char charIn) {
    static int i;
    static unsigned char checksum;
    static int set;
    static int get;
    static int ping;
    unsigned char c = charIn;
    if (state == WAITING) {
        if (charIn == HEAD) {
            state = RHEAD;
            i = 0;
            checksum = 0;
            set = 0;
            get = 0;
            ping = 0;
        }
    } else if (state == RHEAD) {
        inPay.data[inPay.tail].len = charIn;
        state = RLEN;
    } else if (state == RLEN) {
        if (i == 0) {
            if (charIn == ID_LEDS_SET) {
                set = 1;
            }
            if (charIn == ID_LEDS_GET) {
                get = 1;
            }
            if (charIn == ID_PING) {
                ping = 1;
            }
        }
        inPay.data[inPay.tail].payload[i] = charIn;
        checksum = Protocol_CalcIterativeChecksum(charIn, checksum);
        i++;
        if (i >= inPay.data[inPay.tail].len) {
            state = RTAIL;
        }

    } else if (state == RTAIL) {
        if (charIn == TAIL) {
            state = RCHECKSUM;
        } else {
            state = WAITING;
        }
    } else if (state == RCHECKSUM) {
        if (checksum == c) {
            if (set) {
                LEDS_SET(inPay.data[inPay.head].payload[1]);
            } else if (get) {
                inPay.data[inPay.head].payload[1] = LEDS_GET();

                Protocol_SendMessage(0x2, ID_LEDS_STATE, inPay.data[inPay.head].payload);

            } else if (ping) {
                block = 1;
                unsigned int temp = 0;
                //unsigned char pin[MAXPAYLOADLENGTH];
                temp = (inPay.data[inPay.head].payload[4]) & 0xff;
                temp |= (inPay.data[inPay.head].payload[3] & 0xff) << 8;
                temp |= (inPay.data[inPay.head].payload[2] & 0xff) << 16;
                temp |= (inPay.data[inPay.head].payload[1] & 0xff) << 24;


                //temp = Protocol_IntEndednessConversion(temp);
                temp = temp >> 0x1;
                //emp = Protocol_IntEndednessConversion(temp);
                //sprintf(pin, "%c%u", ID_PONG, temp);
                myPay.data[myPay.head].payload[4] = temp & 0xff;
                myPay.data[myPay.head].payload[3] = (temp >> 8) & 0xff;
                myPay.data[myPay.head].payload[2] = (temp >> 16) & 0xff;
                myPay.data[myPay.head].payload[1] = (temp >> 24) & 0xff;
                block = 0;
                //                Protocol_SendMessage(0x5, ID_PONG, myPay.data[myPay.head].payload);
                Protocol_SendMessage(0x5, ID_PONG, myPay.data[myPay.head].payload);

            } else {
                inPay.tail = (inPay.tail + 1) % PACKETBUFFERSIZE;
            }
            //            inPay.tail = (inPay.tail + 1) % PACKETBUFFERSIZE;
        } else {
            //report error
        }
        state = WAITING;
    }
}

/**
 * @Function int Protocol_SendDebugMessage(char *Message)
 * @param Message, Proper C string to send out
 * @return SUCCESS or ERROR
 * @brief Takes in a proper C-formatted string and sends it out using ID_DEBUG
 * @warning this takes an array, do <b>NOT</b> call sprintf as an argument.
 * @author mdunne */
int Protocol_SendDebugMessage(char *Message) {
    char len = strlen(Message);
    char checksum;
    PutChar(HEAD);
    PutChar(len + 1);
    PutChar(ID_DEBUG);
    checksum = Protocol_CalcIterativeChecksum(ID_DEBUG, 0x00);
    int i;
    for (i = 0; i < len; i++) {
        checksum = Protocol_CalcIterativeChecksum(Message[i], checksum);
        PutChar(Message[i]);
    }
    PutChar(TAIL);
    PutChar(checksum);
    PutChar('\r');
    PutChar('\n');
}

/**
 * @Function char Protocol_ShortEndednessConversion(unsigned short inVariable)
 * @param inVariable, short to convert endedness
 * @return converted short
 * @brief Converts endedness of a short. This is a bi-directional operation so only one function is needed
 * @author mdunne */
unsigned short Protocol_ShortEndednessConversion(unsigned short inVariable) {
    unsigned short holder = 0;
    holder = ((inVariable >> 8) & 0xFF);
    holder |= ((inVariable) & 0xFF) << 8;
    return holder;
}

/**
 * @Function char Protocol_IntEndednessConversion(unsigned int inVariable)
 * @param inVariable, int to convert endedness
 * @return converted short
 * @brief Converts endedness of a int. This is a bi-directional operation so only one function is needed
 * @author mdunne */
unsigned int Protocol_IntEndednessConversion(unsigned int inVariable) {
    unsigned int holder = 0;
    holder = ((inVariable >> 24) & 0xff);
    holder |= ((inVariable >> 16) & 0xFF) << 8;
    holder |= ((inVariable >> 8) & 0xFF) << 16;
    holder |= ((inVariable) & 0xFF) << 24;
    return holder;
}

/**
 * @Function unsigned char Protocol_ReadNextID(void)
 * @param None
 * @return Reads ID of next Packet
 * @brief Returns ID_INVALID if no packets are available
 * @author mdunne */
unsigned char Protocol_ReadNextID(void) {
    if (Protocol_IsMessageAvailable()) {
        return inPay.data[inPay.head].payload[0];
    }
}

/**
 * @Function int Protocol_GetPayload(void* payload)
 * @param payload, Memory location to put payload
 * @return SUCCESS or ERROR
 * @brief 
 * @author mdunne */
int Protocol_GetPayload(void* payload) {
    //unsigned char[] c;
    //*c = inPay.data[inPay.tail].payload;
    memcpy(payload, inPay.data[inPay.head].payload, sizeof (inPay.data[inPay.head].payload));
    inPay.head = (inPay.head + 1) % PACKETBUFFERSIZE;
    return SUCCESS;
}

/**
 * @Function char Protocol_IsMessageAvailable(void)
 * @param None
 * @return TRUE if Queue is not Empty
 * @brief 
 * @author mdunne */
char Protocol_IsMessageAvailable(void) {
    if (inPay.tail != inPay.head) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @Function char Protocol_IsQueueFull(void)
 * @param None
 * @return TRUE is QUEUE is Full
 * @brief 
 * @author mdunne */
char Protocol_IsQueueFull(void) {
    if ((inPay.head == ((inPay.tail % PACKETBUFFERSIZE) + 1)) || (inPay.head == 0 && inPay.tail == PACKETBUFFERSIZE - 1)) {
        return SUCCESS;
    } else {
        return ERROR;
    }
}

/**
 * @Function char Protocol_IsError(void)
 * @param None
 * @return TRUE if error
 * @brief Returns if error has occurred in processing, clears on read
 * @author mdunne */
char Protocol_IsError(void) {
    return TRUE;
}

