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

#ifndef RMACNETWORKLAYER_H_
#define RMACNETWORKLAYER_H_

#include <BaseNetwLayer.h>

#include "CarMobility.h"
#include "ClusterAlgorithm.h"

/**
 * This module implements the clustering mechanism for Robust
 * Mobility-Aware Clustering (RMAC). I've made my own extensions.
 */
class ExtendedRmacNetworkLayer : public ClusterAlgorithm, public StatusChangeListener {

public:

    /**
     * @brief Messages used by RMAC.
     */
    enum ClusterMessageKinds {
        INQ_MESSAGE = LAST_BASE_NETW_MESSAGE_KIND+1,  /**< Periodic beacon used to find one-hop neighbours. */
        INQ_RESPONSE_MESSAGE,                         /**< Inquiry Response. */
        JOIN_MESSAGE,                                 /**< Join a CH. */
        JOIN_RESPONSE_MESSAGE,                        /**< Response to permit a node to join a cluster. */
        JOIN_DENIAL_MESSAGE,                          /**< Response to deny a node to join a cluster. */
        JOIN_SUGGEST_MESSAGE,						  /**< Response to suggest an inquiring node join another CH. */
        POLL_MESSAGE,								  /**< A poll message. */
        POLL_ACK_MESSAGE,							  /**< A poll acknowledgement message. */
        SEND_CLUSTER_PRESENCE_MESSAGE,				  /**< Order edge nodes to send CLUSTER_PRESENCE_MESSAGE. */
        CLUSTER_PRESENCE_MESSAGE,					  /**< Broadcast a cluster's existence. */
        CLUSTER_UNIFY_REQUEST_MESSAGE,				  /**< Request to unify disjoint clusters. */
        CLUSTER_UNIFY_RESPONSE_MESSAGE,				  /**< Response to unification request. */
        LEAVE_MESSAGE,								  /**< A CM will send its CH this message if it wishes to leave the cluster. */
        DATA,                                         /**< A datagram. */
        LAST_CLUSTER_MESSAGE_KIND
    };

    /**
     * @brief Contains the possible states of this node.
     */
    enum ClusterState {
        UNCLUSTERED = 0,    	/**< Initial state of all nodes. */
        CLUSTER_MEMBER,         /**< This is a member of a cluster. */
        CLUSTER_HEAD,           /**< This is the head of a cluster. */
        CLUSTER_HEAD_MEMBER     /**< This is the head of a cluster, and a member of another. */
    };

    /**
     * @brief Contains an entry in the neighbour table.
     */
    struct Neighbour {
        unsigned int mId;                   	/**< Identifier of the neighbour. */
        LAddress::L3Type mNetworkAddress;   	/**< The IP address of this neighbour. */
        Coord mPosition;                    	/**< Position of the neighbour. */
        Coord mVelocity;                    	/**< Velocity of the neighbour. */
        bool mIsClusterHead;					/**< Whether this node is a CH or CHM. */
        unsigned int mClusterHead;				/**< ID of the node's CH. */
        unsigned int mConnectionCount;      	/**< Number of connections this node now has. */
        unsigned int mHopCount;             	/**< Number of hops this neighbour is from this node. */
        simtime_t mTimeStamp;               	/**< Last time this entry was updated. */
        unsigned int mProviderId;           	/**< Identifier of the node that last provided this information. */
        double mDistanceToNode;             	/**< Distance from this node to this neighbour. */
        double mLinkExpirationTime;         	/**< Time until this link expires due to mobility. */
        int mMissedPings;						/**< Number of times this neighbour has missed a ping. */
        unsigned int mRouteSimilarity;			/**< How similar this node's route is to ours. */
        double mLossProbability;				/**< The probability that the last message received from this node could have been lost. */
        ExtendedRmacNetworkLayer *mDataOwner;	/**< Owner of this data. */
    };

    /**
     * @typedef NeighbourTable
     * @brief Contains a table of neighbours.
     */
    typedef std::map<unsigned int,Neighbour> NeighbourTable;

    /**
     * @typedef NeighbourSet
     * @brief Contains a set of neighbours.
     */
    typedef std::vector<Neighbour> NeighbourSet;

    /**
     * @typedef NeighbourIterator
     * @brief An iterator for the NeighbourSet.
     */
    typedef std::map<unsigned int,Neighbour>::iterator NeighbourIterator;

    /**
     * @name Getters
     * @brief Get the data members.
     *
     * These get the data members.
     */
    /*@{*/

    unsigned int GetConnectionLimits();         /**< Get maximum cluster size. */
    double GetDistanceThreshold();              /**< Get distance threshold. */
    double GetTimeThreshold();                  /**< Get time threshold. */

    /** Get a neighbour with the given id. */
    const Neighbour &GetNeighbour( const int &id );

    int GetCurrentLevelCount();

    /*@}*/


	int GetStateCount();
	bool IsClusterHead();
	bool IsSubclusterHead();
	bool IsHierarchical();
    int GetClusterState();
	void UpdateMessageString();
	int GetMinimumClusterSize();

