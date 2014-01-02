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
#include <algorithm>

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

#include "ClusterAnalysisScenarioManager.h"
#include "RmacNetworkLayer.h"
#include "RMACData.h"


Define_Module(RmacNetworkLayer);



std::ostream& operator<<( std::ostream& os, const RmacNetworkLayer::Neighbour& n ) {

	os << "Pos = " << n.mPosition << "; Vel = " << n.mVelocity << "; Connection = " << n.mConnectionCount;
	return os;

}

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


/** Get a neighbour with the given id. */
const RmacNetworkLayer::Neighbour &RmacNetworkLayer::GetNeighbour( const int &id ) {
	return mNeighbours[id];
}


int RmacNetworkLayer::GetCurrentLevelCount() {
	return mMaximumLevels;
}


int RmacNetworkLayer::GetStateCount() {
	return CLUSTER_HEAD_MEMBER+1;
}


bool RmacNetworkLayer::IsClusterHead() {
	return mCurrentState == CLUSTER_HEAD || mCurrentState == CLUSTER_HEAD_MEMBER;
}


bool RmacNetworkLayer::IsSubclusterHead() {
	return mCurrentState == CLUSTER_HEAD_MEMBER;
}


bool RmacNetworkLayer::IsHierarchical() {
	return true;
}


int RmacNetworkLayer::GetClusterState() {
	return mCurrentState;
}


void RmacNetworkLayer::UpdateMessageString() {
    char s[100];
    static bool b = true;
    sprintf( s, "%i %i %s", mId, mMaximumLevels, (b?"!":"*") );
    b = !b;
    mMessageString = s;
}


int RmacNetworkLayer::GetMinimumClusterSize() {
	return 0;
}


/*@}*/


void RmacNetworkLayer::ClusterStarted() {

	ClusterAlgorithm::ClusterStarted();
	mCurrentLevels = 0;
	scheduleAt( simTime() + mPollInterval, mPollTriggerMessage );
	scheduleAt( simTime() + mPollInterval*4, mClusterPresenceBeaconMessage );

}


void RmacNetworkLayer::ClusterMemberAdded( int id ) {

	ClusterAlgorithm::ClusterMemberAdded(id);
	// TODO: Assess changes to hierarchy and propagate them.
	NodeIdList record;
	UpdateLevelOfMember( id, record, false );

}




void RmacNetworkLayer::ClusterMemberRemoved( int id ) {

	ClusterAlgorithm::ClusterMemberRemoved(id);
	// Assess changes to hierarchy and propagate them.
	NodeIdList record;
	UpdateLevelOfMember( id, record, true );

}


void RmacNetworkLayer::ClusterDied( int deathType ) {

	ClusterAlgorithm::ClusterDied(deathType);
	// TODO: CLUSTER HAS DIED! Record data.
	//std::cerr << mId << ": Cluster Died!\n";
	if ( mCurrentState == CLUSTER_HEAD_MEMBER ) {
		mCurrentState = CLUSTER_MEMBER;	// Just go back to being a normal CM
	} else if ( mCurrentState == CLUSTER_HEAD ) {
//		std::cerr << "Cluster depth: " << mCurrentLevels << "\n";
		emit( mSigClusterDepth, mCurrentLevels );
		// Go to unclustered state and restart.
		mCurrentState = UNCLUSTERED;
		mProcessState = START;
		mClusterHead = -1;
		Process();
	}
	mMaximumLevels = mCurrentLevels = 0;
	if ( mPollTriggerMessage->isScheduled() )
		cancelEvent( mPollTriggerMessage );
	if ( mPollPeriodFinishedMessage->isScheduled() )
		cancelEvent( mPollPeriodFinishedMessage );
	if ( mClusterPresenceBeaconMessage->isScheduled() )
		cancelEvent( mClusterPresenceBeaconMessage );

}



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
        mProcessState = START;

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
        mPollInterval = par("pollInterval").doubleValue();
        mPollTimeout = par("pollTimeout").doubleValue();
        mMissedPingThreshold = par("missedPingThreshold").longValue();

        // Setup messages
        // Phase 1 clustering messages
        mFirstTimeProcess = new cMessage( "firstTime" );
        mInquiryTimeoutMessage = new cMessage( "timeoutINQ" );
        mInquiryResponseTimeoutMessage = new cMessage( "timeoutINQ_RESP" );
        mJoinTimeoutMessage = new cMessage( "timeoutJOIN" );
        mJoinDenyMessage = new cMessage( "joinDeny" );

        // Phase 2 maintenance messages
        mPollTriggerMessage = new cMessage( "pollTrigger" );
        mPollPeriodFinishedMessage = new cMessage( "pollFinish" );
        mPollTimeoutMessage = new cMessage( "pollTimeout" );

        // Phase 3 unification messages
        mClusterPresenceBeaconMessage = new cMessage( "clusterPresence" );
        mClusterUnifyTimeoutMessage = new cMessage( "unifyTimeout" );

        // schedule the start message
      	scheduleAt( simTime() + float(rand()) / RAND_MAX, mFirstTimeProcess );

      	mMaximumLevels = mCurrentLevels = 0;

		// set up result collection
		mSigClusterDepth = registerSignal( "sigClusterDepth" );

    	// set up watches
    	WATCH_SET( mClusterMembers );
    	WATCH_MAP( mNeighbours );
    	WATCH( mClusterHead );

    }

}




