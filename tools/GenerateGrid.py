#!/usr/bin/python

import sys, os, subprocess, random, math, numpy, csv
from optparse import OptionParser
import sumolib


sumoConfigFormat = """<?xml version="1.0" encoding="iso-8859-1"?>
<configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://sumo.sf.net/xsd/sumoConfiguration.xsd">
    <input>
        <net-file value="{0}"/>
        <route-files value="{1}"/>
    </input>
    <time>
        <begin value="{2}"/>
        <end value="{3}"/>
        <step-length value="0.1"/>
    </time>
</configuration>
"""


launchdConfigFormat = """<?xml version="1.0"?>
<!-- debug config -->
<launch>
        <basedir path="{0}" />
        <copy file="{1}" />
        <copy file="{2}" />
        <copy file="{3}" type="config" />
</launch>
"""



def CreateVehicleDefinitionXML( vDef ):
	vDefStr = "<vType"
	for k in vDef.iterkeys():
		if k == "height" or k == "weight":
			continue	# height doesn't go into the xml file
		vDefStr = vDefStr + " " + k + "=\"" + vDef[k] + "\""
	vDefStr = vDefStr + "/>"
	return vDefStr


def LoadVehicleDefinitions( fname ):
	vehicleDefinitions = []
	with open( fname, "r" ) as f:
		r = csv.reader( f, delimiter=" " )

		for line in r:
			vehicleDef = {}
			vehicleDef["id"] 	 = line[0]
			vehicleDef["accel"]  = line[1]
			vehicleDef["decel"]  = line[2]
			vehicleDef["sigma"]  = line[3]
			vehicleDef["length"] = line[4]
			vehicleDef["color"]  = line[5]
			vehicleDef["width"]  = line[6]
			vehicleDef["height"] = line[7]
			vehicleDef["weight"] = float(line[8])
			vehicleDef["minGap"] = line[9]
			vehicleDefinitions.append( vehicleDef )

	wSum = 0
	for vDef in vehicleDefinitions:
		wSum += vDef["weight"]
	if wSum != 1:
		for vDef in vehicleDefinitions:
			vDef["weight"] /= wSum

	print "Loaded " + str(len(vehicleDefinitions)) + " vehicle definitions."
	return vehicleDefinitions

def PickRandomVehicleType( vehicleDefinitions ):
	if not vehicleDefinitions:
		return "typeWE"

	n = float( random.random() )
	v = None
	for vDef in sorted( vehicleDefinitions, key=lambda v: v["weight"] ):
		if n < vDef["weight"]:
			v = vDef
			break
		else:
			n -= vDef["weight"]
	return v["id"]


def parseOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-d",       "--directory",         dest="directory",          help="Define the maps directory.")
	optParser.add_option("-L",        "--lane.min",          dest="minLanes",            help="Minimum number of lanes.",   type = "int",   default=1 )
	optParser.add_option("-l",        "--lane.max",          dest="maxLanes",            help="Maximum number of lanes.",   type = "int",   default=4 )
	optParser.add_option("-S",         "--runTime",           dest="runTime",   help="Time taken for a car to traverse.",   type = "int", default=900 )
	optParser.add_option("-j",    "--junction.min",       dest="minJunction",        help="Minimum number of junctions.",   type = "int",   default=0 )
	optParser.add_option("-J",    "--junction.max",       dest="maxJunction",        help="Maximum number of junctions.",   type = "int",   default=5 )
	optParser.add_option("-p",      "--cornerPath",        dest="cornerPath", help="Directory of the CORNER classifier." )
	optParser.add_option("-c",     "--clusterPath",       dest="clusterPath",  help="Directory of the ClusterLib tools." )
	optParser.add_option("-C",    "--cbdRoutePath",      dest="cbdRoutePath",    help="Directory of the CBDRouter tool." )
	optParser.add_option("-V",    "--vehicleTypes",      dest="vehicleTypes",       help="Vehicle type definition file." )
	optParser.add_option("-R",    "--route-number",       dest="routeNumber",       help="Number of routes to generate.",   type = "int", default=250 )
	optParser.add_option("-t",    "--maximum-time",           dest="maxTime",   help="The time limit of the simulation.",   type = "int", default=2000 )
	optParser.add_option("-D",      "--do-reverse",         dest="doReverse",     help="Create a reverse of each route.",  default=False, action='store_true' )
	optParser.add_option("-H",         "--highway",           dest="highway",                       help="Do a highway.",  default=False, action='store_true' )
	optParser.add_option("-a",       "--speed.min",          dest="minSpeed",       help="Minimum Highway speed (km/h).",  type = "int", default=40 )
	optParser.add_option("-A",       "--speed.max",          dest="maxSpeed",       help="Maximum Highway speed (km/h).",  type = "int", default=120 )
	optParser.add_option("-f",      "--speed.step",         dest="stepSpeed",          help="Highway speed step (km/h).",  type = "int", default=20 )
	optParser.add_option("-v",    "--minCarNumber",  dest="minVehicleNumber",   help="Minimum car density per tx range.",  type = "float", default=1 )
	optParser.add_option("-y",    "--maxCarNumber",  dest="maxVehicleNumber",   help="Maximum car density per tx range.",  type = "float", default=10 )
	optParser.add_option("-Y",   "--stepCarNumber", dest="stepVehicleNumber",                  help="Vehicle increment.",  type = "float", default=1 )
	optParser.add_option("-m",    "--speedvar.min",  dest="minSpeedVariance",          help="Minimum variance in speed.",  type = "float", default=0 )
	optParser.add_option("-M",    "--speedvar.max",  dest="maxSpeedVariance",          help="Maximum variance in speed.",  type = "float", default=0.2 )
	optParser.add_option("-Z",   "--speedvar.step", dest="stepSpeedVariance",                help="Speed variance step.",  type = "float", default=0.05 )
	optParser.add_option("-r",   "--transmitRange",     dest="transmitRange",         help="Vehicle transmission range.",  type = "float", default=100 )
	optParser.add_option("-w",       "--laneWidth",         dest="laneWidth",                         help="Lane width.",  type = "float", default=2.5 )
	optParser.add_option("-b", "--turnProbability",   dest="turnProbability",       help="Probability of a car turning.",  type = "float", default=0.5 )
	optParser.add_option("-B",      "--filePrefix",        dest="filePrefix",              help="Generated file prefix.", type = "string" )
	optParser.add_option("-q",      "--randomSeed",        dest="randomSeed",                 help="Random seed to set.", type = "int" )
	(options, args) = optParser.parse_args(argv)

	if not options.directory:
		print "Please specify results directory."
		optParser.print_help()
		sys.exit()

	if options.randomSeed:
		random.seed( options.randomSeed )

	return options




def generateGrids( options ):
	fileList = []
	for laneCount in range( options.minLanes, options.maxLanes+1, options.stepLanes ):
		for roadLength in range( options.minRoads, options.maxRoads+1, options.stepRoads ):
			filename = "grid-" + str(options.junctionCount) + "x" + str(options.junctionCount) + "-" + str(laneCount) + "lane-" + str(roadLength) + "m"
			print "Generating '" + filename + "'..."
			p = subprocess.Popen( ['netgen','-g','--grid.number',str(options.junctionCount),'--grid.length',str(roadLength),'-L',str(laneCount),'--no-internal-links','--tls.guess','-o',filename+".net.xml"] )
			p.wait()

			cbdFile = filename + '.cbd'
			with open(cbdFile,'w') as f:
				f.write("0,0,"+str(roadLength)+"\n")
				w = str((options.junctionCount-1)*roadLength)
				f.write(w+","+w+","+str(roadLength)+"\n")

			fileList.append( [ filename, laneCount, roadLength ] )

	return fileList



