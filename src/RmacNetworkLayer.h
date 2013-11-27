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

#include "ClusterAlgorithm.h"

/**
 * This module implements the clustering mechanism for Robust
 * Mobility-Aware Clustering (RMAC).
 */
class RmacNetworkLayer : public ClusterAlgorithm {

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
        POLL_MESSAGE,								  /**< A poll message. */
        POLL_ACK_MESSAGE,							  /**< A poll acknowledgement message. */
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
        unsigned int mId;                   /**< Identifier of the neighbour. */
        LAddress::L3Type mNetworkAddress;   /**< The IP address of this neighbour. */
        Coord mPosition;                    /**< Position of the neighbour. */
        Coord mVelocity;                    /**< Velocity of the neighbour. */
        unsigned int mConnectionCount;      /**< Number of connections this node now has. */
        unsigned int mHopCount;             /**< Number of hops this neighbour is from this node. */
        simtime_t mTimeStamp;               /**< Last time this entry was updated. */
        unsigned int mProviderId;           /**< Identifier of the node that last provided this information. */
        double mDistanceToNode;             /**< Distance from this node to this neighbour. */
        double mLinkExpirationTime;         /**< Time until this link expires due to mobility. */
        RmacNetworkLayer *mDataOwner;		/**< Owner of this data. */
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

    /*@}*/

	int GetStateCount();
	bool IsClusterHead();
	bool IsSubclusterHead();
    int GetClusterState();

    /**
     * Default constructor
     */
    RmacNetworkLayer() {  }

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
    virtual cMessage* decapsMsg(RmacControlMessage*);

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
        CLUSTERED               /**< Clustered phase. */

    };

    ProcessState mProcessState;         /**< Current state the process is in. */
    NeighbourSet mOneHopNeighbours;     /**< One-hop neighbours. */

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
     * @brief Send an control message
     */
    void SendControlMessage( int type, int id = -1 );

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
     * @brief Calculate the Link Expiration Time.
     * @param[in] pos Position of the target node.
     * @param[in] vel Velocity of the target node.
     * @return The time until the link expires.
     */

    double CalculateLinkExpirationTime( Coord pos, Coord vel );

    /**
     * Update neighbour data with the given message.
     */
    void UpdateNeighbour( RmacControlMessage *m );

    /*@}*/


    unsigned int mId;               /**< The identifier of this node. */
    ClusterState mCurrentState;     /**< This is the current state of the node. */

    NeighbourTable mNeighbours;     /**< This node's neighbour table. */

    NodeIdSet mWaitingPollAcks;		/**< IDs of nodes we're awaiting poll responses from. */

    simtime_t mClusterStartTime;    /**< Time at which this node last became a CH. */
    int mCurrentMaximumClusterSize; /**< Highest number of nodes in the cluster of which we are currently head. */

    double mTransmitRangeSq;        /**< Transmission range, squared. */
    double mZoneOfInterest;         /**< ThisCOLLECTING_INQUIRY is double the TX range. Obtained from the PhyLayer module. */

    bool mInitialised;              /**< Set to true if the init function has been called. */


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
    cMessage *mPollTriggerMessage;				/**< Message to trigger a CH to poll its members. */
    cMessage *mPollTimeoutMessage;				/**< Scheduled by CMs and CHMs waiting for poll from their CH. */
    cMessage *mPollPeriodFinishedMessage;		/**< Message to signify end of poll period. */

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

    /*@}*/

    /**
     * @brief Sorting predicate for the Node Precidence Algorithm.
     *
     * This implements the sorting mechanism for the Node Precidence Algorithm.
     */
    static bool NodePrecidenceAlgorithmPredicate( const Neighbour&, const Neighbour& );

};


#define MapHasKey(map,key) ( map.find(key) != map.end() )

#endif /* RMACNETWORKLAYER_H_ */
