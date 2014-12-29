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


#include "TraCIScenarioManager.h"
#include "TraCIMobility.h"

#include "AmacadWeightCluster.h"


Define_Module(AmacadWeightCluster);



/** @brief Initialization of the module and some variables*/
void AmacadWeightCluster::initialize( int stage ) {

    MdmacNetworkLayer::initialize( stage );
    mIncludeDestination = true;
	if ( stage == 1 ) {

        mWeights[0] = par("distanceWeight").doubleValue();
        mWeights[1] = par("speedWeight").doubleValue();
        mWeights[2] = par("destinationWeight").doubleValue();

        // Load this car's destination list.
        GetDestinationList();

        // Schedule the change in destination.
        mChangeDestinationMessage = new cMessage( "changeDestination" );
        scheduleAt( simTime() + mDestinationSet[0].mTime, mChangeDestinationMessage );

        // Set up the first destination.
        mCurrentDestination = mDestinationSet[0].mDestination;

        // Clear the current destination in anticipation of the next one.
        mDestinationSet.erase( mDestinationSet.begin() );

    }

}



/** @brief Handle self messages */
void AmacadWeightCluster::handleSelfMsg(cMessage* msg) {

	if ( mChangeDestinationMessage == msg ) {

		// We have to change the destination of our vehicle.
		if ( mDestinationSet.empty() )
				return;

		// Trigger the change in destination.
		scheduleAt( simTime() + mDestinationSet[0].mTime, mChangeDestinationMessage );

		// Set up the first destination.
		mCurrentDestination = mDestinationSet[0].mDestination;

		// Clear the current destination in anticipation of the next one.
		mDestinationSet.erase( mDestinationSet.begin() );

		return;

	} else {

		MdmacNetworkLayer::handleSelfMsg(msg);

	}

}


/** @brief Cleanup*/
void AmacadWeightCluster::finish() {

	if ( mChangeDestinationMessage->isScheduled() )
		cancelEvent( mChangeDestinationMessage );
	delete mChangeDestinationMessage;
	MdmacNetworkLayer::finish();

}


/**
 * Get the destination from the destination file.
 */
void AmacadWeightCluster::GetDestinationList() {

	mDestinationSet.clear();

	std::ifstream inputStream;
	inputStream.open( par("destinationFile").stringValue() );
	if ( inputStream.fail() )
		throw cRuntimeError( "Cannot load the destination file." );


	TraCIMobility *mob = dynamic_cast<TraCIMobility*>(mMobility);
	std::string carName, myName = mob->getExternalId();
	int count;
	DestinationEntry e;
	int entryCount;
	bool save;

	inputStream >> entryCount;
	for ( int i = 0; i < entryCount; i++ ) {

		inputStream >> carName >> count;
		save = ( carName == myName );
		for ( int j = 0; j < count; j++ ) {

			inputStream >> e.mTime >> e.mDestination.x >> e.mDestination.y;
			if ( save )
				mDestinationSet.push_back( e );

		}

		if ( save )
			break;

	}

	inputStream.close();

}



/** Add the destination data to a packet. */
int AmacadWeightCluster::AddDestinationData( MdmacControlMessage *pkt ) {

	pkt->setXDestination( mCurrentDestination.x );
	pkt->setYDestination( mCurrentDestination.y );
	return 128;

}



/** Store the destination data from a packet. */
void AmacadWeightCluster::StoreDestinationData( MdmacControlMessage *m ) {

	mNeighbours[m->getNodeId()].mDestination.x = m->getXDestination();
	mNeighbours[m->getNodeId()].mDestination.y = m->getYDestination();

}


/** @brief Compute the CH weight for this node. */
double AmacadWeightCluster::calculateWeight() {

	if ( !mMobility || mNeighbours.size() == 0 )
		return -DBL_MAX;

   	Coord p = mMobility->getCurrentPosition();
   	Coord v = mMobility->getCurrentSpeed();

	double dL, dS, dD, ret = 0;
    for ( NeighbourIterator it = mNeighbours.begin(); it != mNeighbours.end(); it++ ) {
		dL = ( p - it->second.mPosition ).length();
		dS = ( v - it->second.mVelocity ).length();
		dD = ( mCurrentDestination - it->second.mDestination ).length();
		ret += dL * mWeights[0] + dS * mWeights[1] + dD * mWeights[2];
	}

	return -ret;

}
