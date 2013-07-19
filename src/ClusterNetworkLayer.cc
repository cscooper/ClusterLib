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

#include "TraCIScenarioManager.h"
#include "TraCIMobility.h"

#include "ClusterNetworkLayer.h"

Define_Module(ClusterNetworkLayer);

std::ostream& operator<<( std::ostream& os, const ClusterNetworkLayer::Neighbour& n ) {

	os << "Weight = " << n.mWeight << "; Pos = " << n.mPosition << "; Vel = " << n.mVelocity << "; C" << ( n.mIsClusterHead ? "H" : "M" ) << "; freshness = " << n.mFreshness;
	return os;

}

void ClusterNetworkLayer::initialize(int stage)
{
    BaseNetwLayer::initialize(stage);

    if(stage == 1) {

    	mInitialised = false;

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

    	// set up result connection
    	mSigOverhead = registerSignal( "sigOverhead" );
    	mSigHelloOverhead = registerSignal( "sigHelloOverhead" );
    	mSigClusterLifetime = registerSignal( "sigClusterLifetime" );
    	mSigClusterSize = registerSignal( "sigClusterSize" );
    	mSigHeadChange = registerSignal( "sigHeadChange" );

    	mSigClusterDeathType = registerSignal( "sigDeathType" );
    	mSigClusterDeathX = registerSignal( "sigDeathX" );
    	mSigClusterDeathY = registerSignal( "sigDeathY" );

    	// set up watches
    	WATCH_SET( mClusterMembers );
    	WATCH_MAP( mNeighbours );
    	WATCH( mWeight );
    	WATCH( mClusterHead );


//     	TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
//     	char strNodeName[50];
//     	sprintf( strNodeName, "node%i_conn", mID );
//     	std::list<Coord> points;
//     	points.push_back( mMobility->getCurrentPosition() );
//     	pManager->commandAddPolygon( strNodeName, "clusterConn", TraCIScenarioManager::Color( 255, 0, 0, 255 ), true, 5, points );

    }

}


/** @brief Cleanup*/
void ClusterNetworkLayer::finish() {

	if ( mSendHelloMessage->isScheduled() )
		cancelEvent( mSendHelloMessage );
	delete mSendHelloMessage;

	if ( mFirstInitMessage && mFirstInitMessage->isScheduled() )
		cancelEvent( mFirstInitMessage );
	delete mFirstInitMessage;

	if ( mBeatMessage->isScheduled() )
		cancelEvent( mBeatMessage );
	delete mBeatMessage;

// 	TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
// 	char strNodeName[10];
// 	sprintf( strNodeName, "node%i_conn", mID );
// 	pManager->commandRemovePolygon( strNodeName, 5 );

	//std::cerr << "Node " << mID << " deleted.\n";
	BaseNetwLayer::finish();

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

		case HELLO_MESSAGE:
			receiveHelloMessage( m );
			break;

		case CH_MESSAGE:
			receiveChMessage( m );
			break;

		case JOIN_MESSAGE:
			receiveJoinMessage( m );
			break;

		case DATA:
			sendUp( decapsMsg( m ) );
			break;

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
		mFirstInitMessage = NULL;

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

    pkt->setBitLength(headerLength);	// ordinary IP packet

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

	// First get the lane we're in.
	TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
	std::string s = pManager->commandGetLaneId( dynamic_cast<TraCIMobility*>(mMobility)->getExternalId() );
	int i = s.find("_");
	mRoadID = s.substr( 0, i );
	mLaneID = s.substr( i+1 );

	int nMax = chooseClusterHead();
	if ( nMax == -1 ) {

		// this node is the best CH around, so declare it
		mIsClusterHead = true;
		mClusterHead = mID;
		mClusterMembers.clear();
		mClusterMembers.insert( mID );
		sendClusterMessage( CH_MESSAGE );

	} else {

		// we found a neighbour that's a better CH
		mIsClusterHead = false;
		mClusterHead = nMax;
		mClusterMembers.clear();
    	//std::cerr << "Node: " << mID << " joined CH!: " << nMax << "\n";
		sendClusterMessage( JOIN_MESSAGE, nMax );

	}

	mInitialised = true;

}



