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

//#include <algorithm>
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

#include "RouteSimilarityCluster.h"


Define_Module(RouteSimilarityCluster);



/** @brief Initialization of the module and some variables*/
void RouteSimilarityCluster::initialize( int stage ) {

    MdmacNetworkLayer::initialize( stage );
    mIncludeDestination = true;
	if ( stage == 1 ) {

		mLinkCount = par("linkCount").longValue();
		TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
		std::string myId = dynamic_cast<TraCIMobility*>(mMobility)->getExternalId();
		std::string myRouteId = pManager->commandGetRouteId( myId );
		mRouteList = pManager->commandGetRouteEdgeIds( myRouteId );

//		std::cerr << "Route:\n";
//		for ( RouteLinkList::iterator it = mRouteList.begin(); it != mRouteList.end(); it++ )
//			std::cerr << *it << ", ";
//		std::cerr << "\n";


    }

}




/** Add the destination data to a packet. */
int RouteSimilarityCluster::AddDestinationData( MdmacControlMessage *pkt ) {

	// First check if the first id in the list is our current link.
	while ( mRoadID != mRouteList.front() )
		mRouteList.erase( mRouteList.begin() );

	// Check to make sure it didn't wipe the entire route list.
	if ( mRouteList.empty() ) {
		std::cerr << "STUB: node[" << mId << "] route end!\n";
		return 0;
	}
//
//	std::cerr << "Route:\n";
//	for ( RouteLinkList::iterator it = mRouteList.begin(); it != mRouteList.end(); it++ )
//		std::cerr << *it << ", ";
//	std::cerr << "\n";

	// Now add at most 'mLinkCount' number of links to the packet.
	int size = 0;
	RouteLinkList::iterator myListStart = mRouteList.begin();
	for ( int i = 0; i < std::min( mLinkCount, (int)mRouteList.size() ); i++ ) {

		std::string currEdge = *myListStart;
		pkt->getRoute().push_back( currEdge );

		// Add the number of bits used to encode the edge name.
		size += currEdge.length()*8;

		myListStart++;

	}

	// Return the size.
	return size;

}


/** Store the destination data from a packet. */
void RouteSimilarityCluster::StoreDestinationData( MdmacControlMessage *m ) {

	mNeighbours[m->getNodeId()].mRouteLinks = m->getRoute();

}



/** @brief Compute the CH weight for this node. */
double RouteSimilarityCluster::calculateWeight() {

	// This weight is the sum of percent route similarity between this node and its neighbours.
	// The node iterates through its neighbour table and checks the route links it has logged.
	// Starting from the beginning, the node adds 1/N to the weight for every matching link it finds.

	if ( !mMobility || mNeighbours.size() == 0 )
		return 0;

	double mScore = 0;

    for ( NeighbourIterator it = mNeighbours.begin(); it != mNeighbours.end(); it++ ) {

    	RouteLinkList::iterator myListStart = mRouteList.begin();
    	RouteLinkList::iterator theirListStart = it->second.mRouteLinks.begin();

    	// Start looking for match ups.
    	for ( int i = 0; i < std::min( mLinkCount, std::min( (int)mRouteList.size(), (int)it->second.mRouteLinks.size() ) ); i++ ) {

    		if ( *myListStart != *theirListStart )
    			break;	// Mismatch between routes, so bail out.

    		// Got a match, so add it to the weight.
    		mScore++;
    		myListStart++;
    		theirListStart++;

    	}

    }

//	mScore /= mLinkCount;
//	std::cerr << "Score(" << mId << ") = " << mScore << "\n";
	return mScore;

}
