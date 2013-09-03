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

#include "BaseNetwLayer.h"

#include <cassert>

#include "NetwControlInfo.h"
#include "BaseMacLayer.h"
#include "AddressingInterface.h"
#include "SimpleAddress.h"
#include "FindModule.h"
#include "MdmacControlMessage_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"
#include "BaseMobility.h"
#include "BaseConnectionManager.h"
#include "ChannelAccess.h"

#include "LSUFCluster.h"


Define_Module(LSUFCluster);

LSUFData *LSUFCluster::mLaneWeightData = NULL;


/** @brief Initialization of the module and some variables*/
void LSUFCluster::initialize( int stage ) {

    if ( stage == 0 ) {

        if ( LSUFCluster::mLaneWeightData == NULL ) {
        	try {
        		LSUFCluster::mLaneWeightData = new LSUFData( par("laneWeightFile").stringValue() );
        	} catch( const char *e ) {
        		opp_error( e );
        	}
        }

        LSUFCluster::mLaneWeightData->addRef();

    }

    MdmacNetworkLayer::initialize( stage );

}

/** @brief Cleanup*/
void LSUFCluster::finish() {

    if ( LSUFCluster::mLaneWeightData != NULL ) {

        LSUFCluster::mLaneWeightData->release();
        if ( LSUFCluster::mLaneWeightData->getReferenceCount() == 0 ) {
            delete LSUFCluster::mLaneWeightData;
            LSUFCluster::mLaneWeightData = NULL;
        }

    }

    MdmacNetworkLayer::finish();

}


/** @brief Compute the CH weight for this node. */
double LSUFCluster::calculateWeight() {

	/*
	 * Overview of LSUF:
	 * This is based on Utility-Function clustering.
	 * The metric is a weight computed from statistics of
	 * node placement around the node computing its own weight,
	 * as well as the lane it is in.
	 */

	// If the module hasn't been initialised yet, return zero.
	if ( !mInitialised )
		return 0;

	// Fetch the weight of this lane, and it's flow ID
	float laneWeight;
	unsigned char flow;
	if ( !LSUFCluster::mLaneWeightData->getLaneWeight( mRoadID+"_"+mLaneID, &laneWeight, &flow ) )
		opp_error( "Tried to get weight for an unknown lane '%s' for current node #%d", (mRoadID+"_"+mLaneID).c_str(), mID );

	/*
	 * Now that we have the weight, we have to iterate through the neighbour table.
	 * We calculate the Network Connectivity Level, the Average Distance Level,
	 * and Average Velocity Level.
	 */
	int alpha = mNeighbours.size();
	int beta = 0;
	float delta = 0, chi = 0, sigma = 0, rho = 0;
	for ( NeighbourIterator it = mNeighbours.begin(); it != mNeighbours.end(); it++ ) {

		// This check does not appear to be part of the original algorithm.
		if( it->second.mRoadID != mRoadID )
			continue;	// We don't want to cluster with cars that are not on the same road as us.

		// Calculate distance and difference in velocity
		float dist, dv;
		dist = mMobility->getCurrentPosition().distance( it->second.mPosition );
		dv = mMobility->getCurrentSpeed().distance( it->second.mVelocity );

		delta += dist;
		sigma += dv;

		// Get the flow ID of this neighbour.
		unsigned char currFlow;
		if ( !LSUFCluster::mLaneWeightData->getLaneWeight( it->second.mRoadID+"_"+it->second.mLaneID, NULL, &currFlow ) )
			opp_error( "Tried to get weight for an unknown lane '%s' for current node #%d", (it->second.mRoadID+"_"+it->second.mLaneID).c_str(), it->first );

		if ( currFlow == flow ) {
			// This car is part of the same flow.
			chi += dist;
			rho += dv;
			beta++;
		}

	}

	delta /= alpha;
	sigma /= alpha;
	chi /= beta;
	rho /= beta;

	float NCL, ADL, AVL;

	NCL =  beta + alpha * laneWeight;
	ADL = delta +   chi * laneWeight;
	AVL = sigma +   rho * laneWeight;

	return NCL + ADL + AVL;

}
