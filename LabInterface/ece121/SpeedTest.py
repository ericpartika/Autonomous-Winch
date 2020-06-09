from ece121 import Protocol
import time

startNumber = 0

def updateLEDs(currentNumber):
	global startNumber
	messageOut = Protocol.MessageIDs.ID_LEDS_SET.value.to_bytes(1, byteorder='big')
	messageOut += startNumber.to_bytes(1, byteorder='big')
	startNumber = (startNumber+1) % 256
	print(startNumber)
	protInstance.sendRawMessage(messageOut)
	protInstance.requestLEDState()
	return


protInstance = Protocol.Protocol()

protInstance.registerHandler(Protocol.MessageIDs.ID_LEDS_STATE, updateLEDs)

protInstance.requestLEDState()

while True:
	time.sleep(1)