/** @brief Cleanup*/
void RmacNetworkLayer::finish() {

	if ( IsClusterHead() )
		ClusterDied( CD_Attrition );

    if ( mFirstTimeProcess->isScheduled() )
        cancelEvent( mFirstTimeProcess );
    delete mFirstTimeProcess;

    if ( mInquiryTimeoutMessage->isScheduled() )
        cancelEvent( mInquiryTimeoutMessage );
    delete mInquiryTimeoutMessage;

    if ( mInquiryResponseTimeoutMessage->isScheduled() )
        cancelEvent( mInquiryResponseTimeoutMessage );
    delete mInquiryResponseTimeoutMessage;

    if ( mJoinTimeoutMessage->isScheduled() )
        cancelEvent( mJoinTimeoutMessage );
    delete mJoinTimeoutMessage;

    if ( mJoinDenyMessage->isScheduled() )
    	cancelEvent( mJoinDenyMessage );
    delete mJoinDenyMessage;

    if ( mPollTriggerMessage->isScheduled() )
        cancelEvent( mPollTriggerMessage );
    delete mPollTriggerMessage;

    if ( mPollPeriodFinishedMessage->isScheduled() )
    	cancelEvent( mPollPeriodFinishedMessage );
    delete mPollPeriodFinishedMessage;

    if ( mPollTimeoutMessage->isScheduled() )
    	cancelEvent( mPollTimeoutMessage );
    delete mPollTimeoutMessage;

    if ( mClusterPresenceBeaconMessage->isScheduled() )
    	cancelEvent( mClusterPresenceBeaconMessage );
    delete mClusterPresenceBeaconMessage;

    if ( mClusterUnifyTimeoutMessage->isScheduled() )
    	cancelEvent( mClusterUnifyTimeoutMessage );
    delete mClusterUnifyTimeoutMessage;

    ClusterAlgorithm::finish();

}






/** @brief Handle messages from upper layer */
void RmacNetworkLayer::handleUpperMsg(cMessage* msg) {

    assert(dynamic_cast<cPacket*>(msg));
    NetwPkt *m = encapsMsg(static_cast<cPacket*>(msg));
    if ( m )
    	sendDown(m);

}