/** @brief Process the neighbour table in one beat. Also, update the node's weight. */
void ClusterNetworkLayer::processBeat() {

	// update the node's weight
	mWeight = calculateWeight();

	// process the neighbour table
	NeighbourIterator it = mNeighbours.begin();
	for ( ; it != mNeighbours.end(); ) {

		it->second.mFreshness -= 1;
		if ( it->second.mFreshness == 0 ) {

			unsigned int nodeId = it->first;
			it++;
			linkFailure( nodeId );

		} else {

			++it;

		}

	}

	TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
	std::string s = pManager->commandGetLaneId( dynamic_cast<TraCIMobility*>(mMobility)->getExternalId() );
	int i = s.find("_");
	mRoadID = s.substr( 0, i );
	mLaneID = s.substr( i+1 );

// 	TraCIScenarioManager *pManager = TraCIScenarioManagerAccess().get();
// 	char strNodeName[10];
// 	sprintf( strNodeName, "node%i_conn", mID );
// 
// 	std::list<Coord> points;
// 	points.push_back( mMobility->getCurrentPosition() );
// 
// 	if ( !mIsClusterHead )
// 		points.push_back( mNeighbours[mClusterHead].mPosition );
// 
// 	pManager->commandSetPolygonShape( strNodeName, points );

}



/** @brief Select a CH from the neighbour table. */
int ClusterNetworkLayer::chooseClusterHead() {

	int nCurr = -1;
	double wCurr = mWeight;

	NeighbourIterator it = mNeighbours.begin();
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
	if ( mIsClusterHead ) {

		if ( mClusterMembers.find( nodeId ) == mClusterMembers.end() )
			return;		// we don't need to do anything, because it wasn't a member of our cluster.

		/*
		 *  A member of this cluster is out of range, so erase it's ID
		 *  and emit the cluster size change signal.
		 */
		mClusterMembers.erase( nodeId );
		emit( mSigClusterSize, mClusterMembers.size() );

		// also check if we've lost all our CMs
		if ( mClusterMembers.size() == 1 ) {

			Coord pos = mMobility->getCurrentPosition();
			/*
			 * We've lost all our CMs, so this cluster counts as dead.
			 * Calculate it's lifetime, type of death, position of death, and maximum size, and log it.
			 */
			emit( mSigClusterLifetime, simTime() - mClusterStartTime );
			emit( mSigClusterSize, (double)mCurrentMaximumClusterSize );
			emit( mSigClusterDeathType, (double)CD_Attrition );
			emit( mSigClusterDeathX, pos.x );
			emit( mSigClusterDeathY, pos.y );

			mCurrentMaximumClusterSize = 0;
			mClusterStartTime = 0;
	    	//std::cerr << "Cluster (CH: " << mID << ") has died of attrition!\n";

		}

	} else if ( nodeId == mClusterHead ) {

		/*
		 *	This CM lost its CH, so recluster and emit the CH change signal.
		 */
		init();
		emit( mSigHeadChange, 1 );

	}

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
		unsigned int r = UINT_MAX;
		if ( detSq > 0 )
			r = (int)floor( ( sqrt( detSq ) - b ) / BEAT_LENGTH );

		freshness = std::min( 3*mInitialFreshness, r );

	}

	mNeighbours[nodeId].mFreshness = freshness;

}



/** @brief Determine whether the given node is a suitable CH. */
bool ClusterNetworkLayer::testClusterHeadChange( unsigned int nodeId ) {

	bool t1 = mNeighbours[nodeId].mIsClusterHead;
	bool t2 = mNeighbours[nodeId].mWeight > ( mIsClusterHead ? mWeight : mNeighbours[mClusterHead].mWeight );
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

		// If this was a CH, the cluster is dead, so log lifetime
		if ( mIsClusterHead && mClusterMembers.size() > 1 ) {

			Coord pos = mMobility->getCurrentPosition();
			emit( mSigClusterLifetime, simTime() - mClusterStartTime );
			emit( mSigClusterSize, (double)mCurrentMaximumClusterSize );
			emit( mSigClusterDeathType, (double)CD_Cannibal );
			emit( mSigClusterDeathX, pos.x );
			emit( mSigClusterDeathY, pos.y );

			mCurrentMaximumClusterSize = 0;
			mClusterStartTime = 0;
	    	//std::cerr << "Cluster (CH: " << mID << ") has died! " << mID << " joined CH" << m->getNodeId() << ". w("  << mID <<  ")=" << mWeight << "; w("  << m->getNodeId() <<  ")=" << m->getWeight() << "; \n";

		} else if ( mClusterHead != m->getNodeId() ) {

			// we just changed clusterhead.
	    	//std::cerr << "Node: " << mID << " joined CH: " << m->getNodeId() << "\n";

		}

        emit( mSigHeadChange, 1 );
		mIsClusterHead = false;
		mClusterMembers.clear();
		mClusterHead = m->getNodeId();
		sendClusterMessage( JOIN_MESSAGE, m->getNodeId() );

	}

	if ( m->getTtl() > 1 )
		sendClusterMessage( HELLO_MESSAGE, -1, m->getTtl()-1 );

	delete m;

}



