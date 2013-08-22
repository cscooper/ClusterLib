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
#include "RmacControlMessage_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"
#include "BaseMobility.h"
#include "BaseConnectionManager.h"
#include "ChannelAccess.h"

#include "TraCIScenarioManager.h"
#include "TraCIMobility.h"

#include "RmacNetworkLayer.h"


Define_Module(RmacNetworkLayer);


/**
 * @name Getters
 * @brief Get the data members.
 *
 * These get the data members.
 */
/*@{*/

/** Get maximum cluster size. */
unsigned int RmacNetworkLayer::GetConnectionLimits() {
	return mConnectionLimits;
}


/** Get distance threshold. */
double RmacNetworkLayer::GetDistanceThreshold() {
	return mDistanceThreshold;
}


double RmacNetworkLayer::GetTimeThreshold() {
	return mTimeThreshold;
}



int RmacNetworkLayer::GetStateCount() {
	return CLUSTER_HEAD_MEMBER+1;
}


bool RmacNetworkLayer::IsClusterHead() {
	return mCurrentState == CLUSTER_HEAD || mCurrentState == CLUSTER_HEAD_MEMBER;
}


int RmacNetworkLayer::GetClusterState() {
	return mCurrentState;
}




/*@}*/


/** @brief Initialization of the module and some variables. */
void RmacNetworkLayer::initialize(int stage)
{
    ClusterAlgorithm::initialize(stage);

    if( stage == 1 ) {

        mInitialised = false;

        mId = getId();
        mMobility = FindModule<BaseMobility*>::findSubModule(findHost());
        mClusterHead = -1;
        mCurrentState = UNCLUSTERED;

        ChannelAccess *channelAccess = FindModule<ChannelAccess*>::findSubModule(findHost());
        mZoneOfInterest = 2 * channelAccess->getConnectionManager( channelAccess->getParentModule() )->getMaxInterferenceDistance();
        mTransmitRangeSq = pow( mZoneOfInterest/2, 2 );

        // get parameters
        mConnectionLimits = par("connectionLimits").longValue();
        mDistanceThreshold = par("distanceThreshold").doubleValue();
        mTimeThreshold = par("timeThreshold").doubleValue();
        mInquiryPeriod = par("inquiryPeriod").doubleValue();
        mInquiryResponsePeriod = par("inquiryResponsePeriod").doubleValue();
        mJoinTimeoutPeriod = par("joinTimeoutPeriod").doubleValue();

        // Setup messages
        mTimeoutMessage = new cMessage( "timeout" );

    }

}




/** @brief Cleanup*/
void RmacNetworkLayer::finish() {

    if ( mTimeoutMessage->isScheduled() )
        cancelEvent( mTimeoutMessage );
    delete mTimeoutMessage;

    ClusterAlgorithm::finish();

}




/** @brief Handle messages from upper layer */
void RmacNetworkLayer::handleUpperMsg(cMessage* msg) {

    assert(dynamic_cast<cPacket*>(msg));
    sendDown(encapsMsg(static_cast<cPacket*>(msg)));

}