	virtual void ClusterStarted();
	virtual void ClusterMemberAdded( int id );
	virtual void ClusterMemberRemoved( int id );
	virtual void ClusterDied( int deathType );

    /**
     * Default constructor
     */
    ExtendedRmacNetworkLayer() {  }

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

    /** @brief Cleanup*/
    virtual void finish();


protected:

    /**
     * @name Handlers
     * @brief OMNeT++ message handler.
     *
     * These methods handle messages passed to this module.
     *
     **/
    /*@{*/

    /** @brief Handle messages from upper layer */
    virtual void handleUpperMsg(cMessage* msg);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage* msg);

    /** @brief Handle self messages */
    virtual void handleSelfMsg(cMessage* msg);

    /** @brief decapsulate higher layer message from RmacControlMessage */
    virtual cMessage* decapsMsg(ExtendedRmacControlMessage*);

    /** @brief Encapsulate higher layer packet into a RmacControlMessage */
    virtual NetwPkt* encapsMsg(cPacket*);

    /*@}*/


    /**
     * @name Cluster Methods
     * @brief Methods handling the formation and maintenance of clusters.
     *
     * These methods are implementations of the RMAC functions as specified
     * in the paper "Robust mobility adaptive clustering scheme with support for
     * geographic routing for vehicular ad hoc networks" by R.T. Goonewardene et al.
     *
     **/
    /*@{*/

    /**
     * The set of states the algorithm can be in.
     */
    enum ProcessState {

        START = 0,              /**< Start of the algorithm. */
        INQUIRY,                /**< Inquiry phase. */
        COLLECTING_INQUIRY,     /**< We're collecting INQ responses. */
        JOINING,                /**< Join request phase. */
        CLUSTERED,              /**< Clustered phase. */
        UNIFYING				/**< We are unifying with a disjoint cluster. */

    };

    ProcessState mProcessState;   /**< Current state the process is in. */
    NodeIdList mOneHopNeighbours; /**< One-hop neighbours. */

    /**
     * @brief Process the clustering algorithm.
     * @param[in] msg Timeout message, or NULL if none occurred.
     *
     * This processes the clustering algorithm.
     *
     * @return Returns true if the function needs to be called again, e.g. if a state change occurred.
     */
    bool Process( cMessage *msg = NULL );

    /**
     * @brief Get the one-hop neighbours.
     */
    void GetOneHopNeighbours();

    /**
     * @brief Determines whether a nearby cluster is disjoint (true) or connected (false).
     */
    bool EvaluateClusterPresence( ExtendedRmacControlMessage *m );

    /**
     * @brief Send an control message
     */
    void SendControlMessage( int type, int id = -1, int role = -1 );

    /**
     * @brief Send an INQ broadcast.
     */
    void SendInquiry();

    /**
     * @brief Send a INQ_RESP unicast.
     * @param[in] id Identifier of the target node.
     */
    void SendInquiryResponse( int id );

    /**
     * @brief Send a JOIN request unicast.
     * @param[in] id Identifier of the target node.
     */
    void SendJoinRequest( int id );

    /**
     * @brief Send a JOIN response unicast.
     * @param[in] id Identifier of the target node.
     */
    void SendJoinResponse( int id );

    /**
     * @brief Send a JOIN denial unicast.
     * @param[in] id Identifier of the target node.
     */
    void SendJoinDenial( int id );

    /**
     * @brief Send a POLL unicast to all CMs.
     */
    void PollClusterMembers();

    /**
     * @brief Respond to a POLL unicast.
     */
    void AcknowledgePoll( int id );

    /**
     * @brief Tell edge nodes to broadcast the cluster presence frame.
     */
    void OrderClusterPresenceBroadcast();

    /**
     * @brief Broadcast the cluster presence frame.
     */
    void BroadcastClusterPresence();

    /**
     * @brief Send a cluster unification request.
     */
    void RequestClusterUnification( int id );

    /**
     * @brief Send a cluster unification response.
     */
    void SendUnificationResponse( int id, int role );

    /**
     * @brief Leave the cluster.
     */
    void LeaveCluster();


    /**
     * @brief Calculate the Link Expiration Time.
     * @param[in] pos Position of the target node.
     * @param[in] vel Velocity of the target node.
     * @return The time until the link expires.
     */

    double CalculateLinkExpirationTime( Coord pos, Coord vel );

    /**
     * @brief Calculate the loss probability of a node.
     * @param[in] a Parameter A of the channel's Rice distribution.
     * @param[in] sigma Parameter Sigma of the channel's Rice distribution.
     * @param[in] dSq Square of distance from the neighbour to this node.
     * @return The probability that the last received message could have been lost.
     */

    double CalculateLossProbability( double a, double sigma, double dSq );


    /**
     * @brief Calculate the route similarity of a node.
     * @param[in] r Route of the node.
     * @return The number of consecutive route links this node has in common with ours.
     */

    int CalculateRouteSimilarity( Route &r );



    /**
     * Update neighbour data with the given message.
     */
    void UpdateNeighbour( ExtendedRmacControlMessage *m );

    /*@}*/


    //unsigned int mId;               /**< The identifier of this node. */
    ClusterState mCurrentState;     /**< This is the current state of the node. */

    NeighbourTable mNeighbours;     /**< This node's neighbour table. */

    NodeIdSet mWaitingPollAcks;		/**< IDs of nodes we're awaiting poll responses from. */

    double mTransmitRangeSq;        /**< Transmission range, squared. */
    double mZoneOfInterest;         /**< This is double the TX range. Obtained from the PhyLayer module. */

    bool mInitialised;              /**< Set to true if the init function has been called. */

