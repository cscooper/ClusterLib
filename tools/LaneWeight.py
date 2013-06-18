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

		# Remove duplicate flow entries for each lane and count the number of lanes belonging to each flow.
		for lane in flowLanePair.iterkeys():
			flowLanePair[lane] = list( set( flowLanePair[lane] ) )
			for dest in flowLanePair[lane]:
				if dest not in flows.keys():
					flows[dest] = [0,len(flows.keys())]
				flows[dest][0] += 1

		# We need to be able to determine whether a lane connection goes straight or turns.
		# Thus we scan the keys in linkLookup, working out whether the connection goes straight or turns.
		# The criteria is the angle between this link and the destination. This assumes that there are at most
		# five lanes at any node. It sorts the links in order of angle, and selects the two with the highest
		# angle as ones that go straight (unless there are only two, in which case it selects the highest one).

		# NOTE! This assumes that the source link's _to is equal to the _from of each destination link.

		# Compute the vector of this edge
		centrePoint = Vector2D( edge._to._coord[0], edge._to._coord[1] )
		thisEdge = Vector2D( edge._from._coord[0], edge._from._coord[1] ) - centrePoint

		# Calculate the angle to rotate each vector, such that the current edge vector is the positive y axis.
		rotAngle = math.acos( thisEdge.y / thisEdge.Magnitude() )
		if thisEdge.x != 0:
			rotAngle *= thisEdge.x / abs( thisEdge.x )

		# Now, calculate the angles between each outgoing edge and this edge.
		connAngles = []
		for link in linkLookup.keys():
			# Compute the link's vector.
			currLink = Vector2D( linkLookup[link]._to._coord[0], linkLookup[link]._to._coord[1] ) - centrePoint

			# Determine what side of the y axis the vector lies on. If rotX is -ive, the link is a left turn. +ive means a right turn.
			rotX = currLink.x * math.cos( rotAngle ) - currLink.y * math.sin( rotAngle )
			if rotX != 0:
				rotX /= abs(rotX)

			# Calculate the angle between this link and the current edge being processed. Add it to the list.
			connAngles.append( [ link, thisEdge.AngleBetween( currLink ), rotX ] )

		# We have the connection angles. Sort in order of increasing angle.
		connAngles = sorted( connAngles, key=lambda p: p[1] )

		# If there's only one connecting link, then just assume that it's going straight ahead. 
		if len(connAngles) == 1:
			connAngles[0][2] = 0
		# If there's only two connecting links, the one with the largest angle is going straight.
		elif len(connAngles) == 2:
			connAngles[1][2] = 0
		# If there are more than two, the lowest two are going to turn, and the rest are straight.
		elif len(connAngles) > 2:
			for i in range(2,len(connAngles)):
				connAngles[i][2] = 0

		# Now that we've worked that out, construct a lookup linking the link name to whether it turns left, right, or goes straight.
		angleLookup = {}
		for a in connAngles:
			angleLookup[a[0]] = a[2]

		# Now for all the lanes in the edge, find the ones that belong to multiple flows. For each of these, ignore the flows
		# that are not indicated as priority by 'options.prioritiseTurns'
		for lane in flowLanePair.iterkeys():
			if len(flowLanePair[lane]) == 1:
				continue	# this lane is part of only one flow

			# If the lane is part of two flows, then it can either be straight or a turn.
			# We treat it differently than whether there are more than two, since if we treated this case the same,
			# any right turns without corresponding left turns would be pruned if options.prioritiseTurns = 'left'.
			# Thus in the case of two flows, we treat any turns the same.
			if len(flowLanePair[lane]) == 2:
				if options.prioritiseTurns == 0:
					keepers = [flow for flow in flowLanePair[lane] if angleLookup[flow] == 0]
				else:
					keepers = [flow for flow in flowLanePair[lane] if angleLookup[flow] != 0]
			# In the other case, we just need to make sure that the turn code is the type of turn we're prioritising.
			else:
				keepers = [flow for flow in flowLanePair[lane] if angleLookup[flow] == options.prioritiseTurns]

			# Prune the losers.
			losers = [flow for flow in flowLanePair[lane] if flow not in keepers]

			# Keep the keepers.
			flowLanePair[lane] = keepers

			# Decrement the flow member count for each of the losers.
			for flow in losers:
				flows[flow][0] -= 1

		# By now, all lanes should be associated with ONLY ONE FLOW!

		for lane in flowLanePair.iterkeys():
			# Now we've determined the flow this lane belongs to, we compute the lane weight, and add it to the list of weights
			flow = flowLanePair[lane][0]
			laneWeights.append( [ lane, float(flows[flow][0])/edge.getLaneNumber(), flows[flow][1] ] )

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



