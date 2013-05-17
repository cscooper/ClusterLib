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
#include "ClusterControlMessage_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"
#include "BaseMobility.h"
#include "BaseConnectionManager.h"
#include "ChannelAccess.h"

#include "ClusterNetworkLayer.h"

Define_Module(ClusterNetworkLayer);


void ClusterNetworkLayer::initialize(int stage)
{
    BaseNetwLayer::initialize(stage);

    if(stage == 1) {

    	// set up the node.
    	mID = getId();
    	mWeight = calculateWeight();
    	mMobility = FindModule<BaseMobility*>::findSubModule(findHost());
    	mClusterHead = -1;
    	mIsClusterHead = false;

    	ChannelAccess *channelAccess = FindModule<ChannelAccess*>::findSubModule(findHost());
    	mTransmitRangeSq = pow( channelAccess->getConnectionManager( channelAccess->getParentModule() )->getMaxInterferenceDistance(), 2 );

    	// load configurations
    	mInitialFreshness = par("initialFreshness").longValue();
    	mFreshnessThreshold = par("freshnessThreshold").longValue();
    	mAngleThreshold = par("angleThreshold").doubleValue() * M_PI;
    	mHopCount = par("hopCount").longValue();
    	mBeaconInterval = par("beaconInterval").doubleValue();

    	// set up self-messages
    	mSendHelloMessage = new cMessage();
    	scheduleAt( simTime() + mBeaconInterval, mSendHelloMessage );
    	mFirstInitMessage = new cMessage();
    	scheduleAt( simTime(), mFirstInitMessage );
    	mBeatMessage = new cMessage();
    	scheduleAt( simTime() + BEAT_LENGTH, mBeatMessage );

    }

}





/** @brief Handle messages from upper layer */
void ClusterNetworkLayer::handleUpperMsg(cMessage* msg) {

	assert(dynamic_cast<cPacket*>(msg));
    sendDown(encapsMsg(static_cast<cPacket*>(msg)));

}



/** @brief Handle messages from lower layer */
void ClusterNetworkLayer::handleLowerMsg(cMessage* msg) {

    ClusterControlMessage *m = static_cast<ClusterControlMessage *>(msg);
    coreEV << " handling packet from " << m->getSrcAddr() << std::endl;

    switch( m->getKind() ) {

		case HELLO_MESSAGE: receiveHelloMessage( m ); break;
		case    CH_MESSAGE:    receiveChMessage( m ); break;
		case  JOIN_MESSAGE:  receiveJoinMessage( m ); break;
		case 		  DATA: sendUp( decapsMsg( m ) ); break;

		default:
		    coreEV << " received packet from " << m->getSrcAddr() << " of unknown type: " << m->getKind() << std::endl;
		    delete msg;
		    break;

    };

}

/** @brief Handle self messages */
void ClusterNetworkLayer::handleSelfMsg(cMessage* msg) {

	if ( msg == mSendHelloMessage ) {

		sendClusterMessage( HELLO_MESSAGE );
    	scheduleAt( simTime() + mBeaconInterval, mSendHelloMessage );

	} else if ( msg == mFirstInitMessage ) {

		init();
		delete mFirstInitMessage;

	} else if ( msg == mBeatMessage ) {

		processBeat();
		scheduleAt( simTime() + BEAT_LENGTH, mBeatMessage );

	}

}



/** @brief decapsulate higher layer message from ClusterControlMessage */
cMessage* ClusterNetworkLayer::decapsMsg( ClusterControlMessage *msg ) {

    cMessage *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());

    // delete the netw packet
    delete msg;
    return m;

}