/** @brief Handle messages from lower layer */
void RmacNetworkLayer::handleLowerMsg(cMessage* msg) {

    RmacControlMessage *m = static_cast<RmacControlMessage *>(msg);
    coreEV << " handling packet from " << m->getSrcAddr() << std::endl;

    // Update the data for the neighbour this came from.
    int id = m->getNodeId();

    switch( m->getKind() ) {

        case INQ_MESSAGE:               // We received an INQ message.
            // We respond to the inquiry.
            SendInquiryResponse( id );
            break;

        case INQ_RESPONSE_MESSAGE:      // We received an INQ response.

            if ( mProcessState == INQUIRY ) {
                if ( mTimeoutMessage->isScheduled() )
                    cancelEvent( mTimeoutMessage );
                mProcessState = COLLECTING_INQUIRY;
                scheduleAt( simTime() + mInquiryResponsePeriod, mTimeoutMessage );
            }

            if ( mProcessState != COLLECTING_INQUIRY )
            	UpdateNeighbour(m);

            break;

        case JOIN_MESSAGE:              // We received a JOIN message.

            // Update the data for this neighbour.
        	UpdateNeighbour(m);

            // Check if we've reached our connection limits?
            if ( mClusterMembers.size() == mConnectionLimits ) {

                // Deny the node to become a CM.
                SendJoinDenial( id );

            } else {

                // A neighbour has asked this node to become a CH.
                ClusterState prevState = mCurrentState;

                // Update the state.
                if ( mCurrentState == UNCLUSTERED )
                    mCurrentState = CLUSTER_HEAD;
                else if ( mCurrentState == CLUSTER_MEMBER )
                    mCurrentState = CLUSTER_HEAD_MEMBER;

                // Has the state changed?
                if ( prevState != mCurrentState ) {      // Start the cluster.
                    mClusterStartTime = simTime();
                    mCurrentMaximumClusterSize = 1;
                }

                // Add this node ID to the cluster members.
                mClusterMembers.insert( id );

                // Send the node a Join response.
                SendJoinResponse( id );

            }

            break;

        case JOIN_RESPONSE_MESSAGE:     // We received a message permitting this node to join a cluster.

            // Update the data for this neighbour.
        	UpdateNeighbour(m);

            if ( mProcessState == JOINING ) {
                // We've received a JOIN response.
                mClusterHead = id;
                mCurrentState = CLUSTER_MEMBER;
                mProcessState = CLUSTERED;
            }

            break;

        case JOIN_DENIAL_MESSAGE:   // We received a message denying to join a cluster.

            if ( mProcessState == JOINING ) {
                cancelEvent( mTimeoutMessage );
                if ( Process( true ) )
                    Process( false );
            }

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
void RmacNetworkLayer::handleSelfMsg(cMessage* msg) {

    if ( msg == mTimeoutMessage )
        if ( Process( true ) )
            Process( false );

}



/** @brief decapsulate higher layer message from RmacControlMessage */
cMessage* RmacNetworkLayer::decapsMsg( RmacControlMessage *msg ) {

    cMessage *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());

    // delete the netw packet
    delete msg;
    return m;

}


/** @brief Encapsulate higher layer packet into a RmacControlMessage */
NetwPkt* RmacNetworkLayer::encapsMsg( cPacket *appPkt ) {

    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    coreEV <<"in encaps...\n";

    RmacControlMessage *pkt = new RmacControlMessage(appPkt->getName(), DATA);

    pkt->setBitLength(headerLength);    // ordinary IP packet

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




/**
 * @brief Process the clustering algorithm.
 * @param[in] bTimeout If true, this must process the current state behaviour in the event of a timeout.
 *
 * This processes the clustering algorithm.
 *
 * @return Returns true if the function needs to be called again, e.g. if a state change occurred.
 */
bool RmacNetworkLayer::Process( bool bTimeout ) {

    if ( bTimeout && ( mProcessState == START || mProcessState == CLUSTERED ) )
        return false;     // We don't have timeouts for these.

    bool returnValue = false;
    switch( mProcessState ) {

        case START: // Start the clustering process!

            // Get the one-hop neighbours
            GetOneHopNeighbours();

            if ( mOneHopNeighbours.empty() ) {

                // There are no one-hop neighbours, so start the Inquiry process.
                mProcessState = INQUIRY;

            } else {

                // We have a set of one hop neighbours. Apply the NPA.
                std::sort( mOneHopNeighbours.begin(), mOneHopNeighbours.end(), RmacNetworkLayer::NodePrecidenceAlgorithmPredicate );

                // Now go to the joining phase.
                mProcessState = JOINING;

            }

            returnValue = true;

            break;

        case INQUIRY:       // We're looking for our one-hop neighbours.

            if ( bTimeout ) // The timeout has occurred while we're waiting for an INQ response.
                coreEV << "INQ timeout!\n";

            SendInquiry();
            scheduleAt( simTime() + mInquiryPeriod, mTimeoutMessage );

            break;

        case COLLECTING_INQUIRY:    // We're collecting INQ responses.

            if ( bTimeout ) // We've reached the end of the collection period.
                mProcessState = START;

            break;

        case JOINING:       // We're going through our sorted one-hop neighbours and joining the optimal CH

            if ( bTimeout ) { // Join timeout. Remove this neighbour from the list.

                mOneHopNeighbours.erase( mOneHopNeighbours.begin() );

                // Check if we've exhausted all one-hop neighbours. If we have, go back to the INQUIRY phase.
                if ( mOneHopNeighbours.empty() ) {
                    mProcessState = INQUIRY;
                    returnValue = true;
                    break;
                }

            }

            SendJoinRequest( mOneHopNeighbours[0].mId );
            scheduleAt( simTime() + mJoinTimeoutPeriod, mTimeoutMessage );

            break;

    };

    return returnValue;

}




/**
 * @brief Get the one-hop neighbours.
 */
void RmacNetworkLayer::GetOneHopNeighbours() {

    mOneHopNeighbours.clear();
    NeighbourIterator it = mNeighbours.begin();
    for ( ; it != mNeighbours.end(); it++ )
        if ( it->second.mHopCount == 1 )
            mOneHopNeighbours.push_back( it->second );

}




/**
 * @brief Send an control message
 */
void RmacNetworkLayer::SendControlMessage( int type, int id ) {

    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    if ( id == -1 )
        netwAddr = LAddress::L3BROADCAST;
    else
        netwAddr = mNeighbours[id].mNetworkAddress;

    coreEV << "sending RMAC ";
    if ( type == INQ_MESSAGE )
        coreEV << "INQ";
    else if ( type == INQ_RESPONSE_MESSAGE )
        coreEV << "INQ_RESP";
    else if ( type == JOIN_MESSAGE )
        coreEV << "JOIN_REQ";
    else if ( type == JOIN_RESPONSE_MESSAGE )
        coreEV << "JOIN_RESP";
    else if ( type == JOIN_DENIAL_MESSAGE )
        coreEV << "JOIN_DENY";
    else if ( type == DATA )
        coreEV << "DATA";
    coreEV << " message...\n";

    RmacControlMessage *pkt = new RmacControlMessage( "cluster-ctrl", type );
    pkt->setBitLength(352); // size of the control packet packet.

    // fill the cluster control fields
    pkt->setNodeId( mId );

    Coord p = mMobility->getCurrentPosition();
    pkt->setXPosition( p.x );
    pkt->setYPosition( p.y );

    p = mMobility->getCurrentSpeed();
    pkt->setXVelocity( p.x );
    pkt->setYVelocity( p.y );

    pkt->setConnectionCount( mNeighbours.size() );

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

    sendDown( pkt );


}



/**
 * @brief Send an INQ broadcast.
 */
void RmacNetworkLayer::SendInquiry() {
    SendControlMessage( INQ_MESSAGE );
}


/**
 * @brief Send a INQ_RESP unicast.
 * @param[in] id Identifier of the target node.
 */
void RmacNetworkLayer::SendInquiryResponse( int id ) {
    SendControlMessage( INQ_RESPONSE_MESSAGE, id );
}


/**
 * @brief Send a JOIN request unicast.
 * @param[in] id Identifier of the target node.
 */
void RmacNetworkLayer::SendJoinRequest( int id ) {
    SendControlMessage( JOIN_MESSAGE, id );
}


/**
 * @brief Send a JOIN response unicast.
 * @param[in] id Identifier of the target node.
 */
void RmacNetworkLayer::SendJoinResponse( int id ) {
    SendControlMessage( JOIN_RESPONSE_MESSAGE, id );
}


/**
 * @brief Send a JOIN denial unicast.
 * @param[in] id Identifier of the target node.
 */
void RmacNetworkLayer::SendJoinDenial( int id ) {
    SendControlMessage( JOIN_DENIAL_MESSAGE, id );
}




/**
 * @brief Calculate the Link Expiration Time.
 * @param[in] pos Position of the target node.
 * @param[in] vel Velocity of the target node.
 * @return The time until the link expires.
 */
double RmacNetworkLayer::CalculateLinkExpirationTime( Coord pos, Coord vel ) {

    Coord p = mMobility->getCurrentPosition();
    Coord v = mMobility->getCurrentSpeed();

    double a = v.x - vel.x;
    double b = p.x - pos.x;
    double c = v.y - vel.y;
    double d = p.y - pos.y;
    double e = ( a*a + c*c );

    return ( sqrt( e * mTransmitRangeSq - pow( a*d - b*c, 2 ) ) - ( a*b + c*d ) ) / e;

}


/**
 * Update neighbour data with the given message.
 */
void RmacNetworkLayer::UpdateNeighbour( RmacControlMessage *m ) {

	int id = m->getNodeId();
    mNeighbours[id].mId = id;
    mNeighbours[id].mPosition.x = m->getXPosition();
    mNeighbours[id].mPosition.y = m->getYPosition();
    mNeighbours[id].mVelocity.x = m->getXVelocity();
    mNeighbours[id].mVelocity.y = m->getYVelocity();
    mNeighbours[id].mConnectionCount = m->getConnectionCount();
    mNeighbours[id].mNetworkAddress = m->getSrcAddr();
    mNeighbours[id].mHopCount = 1;
    mNeighbours[id].mProviderId = id;
    mNeighbours[id].mDistanceToNode = mMobility->getCurrentPosition().distance( mNeighbours[id].mPosition );
    mNeighbours[id].mLinkExpirationTime = CalculateLinkExpirationTime( mNeighbours[id].mPosition, mNeighbours[id].mVelocity );
    mNeighbours[id].mTimeStamp = simTime();
    mNeighbours[id].mDataOwner = this;

}




/**
 * @brief Sorting predicate for the Node Precidence Algorithm.
 *
 * This implements the sorting mechanism for the Node Precidence Algorithm.
 */
bool RmacNetworkLayer::NodePrecidenceAlgorithmPredicate( const Neighbour &n, const Neighbour &m ) {

    // Has 'm' reached its maximum connection limits?
    if ( m.mConnectionCount == n.mDataOwner->GetConnectionLimits() )
        return true;    // 'n' has higher precidence.

    // Has 'n' reached its maximum connection limits?
    if ( n.mConnectionCount == n.mDataOwner->GetConnectionLimits() )
        return false;   // 'm' has higher precidence.

    // Is the difference in distance between this node and the others greater than a certain threshold?
    if ( fabs( n.mDistanceToNode - m.mDistanceToNode ) >= n.mDataOwner->GetDistanceThreshold() ) {

        // Is 'n' closer than 'm'?
        if ( n.mDistanceToNode < m.mDistanceToNode )
            return true;    // 'n' has higher precidence.
        else
            return false;   // 'm' has higher precidence.

    }

    // Is the difference in LET greater than a certain threshold?
    if ( fabs( n.mLinkExpirationTime - m.mLinkExpirationTime ) >= n.mDataOwner->GetTimeThreshold() ) {

        // Is 'n' going to be in range longer than 'm'?
        if ( n.mLinkExpirationTime > m.mLinkExpirationTime )
            return true;    // 'n' has higher precidence.
        else
            return false;   // 'm' has higher precidence.
    }

    // Does 'n' have a larger cluster than 'm'?
    if ( n.mConnectionCount > m.mConnectionCount )
        return true;    // 'n' has higher precidence.

    return false;       // 'm' has higher precidence.

}