def generateHighway( junctionCount, roadLength, laneCount, speed, filename ):
	# First create node file
	with open( filename+".nod.xml", "w" ) as f:
		f.write("<?xml version=\"1.0\"?>\n")
		f.write("<nodes>\n")
		for c in range(0,junctionCount+2):
			f.write("\t<node id=\"" + str(c) + "\" x=\"" + str(c*roadLength) + "\" y=\"100.0\" type=\"unregulated\" />\n")
			if c > 0 and c < junctionCount+1:
				f.write("\t<node id=\"" + str(c) + "_up\" x=\"" + str(c*roadLength) + "\" y=\"0.0\" type=\"unregulated\" />\n")
				f.write("\t<node id=\"" + str(c) + "_down\" x=\"" + str(c*roadLength) + "\" y=\"200.0\" type=\"unregulated\" />\n")
		f.write("</nodes>\n")

	# Next create edge file
	with open( filename+".edg.xml", "w" ) as f:
		f.write("<?xml version=\"1.0\"?>\n")
		f.write("<edges>\n")
		for c in range(0,junctionCount+1):
			f.write("\t<edge id=\"" + str(c) + "_" + str(c+1) + "\" from=\"" + str(c) + "\" to=\"" + str(c+1) + "\" priority=\"1\" numLanes=\"" + str(laneCount) + "\" speed=\"" + str(speed) + "\" />\n")
			if c > 0 and c < junctionCount+1:
				l = laneCount / 2
				if l == 0:
					l = 1
				f.write("\t<edge id=\"" + str(c) + "_goup" + "\" from=\"" + str(c) + "\" to=\"" + str(c) + "_up\" priority=\"1\" numLanes=\"" + str(l) + "\" speed=\"" + str(speed) + "\" />\n")
				f.write("\t<edge id=\"" + str(c) + "_godown" + "\" from=\"" + str(c) + "\" to=\"" + str(c) + "_down\" priority=\"1\" numLanes=\"" + str(l) + "\" speed=\"" + str(speed) + "\" />\n")
		f.write("</edges>\n")

	# Convert the files to net.xml
	p = subprocess.Popen( ['netconvert','-n',filename+'.nod.xml','-e',filename+'.edg.xml','-o',filename+'.net.xml','--no-internal-links'] )
	p.wait()

	# Remove the temp files.
	os.remove( filename+".nod.xml" )
	os.remove( filename+".edg.xml" )
	return filename+".net.xml"



def generateHighways( options ):
	fileList = []
	for laneCount in range( options.minLanes, options.maxLanes+1, 1 ):
		for junctionCount in range( options.minJunction, options.maxJunction+1, 1 ):
			for speed in range( options.minSpeed, options.maxSpeed+1, options.stepSpeed ):
				for var in numpy.arange( options.minSpeedVariance, options.maxSpeedVariance+options.stepSpeedVariance/2, options.stepSpeedVariance ):
					roadLength = options.runTime * speed / ( 3.6 * ( junctionCount + 1 ) )
					if options.filePrefix:
						filename = options.filePrefix
					else:
						filename = "highway-" + str(junctionCount) + "-" + str(laneCount) + "lane-" + str(speed) + "kmph"
					print "Generating '" + filename + "'..."
					generateHighway( junctionCount, roadLength, laneCount, speed / 3.6, filename )
					fileList.append( [filename, laneCount, roadLength, speed, junctionCount, str(var)] )

	return fileList