/** @brief Encapsulate higher layer packet into a ClusterControlMessage */
NetwPkt* ClusterNetworkLayer::encapsMsg( cPacket *appPkt ) {

    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    coreEV <<"in encaps...\n";

    ClusterControlMessage *pkt = new ClusterControlMessage(appPkt->getName(), DATA);

    pkt->setBitLength(headerLength);

    cObject* cInfo = appPkt->removeControlInfo();

    if(cInfo == NULL){
		EV << "warning: Application layer did not specifiy a destination L3 address\n"
		   << "\tusing broadcast address instead\n";
		netwAddr = LAddress::L3BROADCAST;
    } else {
		coreEV <<"CInfo removed, netw addr="<< NetwControlInfo::getAddressFromControlInfo( cInfo ) << std::endl;
			netwAddr = NetwControlInfo::getAddressFromControlInfo( cInfo );
		delete cInfo;
    }

    pkt->setSrcAddr(myNetwAddr);
    pkt->setDestAddr(netwAddr);
    coreEV << " netw "<< myNetwAddr << " sending packet" <<std::endl;
    if(LAddress::isL3Broadcast( netwAddr )) {
        coreEV << "sendDown: nHop=L3BROADCAST -> message has to be broadcasted"
           << " -> set destMac=L2BROADCAST\n";
        macAddr = LAddress::L2BROADCAST;
    }
    else{
        coreEV <<"sendDown: get the MAC address\n";
        macAddr = arp->getMacAddr(netwAddr);
    }

    setDownControlInfo(pkt, macAddr);

    //encapsulate the application packet
    pkt->encapsulate(appPkt);
    coreEV <<" pkt encapsulated\n";
    return (NetwPkt*)pkt;

}


/** @brief Compute the CH weight for this node. */
double ClusterNetworkLayer::calculateWeight() {

	error( "ClusterNetworkLayer::calculateWeight MUST be overloaded to specify weight metric!" );
	return 0;

}




/** @brief Initiate clustering. */
void ClusterNetworkLayer::init() {

	int nMax = chooseClusterHead();
	if ( nMax == -1 ) {

		// this node is the best CH around, so declare it
		sendClusterMessage( CH_MESSAGE );
		mIsClusterHead = true;
		mClusterHead = mID;
		mClusterMembers.clear();
		mClusterMembers.insert( mID );

	} else {

		// we found a neighbour that's a better CH
		sendClusterMessage( JOIN_MESSAGE, nMax );
		mIsClusterHead = false;
		mClusterHead = nMax;
		mClusterMembers.clear();

	}

}



/** @brief Process the neighbour table in one beat. Also, update the node's weight. */
void ClusterNetworkLayer::processBeat() {

	// process the neighbour table
	NeighbourSet::iterator it = mNeighbours.begin();
	for ( ; it != mNeighbours.end(); it++ ) {

		it->second.mFreshness -= 1;
		if ( it->second.mFreshness == 0 ) {

			linkFailure( it->first );

		}

	}

	// update the node's weight
	mWeight = calculateWeight();

}



/** @brief Select a CH from the neighbour table. */
int ClusterNetworkLayer::chooseClusterHead() {

	int nCurr = -1;
	double wCurr = mWeight;

	NeighbourSet::iterator it = mNeighbours.begin();
	for ( ; it != mNeighbours.end(); it++ ) {

		if ( it->second.mIsClusterHead && it->second.mWeight > wCurr ) {

			nCurr = it->first;
			wCurr = it->second.mWeight;

		}

	}

	return nCurr;

}



/** @brief Handle a link failure. Link failure is detected when a CMs freshness reaches 0. */
void ClusterNetworkLayer::linkFailure( unsigned int nodeId ) {

	mNeighbours.erase( nodeId );
	if ( mIsClusterHead )
		mClusterMembers.erase( nodeId );
	else if ( nodeId == mClusterHead )
		init();

}



/** @brief Calculate the freshness of the given neighbour. */
void ClusterNetworkLayer::calculateFreshness( unsigned int nodeId ) {

	unsigned int freshness = mInitialFreshness;
	Coord v =    mMobility->getCurrentSpeed() - mNeighbours[ nodeId ].mVelocity;
	Coord p = mMobility->getCurrentPosition() - mNeighbours[ nodeId ].mPosition;

	double a = v.squareLength(), b, c;
	if ( a > 0 ) {

		b = ( v.x * p.x + v.y * p.y ) / a;
		c = ( p.squareLength() - mTransmitRangeSq ) / a;

		double detSq = b*b - c;

		if ( detSq > 0 ) {

			unsigned int r1 = (int)floor( ( -b + sqrt( detSq ) ) / BEAT_LENGTH );
			unsigned int r2 = (int)floor( ( -b - sqrt( detSq ) ) / BEAT_LENGTH );
			if ( r1 > 0 && r2 > 0 )
				freshness = std::min( 3*mInitialFreshness, std::min( r1, r2 ) );
			else if ( r1 > 0 )
				freshness = std::min( 3*mInitialFreshness, r1 );
			else if ( r2 > 0 )
				freshness = std::min( 3*mInitialFreshness, r2 );

		}

	}

	mNeighbours[nodeId].mFreshness = freshness;

}



