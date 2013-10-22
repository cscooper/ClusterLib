#!/usr/bin/python

import sys, os, subprocess, random, math, numpy
from optparse import OptionParser


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


def parseOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-d",     "--directory",         dest="directory",          help="Define the maps directory.")
	optParser.add_option("-L",      "--lane.min",          dest="minLanes",            help="Minimum number of lanes.",   type = "int",   default=1 )
	optParser.add_option("-l",      "--lane.max",          dest="maxLanes",            help="Maximum number of lanes.",   type = "int",   default=4 )
	optParser.add_option("-s",     "--lane.step",         dest="stepLanes",               help="Lane count increment.",   type = "int",   default=1 )
	optParser.add_option("-E",      "--edge.min",          dest="minRoads",            help="Minimum length of roads.",   type = "int", default=100 )
	optParser.add_option("-e",      "--edge.max",          dest="maxRoads",            help="Maximum length of roads.",   type = "int", default=400 )
	optParser.add_option("-S",     "--edge.step",         dest="stepRoads",              help="Road length increment.",   type = "int", default=100 )
	optParser.add_option("-J", "--junctionCount",     dest="junctionCount",                help="Number of junctions.",   type = "int",   default=5 )
	optParser.add_option("-p",    "--cornerPath",        dest="cornerPath", help="Directory of the CORNER classifier." )
	optParser.add_option("-c",   "--clusterPath",       dest="clusterPath",  help="Directory of the ClusterLib tools." )
	optParser.add_option("-C",  "--cbdRoutePath",      dest="cbdRoutePath",    help="Directory of the CBDRouter tool." )
	optParser.add_option("-V",  "--vehicleTypes",      dest="vehicleTypes",       help="Vehicle type definition file." )
	optParser.add_option("-R",  "--route-number",       dest="routeNumber",       help="Number of routes to generate.",   type = "int", default=250 )
	optParser.add_option("-t",  "--maximum-time",           dest="maxTime",   help="The time limit of the simulation.",   type = "int", default=2000 )
	optParser.add_option("-D",    "--do-reverse",         dest="doReverse",     help="Create a reverse of each route.",  default=False, action='store_true' )
	optParser.add_option("-H",       "--highway",           dest="highway",                       help="Do a highway.",  default=False, action='store_true' )
	optParser.add_option("-a",     "--speed.min",          dest="minSpeed",       help="Minimum Highway speed (km/h).", type = "int", default=40 )
	optParser.add_option("-A",     "--speed.max",          dest="maxSpeed",       help="Maximum Highway speed (km/h).", type = "int", default=120 )
	optParser.add_option("-f",    "--speed.step",         dest="stepSpeed",          help="Highway speed step (km/h).", type = "int", default=20 )
	optParser.add_option("-v",  "--minCarNumber",  dest="minVehicleNumber",       help="Minimum vehicles to generate.", type = "float", default=500 )
	optParser.add_option("-y",  "--maxCarNumber",  dest="maxVehicleNumber",       help="Maximum vehicles to generate.", type = "float", default=1000 )
	optParser.add_option("-Y", "--stepCarNumber", dest="stepVehicleNumber",                  help="Vehicle increment.", type = "float", default=100 )
	(options, args) = optParser.parse_args(argv)

	if not options.directory:
		print "Please specify results directory."
		optParser.print_help()
		sys.exit()

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
		for c in range(0,junctionCount):
			f.write("\t<node id=\"" + str(c) + "\" x=\"" + str(c*roadLength) + "\" y=\"100.0\" type=\"unregulated\" />\n")
			if c > 0 and c < junctionCount-1:
				f.write("\t<node id=\"" + str(c) + "_up\" x=\"" + str(c*roadLength) + "\" y=\"0.0\" type=\"unregulated\" />\n")
				f.write("\t<node id=\"" + str(c) + "_down\" x=\"" + str(c*roadLength) + "\" y=\"200.0\" type=\"unregulated\" />\n")
		f.write("</nodes>\n")

	# Next create edge file
	with open( filename+".edg.xml", "w" ) as f:
		f.write("<?xml version=\"1.0\"?>\n")
		f.write("<edges>\n")
		for c in range(0,junctionCount-1):
			f.write("\t<edge id=\"" + str(c) + "_" + str(c+1) + "\" from=\"" + str(c) + "\" to=\"" + str(c+1) + "\" priority=\"1\" numLanes=\"" + str(laneCount) + "\" speed=\"" + str(speed) + "\" />\n")
			if c > 0 and c < junctionCount-1:
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
	for laneCount in range( options.minLanes, options.maxLanes+1, options.stepLanes ):
		for roadLength in range( options.minRoads, options.maxRoads+1, options.stepRoads ):
			for speed in range( options.minSpeed, options.maxSpeed+1, options.stepSpeed ):
				filename = "highway-" + str(options.junctionCount) + "-" + str(laneCount) + "lane-" + str(roadLength) + "m-" + str(speed) + "kmph"
				print "Generating '" + filename + "'..."
				generateHighway( options.junctionCount, roadLength, laneCount, speed / 3.6, filename )
				fileList.append( [filename, laneCount, roadLength, speed] )

	return fileList



