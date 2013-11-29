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

#ifndef RMACDATA_H_
#define RMACDATA_H_

class RmacNetworkLayer;

/**
 * @brief Contains an entry in the RMAC neighbour table.
 */
struct NeighbourEntry {
	unsigned int mId;                   /**< Identifier of the neighbour. */
	LAddress::L3Type mNetworkAddress;   /**< The IP address of this neighbour. */
	double mPositionX;                  /**< Position of the neighbour (X). */
	double mPositionY;                  /**< Position of the neighbour (Y). */
	double mVelocityX;                  /**< Velocity of the neighbour (X). */
	double mVelocityY;                  /**< Velocity of the neighbour (Y). */
	unsigned int mConnectionCount;      /**< Number of connections this node now has. */
	unsigned int mHopCount;             /**< Number of hops this neighbour is from this node. */
	simtime_t mTimeStamp;				/**< Timestamp of this data. */
};

typedef std::vector<NeighbourEntry> NeighbourEntrySet;
typedef NeighbourEntrySet::iterator NeighbourEntrySetIterator;


#endif /* #define RMACDATA_H_ */
