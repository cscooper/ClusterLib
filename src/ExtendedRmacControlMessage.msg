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
#include "RMACData.h"
}}
packet NetwPkt;


class noncobject NeighbourEntrySet;
class noncobject NeighbourIdSet;
class noncobject Route;

//
// Describes the RMAC cluster control message.
// Extends the NetwPkt, so this can be used to encapsulate
// application layer packets as well.
//
packet ExtendedRmacControlMessage extends NetwPkt {

	int nodeId;							// Identifier of the neighbour.
	double xPosition;					// X-Position of the neighbour.
	double yPosition;					// Y-Position of the neighbour.
	double xVelocity;					// X-Velocity of the neighbour.
	double yVelocity;					// Y-Velocity of the neighbour.
	bool isClusterHead;					// Is this node a CH or CHM?
	int clusterHead;					// ID of this node's CH.
    unsigned int connectionCount;  		// Number of connections this node has.
	NeighbourEntrySet neighbourTable;	// Neighbour table of this node.
	NeighbourIdSet neighbourIdTable;	// Set of IDs in neighbour table (used for CLUS_PRES frames).
	NeighbourIdSet clusterHierarchy;	// Sequence of cluster heads (used to prevent cyclical clusters).
	Route nodeRoute;					// The route this node will take.
	int proposedRole;					// Proposed role of this node (used for CLUS_UNIFY_REQ frames).

}
