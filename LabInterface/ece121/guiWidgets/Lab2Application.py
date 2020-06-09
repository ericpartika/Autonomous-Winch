from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
import PyQt5
import struct

from ece121 import Protocol

widgetName = "Lab 2 Application"

statusDict = {0b10: "In Range", 0b100: "Out Negative Range", 0b1: "Out Positive Range"}

class Lab2Application(QWidget):
	signal = pyqtSignal(tuple)
	def __init__(self, portInstance, parent=None):
		super(Lab2Application, self).__init__(parent)

		self.portInstance = portInstance

		self.usedLayout = QVBoxLayout()
		self.setLayout(self.usedLayout)

		# self.usedLayout.addWidget(QLabel("Hello Widget"))

		compression = QHBoxLayout()
		compression.addWidget(QLabel("Current Angle (millidegrees): "))
		self.usedLayout.addLayout(compression)
		self.milliDegreeReading = QLabel("0")
		compression.addWidget(self.milliDegreeReading)
		compression.addStretch()

		compression = QHBoxLayout()
		compression.addWidget(QLabel("Current Angle (degrees): "))
		self.usedLayout.addLayout(compression)
		self.DegreeReading = QLabel("0.0")
		compression.addWidget(self.DegreeReading)
		compression.addStretch()

		compression = QHBoxLayout()
		compression.addWidget(QLabel("Status: "))
		self.usedLayout.addLayout(compression)
		self.statusMessage = QLabel(statusDict[0b10])
		compression.addWidget(self.statusMessage)
		compression.addStretch()
		self.signal.connect(self.updateGui)

		# self.angleReading = QLabel()
		# self.usedLayout.addWidget(self.angleReading)

		compression = QHBoxLayout()
		self.angleDisplay = QDial()
		self.angleDisplay.setNotchesVisible(True)
		self.angleDisplay.setWrapping(True)
		self.angleDisplay.setRange(-1800,1800)
		# self.angleDisplay.setNotchTarget()

		compression.addWidget(self.angleDisplay)
		self.usedLayout.addLayout(compression)
		compression.addStretch()


		self.portInstance.registerMessageHandler(Protocol.MessageIDs.ID_ENCODER_REPORT, self.RotaryUpdates)

		# self.usedLayout.addSpacerItem(QSpacerItem())
		self.usedLayout.addStretch()
		return

	def RotaryUpdates(self, inBytes):
		# print(inBytes)
		# print(inBytes[1:].hex())
		number = struct.unpack("!iB", inBytes[1:])
		# print(number)
		self.signal.emit(number)
		return

	def updateGui(self, newNumber):
		# print('woo', newNumber)
		self.milliDegreeReading.setText(str(newNumber[0]))
		self.DegreeReading.setText("{:.2f}".format(newNumber[0]/1000))
		# self.angleReading.setText("{}".format((newNumber/16535)*360))
		self.angleDisplay.setValue(int(newNumber[0]/100))
		self.statusMessage.setText(statusDict[newNumber[1]])
		return


