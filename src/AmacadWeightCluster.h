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

#ifndef __CLUSTERLIB_AMACADWEIGHTCLUSTER_H_
#define __CLUSTERLIB_AMACADWEIGHTCLUSTER_H_

#include <omnetpp.h>

#include <MiXiMDefs.h>
#include <BaseNetwLayer.h>

#include <set>
#include <map>

#include "MdmacControlMessage_m.h"
#include "MdmacNetworkLayer.h"


/**
 * Implements the Lane-Sense Utility Function (LSUF) Clustering metric.
 */
class AmacadWeightCluster : public MdmacNetworkLayer
{
public:
	/** @brief Initialization of the module and some variables*/
	virtual void initialize(int);

	/** @brief Handle self messages */
	virtual void handleSelfMsg(cMessage* msg);

    /** @brief Cleanup*/
    void finish();

protected:

	/** Get the destination from the destination file. */
	void GetDestinationList();

	struct DestinationEntry {
		double mTime;                   /**< The time until which this is our destination. */
		Coord mDestination;             /**< The coordinates of this destination. */
	};

	typedef std::vector<DestinationEntry> DestinationSet;

	double mWeights[3];                     /**< Set of weights. */
    DestinationSet mDestinationSet;         /**< Set of destinations. */
	cMessage *mChangeDestinationMessage;	/**< Message to change the destination. */
	Coord mCurrentDestination;				/**< Current destination of the node. */


	/** Add the destination data to a packet. */
	int AddDestinationData( MdmacControlMessage *pkt );

	/** Store the destination data from a packet. */
	void StoreDestinationData( MdmacControlMessage *m );


	/** @brief Compute the CH weight for this node. */
	double calculateWeight();
};

#endif