def generateHighwayRoutes( filename, roadLength, vehicleRate, options ):
	with open( "tmp.trip", "w" ) as f:
		f.write( '<?xml version="1.0"?>\n' )
		f.write( '<trips>\n' )
		genPeriod = 60/int(vehicleRate*roadLength)
		numGen = 1
		if genPeriod < 1:
			genPeriod = 1
			numGen = int( math.ceil( int(vehicleRate*roadLength) / 60 ) )
		
		lastGen = -genPeriod
		ID = 0
		for t in range(0,options.maxTime):
			if ( t - lastGen ) == genPeriod:
				for n in range(0,numGen):
					turnOff = random.randint(0,1) == 1
					if turnOff:
						sinkEdge = str( random.randrange( 1, options.junctionCount-1 ) ) + "_"
						if random.randint(0,1) == 1:
							sinkEdge += "goup"
						else:
							sinkEdge += "godown"
					else:
						sinkEdge = str(options.junctionCount-2) + "_" + str(options.junctionCount-1)
					ID += 1
					f.write( '<trip id="car_{0}" depart="{1}" from="0_1" to="{2}" departLane="free" />\n'.format( ID, t, sinkEdge ) )
				lastGen = t
		f.write( '</trips>\n' )

	p = subprocess.Popen( ['duarouter','-n',filename+'.net.xml','-t','tmp.trip','-o',filename + "-" + str(vehicleRate) + "cpd.rou.xml"] )
	p.wait()
	os.remove("tmp.trip")

def analyseFiles( fileList, options ):
	for f in fileList:
		netFile = f[0]+'.net.xml'
		print "Running CORNER on '" + f[0] + "'..."
		p = subprocess.Popen( ['python',options.cornerPath+'/Sumo2Corner.py','-n',netFile] )
		p.wait()
		print "Running LSUF on '" + f[0] + "'..."
		p = subprocess.Popen( ['python',options.clusterPath+'/LaneWeight.py','-n',netFile] )
		p.wait()

		for carRate in numpy.arange( options.minVehicleNumber, options.maxVehicleNumber+options.stepVehicleNumber, options.stepVehicleNumber ): 
			baseFile = f[0] + "-" + str(carRate) + "cpd"
			rouFile = baseFile + ".rou.xml"
			print "Generating route file '" + rouFile + "'..."

			if options.highway:
				generateHighwayRoutes( f[0], f[2], carRate, options )

			else:
				genCmd = ['python',options.cbdRoutePath+'/CBDRouter.py']
				genCmd += ['-n',netFile]
				genCmd += ['-r',rouFile]
				genCmd += ['-V',options.vehicleTypes]
				genCmd += ['-v',str(carRate)]
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

