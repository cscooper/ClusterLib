#!/usr/bin/python

import os, numpy
from VectorMath import *
import subprocess

class VersionException(Exception):
	def __init__(self,receivedVersionNumber):
		Exception.__init__(self,"Invalid result file version")
		self.receivedVersionNumber = receivedVersionNumber

class Scalar:
	def __init__(self,name,moduleName,value):
		self.name = name
		self.moduleName = moduleName
		self.value = value
		self.attributes = {}

class Statistic:
	def __init__(self,name,moduleName):
		self.name = name
		self.moduleName = moduleName
		self.fields = {}
		self.attributes = {}

class Vector:
	def __init__(self,vectorId,name,moduleName,paramOrder,parentRun):
		self.vectorId = vectorId
		self.name = name
		self.moduleName = moduleName
		self.paramOrder = paramOrder
		self.indexData = []
		self.attributes = {}
		self.parentRun = parentRun

	def addIndexData( self, blockOffset, blockLength ):
		self.indexData.append( [ blockOffset, blockLength ] )

	def getVectorData(self):
		vectorData = []
		fileName = self.parentRun.directory+"/"+self.parentRun.configName+"-"+str(self.parentRun.runId)+".vec"
		f = open(fileName, "r")
		for i in sorted(self.indexData, key=lambda tup: tup[0]):
			f.seek(i[0],0)
			data = f.read(i[1])
			for line in data.split("\n"):
				l = line.split()
				if len(l) == 0:
					continue
				d = [0,0,0]
				for i in range(0,3):
					try:
						if self.paramOrder[i] == 'E':
							d[0] = int(l[i+1])
						elif self.paramOrder[i] == 'T':
							d[1] = float(l[i+1])
						elif self.paramOrder[i] == 'V':
							d[2] = float(l[i+1])
					except IndexError:
						print l
						raise IndexError
				vectorData.append(d)
		return numpy.array( vectorData )