//    Route mThisRoute;				/**< The route of this node. */

    /**
     * @name Messages
     * @brief Messages this module sends itself.
     *
     * These are used for timed events.
     *
     **/
    /*@{*/

    cMessage *mFirstTimeProcess;				/**< Scheduled when the cluster algorithm starts, to signal the first calling of the Process function. */
    cMessage *mInquiryTimeoutMessage;			/**< Scheduled for INQ timeout. */
    cMessage *mInquiryResponseTimeoutMessage;	/**< Scheduled for INQ_RESP timeout. */
    cMessage *mJoinTimeoutMessage;				/**< Scheduled for JOIN timeout. */
    cMessage *mJoinDenyMessage;					/**< Message to signify a JOIN request was denied. */
    cMessage *mPollTriggerMessage;				/**< Message to trigger a CH to poll its members. */
    cMessage *mPollTimeoutMessage;				/**< Scheduled by CMs and CHMs waiting for poll from their CH. */
    cMessage *mPollPeriodFinishedMessage;		/**< Message to signify end of poll period. */
    cMessage *mClusterPresenceBeaconMessage;	/**< Message to trigger beaconing of CLUSTER_PRESENCE. */
    cMessage *mClusterUnifyTimeoutMessage;		/**< Scheduled for timeout of a unify request. */
    cMessage *mPrepareForSimulationEnd;		/**< Signal to log results before simulation ends. */

    /*@}*/


    /**
     * @name Parameters
     * @brief Module parameters.
     *
     * These are uses to configure the algorithm.
     *
     **/
    /*@{*/

    unsigned int mConnectionLimits;         /**< Maximum number of connections this node can have. This is basically an upper limit on the size of the cluster. */
    double mDistanceThreshold;              /**< Distance threshold. */
    double mTimeThreshold;                  /**< Time threshold. */
    double mInquiryPeriod;                  /**< Period for INQ broadcasts. */
    double mInquiryResponsePeriod;          /**< Period during which INQ responses are collated. */
    double mJoinTimeoutPeriod;              /**< Period for JOIN timeout. */
    double mPollInterval;					/**< Period for CHs polling  */
    double mPollTimeout;					/**< If a CM doesn't hear a POLL from the CH in this time, it departs the cluster. */
    unsigned int mMissedPingThreshold;		/**< Number of pings a node will miss before it is considered gone. */
    unsigned int mRouteSimilarityThreshold;	/**< Number of links in a route that will be compared. */
    double mCriticalLossProbability;		/**< The highest loss probability before a CM connection is considered dead. */

    /*@}*/



    /**
     * @name Results
     * @brief Results to be recorded.
     *
     * These members record the performance of the algorithm.
     * The following performance metrics are recorded:
     * 		1. Cluster Depth
     * 		   This is the maximum number of levels reached by
     * 		   a cluster while this node has served as its head.
     * 		   Only CHs record this.
     * 		   Type: Scalar
     *
     **/
    /*@{*/

	simsignal_t mSigClusterDepth;			/**< ClusterDepth */

	/*@}*/

    /**
     * This implements the sorting mechanism for the Node Precidence Algorithm.
     */
    struct ExtendedNPA {
    	bool operator()( const int&, const int& );
    	ExtendedRmacNetworkLayer *mClient;
    };

    NodeIdSet mTemporaryClusterRecord;		/**< When a node receives a SEND_CLUSTER_PRESENCE_MESSAGE, it stores the cluster member record here. */
    NodeIdList mClusterHierarchy;			/**< List of heads of clusters within the hierarchy. This is used to prevent cyclical clusters. */
    std::map<int,int> mLevelLookup;			/**< Lookup table of node IDs and the corresponding depth of their branches in the hierarchy. */
    int mMaximumLevels;						/**< Length of the longest branch at this point in the hierarchy. */
    int mCurrentLevels;						/**< The largest length this cluster has ever reached. */

    void UpdateLevelOfMember( int id, NodeIdList& record, bool eraseThis=false );

	int mReaffiliationCount;


	/**
	 * @name StatusChangeListener
	 * @brief Implementation of the StatusChangeListener class.
	 **/
	/*@{*/

	void LaneChanged( int newLane );
	void CrossedIntersection( std::string roadId );

	/*@}*/

};


#define MapHasKey(map,key) ( map.find(key) != map.end() )
#define ListHasValue(l,v)  ( std::find(l.begin(),l.end(),v) != l.end() )

#endif /* RMACNETWORKLAYER_H_ */
