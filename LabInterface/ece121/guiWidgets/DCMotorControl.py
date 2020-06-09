from sys import byteorder

from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
import PyQt5
from functools import partial
from ece121 import Protocol
import struct


widgetName = "DC Motor Control and Rate"

titleFontSize = 20

class DCMotorControl(QWidget):
	signal = pyqtSignal(int)
	def __init__(self, portInstance, parent=None):
		super(DCMotorControl, self).__init__(parent)

		self.portInstance = portInstance

		self.usedLayout = QVBoxLayout()
		self.setLayout(self.usedLayout)

		sectionLabel = QLabel("Depth Control")
		currentFont = sectionLabel.font()
		currentFont.setPointSize(titleFontSize)
		sectionLabel.setFont(currentFont)
		self.usedLayout.addWidget(sectionLabel)

		self.CurrentMotorSpeed = 0

		self.usedLayout.addWidget(QLabel("Commanded Depth"))
		self.PWMValue = QLabel()
		self.usedLayout.addWidget(self.PWMValue)
		self.PWMValue.setText(str(self.CurrentMotorSpeed))
		# self.signal.connect(self.updateGui)

		self.SpeedDisplay = QSlider(Qt.Horizontal)
		self.SpeedDisplay.setValue(self.CurrentMotorSpeed)
		self.SpeedDisplay.setRange(0,1000)
		self.SpeedDisplay.setTickPosition(2)
		self.SpeedDisplay.setTickInterval(25)
		self.SpeedDisplay.setSingleStep(25)
		# self.SpeedDisplay.setTracking(False)
		self.SpeedDisplay.valueChanged.connect(self.sliderChanged)

		self.usedLayout.addWidget(self.SpeedDisplay)

		buttonLayout = QHBoxLayout()
		self.usedLayout.addLayout(buttonLayout)

		# fullRev = QPushButton("Full Reverse")
		# buttonLayout.addWidget(fullRev)
		# fullRev.clicked.connect(partial(self.sliderChanged, 0, True))

		speedsWanted = [10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]

		# fullRev = QPushButton("Reel Up")
		# buttonLayout.addWidget(fullRev)
		# fullRev.clicked.connect(partial(self.sliderChanged, 0, True))

		# for speed in speedsWanted:
		# 	newSpeed = speed*-1
		# 	speedButton = QPushButton("{}".format(newSpeed))
		# 	buttonLayout.addWidget(speedButton)
		# 	speedButton.clicked.connect(partial(self.speedByChange, newSpeed))


		for speed in reversed(speedsWanted):
			newSpeed = speed
			speedButton = QPushButton("{}".format(newSpeed))
			buttonLayout.addWidget(speedButton)
			speedButton.clicked.connect(partial(self.speedByChange, newSpeed * 100))

		# fullRev = QPushButton("Full Reel Out")
		# buttonLayout.addWidget(fullRev)
		# fullRev.clicked.connect(partial(self.sliderChanged, 1000, True))

		line = QFrame()
		line.setFrameShape(QFrame.HLine)
		line.setFrameShadow(QFrame.Sunken)
		self.usedLayout.addWidget(line)

		sectionLabel = QLabel("Depth")
		currentFont = sectionLabel.font()
		currentFont.setPointSize(titleFontSize)
		sectionLabel.setFont(currentFont)
		self.usedLayout.addWidget(sectionLabel)

		compression = QHBoxLayout()
		self.usedLayout.addLayout(compression)
		compression.addWidget(QLabel("Depth:"))
		self.rateValue = QLabel("N/A")
		compression.addWidget(self.rateValue)
		compression.addStretch()


		self.rateHistory = list()
		compression = QHBoxLayout()
		self.usedLayout.addLayout(compression)
		compression.addWidget(QLabel("Average Depth:"))
		self.averateRate = QLabel("N/A")
		compression.addWidget(self.averateRate)
		compression.addStretch()

		self.portInstance.registerMessageHandler(Protocol.MessageIDs.ID_REPORT_RATE, self.rateChange)

		self.usedLayout.addStretch()
		return

	def rateChange(self, newBytes):
		# print(newBytes)
		payload = newBytes[1:]
		newRate = struct.unpack("!i", payload)[0]
		self.rateHistory.append(newRate)
		if len(self.rateHistory) > 20:
			self.rateHistory.pop(0)
		mean = int(sum(self.rateHistory)/len(self.rateHistory))
		# if abs(newRate) < 8000:
		self.rateValue.setText(str(newRate/10000))
		self.averateRate.setText(str(mean/10000))
		return

	def sliderChanged(self, newValue, overWrite=False):

		newValue = max(0,min(1000, newValue))
		if newValue != self.CurrentMotorSpeed or overWrite is True:
			if abs(newValue - self.CurrentMotorSpeed) > 50 or overWrite is True:
				self.SpeedDisplay.setSliderPosition(newValue)
				# print(newValue)
				self.CurrentMotorSpeed = newValue
				self.PWMValue.setText(str(self.CurrentMotorSpeed//100))
				# message = Protocol.MessageIDs.ID_COMMAND_OPEN_MOTOR_SPEED.value.to_bytes(1, byteorder='big')
				# message += self.CurrentMotorSpeed.to_bytes(4,byteorder='little', signed=True)
				self.portInstance.sendMessage(Protocol.MessageIDs.ID_COMMAND_OPEN_MOTOR_SPEED, struct.pack("!i", newValue))
				# self.portInstance.sendRawMessage(message)

		return

	def speedByChange(self, change):
		self.sliderChanged(change, True)
		return