class Run:
	def __init__(self,runId,configName,directory):
		self.runId = runId
		self.configName = configName
		self.directory = directory
		self.loadScalars()
		self.loadVectors()

	def loadScalars(self):
		foundRun = False
		lastData = None
		lastScalar = None
	
		self.scalars = {} # module names are used as keys here
		self.statistics = {}
		self.scalarIndices = []
		self.statisticsIndices = []
		self.runAttributes = {}

		fileName = self.directory+"/"+self.configName+"-"+str(self.runId)+".sca"
		lineIndex = 0
		with open(fileName, "r") as f:
			for line in f:
				try:
					# parse the current line
					lineIndex += 1
					data = line.split()
					if len(data) == 0:
						continue
					if data[0] == "version":
						self.version = int(data[1])
						if self.version != 2:
							raise VersionException(self.version)

					elif data[0] == "run":
						if foundRun:
							raise Exception("Run defined multiple times in '"+fileName+"'")
						foundRun = True
						lastData = "run"

					elif data[0] == "attr":
						if data[1] == '""':
							continue # Skip empties
						if lastData == "run":
							attrStr = ""
							for a in data[2:len(data)]:
								attrStr += a
							attrStr = attrStr.translate(None,'"\\')
							self.runAttributes[data[1]] = attrStr
						elif lastData == "scalar":
							self.scalars[lastScalar.moduleName][lastScalar.name].attributes[data[1]] = data[2]
						elif lastData == "statistic":
							self.statistics[lastScalar.moduleName][lastScalar.name].attributes[data[1]] = data[2]
						else:
							raise Exception("Unknown attribute '"+data[1]+"' at line "+str(lineIndex)+" in '"+fileName+"'")
					elif data[0] == "scalar":
						moduleName = data[1]
						scalarName = data[2].split(":")[0]
						scalarValue = float(data[3])
						if not moduleName in self.scalars.keys():
							self.scalars[moduleName] = {}
						else:
							if scalarName in self.scalars[moduleName].keys():
								raise Exception("Scalar '"+moduleName+"/"+scalarName+"' redefined at line "+str(lineIndex)+" in '"+fileName+"'")
						self.scalars[moduleName][scalarName] = Scalar(scalarName,moduleName,scalarValue)
						lastScalar = self.scalars[moduleName][scalarName]
						lastData = "scalar"
						self.scalarIndices.append( [moduleName, scalarName] )
					elif data[0] == "statistic":
						moduleName = data[1]
						statisticName = data[2].split(":")[0]
						if not moduleName in self.statistics.keys():
							self.statistics[moduleName] = {}
						else:
							if statisticName in self.statistics[moduleName].keys():
								raise Exception("Statistic '"+moduleName+"/"+scalarName+"' redefined at line "+str(lineIndex)+" in '"+fileName+"'")
						self.statistics[moduleName][statisticName] = Statistic(statisticName,moduleName)
						lastScalar = self.statistics[moduleName][statisticName]
						lastData = "statistic"
						self.statisticsIndices.append( [moduleName, statisticName] )
					elif data[0] == "field":
						if lastData is not "statistic":
							raise Exception("Found field '" + data[1] + "' without statistic at line "+str(lineIndex)+" in '"+fileName+"'")
						if data[1] in self.statistics[lastScalar.moduleName][lastScalar.name].fields:
							raise Exception("Statistic field '" + data[1] + "' redefined at line "+str(lineIndex)+" in '"+fileName+"'")
						self.statistics[lastScalar.moduleName][lastScalar.name].fields[data[1]] = float(data[2])

				except IndexError:
					print "IndexError exception raised at line " + str(lineIndex) + " in file " + fileName + " (" + line + ")"

	def loadVectors(self):
		foundRun = False
		lastData = None
		lastVector = None
	
		self.vectors = {} # module names are used as keys here
		self.vectorIndices = {} # contains tuples of moduleName,vectorName. VectorId used as keys

		fileName = self.directory+"/"+self.configName+"-"+str(self.runId)+".vci"
		lineIndex = 0
		with open(fileName, "r") as f:
			for line in f:
				try:
					lineIndex += 1
					# parse the current line
					data = line.split()

					if len(data) == 0:
						continue
					if data[0] == "version":
						self.version = int(data[1])
						if self.version != 2:
							raise VersionException(self.version)

					elif data[0] == "run":
						if foundRun:
							raise Exception("Run defined multiple times in '"+fileName+"'")
						foundRun = True
						lastData = "run"

					elif data[0] == "attr":
						if data[1] == '""':
							continue # Skip empties
						if lastData == "run":
							pass
						elif lastData == "vector":
							self.vectors[lastVector.moduleName][lastVector.name].attributes[data[1]] = data[2]
						else:
							raise Exception("Unknown attribute '"+data[1]+"' at line "+str(lineIndex)+" in '"+fileName+"'")

					elif data[0] == "vector":
						vectorNumber = int(data[1])
						vectorModule = data[2]
						vectorName = data[3]
						if not vectorModule in self.vectors.keys():
							self.vectors[vectorModule] = {}
						else:
							if vectorName in self.vectors[vectorModule].keys():
								raise Exception("Vector '"+vectorModule+"/"+vectorName+"' redefined at line "+str(lineIndex)+" in '"+fileName+"'")

						self.vectors[vectorModule][vectorName] = Vector( vectorNumber, vectorName, vectorModule, data[4], self )
						lastVector = self.vectors[vectorModule][vectorName]
						lastData = "vector"
						self.vectorIndices[vectorNumber] = (vectorModule,vectorName)

					elif data[0].isdigit():
						vectorNumber = int(data[0])
						v = self.vectorIndices[vectorNumber]
						blockOffset = int(data[1])
						blockLength = int(data[2])
						self.vectors[v[0]][v[1]].addIndexData( blockOffset, blockLength )

				except IndexError:
					print "IndexError exception raised at line " + str(lineIndex) + " in file " + fileName


