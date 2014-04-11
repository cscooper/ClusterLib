//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

cplusplus {{
#include "NetwPkt_m.h"
#include "AmacadData.h"
}}
packet NetwPkt;

class noncobject NeighbourEntrySet;
class noncobject NeighbourIdSet;

//
// Describes the AMACAD cluster control message.
// Extends the NetwPkt, so this can be used to encapsulate
// application layer packets as well.
//
packet AmacadControlMessage extends NetwPkt {

	int nodeId;							// Identifier of the neighbour.
	double xPosition;					// X-Position of the neighbour.
	double yPosition;					// Y-Position of the neighbour.
	double speed;					    // Speed of the neighbour.
	double xDestination;				// X-Destination of the neighbour.
	double yDestination;				// Y-Destination of the neighbour.
	bool isClusterHead;					// Is this node a CH or CHM?
	int clusterHead;					// The cluster head of this node.
	int neighbourCount;					// Number of neighbours this node has.
	int clusterSize;					// If CH, this is the number of nodes in its cluster.
	NeighbourEntrySet clusterTable;		// Cluster Member table.

}