/** @brief Handle messages from lower layer */
void RmacNetworkLayer::handleLowerMsg(cMessage* msg) {

	bool deleteMsg = true;
	int role = -1;
    RmacControlMessage *m = static_cast<RmacControlMessage *>(msg);
    coreEV << " handling packet from " << m->getSrcAddr() << std::endl;

    // Update the data for the neighbour this came from.
    int id = m->getNodeId();

    // Update the data for this neighbour.
    UpdateNeighbour(m);
    switch( m->getKind() ) {

        case INQ_MESSAGE:               // We received an INQ message.
            // We respond to the inquiry.
            SendInquiryResponse( id );
            break;

        case INQ_RESPONSE_MESSAGE:      // We received an INQ response.

            if ( mProcessState == INQUIRY ) {
                if ( mInquiryTimeoutMessage->isScheduled() )
                    cancelEvent( mInquiryTimeoutMessage );
                mProcessState = COLLECTING_INQUIRY;
                scheduleAt( simTime() + mInquiryResponsePeriod, mInquiryResponseTimeoutMessage );
            }

            break;

        case JOIN_MESSAGE:              // We received a JOIN message.

            //std::cerr << mId << ": Received JOIN from " << id << "... ";

            // Check if we've reached our connection limits?
            if ( mClusterMembers.size() == mConnectionLimits ) {

                // Deny the node to become a CM.
                SendJoinDenial( id );
                //std::cerr << "Denied!" << std::endl;

            } else {

            	// We're accepting this JOIN request.
            	// If we got a join request from the same node we're trying to join,
            	// ignore this request.
            	if ( mProcessState == JOINING && id == mOneHopNeighbours[0] ) {
            		//std::cerr << "IGNORED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            		break;
            	}

            	// If we're in the middle of an INQ phase, cancel any timeout events.
            	if ( mProcessState == INQUIRY )
            		cancelEvent( mInquiryTimeoutMessage );
            	else if ( mProcessState == COLLECTING_INQUIRY )
            		cancelEvent( mInquiryResponseTimeoutMessage );

                //std::cerr << "Accepted!";

                // A neighbour has asked this node to become a CH.
                ClusterState prevState = mCurrentState;

                // Update the state.
                if ( mCurrentState == UNCLUSTERED )
                    mCurrentState = CLUSTER_HEAD;
                else if ( mCurrentState == CLUSTER_MEMBER )
                	mCurrentState = CLUSTER_HEAD_MEMBER;

                // Has the state changed?
                if ( prevState != mCurrentState ) {
                	// Start the cluster.
                	if ( prevState == UNCLUSTERED || prevState == CLUSTER_MEMBER ) {
                		// Cluster has started!
                		ClusterStarted();
                	}
                	if ( mCurrentState == CLUSTER_HEAD_MEMBER ) {
                		mClusterHierarchy.clear();
                		mClusterHierarchy.push_back( mClusterHead );
                	}
                    //std::cerr << " Became " << ( mCurrentState == CLUSTER_HEAD_MEMBER ? "CHM" : "CH" ) << "!";
                }

                // Add this node ID to the cluster members.
                ClusterMemberAdded( id );

                //std::cerr << "|C|=" << mClusterMembers.size() << "\n";

                mProcessState = CLUSTERED;
                Process();

                // Send the node a Join response.
                SendJoinResponse( id );

            }

            break;

        case JOIN_RESPONSE_MESSAGE:     // We received a message permitting this node to join a cluster.

            if ( mProcessState == JOINING || mProcessState == CLUSTERED ) {
                // We've received a JOIN response.
                mClusterHead = id;
                if ( mCurrentState == UNCLUSTERED )
                	mCurrentState = CLUSTER_MEMBER;
                else if ( mCurrentState == CLUSTER_HEAD )
                	mCurrentState = CLUSTER_HEAD_MEMBER;
                mProcessState = CLUSTERED;
                cancelEvent( mJoinTimeoutMessage );
                Process();
                if ( mPollTimeoutMessage->isScheduled() )
                	cancelEvent( mPollTimeoutMessage );
                scheduleAt( simTime() + mPollTimeout, mPollTimeoutMessage );
                //std::cerr << mId << ": Joined " <<  mOneHopNeighbours[0] << std::endl;
            }

            break;

        case JOIN_DENIAL_MESSAGE:   // We received a message denying to join a cluster.

            //std::cerr << mId << ": Request to join " <<  mOneHopNeighbours[0] << " denied!" << std::endl;
            if ( mProcessState == JOINING ) {
                cancelEvent( mJoinTimeoutMessage );
                Process( mJoinDenyMessage );	// Process the JOIN_DENY
            }

            break;

        case POLL_MESSAGE:

        	if ( id != mClusterHead ) {
        		//std::cerr << mId << ": Polled by CH that is not mine!\n";
        		break;	// Ignore this POLL if it's not from our CH.
        	}

        	// Get the cluster member table out of the frame.
        	mTemporaryClusterRecord.clear();
        	for ( NeighbourIdSetIterator it = m->getNeighbourIdTable().begin(); it != m->getNeighbourIdTable().end(); it++ )
        		mTemporaryClusterRecord.insert(*it);

        	cancelEvent( mPollTimeoutMessage );
        	// Update our view of the hierarchy
        	mClusterHierarchy = m->getClusterHierarchy();
        	if ( ListHasValue( mClusterHierarchy, mId ) ) {
        		// We're already part of the hierarchy of the CH that just polled us.
        		// We may be the CH of it's CH or further up the hierarchy.
        		// Thus we have detected a cyclical structure.
        		//std::cerr << mId << ": Detected cyclical cluster structure!!!\n";
        		LeaveCluster();
        		break;
        	}

            scheduleAt( simTime() + mPollTimeout, mPollTimeoutMessage );
        	AcknowledgePoll( id );

        	break;

        case POLL_ACK_MESSAGE:

        	mWaitingPollAcks.erase(id);
        	mNeighbours[id].mMissedPings = 0;
        	if ( mWaitingPollAcks.empty() ) {
        		// All nodes ACK'd
        		cancelEvent( mPollPeriodFinishedMessage );
        		scheduleAt( simTime()+mPollInterval, mPollTriggerMessage );
        	}

        	break;

        case SEND_CLUSTER_PRESENCE_MESSAGE:

        	if ( id != mClusterHead ) {
        		//std::cerr << mId << ": Asked to broadcast CLUS_PRES by CH (" << id << ") that is not ours (" << mClusterHead << ")!\n";
        		break;	// Ignore this command if it's from a different CH.
        	}

        	// Get the cluster member table out of the frame.
        	mTemporaryClusterRecord.clear();
        	//std::cerr << mId << ": Sending cluster presence message with cluster table: [";
        	for ( NeighbourIdSetIterator it = m->getNeighbourIdTable().begin(); it != m->getNeighbourIdTable().end(); it++ ) {
        		mTemporaryClusterRecord.insert(*it);
        		//std::cerr << *it << " ";
        	}
        	//std::cerr << "]\n";

        	BroadcastClusterPresence();

        	break;

        case CLUSTER_PRESENCE_MESSAGE:

    		// If this CLUS_PRES came from a CHM or CM, and we're also a CHM, we
    		// DO NOT UNIFY!
    		if ( mCurrentState == CLUSTER_HEAD_MEMBER && m->getClusterHead() != -1 )
    			break;

        	// Only CHs need to do this.
        	if ( !IsClusterHead() )
        		break;

        	// Ignore this if we're already trying to unify with another cluster.
        	if ( mProcessState == UNIFYING )
        		break;

        	// Check if this is from a disjoint cluster
        	if ( !EvaluateClusterPresence(m) )
        		break; // This is a connected cluster!

			// This is from a disjoint cluster, so we need to make
			// a unification request.
			RequestClusterUnification( m->getNodeId() );
			mProcessState = UNIFYING;
			Process();

        	break;

        case CLUSTER_UNIFY_REQUEST_MESSAGE:

        	// We just got a unify request message.

        	// If we're UNCLUSTERED, we send a rejection message.
        	if ( mCurrentState == UNCLUSTERED ) {
        		SendUnificationResponse( m->getNodeId(), -1 );
        	}

        	// Determine the role of the requesting node in our cluster.
        	role = -1;

        	if ( IsClusterHead() ) {

        		// I am a CH or CHM
        		if ( m->getNeighbourIdTable().size() <= mClusterMembers.size() ) {
        			// My cluster's bigger
        			if ( m->getClusterHead() == -1 ) {	// The requesting node is a CH
        				role = CLUSTER_HEAD_MEMBER;	// So I'll make it one of my members.
        			}
        		} else if ( mCurrentState == CLUSTER_HEAD )	{
        			// Mine's smaller.
        			role = CLUSTER_HEAD;		// So I'll have to become one of it's members.
        		}

        	} else {

        		if ( m->getClusterHead() == -1 && m->getNeighbourIdTable().size() <= mTemporaryClusterRecord.size() ) {
        			// I'll become CHM, and the node will become one of my members.
        			role = CLUSTER_HEAD_MEMBER;
        		}

        	}

        	if ( role == CLUSTER_HEAD ) {
        		// We've decided the requesting node will be a CH,
        		// so I must set my state to CHM and set this node
        		// as my CH.
        		mCurrentState = CLUSTER_HEAD_MEMBER;
        		mClusterHead = m->getNodeId();
        		if ( mPollTimeoutMessage->isScheduled() )
        			cancelEvent( mPollTimeoutMessage );
                scheduleAt( simTime() + mPollTimeout, mPollTimeoutMessage );
        	} else if ( role == CLUSTER_HEAD_MEMBER ) {
        		// The requesting node will become a CHM and member of my
        		// cluster. So I add it to my node list.
        		if ( mCurrentState == CLUSTER_MEMBER ) {
        			mCurrentState = CLUSTER_HEAD_MEMBER;
            		if ( mPollTriggerMessage->isScheduled() )
            			cancelEvent( mPollTriggerMessage );
                    scheduleAt( simTime() + mPollInterval, mPollTriggerMessage );
        		}
        		ClusterMemberAdded( m->getNodeId() );
        	}

        	// Now send the reply
        	SendUnificationResponse( m->getNodeId(), role );

        	break;

        case CLUSTER_UNIFY_RESPONSE_MESSAGE:

        	// We got a response to our unify request!

        	if ( mProcessState != UNIFYING )
        		break;	// Unless we weren't trying to unify.
        	cancelEvent( mClusterUnifyTimeoutMessage );
        	mProcessState = CLUSTERED;

        	role = m->getProposedRole();
        	if ( role == -1 ) {
        		// We were rejected. So pass.
        		break;
        	} else if ( role == CLUSTER_HEAD ) {
        		// We've been asked to be a CH. So add this node to our list.
        		if ( mCurrentState == CLUSTER_MEMBER ) {
        			mCurrentState = CLUSTER_HEAD_MEMBER;
        			scheduleAt( simTime() + mPollInterval, mPollTriggerMessage );
        		}
        		ClusterMemberAdded( m->getNodeId() );
        	} else if ( role == CLUSTER_HEAD_MEMBER ) {
        		// We've been told to become a CHM to this node.
        		if ( mCurrentState == CLUSTER_HEAD_MEMBER )
        			break;		// We've already got a CH, so forget about the whole thing.
        		else if ( mCurrentState == CLUSTER_MEMBER )
        			ClusterStarted();
        		else if ( mCurrentState == CLUSTER_HEAD )
        			scheduleAt( simTime() + mPollTimeout, mPollTimeoutMessage );
        		mCurrentState = CLUSTER_HEAD_MEMBER;
        		mClusterHead = m->getNodeId();
        	}

        	// Unification complete!

        	break;

        case LEAVE_MESSAGE:

        	// Remove the node from our Cluster record.
        	ClusterMemberRemoved( m->getNodeId() );

        	break;

        case DATA:
            sendUp( decapsMsg( m ) );
            deleteMsg = false;
            break;

        default:
            coreEV << " received packet from " << m->getSrcAddr() << " of unknown type: " << m->getKind() << std::endl;
            break;

    };

    if ( deleteMsg )
    	delete msg;

}