class DataContainer:

	def __init__(self,configName,directory,useTar=False):
		self.useTar = useTar
		self.configName = configName
		self.directory = directory
		self.loadRuns()	
		self.currentRun = None

	def loadRuns(self):
		# Get all the run numbers
		if not self.useTar:
			self.runList = [ int( file[file.rfind("-")+1:file.rfind(".")] ) for file in os.listdir(self.directory) if self.configName in file and "sca" in file ]
		else:
			self.runList = [ int( file[file.find("-")+1:file.find(".tar")] ) for file in os.listdir(self.directory) if self.configName in file and "tar" in file ]

		# get list of files matching the config name
		#fileList = [ file for file in os.listdir(self.directory) if self.configName in file and "sca" in file ]
		#print self.configName + ": " + str(len(fileList)) + " runs found."
		#self.runs = {}
		#for f in fileList:
			#runId = int( f[f.rfind("-")+1:f.rfind(".")] )
			#self.runs[runId] = Run(runId,self.configName,self.directory)

	def getRunList(self):
		return self.runList

	def selectRun(self,runNumber):
		loc = self.directory
		if self.useTar:
			subprocess.Popen( ['tar','-xf',self.directory+self.configName+'-'+str(runNumber)+'.tar.xz'] ).wait()
			loc = "results"
		hasError = False
		try:
			self.currentRun = Run(runNumber,self.configName,loc)
		except ValueError:
			hasError = True
		if self.useTar:
			subprocess.Popen( ['rm','-r','results'], stdout=subprocess.PIPE, stderr=subprocess.PIPE ).wait()
		if hasError:
			raise Exception( "Run data has errors." )
		#self.currentRun = runNumber

	def getSelectedRun(self):
		return self.currentRun
		#return self.runs[self.currentRun]

	def findModule(self,moduleName):
		# this tries to locate an entry in the data container corresponding to the module name
		return [mod for mod in self.currentRun.scalars.iterkeys() if moduleName in mod]

	def getRunAttributes(self):
		if self.currentRun == None:
			raise Exception( "Asked for run attributes when no run has been selected." )
		return self.currentRun.runAttributes

	def getVectorList(self):
		if self.currentRun == None:
			raise Exception( "Asked for vector list when no run has been selected." )
		return self.currentRun.vectorIndices.values()

	def getVector(self,moduleName,vectorName):
		if self.currentRun == None:
			raise Exception( "Asked for vector data when no run has been selected." )
		return self.currentRun.vectors[moduleName][vectorName].getVectorData()

	def getScalarList(self):
		if self.currentRun == None:
			raise Exception( "Asked for scalar list when no run has been selected." )
		return self.currentRun.scalarIndices

	def getScalar(self,moduleName,scalarName):
		if self.currentRun == None:
			raise Exception( "Asked for scalar data when no run has been selected." )
		return self.currentRun.scalars[moduleName][scalarName]

	def getStatisticsList(self):
		if self.currentRun == None:
			raise Exception( "Asked for statistics list when no run has been selected." )
		return self.currentRun.statisticsIndices

	def getStatistic(self,moduleName,statisticName):
		if self.currentRun == None:
			raise Exception( "Asked for statistics data when no run has been selected." )
		return self.currentRun.statistics[moduleName][statisticName]


def FindNearestTimeInVector(time,vec,lastIndex):
	diffMin = numpy.inf
	index = -1
	for i in range(lastIndex,len(vec)):
		d = numpy.abs(vec[i,1] - time)
		if d < diffMin:
			diffMin = d
			index = i
		else:
			break
	return index


