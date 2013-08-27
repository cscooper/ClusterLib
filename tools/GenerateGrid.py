#!/usr/bin/python

import sys, os, subprocess
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
	optParser.add_option("-d",     "--directory",     dest="directory",          help="Define the maps directory.")
	optParser.add_option("-L",      "--lane.min",      dest="minLanes",            help="Minimum number of lanes.",   default=1 )
	optParser.add_option("-l",      "--lane.max",      dest="maxLanes",            help="Maximum number of lanes.",   default=4 )
	optParser.add_option("-s",     "--lane.step",     dest="stepLanes",               help="Lane count increment.",   default=1 )
	optParser.add_option("-E",      "--edge.min",      dest="minRoads",            help="Minimum length of roads.", default=100 )
	optParser.add_option("-e",      "--edge.max",      dest="maxRoads",            help="Maximum length of roads.", default=400 )
	optParser.add_option("-S",     "--edge.step",     dest="stepRoads",              help="Road length increment.", default=100 )
	optParser.add_option("-J", "--junctionCount", dest="junctionCount",                help="Number of junctions.",   default=5 )
	optParser.add_option("-p",    "--cornerPath",    dest="cornerPath", help="Directory of the CORNER classifier." )
	optParser.add_option("-c",   "--clusterPath",   dest="clusterPath",  help="Directory of the ClusterLib tools." )
	optParser.add_option("-C",  "--cbdRoutePath",  dest="cbdRoutePath",    help="Directory of the CBDRouter tool." )
	optParser.add_option("-V",  "--vehicleTypes",  dest="vehicleTypes",       help="Vehicle type definition file." )
	optParser.add_option("-v",     "--carNumber", dest="vehicleNumber",     help="Number of vehicles to generate.", default=500 )
	optParser.add_option("-R",  "--route-number",   dest="routeNumber",       help="Number of routes to generate.", default=250 )
	optParser.add_option("-t",  "--maximum-time",       dest="maxTime",   help="The time limit of the simulation.", default=2000 )
	optParser.add_option("-D",    "--do-reverse",     dest="doReverse",     help="Create a reverse of each route.", default=False, action='store_true' )
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
			p = subprocess.Popen( ['netgen','-g','--grid.number',str(options.junctionCount),'--grid.length',str(roadLength),'-L',str(laneCount),'--no-internal-links','-o',filename+".net.xml"] )
			p.wait()

			cbdFile = filename + '.cbd'
			with open(cbdFile,'w') as f:
				f.write("0,0,"+str(roadLength)+"\n")
				w = str((options.junctionCount-1)*roadLength)
				f.write(w+","+w+","+str(roadLength)+"\n")

			fileList.append( filename )

	return fileList


def analyseFiles( fileList, options ):
	for f in fileList:
		netFile = f+'.net.xml'
		print "Running CORNER on '" + f + "'..."
		p = subprocess.Popen( [options.cornerPath+'/Sumo2Corner.py','-n',netFile] )
		p.wait()
		print "Running LSUF on '" + f + "'..."
		p = subprocess.Popen( [options.clusterPath+'/LaneWeight.py','-n',netFile] )
		p.wait()
		print "Generating route file..."
		rouFile = f + ".rou.xml"
		genCmd = [options.cbdRoutePath+'/CBDRouter.py']
		genCmd += ['-n',netFile]
		genCmd += ['-r',rouFile]
		genCmd += ['-V',options.vehicleTypes]
		genCmd += ['-v',str(options.vehicleNumber)]
		genCmd += ['-R',str(options.routeNumber)]
		genCmd += ['-C',f+'.cbd']
		genCmd += ['-t',str(options.maxTime)]
		if options.doReverse:
			genCmd += ['-d']
		p = subprocess.Popen( genCmd )
		p.wait()
		with open( f+".sumo.cfg", "w" ) as fout:
			fout.write( sumoConfigFormat.format( netFile, rouFile, 0, options.maxTime ) )
		with open( f+".launchd.xml", "w" ) as fout:
			fout.write( launchdConfigFormat.format( options.directory, netFile, rouFile, f+".sumo.cfg" ) )


if __name__ == "__main__":
	options = parseOptions( sys.argv )
	os.chdir( options.directory )
	analyseFiles( generateGrids( options ), options )