/** @brief Handle self messages */
void RmacNetworkLayer::handleSelfMsg(cMessage* msg) {

	if ( msg == mFirstTimeProcess )
		Process();

    if ( msg == mInquiryTimeoutMessage || msg == mInquiryResponseTimeoutMessage || msg == mJoinTimeoutMessage || msg == mPollTimeoutMessage || msg == mClusterUnifyTimeoutMessage )
        Process( msg );

    if ( msg == mPollTriggerMessage )
    	PollClusterMembers();

    if ( msg == mClusterPresenceBeaconMessage )
    	OrderClusterPresenceBroadcast();

    if ( msg == mPollPeriodFinishedMessage ) {

    	if ( mCurrentState == UNCLUSTERED || mCurrentState == CLUSTER_MEMBER )
    		return;

    	// Polling interval finished
    	// Check for nodes that have not responded
		//std::cerr << mId << ": Poll Period finished!\n";
    	for ( NodeIdSet::iterator it = mWaitingPollAcks.begin(); it != mWaitingPollAcks.end(); it++ ) {
    		// Increment the missed ping counter.
    		mNeighbours[*it].mMissedPings++;
    		if ( mNeighbours[*it].mMissedPings >= (int)mMissedPingThreshold ) {
				// Remove from both the neighbour table and the cluster
				mNeighbours.erase(*it);
				ClusterMemberRemoved(*it);
    		}
    	}
    	mWaitingPollAcks.clear();

    	if ( mCurrentState == CLUSTER_HEAD_MEMBER || mCurrentState == CLUSTER_HEAD ) {
    		scheduleAt( simTime()+mPollInterval, mPollTriggerMessage );
    	}
    }

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
        if ( simulation.getModule( static_cast<int>(netwAddr ) ) ) {
        	macAddr = arp->getMacAddr(netwAddr);
        } else {
        	coreEV << "sendDown: Cannot find a module with the network address: " << netwAddr << std::endl;
        	return NULL;
        }
    }

    setDownControlInfo(pkt, macAddr);

    //encapsulate the application packet
    pkt->encapsulate(appPkt);
    coreEV <<" pkt encapsulated\n";
    return (NetwPkt*)pkt;

}




/**
 * @brief Process the clustering algorithm.
 * @param[in] msg Timeout message, or NULL if none occurred.
 *
 * This processes the clustering algorithm.
 *
 * @return Returns true if the function needs to be called again, e.g. if a state change occurred.
 */
