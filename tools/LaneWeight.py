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
		# Create empty lane weight lookup
		laneWeightLookup = {}
		linkLookup = {}
		flows = {}

		# scan through all the outgoing connections
		conns = []
		outgoingConnections = edge.getOutgoing()
		for connection in outgoingConnections.iterkeys():
			conns += outgoingConnections[connection]

		if len(conns) == 0:
			continue

		for connection in conns:
			# get the name of the destination link
			dest = connection._to._id
			if dest not in laneWeightLookup:
				laneWeightLookup[dest] = []
				linkLookup[dest] = connection._to
				flows[dest] = [0,len(flows.keys())]
			laneWeightLookup[dest].append( connection._fromLane.getID() )
			flows[dest][0] += 1

		# We need to be able to determine whether a lane connection goes straight or turns.
		# Thus we scan the keys in laneWeightLookup, working out whether the connection goes straight or turns.
		# The criteria is the angle between this link and the destination. This assumes that there are at most
		# five lanes at any node. It sorts the links in order of angle, and selects the two with the highest
		# angle as ones that turn (unless there are only two, in which case it selects the highest one).

		# NOTE! This assumes that the source link's _to is equal to the _from of each destination link.

		# Compute the vector of this edge
		centrePoint = Vector2D( edge._to._coord[0], edge._to._coord[1] )
		thisEdge = Vector2D( edge._from._coord[0], edge._from._coord[1] ) - centrePoint
		rotAngle = math.acos( thisEdge.y / thisEdge.Magnitude() )
		if thisEdge.x != 0:
			rotAngle *= thisEdge.x / abs( thisEdge.x )
		connAngles = []
		for link in linkLookup.keys():
			# Compute the link's vector.
			currLink = Vector2D( linkLookup[link]._to._coord[0], linkLookup[link]._to._coord[1] ) - centrePoint

			# Determine what side of the y axis the vector lies on.
			rotX = currLink.x * math.cos( rotAngle ) - currLink.y * math.sin( rotAngle )
			if rotX != 0:
				rotX /= abs(rotX)

			# Calculate the angle between this link and the current edge being processed. Add it to the list.
			connAngles.append( [ link, thisEdge.AngleBetween( currLink ), rotX ] )

		# We have the connection angles. Sort in order of increasing angle.
		# Then set all the 
#		print connAngles
		connAngles = sorted( connAngles, key=lambda p: p[1] )
		if len(connAngles) == 1:
			connAngles[0][2] = 0
		if len(connAngles) == 2:
			connAngles[1][2] = 0
		elif len(connAngles) > 2:
			for i in range(3,len(connAngles)):
				connAngles[i][2] = 0

		angleLookup = {}
		for a in connAngles:
			angleLookup[a[0]] = a[2]

		lanesProcessed = []

		for dest in laneWeightLookup.iterkeys():
			for lane in laneWeightLookup[dest]:

				# Check if we've already processed this lane.
				if lane in lanesProcessed:
					continue

				# First check if this lane belongs to multiple traffic flows
				currFlows = [dest]
				for d in laneWeightLookup.iterkeys():
					if d == dest:
						continue
					if lane in laneWeightLookup[d]:
						currFlows.append( d )

				if len( currFlows ) > 1:
					# We have multiple flows.
					# If the field 'options.prioritiseTurns' is not zero, then we go with the
					# flow that will turn. If the lane can go straight, or turn left or right,
					# the sign of 'options.prioritiseTurns' determines if the left turn (-1) is
					# prioritised, or the right turn (+1).
					if len( currFlows ) == 2:
						for flow in currFlows:
							if options.prioritiseTurns == 0:
								if angleLookup[flow] == 0:
									currFlows = flow
									break
							else:
								if angleLookup[flow] != 0:
									currFlows = flow
									break

					elif len( currFlows ) == 3:
						for flow in currFlows:
							if options.prioritiseTurns == angleLookup[flow]:
								currFlows = flow
								break

				else:
					currFlows = currFlows[0]

				# Now we've determined the flow this lane belongs to, we compute the lane weight, and add it to the list of weights
				laneWeights.append( [ lane, float(flows[dest][0])/edge.getLaneNumber(), flows[dest][1] ] )

				# Add this to the set of processed lanes.
				lanesProcessed.append( lane )

	# Got all the weights.
	# Now we save the data to a file.
	f = open( options.outFile, "w" )
	f.write( str(len(laneWeights)) + "\n" )
	for lane in laneWeights:
		f.write( lane[0] + " " + str(lane[1]) + " " + str(lane[2]) + "\n" )
	f.close()



def ParseOptions():
	optParser = OptionParser()
	optParser.add_option("-o", "--out-file", dest="outFile", help="Define the output file.")
	optParser.add_option("-n", "--net-file", dest="netFile", help="Define the SUMO net file (manditory)")
	optParser.add_option("-p", "--prioritise-turns", dest="prioritiseTurns", default="straight", help="Flow priority. Straight, left, or right.")
	(options, args) = optParser.parse_args()

	if not options.netFile:
		optParser.print_help()
		sys.exit()

	if options.prioritiseTurns.lower() == "straight":
		options.prioritiseTurns = 0
	elif options.prioritiseTurns.lower() == "left":
		options.prioritiseTurns = -1
	elif options.prioritiseTurns.lower() == "right":
		options.prioritiseTurns = 1

	if not options.outFile:
		options.outFile = options.netFile.split(".")[0] + ".lsuf"

	return options


if __name__ == "__main__":
	ComputeWeights( ParseOptions() )