/** @brief Handle a CH message. */
void ClusterNetworkLayer::receiveChMessage( ClusterControlMessage *m ) {

	updateNeighbour(m);

	if ( testClusterHeadChange( m->getNodeId() ) ) {

		// If this was a CH, the cluster is dead, so log lifetime
		if ( mIsClusterHead ) {

			Coord pos = mMobility->getCurrentPosition();
			emit( mSigClusterLifetime, simTime() - mClusterStartTime );
			emit( mSigClusterSize, (double)mCurrentMaximumClusterSize );
			emit( mSigClusterDeathType, (double)CD_Cannibal );
			emit( mSigClusterDeathX, pos.x );
			emit( mSigClusterDeathY, pos.y );

			mCurrentMaximumClusterSize = 0;
			mClusterStartTime = 0;

	    	//std::cerr << "Cluster (CH: " << mID << ") has died! " << mID << " joined CH" << m->getNodeId() << ". w("  << mID <<  ")=" << mWeight << "; w("  << m->getNodeId() <<  ")=" << m->getWeight() << "; \n";
            emit( mSigHeadChange, 1 );

		}

        emit( mSigHeadChange, 1 );
		mIsClusterHead = false;
		mClusterMembers.clear();
		mClusterHead = m->getNodeId();
		sendClusterMessage( JOIN_MESSAGE, m->getNodeId() );

	} else {

		if ( mIsClusterHead ) {

			bool sizeChanged = false;

			if ( mClusterMembers.find( m->getNodeId() ) != mClusterMembers.end() ) {

				mClusterMembers.erase( m->getNodeId() );
				sizeChanged = true;

			}

			if ( sizeChanged )
				mCurrentMaximumClusterSize = std::max( mCurrentMaximumClusterSize, (int)mClusterMembers.size() );

		}

	}

	if ( m->getTtl() > 1 )
		sendClusterMessage( CH_MESSAGE, -1, m->getTtl()-1 );

	delete m;

}



/** @brief Handle a JOIN message. */
void ClusterNetworkLayer::receiveJoinMessage( ClusterControlMessage *m ) {

	updateNeighbour(m);

	if ( mIsClusterHead ) {

		bool sizeChanged = false;
		if ( m->getTargetNodeId() == mID ) {

			mClusterMembers.insert( m->getNodeId() );
			sizeChanged = true;
			if ( mClusterMembers.size() == 2 ) {
				mClusterStartTime = simTime();	// start of cluster lifetime.
		        //std::cerr << "Node: " << mID << " has become a CH!\n";
			}

		} else if ( mClusterMembers.find( m->getNodeId() ) != mClusterMembers.end() ) {

			mClusterMembers.erase( m->getNodeId() );
			sizeChanged = true;
			if ( mClusterMembers.size() == 1 ) {
		    	//std::cerr << "Cluster (CH: " << mID << ") has died of attrition!\n";
	            emit( mSigClusterLifetime, simTime() - mClusterStartTime );
				Coord pos = mMobility->getCurrentPosition();
				emit( mSigClusterLifetime, simTime() - mClusterStartTime );
				emit( mSigClusterSize, (double)mCurrentMaximumClusterSize );
				emit( mSigClusterDeathType, (double)CD_Attrition );
				emit( mSigClusterDeathX, pos.x );
				emit( mSigClusterDeathY, pos.y );

				mCurrentMaximumClusterSize = 0;
				mClusterStartTime = 0;
				sizeChanged = false;
			}

		}

		if ( sizeChanged )
			mCurrentMaximumClusterSize = std::max( mCurrentMaximumClusterSize, (int)mClusterMembers.size() );

	} else if ( mClusterHead == m->getNodeId() ) {

		init();
		emit( mSigHeadChange, 1 );

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
	mNeighbours[m->getNodeId()].mRoadID = m->getRoadId();
	mNeighbours[m->getNodeId()].mLaneID = m->getLaneId();
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
    pkt->setBitLength(432);	// size of the control packet packet.

    if ( kind != HELLO_MESSAGE )
    	emit( mSigOverhead, 432 );
    else
    	emit( mSigHelloOverhead, 432 );

    // fill the cluster control fields
    pkt->setNodeId( mID );
    pkt->setWeight( mWeight );
    if ( nHops == -1 )
    	nHops = mHopCount;
    pkt->setTtl( nHops );

    pkt->setRoadId( mRoadID.c_str() );
    pkt->setLaneId( mLaneID.c_str() );

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




