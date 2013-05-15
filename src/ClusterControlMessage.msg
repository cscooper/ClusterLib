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
}}
packet NetwPkt;

//
// Describes the cluster control message.
// Extends the NetwPkt, so this can be used to encapsulate
// application layer packets as well.
//
packet ClusterControlMessage extends NetwPkt {

	int nodeId;
	double weight;
	bool isClusterHead;
	double xPosition;
	double yPosition;
	double xVelocity;
	double yVelocity;
	int targetNodeId;	// ID of the CH I wish to join (used for JOIN_MESSAGE type)

}