def generateHighwayRoutes( filename, roadLength, vehicleRate, junctionCount, laneCount, speed, speedVar, options ):
	# Load vehicle types
	vTypes = LoadVehicleDefinitions( options.vehicleTypes )

	carRate = 2 * speed * vehicleRate / ( 3.6 * options.transmitRange * laneCount )

	if options.filePrefix:
		tempFileName = options.filePrefix + ".trip"
	else:
		tempFileName = filename + "-" + str(vehicleRate) + ".trip"

	# Load the highway maps
	net = sumolib.net.readNet(filename+'.net.xml')

	destinationLookup = {}

	with open( tempFileName, "w" ) as f:
		f.write( '<?xml version="1.0"?>\n' )

		f.write( '<trips>\n' )

		# Write the definitions
		for t in vTypes:
			t['speedDev'] = speedVar
			f.write( "\t" + CreateVehicleDefinitionXML( t ) + '\n' )

		genPeriod = math.ceil( 1/carRate )
		numGen = 1
		if genPeriod <= 1:
			genPeriod = 1
			numGen = int( math.ceil( carRate ) )
		
		lastGen = -genPeriod
		ID = 0
		for t in range(0,options.maxTime):
			if ( t - lastGen ) == genPeriod:
				for n in range(0,numGen):
					turnOff = junctionCount > 0
					sinkIndex = None
					if turnOff:
						for n in range(1,junctionCount+1):
							if numpy.random.random() < options.turnProbability:
								sinkIndex = n
								break
						if not sinkIndex:
							turnOff = False

					if turnOff:
						sinkEdge = str(sinkIndex)
						if random.randint(0,1) == 1:
							sinkEdge += "_goup"
						else:
							sinkEdge += "_godown"
					else:
						sinkEdge = str(junctionCount) + "_" + str(junctionCount+1)
					ID += 1
					destinationLookup["car_"+str(ID)] = [ options.maxTime ] + net.getEdge( sinkEdge )._to._coord
					f.write( '<trip id="car_{0}" depart="{1}" from="0_1" to="{2}" departLane="free" type="{3}" departSpeed="max" />\n'.format( ID, t, sinkEdge, PickRandomVehicleType(vTypes) ) )
				lastGen = t
		f.write( '</trips>\n' )

	if options.filePrefix:
		outputFilename = options.filePrefix + ".rou.xml"
	else:
		outputFilename = filename + "-" + str(vehicleRate) + "cars.rou.xml"
	p = subprocess.Popen( ['duarouter','-n',filename+'.net.xml','-t',tempFileName,'-o',outputFilename] )
	p.wait()
	os.remove(tempFileName)

	# Dump a destination lookup for each node.
	with open(outputFilename+".dest","w") as f:
		f.write( str( len( destinationLookup ) ) + "\n" )
		for car in destinationLookup:
			f.write( car + " 1\n" )
			f.write( str( destinationLookup[car][0] ) + " " + str( destinationLookup[car][1] ) + " " + str( destinationLookup[car][2] ) + "\n" )


def analyseFiles( fileList, options ):
	for f in fileList:
		netFile = f[0]+'.net.xml'
		print "Running CORNER on '" + f[0] + "'..."
		p = subprocess.Popen( ['python',options.cornerPath+'/Sumo2Corner.py','-n',netFile] )
		p.wait()
		print "Running LSUF on '" + f[0] + "'..."
		p = subprocess.Popen( ['python',options.clusterPath+'/LaneWeight.py','-n',netFile] )
		p.wait()

		for carDensity in numpy.arange( options.minVehicleNumber, options.maxVehicleNumber+options.stepVehicleNumber, options.stepVehicleNumber ): 
			if options.filePrefix:
				baseFile = options.filePrefix
			else:
				baseFile = f[0] + "-" + str(carDensity) + "cars"
			rouFile = baseFile + ".rou.xml"
			print "Generating route file '" + rouFile + "'..."

			if options.highway:
				generateHighwayRoutes( f[0], f[2], carDensity, f[4], f[1], f[3], f[5], options )

			else:
				genCmd = ['python',options.cbdRoutePath+'/CBDRouter.py']
				genCmd += ['-n',netFile]
				genCmd += ['-r',rouFile]
				genCmd += ['-V',options.vehicleTypes]
				genCmd += ['-v',str(carDensity)]
				genCmd += ['-R',str(options.routeNumber)]
				genCmd += ['-C',f[0]+'.cbd']
				genCmd += ['-t',str(options.maxTime)]

				if options.doReverse:
					genCmd += ['-d']
				p = subprocess.Popen( genCmd )
				p.wait()

			with open( baseFile+".sumo.cfg", "w" ) as fout:
				fout.write( sumoConfigFormat.format( netFile, rouFile, 0, options.maxTime ) )
			with open( baseFile+".launchd.xml", "w" ) as fout:
				fout.write( launchdConfigFormat.format( options.directory, netFile, rouFile, baseFile+".sumo.cfg" ) )


if __name__ == "__main__":
	options = parseOptions( sys.argv )
	os.chdir( options.directory )
	if options.highway:
		analyseFiles( generateHighways( options ), options )
	else:
		analyseFiles( generateGrids( options ), options )

