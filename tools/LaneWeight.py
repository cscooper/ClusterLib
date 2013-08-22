#!/usr/bin/python

import sumolib
from VectorMath import *
from optparse import OptionParser

def ComputeWeights( options ):

	netFile = sumolib.net.readNet( options.netFile )
	laneWeights = []

	# scan the edges in the net.
	netEdges = netFile.getEdges()
	for edge in netEdges:
		linkLookup = {}
		flows = {}
		flowLanePair = {}

		# Get a list of all outgoing connctions from this edge
		conns = []
		outgoingConnections = edge.getOutgoing()
		for connection in outgoingConnections.iterkeys():
			conns += outgoingConnections[connection]

		# If there are no outgoing connections, skip this edge.
		if len(conns) == 0:
			for i in range(0,edge.getLaneNumber()):
				laneWeights.append( [edge.getLane(i).getID(), 1.0, 0] )
			continue

		# Scan the connections in the list.
		for connection in conns:
			# Get the ID of the destination link
			dest = connection._to._id

			# Add this to the link look-up
			if dest not in linkLookup:
				linkLookup[dest] = connection._to

			# Add the source lane to the flow-lane lookup.
			src = connection._fromLane.getID()
			if src not in flowLanePair.keys():
				flowLanePair[src] = []

			# Add this flow to the set of flows to which this lane belongs.
			flowLanePair[src].append(dest)

		# Remove duplicate flow entries for each lane, sort them, and produce a unique string ID for each flow.
		# For each unique ID, increment a counter in 'flows.'
		for lane in flowLanePair.iterkeys():
			flowLanePair[lane] = str( sorted( list( set( flowLanePair[lane] ) ) ) )
			if flowLanePair[lane] not in flows.keys():
				flows[flowLanePair[lane]] = [0,len(flows.keys())]
			flows[flowLanePair[lane]][0] += 1

		# Now we've determined the flow each lane belongs to, we compute the lane weight, and add it to the list of weights
		for lane in flowLanePair.iterkeys():
			flow = flowLanePair[lane]
			laneWeights.append( [ lane, float(flows[flow][0])/edge.getLaneNumber(), flows[flow][1] ] )

	# Got all the weights.
	# Now we save the data to a file.
	print "Saving weights to " + options.outFile
	f = open( options.outFile, "w" )
	f.write( str(len(laneWeights)) + "\n" )
	for lane in laneWeights:
		f.write( lane[0] + " " + str(lane[1]) + " " + str(lane[2]) + "\n" )
	f.close()


def ParseOptions():
	optParser = OptionParser()
	optParser.add_option("-o", "--out-file", dest="outFile", help="Define the output file.")
	optParser.add_option("-n", "--net-file", dest="netFile", help="Define the SUMO net file (manditory)")
	(options, args) = optParser.parse_args()

	if not options.netFile:
		optParser.print_help()
		sys.exit()

	if not options.outFile:
		options.outFile = options.netFile.split(".net.")[0] + ".lsuf"

	return options


if __name__ == "__main__":
	ComputeWeights( ParseOptions() )



