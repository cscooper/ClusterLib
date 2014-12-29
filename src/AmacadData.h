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

#ifndef AMACADDATA_H_
#define AMACADDATA_H_

class AmacadNetworkLayer;

/**
 * @brief Contains an entry in the AMACAD neighbour table.
 */
struct NeighbourEntry {
	unsigned int mId;                   /**< Identifier of the neighbour. */
	LAddress::L3Type mNetworkAddress;   /**< The IP address of this neighbour. */
	double mPositionX;					/**< X Position of this node. */
	double mPositionY;					/**< Y Position of this node. */
	double mSpeed;						/**< Speed of the node. */
	double mDestinationX;				/**< X Destination of the node in euclidean coordinates. */
	double mDestinationY;				/**< Y Destination of the node in euclidean coordinates. */
	int mNeighbourCount;				/**< Size of this node's neighbour table. */
	int mClusterHead;					/**< ID of this node's CH. */
	bool mIsClusterHead;				/**< Is this node a CH? */
	int mClusterSize;				    /**< Size of this node's cluster, if CH. */
	simtime_t mLastHeard;				/**< Time since we last heard from this node. */
};

typedef std::vector<NeighbourEntry> NeighbourEntrySet;
typedef NeighbourEntrySet::iterator NeighbourEntrySetIterator;

typedef std::vector<unsigned int> NeighbourIdSet;
typedef NeighbourIdSet::iterator NeighbourIdSetIterator;

#endif /* #define AMACADDATA_H_ */