bool RmacNetworkLayer::Process( cMessage *msg ) {

//    if ( msg && ( mProcessState == START || mProcessState == CLUSTERED ) )
//        return false;     // We don't have timeouts for these.

    bool returnValue = false;
    do
    {

    	if ( mCurrentState == UNCLUSTERED || mCurrentState == CLUSTER_MEMBER )
    		mClusterHierarchy.clear();

        returnValue = false;
    	switch( mProcessState ) {

			case START: // Start the clustering process!

				if ( msg )
					break;

				// Get the one-hop neighbours
				GetOneHopNeighbours();

				if ( mOneHopNeighbours.empty() ) {

					// There are no one-hop neighbours, so start the Inquiry process.
					mProcessState = INQUIRY;
					//std::cerr << mId << ": Entering INQ phase.\n";

				} else {

					// We have a set of one hop neighbours. Apply the NPA.
					NPA predicate;
					predicate.mClient = this;
					std::sort( mOneHopNeighbours.begin(), mOneHopNeighbours.end(), predicate );
					//std::cerr << mId << ": Have " << mOneHopNeighbours.size() << " 1-hop neighbours. Entering JOIN phase.\n";

					// Now go to the joining phase.
					mProcessState = JOINING;

				}

				returnValue = true;

				break;

			case INQUIRY:       // We're looking for our one-hop neighbours.

				if ( mInquiryTimeoutMessage ) // The timeout has occurred while we're waiting for an INQ response.
					coreEV << "INQ timeout!\n";
				else if ( msg )
					break;

				SendInquiry();
				scheduleAt( simTime() + mInquiryPeriod, mInquiryTimeoutMessage );

				break;

			case COLLECTING_INQUIRY:    // We're collecting INQ responses.

				if ( msg == mInquiryResponseTimeoutMessage ) { // We've reached the end of the collection period.
					//std::cerr << mId << ": INQ phase complete. Have " << mNeighbours.size() << " neighbours.\n";
					mProcessState = START;
					returnValue = true;
				}
				break;

			case JOINING:       // We're going through our sorted one-hop neighbours and joining the optimal CH

				if ( msg == mJoinTimeoutMessage ) { // Join timeout. Remove this neighbour from the list.
					//std::cerr << mId << ": JOIN timeout!\n";
					// The JOIN timeout occurred. We must have gone out of range of this neighbour.
					// We erase this from the neighbour table as well as the one-hop neighbours.
					mNeighbours.erase( mOneHopNeighbours[0] );
					mOneHopNeighbours.erase( mOneHopNeighbours.begin() );
				} else if ( msg == mJoinDenyMessage ) {
					//std::cerr << mId << ": JOIN denied!\n";
					mOneHopNeighbours.erase( mOneHopNeighbours.begin() );
				} else if ( msg ) {
					break;
				}

				// This traps a weird error that results in a zero being put in here somewhere.
				if ( mOneHopNeighbours.size() == 1 && mOneHopNeighbours[0] == 0 )
					mOneHopNeighbours.clear();

				// Check if we've exhausted all one-hop neighbours. If we have, go back to the INQUIRY phase.
				if ( mOneHopNeighbours.empty() ) {
					//std::cerr << mId << ": Entering INQ phase.\n";
					mProcessState = INQUIRY;
					returnValue = true;
					break;
				}
				//std::cerr << mId << ": Sending JOIN to " << mOneHopNeighbours[0] << std::endl;
				SendJoinRequest( mOneHopNeighbours[0] );
				scheduleAt( simTime() + mJoinTimeoutPeriod, mJoinTimeoutMessage );

				break;

			case CLUSTERED:
			case UNIFYING:

			    if ( msg == mPollTimeoutMessage ) {
			    	// We haven't heard from the CH in a while.
		    		//std::cerr << mId << ": POLL timeout, leaving " << mClusterHead << std::endl;
		    		mClusterHierarchy.clear();
		    		mNeighbours.erase(mClusterHead);
		    		mClusterHead = -1;
		    		if ( mCurrentState == CLUSTER_MEMBER ) {
				    	// If we're a CM, we go to the unclustered state.
			    		mCurrentState = UNCLUSTERED;
			    		mProcessState = START;
			    		returnValue = true;
			    		emit( mSigHeadChange, 1 );
			    	} else if ( mCurrentState == CLUSTER_HEAD_MEMBER ) {
				    	// If we're a CHM, we go to the CH state.
			    		mCurrentState = CLUSTER_HEAD;
			    		// TODO: Propagate new hierarchy changes down.
			    	}
			    } else if ( msg == mClusterUnifyTimeoutMessage ) {
					mProcessState = CLUSTERED;
					returnValue = true;
				}

				break;

		};

    	msg = NULL;
    } while ( returnValue );

    UpdateMessageString();
    return returnValue;

}




/**
 * @brief Get the one-hop neighbours.
 */
void RmacNetworkLayer::GetOneHopNeighbours() {

    mOneHopNeighbours.clear();
    if ( mNeighbours.empty() )
    	return;

    NeighbourIterator it = mNeighbours.begin();
    for ( ; it != mNeighbours.end(); it++ )
        if ( it->second.mHopCount == 1 )
            mOneHopNeighbours.push_back( it->second.mId );

}



/**
 * @brief Determines whether a nearby cluster is disjoint or connected.
 */
bool RmacNetworkLayer::EvaluateClusterPresence( RmacControlMessage *m ) {

	// First let's check if the sending node is in our cluster.
	if ( ListHasValue( mClusterMembers, m->getNodeId() ) )
		return false;	// Connected cluster

	//std::cerr << mId << ": Looking for intersection between [";
/*	NodeIdSet::iterator it = mClusterMembers.begin();
	for ( ; it != mClusterMembers.end(); it++ )
		std::cerr << *it << " ";
	std::cerr << "] and [";
	for ( NodeIdList::iterator it = m->getNeighbourIdTable().begin(); it != m->getNeighbourIdTable().end(); it++ )
		std::cerr << *it << " ";
	std::cerr << "]\n";*/

	// Check if there are any common neighbours.
	NodeIdSet::iterator it = mClusterMembers.begin();
	for ( it = mClusterMembers.begin(); it != mClusterMembers.end(); it++ )
		if( ListHasValue( m->getNeighbourIdTable(), *it ) )
			return false;

	return true;

}



