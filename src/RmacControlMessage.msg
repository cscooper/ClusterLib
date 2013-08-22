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
// Describes the RMAC cluster control message.
// Extends the NetwPkt, so this can be used to encapsulate
// application layer packets as well.
//
packet RmacControlMessage extends NetwPkt {

	int nodeId;						// Identifier of the neighbour.
	double xPosition;				// X-Position of the neighbour.
	double yPosition;				// Y-Position of the neighbour.
	double xVelocity;				// X-Velocity of the neighbour.
	double yVelocity;				// Y-Velocity of the neighbour.
    unsigned int connectionCount;   // Number of connections this node has.

}
