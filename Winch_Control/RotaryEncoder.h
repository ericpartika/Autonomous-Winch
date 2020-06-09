

#ifndef ROTARYENCODER_H
#define	ROTARYENCODER_H

/*******************************************************************************
 * PUBLIC #DEFINES                                                            *
 ******************************************************************************/

#define ENCODER_SPI_MODE 0
#define ENCODER_IC_MODE 1

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                           *
 ******************************************************************************/

/**
 * @Function RCServo_Init(char interfaceMode)
 * @param interfaceMode, one of the two #defines determining the interface
 * @return SUCCESS or ERROR
 * @brief initializes hardware in appropriate mode along with the needed interrupts */
int RotaryEncoder_Init(char interfaceMode);

/**
 * @Function int RotaryEncoder_ReadRawAngle(void)
 * @param None
 * @return 14-bit number representing the raw encoder angle (0-16384) */
unsigned short RotaryEncoder_ReadRawAngle(void);

int Accumulated_angle(int current);


#endif	/* ROTARYENCODER_H */