/** @brief Determine whether the given node is a suitable CH. */
bool ClusterNetworkLayer::testClusterHeadChange( unsigned int nodeId ) {

	bool t1 = mNeighbours[nodeId].mIsClusterHead;
	bool t2 = mNeighbours[nodeId].mWeight > mNeighbours[mClusterHead].mWeight;
	bool t3 = mNeighbours[nodeId].mFreshness >= mFreshnessThreshold;

	Coord v1 = mMobility->getCurrentSpeed();
	Coord v2 = mNeighbours[ nodeId ].mVelocity;
	double angle = cos( ( v1.x*v2.x + v1.y*v2.y ) / ( v1.length() * v2.length() ) );

	bool t4 = angle <= mAngleThreshold;

	return t1 && t2 && t3 && t4;

}


/** @brief Handle a HELLO message. */
void ClusterNetworkLayer::receiveHelloMessage( ClusterControlMessage *m ) {

	updateNeighbour(m);

	if ( testClusterHeadChange( m->getNodeId() ) ) {

		sendClusterMessage( JOIN_MESSAGE, m->getNodeId() );
		mIsClusterHead = false;
		mClusterMembers.clear();

	}

	if ( m->getTtl() > 1 )
		sendClusterMessage( HELLO_MESSAGE, -1, m->getTtl()-1 );

	delete m;

}



/** @brief Handle a CH message. */
void ClusterNetworkLayer::receiveChMessage( ClusterControlMessage *m ) {

	updateNeighbour(m);

	if ( testClusterHeadChange( m->getNodeId() ) ) {

		sendClusterMessage( JOIN_MESSAGE, m->getNodeId() );
		mIsClusterHead = false;
		mClusterMembers.clear();

	}

	if ( m->getTtl() > 1 )
		sendClusterMessage( CH_MESSAGE, -1, m->getTtl()-1 );

	delete m;

}



/** @brief Handle a JOIN message. */
void ClusterNetworkLayer::receiveJoinMessage( ClusterControlMessage *m ) {

	updateNeighbour(m);

	if ( mIsClusterHead ) {

		if ( m->getTargetNodeId() == mID )
			mClusterMembers.insert( m->getNodeId() );
		else
			mClusterMembers.erase( m->getNodeId() );

	} else if ( mClusterHead == m->getNodeId() ) {

		init();

	}

	if ( m->getTtl() > 1 )
		sendClusterMessage( JOIN_MESSAGE, m->getTargetNodeId(), m->getTtl()-1 );

	delete m;

}

/** @brief Update a neighbour's data. */
void ClusterNetworkLayer::updateNeighbour( ClusterControlMessage *m ) {

	// update the neighbour data
	mNeighbours[m->getNodeId()].mWeight = m->getWeight();
	mNeighbours[m->getNodeId()].mIsClusterHead = m->getIsClusterHead();
	mNeighbours[m->getNodeId()].mPosition.x = m->getXPosition();
	mNeighbours[m->getNodeId()].mPosition.y = m->getYPosition();
	mNeighbours[m->getNodeId()].mVelocity.x = m->getXVelocity();
	mNeighbours[m->getNodeId()].mVelocity.y = m->getYVelocity();
	calculateFreshness( m->getNodeId() );

}

/** @brief Sends a given cluster message. */
void ClusterNetworkLayer::sendClusterMessage( int kind, int dest, int nHops ) {

    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    coreEV <<"sending cluster control message...\n";

	ClusterControlMessage *pkt = new ClusterControlMessage( "cluster-ctrl", kind );
    pkt->setBitLength(headerLength);

    // fill the cluster control fields
    pkt->setNodeId( mID );
    pkt->setWeight( mWeight );
    pkt->setTtl( nHops );

    Coord p = mMobility->getCurrentPosition();
    pkt->setXPosition( p.x );
    pkt->setYPosition( p.y );

    p = mMobility->getCurrentSpeed();
    pkt->setXVelocity( p.x );
    pkt->setYVelocity( p.y );

    pkt->setIsClusterHead( mIsClusterHead );
    pkt->setTargetNodeId( dest );

    netwAddr = LAddress::L3BROADCAST;

    pkt->setSrcAddr(myNetwAddr);
    pkt->setDestAddr(netwAddr);
    coreEV << " netw "<< myNetwAddr << " sending packet" <<std::endl;
	macAddr = LAddress::L2BROADCAST;

    setDownControlInfo(pkt, macAddr);

    sendDown( pkt );

}