class kReader:
	def __init__(self,directory,granularity,fileName):
		# this reads all the results in the experiment and puts it into a tuppled database
		self.granularity = granularity
		self.baseDirectory = directory
		self.resultDirectory = self.baseDirectory + "/simulations/results/"
		self.ScanExperiments()
		self.powerResults = {}
		for location in self.locations.iterkeys():
			self.powerResults[location] = {"staticK":{},"raytracing":{},"raytracing-cars":{}}
			self.Analyse(location)
		self.SaveToFile(fileName)

	def FindNearestTimeInVector(self,time,vec,lastIndex):
		diffMin = numpy.inf
		index = -1
		for i in range(lastIndex,len(vec)):
			d = numpy.abs(vec[i,1] - time)
			if d < diffMin:
				diffMin = d
				index = i
		return index

	def ScanExperiments(self):
		# first get all files in the directory
		fileList = [ file for file in os.listdir(self.resultDirectory) if "sca" in file ]
		# then construct data container for each configuration
		configs = []
		self.locations = {}
		for f in fileList:
			c = f[0:f.rfind("-")]
			if not c in configs:
				configs.append(c)

		for c in configs:
			container = DataContainer( c, self.resultDirectory )
			location = c.split("-")[1]
			if location not in self.locations.keys():
				self.locations[location] = []
			self.locations[location].append( container )

	def Analyse(self,locationName):
		for location in self.locations[locationName]:
			runs = location.getRunList()
			for run in runs:
				location.selectRun(run)
				if location.getScalar("rsu_scenario.node[0].nic.phy","useRaytracer") == 1:
					expType = "raytracing"
					if location.getScalar("rsu_scenario.node[0].nic.phy","useVehicles") == 1:
						expType = "raytracing-cars"
					expParam = float(location.getScalar("rsu_scenario.node[0].nic.phy","rayCount")[0])
				else:
					expType = "staticK"
					expParam = float(location.getScalar("rsu_scenario.node[0].nic.phy","constantK")[0])

				if not expParam in self.powerResults[locationName][expType]:
					self.powerResults[locationName][expType][expParam] = {}

				# Get receiver position for this run
				rxPosStr = location.getSelectedRun().runAttributes["iterationvars"]
				elems = rxPosStr.split(",")
				if "posX" in elems[0]:
					rxPosX = float(elems[0].split("=")[1])
					rxPosY = float(elems[1].split("=")[1])
				elif "posX" in elems[1]:
					rxPosX = float(elems[1].split("=")[1])
					rxPosY = float(elems[2].split("=")[1])
				else:
					raise Exception("Invalid run attribute 'iterationvars'")
				rxPos = ((round(rxPosX/self.granularity)*self.granularity), (round(rxPosY/self.granularity)*self.granularity))

				if not rxPos in self.powerResults[locationName][expType][expParam]:
					self.powerResults[locationName][expType][expParam][rxPos] = {}

				# Get transmitter positions
				vecTxPosX = location.getVector("rsu_scenario.receiver.appl","positionX")
				vecTxPosY = location.getVector("rsu_scenario.receiver.appl","positionY")
				rxPower = location.getVector("rsu_scenario.receiver.nic","RcvPower")
				lastTimePos = 0
				print "Processing...",
				for i in range(0,len(vecTxPosX)):
					currPos = ((round(vecTxPosX[i,2]/self.granularity)*self.granularity), (round(vecTxPosY[i,2]/self.granularity)*self.granularity))
					if not currPos in self.powerResults[locationName][expType][expParam][rxPos]:
						self.powerResults[locationName][expType][expParam][rxPos][currPos] = []
					nearIndex = self.FindNearestTimeInVector(vecTxPosX[i,1],rxPower,lastTimePos)
					self.powerResults[locationName][expType][expParam][rxPos][currPos] += list(rxPower[lastTimePos:nearIndex,2])
					lastTimePos = nearIndex

					percentComplete = 100*i/float(len(vecTxPosX))
					print "\rProcessing " + locationName + "-" + expType + "-" + str(run) + "... (%.2f percent complete)" % percentComplete,

	def SaveToFile(self,fileName):
		for locationName in self.locations.iterkeys():
			f=open(locationName+"-"+fileName, 'w')
			pickle.dump(self.powerResults[locationName], f)
			f.close()