/**
 * @brief Send an control message
 */
void RmacNetworkLayer::SendControlMessage( int type, int id, int role ) {

    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    if ( id == -1 )
        netwAddr = LAddress::L3BROADCAST;
    else
        netwAddr = mNeighbours[id].mNetworkAddress;

    RmacControlMessage *pkt = new RmacControlMessage( "cluster-ctrl", type );
    int s = 360;

    coreEV << "sending RMAC ";
    if ( type == INQ_MESSAGE ) {
        coreEV << "INQ";
    } else if ( type == INQ_RESPONSE_MESSAGE ) {
   		coreEV << "INQ_RESP";
    } else if ( type == JOIN_MESSAGE ) {
        coreEV << "JOIN_REQ";
    } else if ( type == JOIN_RESPONSE_MESSAGE ) {
        coreEV << "JOIN_RESP";
        // Add the list of CHs in the hierarchy to the packet.
        NodeIdList &pList = pkt->getClusterHierarchy();
        for ( NodeIdList::iterator it = mClusterHierarchy.begin(); it != mClusterHierarchy.end(); it++ ) {
        	pList.push_back(*it);
        	s += 8;
        }
        pList.push_back( mId );
    	s += 8;

    } else if ( type == JOIN_DENIAL_MESSAGE ) {
        coreEV << "JOIN_DENY";
    } else if ( type == POLL_MESSAGE || type == POLL_ACK_MESSAGE ) {

    	coreEV << "POLL" << ( type == POLL_ACK_MESSAGE ? "_ACK" : "" );

    	NeighbourEntrySet &pSet = pkt->getNeighbourTable();
    	for ( NeighbourIterator it = mNeighbours.begin(); it != mNeighbours.end(); it++ ) {

    		if ( it->second.mId == id || it->second.mProviderId == id )
				continue;
			if ( it->second.mPosition.distance( mNeighbours[id].mPosition ) > mZoneOfInterest )
				continue;
    		NeighbourEntry e;
    		e.mId = it->second.mId;
    		e.mNetworkAddress = it->second.mNetworkAddress;
    		e.mPositionX = it->second.mPosition.x;
    		e.mPositionY = it->second.mPosition.y;
    		e.mVelocityX = it->second.mVelocity.x;
    		e.mVelocityY = it->second.mVelocity.y;
            e.mConnectionCount = it->second.mConnectionCount;
            e.mHopCount = it->second.mHopCount+1;
            e.mMissedPings = it->second.mMissedPings;
            e.mTimeStamp = it->second.mTimeStamp;
            pSet.push_back(e);
            s += 328;
            if ( s + 328 > 18496 )
            	break;
    	}

    	int sizeToAdd = 8 * ( mTemporaryClusterRecord.size() + mClusterHierarchy.size() + 1 );
    	if ( ( type == POLL_MESSAGE ) && ( s + sizeToAdd < 18496 ) ) {

    		s += sizeToAdd;

    		// Add the cluster record
    		NeighbourIdSet &pIdSet = pkt->getNeighbourIdTable();
			NodeIdSet::iterator it = mTemporaryClusterRecord.begin();
			for ( ; it != mTemporaryClusterRecord.end(); it++ ) {
				pIdSet.push_back( *it );
			}

	        // Add the list of CHs in the hierarchy to the packet.
	        NodeIdList &pList = pkt->getClusterHierarchy();
	        for ( NodeIdList::iterator it = mClusterHierarchy.begin(); it != mClusterHierarchy.end(); it++ ) {
	        	pList.push_back(*it);
	        }
	        pList.push_back( mId );

    	}

    } else if ( type == SEND_CLUSTER_PRESENCE_MESSAGE || type == CLUSTER_PRESENCE_MESSAGE ) {

    	if ( type == SEND_CLUSTER_PRESENCE_MESSAGE )
            coreEV << "CMD_CLUS_PRES";
    	else
    		coreEV << "CLUS_PRES";

    	int sizeToAdd = 8 * ( mTemporaryClusterRecord.size() + mClusterHierarchy.size() + 1 );
    	if ( s + sizeToAdd < 18496 ) {

    		s += sizeToAdd;

    		// Add the cluster record
    		NeighbourIdSet &pIdSet = pkt->getNeighbourIdTable();
			NodeIdSet::iterator it = mTemporaryClusterRecord.begin();
	        for ( NodeIdSet::iterator it = mTemporaryClusterRecord.begin(); it != mTemporaryClusterRecord.end(); it++ ) {
	        	pIdSet.push_back(*it);
	        }

	        // Add the list of CHs in the hierarchy to the packet.
	        NodeIdList &pList = pkt->getClusterHierarchy();
	        for ( NodeIdList::iterator it = mClusterHierarchy.begin(); it != mClusterHierarchy.end(); it++ ) {
	        	pList.push_back(*it);
	        }
	        pList.push_back( mId );

    	} else {
    		coreEV << " CANCELLED!";
    		return;
    	}


    } else if ( type == CLUSTER_UNIFY_REQUEST_MESSAGE ) {
        coreEV << "CLUS_UNIFY_REQ";
    } else if ( type == CLUSTER_UNIFY_RESPONSE_MESSAGE ) {
    	coreEV << "CLUS_UNIFY_RESP";
        pkt->setProposedRole( role );
        s += 8;
    } else if ( type == LEAVE_MESSAGE ) {
    	coreEV << "LEAVE_CLUSTER";
    } else if ( type == DATA ) {
        coreEV << "DATA";
    }
    coreEV << " message...\n";

    pkt->setBitLength(s); // size of the control packet packet.

    if ( type == INQ_MESSAGE || type == INQ_RESPONSE_MESSAGE )
    	emit( mSigHelloOverhead, s );
    else
    	emit( mSigOverhead, s );

    // fill the cluster control fields
    pkt->setNodeId( mId );

    Coord p = mMobility->getCurrentPosition();
    pkt->setXPosition( p.x );
    pkt->setYPosition( p.y );

    p = mMobility->getCurrentSpeed();
    pkt->setXVelocity( p.x );
    pkt->setYVelocity( p.y );

    pkt->setIsClusterHead( IsClusterHead() );
    pkt->setClusterHead( mClusterHead );
    pkt->setConnectionCount( mClusterMembers.size() );

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
        if ( simulation.getModule( static_cast<int>(netwAddr ) ) ) {
        	macAddr = arp->getMacAddr(netwAddr);
        } else {
        	coreEV << "sendDown: Cannot find a module with the network address: " << netwAddr << std::endl;
        	return;
        }
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
 * @brief Send a POLL unicase to all CMs.
 */
void RmacNetworkLayer::PollClusterMembers() {
	for ( NodeIdSet::iterator cmIt = mClusterMembers.begin();  cmIt != mClusterMembers.end(); cmIt++ ) {
		SendControlMessage( POLL_MESSAGE, *cmIt );
		mWaitingPollAcks.insert( *cmIt );
	}
	scheduleAt( simTime()+mPollInterval, mPollPeriodFinishedMessage );
}



/**
 * @brief Respond to a POLL unicast.
 */
void RmacNetworkLayer::AcknowledgePoll( int id ) {
	SendControlMessage( POLL_ACK_MESSAGE, id );
}


/**
 * @brief Tell edge nodes to broadcast the cluster presence frame.
 */
void RmacNetworkLayer::OrderClusterPresenceBroadcast() {

	// CMs ARE NOT ALLOWED TO CALL THIS!
	if ( mCurrentState == UNCLUSTERED || mCurrentState == CLUSTER_MEMBER )
		throw cRuntimeError( "Non-CM node tried to order a cluster presence broadcast!" );

	// Save the cluster members.
	mTemporaryClusterRecord = mClusterMembers;
	mTemporaryClusterRecord.insert(mId);

	// First determine cluster edge nodes
	NodeIdSet::iterator it1, it2;
	std::vector<NodePair> checkedPairs;
	NodePair best(-1,-1);
	double maxDist = 0;
	Coord v = mMobility->getCurrentSpeed();
	v = v / v.length();

	if ( mClusterMembers.size() > 1 ) {

		// For each member in the cluster.
		for ( it1 = mTemporaryClusterRecord.begin(); it1 != mTemporaryClusterRecord.end(); it1++ ) {

			// Go through all the other members in the cluster.
			for ( it2 = mTemporaryClusterRecord.begin(); it2 != mTemporaryClusterRecord.end(); it2++ ) {
				if ( *it1 == *it2 || ListHasValue(checkedPairs,NodePair(*it1,*it2)) || ListHasValue(checkedPairs,NodePair(*it2,*it1)) )
					continue;
				checkedPairs.push_back(NodePair(*it1,*it2));
				Coord ab = ( mNeighbours[*it1].mPosition - mNeighbours[*it2].mPosition );
				double d = ( ab.x*v.x + ab.y*v.y );
				if ( d > maxDist ) {
					maxDist = d;
					best = checkedPairs.back();
				}
			}

		}

	} else {

		best = NodePair( mId, *mClusterMembers.begin() );

	}

	if ( best.first == -1 || best.second == -1 ) {

		//std::cerr << mId << ": Found edge node pair (" << best.first << "," << best.second << ")";
		//std::cerr << "\t Cluster(" << mId << "): ";
		//for ( it1 = mClusterMembers.begin(); it1 != mClusterMembers.end(); it1++ )
		//	std::cerr << *it1 << " ";
		//std::cerr << "\n";

	} else {

		// Now we have the two edge nodes! Order them to send the broadcasts.
		if ( best.first == mId ) {
			BroadcastClusterPresence();	// One of them is us!
			SendControlMessage( SEND_CLUSTER_PRESENCE_MESSAGE, best.second );	// send the order to the other.
		} else if ( best.second == mId ) {
			SendControlMessage( SEND_CLUSTER_PRESENCE_MESSAGE, best.first );	// send the order to the other.
			BroadcastClusterPresence();	// One of them is us!
		} else {
			SendControlMessage( SEND_CLUSTER_PRESENCE_MESSAGE, best.first );
			SendControlMessage( SEND_CLUSTER_PRESENCE_MESSAGE, best.second );
		}

	}

	scheduleAt( simTime()+mPollInterval*4, mClusterPresenceBeaconMessage );

}


/**
 * @brief Broadcast the cluster presence frame.
 */
void RmacNetworkLayer::BroadcastClusterPresence() {
	SendControlMessage( CLUSTER_PRESENCE_MESSAGE );
}



/**
 * @brief Send a cluster unification request.
 */
void RmacNetworkLayer::RequestClusterUnification( int id ) {
	SendControlMessage( CLUSTER_UNIFY_REQUEST_MESSAGE, id );
	scheduleAt( simTime()+mJoinTimeoutPeriod, mClusterUnifyTimeoutMessage );
}



/**
 * @brief Send a cluster unification response.
 */
void RmacNetworkLayer::SendUnificationResponse( int id, int role ) {
	SendControlMessage( CLUSTER_UNIFY_RESPONSE_MESSAGE, id, role );
}



/**
 * @brief Leave the cluster.
 */
void RmacNetworkLayer::LeaveCluster() {

	if ( mProcessState != CLUSTERED && mProcessState != UNIFYING && mCurrentState == CLUSTER_HEAD )
		return;

	//std::cerr << mId << ": Left Cluster(" << mClusterHead << ")\n";

	SendControlMessage( LEAVE_MESSAGE, mClusterHead );
	mClusterHead = -1;
	if ( mCurrentState == CLUSTER_HEAD_MEMBER ) {
		mCurrentState = CLUSTER_HEAD;
		mClusterHierarchy.clear();
	} else {
		if ( mPollTimeoutMessage->isScheduled() )
			cancelEvent( mPollTimeoutMessage );
		mCurrentState = UNCLUSTERED;
		mProcessState = START;
		Process();
	}

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
    mNeighbours[id].mIsClusterHead = m->getIsClusterHead();
    mNeighbours[id].mClusterHead = m->getClusterHead();
    mNeighbours[id].mConnectionCount = m->getConnectionCount();
    mNeighbours[id].mNetworkAddress = m->getSrcAddr();
    mNeighbours[id].mHopCount = 1;
    mNeighbours[id].mProviderId = id;
    mNeighbours[id].mDistanceToNode = mMobility->getCurrentPosition().distance( mNeighbours[id].mPosition );
    mNeighbours[id].mLinkExpirationTime = CalculateLinkExpirationTime( mNeighbours[id].mPosition, mNeighbours[id].mVelocity );
    mNeighbours[id].mTimeStamp = simTime();
    mNeighbours[id].mMissedPings = 0;
    mNeighbours[id].mDataOwner = this;

	NeighbourEntrySet &pSet = m->getNeighbourTable();
	if ( !pSet.empty() ) {

		NeighbourEntrySetIterator it = pSet.begin();
		for ( ; it != pSet.end(); it++ ) {

			if ( MapHasKey( mNeighbours, it->mId ) && ( mNeighbours[it->mId].mTimeStamp > it->mTimeStamp || mNeighbours[it->mId].mHopCount < it->mHopCount ) )
				continue;	// We have more recent or closer data than this.
			mNeighbours[it->mId].mId = it->mId;
			mNeighbours[it->mId].mNetworkAddress = it->mNetworkAddress;
			mNeighbours[it->mId].mPosition.x = it->mPositionX;
			mNeighbours[it->mId].mPosition.y = it->mPositionY;
			mNeighbours[it->mId].mVelocity.x = it->mVelocityX;
			mNeighbours[it->mId].mVelocity.y = it->mVelocityY;
		    mNeighbours[it->mId].mIsClusterHead = it->mIsClusterHead;
		    mNeighbours[it->mId].mClusterHead = it->mClusterHead;
			mNeighbours[it->mId].mConnectionCount = it->mConnectionCount;
			mNeighbours[it->mId].mHopCount = it->mHopCount;
			mNeighbours[it->mId].mTimeStamp = it->mTimeStamp;
			mNeighbours[it->mId].mMissedPings = it->mMissedPings;
		    mNeighbours[it->mId].mProviderId = id;
		    mNeighbours[it->mId].mDistanceToNode = mMobility->getCurrentPosition().distance( mNeighbours[it->mId].mPosition );
		    mNeighbours[it->mId].mLinkExpirationTime = CalculateLinkExpirationTime( mNeighbours[it->mId].mPosition, mNeighbours[it->mId].mVelocity );
		    mNeighbours[it->mId].mDataOwner = this;

		}

	}

}





/**
 * @brief Sorting predicate for the Node Precidence Algorithm.
 *
 * This implements the sorting mechanism for the Node Precidence Algorithm.
 */
bool RmacNetworkLayer::NPA::operator()( const int &id1, const int &id2 ) {

	const Neighbour &n = mClient->GetNeighbour( id1 );
	const Neighbour &m = mClient->GetNeighbour( id2 );

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




void RmacNetworkLayer::UpdateLevelOfMember( int id, NodeIdList& record, bool eraseThis ) {

	if ( ListHasValue( record, mId ) )
		return;	// CYCLICAL CLUSTER STRUCTURE!

	if ( !IsClusterHead() || !ListHasValue( mClusterMembers, id ) )
		return;

	// Assess changes to hierarchy and propagate them.
	RmacNetworkLayer *p;
	if ( eraseThis ) {
		mLevelLookup.erase(id);
	} else {
		p = dynamic_cast<RmacNetworkLayer*>( cSimulation::getActiveSimulation()->getModule( id ) );
		mLevelLookup[id] = p->GetCurrentLevelCount();
	}
	mMaximumLevels = 0;
	std::map<int,int>::iterator it;
	for ( AllInVector( it, mLevelLookup ) ) {
		mMaximumLevels = std::max( mMaximumLevels, it->second+1 );
	}

	mCurrentLevels = std::max( mMaximumLevels, mCurrentLevels );

	if ( mClusterHead != -1 ) {
		p = dynamic_cast<RmacNetworkLayer*>( cSimulation::getActiveSimulation()->getModule( mClusterHead ) );
		if ( p ) {
			record.push_back(mId);
			p->UpdateLevelOfMember( mId, record, false );
		}
	}
	UpdateMessageString();

}







