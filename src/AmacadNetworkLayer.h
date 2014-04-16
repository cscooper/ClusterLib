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

#ifndef AMACADNETWORKLAYER_H_
#define AMACADNETWORKLAYER_H_

#include <BaseNetwLayer.h>

#include "ClusterAlgorithm.h"
#include "AmacadControlMessage_m.h"

class AmacadNetworkLayer: public ClusterAlgorithm {

public:
	AmacadNetworkLayer();
	virtual ~AmacadNetworkLayer();

	virtual int GetStateCount();
	virtual int GetClusterState();
	virtual bool IsClusterHead();
	virtual bool IsSubclusterHead();
	virtual bool IsHierarchical();
	virtual int GetMinimumClusterSize();

	virtual void UpdateMessageString();

	/** @brief Initialization of the module and some variables*/
	virtual void initialize(int);

	/** @brief Cleanup*/
	virtual void finish();

protected:

	enum ClusterMessageKinds {
        AFFILIATION_MESSAGE = LAST_BASE_NETW_MESSAGE_KIND+1,  	/**< Message for unclustered nodes to find CHs. */
        AFFILIATION_ACK_MESSAGE,								/**< Affiliation ACK message. */
        HELLO_MESSAGE,											/**< Inquiry frame. */
        HELLO_ACK_MESSAGE,										/**< Inquiry response. */
        ADD_MESSAGE,											/**< Request to join a cluster. */
        MEMBER_ACK_MESSAGE,										/**< Response to allow a member to join a cluster. */
        CLUSTERHEAD_ACK_MESSAGE,								/**< Response requiring a new member to become a CH. */
        MEMBER_UPDATE_MESSAGE,									/**< Tell cluster members to change to a new CH. */
        DELETE_MESSAGE,											/**< A CM has left the cluster. */
        WARNING_MESSAGE,										/**< Message warning a CH of a significant change in speed or bandwidth. */
        RECLUSTERING_MESSAGE,									/**< Message to trigger a reclustering process. */
        RECLUSTERING_ACK_MESSAGE,								/**< Response to the reclustering message. */
        DATA													/**< Data packet. */
	};

	enum ClusterState {
		Unclustered = 0,		/**< Initial state. Send out an affiliation message. */
		//NeighbourDiscovery,		/**< We've declared ourselves a CH, and start looking for our neighbours. */
		Joining,				/**< We've found a suitable CH, and are trying to join it. */
		ClusterMember,			/**< We're a CM. */
		ClusterHead,			/**< We're a CH. */
		Reclustering			/**< We're in the middle of a reclustering process. */
	};

	struct DestinationEntry {
		double mTime;			/**< The time until which this is our destination. */
		Coord mDestination;		/**< The coordinates of this destination. */
	};

	typedef std::vector<DestinationEntry> DestinationSet;

	struct Neighbour {

		int mID;							/**< ID of the node. */
        LAddress::L3Type mNetworkAddress;   /**< The IP address of this neighbour. */
		Coord mPosition;					/**< Position of this node. */
		double mSpeed;						/**< Speed of the node. */
		Coord mDestination;					/**< Destination of the node in euclidean coordinates. */
		double mValueF;						/**< Fvz calculated for this node. */
		int mNeighbourCount;				/**< Size of this node's neighbour table. */
		int mClusterHead;					/**< ID of this node's CH. */
		bool mIsClusterHead;				/**< Is this node a CH? */
		int mClusterSize;				    /**< Size of this node's cluster, if CH. */

		simtime_t mLastHeard;				/**< Time since we last heard from this node. */

	};

	typedef std::map<unsigned int,Neighbour> NeighbourSet;
	typedef std::map<unsigned int,Neighbour>::iterator NeighbourIterator;

	bool mInitialised;

	int mID;							/**< ID of this node. */
	bool mIsClusterHead;				/**< Is this node a CH? */
	NeighbourSet mNeighbours;			/**< This node's neighbour table. */
	ClusterState mCurrentState;			/**< State of the algorithm. */
	NodeIdList mClusterHeadNeighbours;	/**< Set of neighbours near CH. */
	int mWarningMessageCount;			/**< Number of warning messages. */

	double mWeights[3];					/**< Set of weights. */
	Coord mCurrentDestination;			/**< Current destination. */
	DestinationSet mDestinationSet;		/**< Set of destinations. */

	double mLastSpeed;					/**< Last speed we calculated between our neighbours. */
	double mLastBandwidth;				/**< Last bandwidth we calculated between our neighbours. */

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
    virtual cMessage* decapsMsg(AmacadControlMessage*);

    /** @brief Encapsulate higher layer packet into a RmacControlMessage */
    virtual NetwPkt* encapsMsg(cPacket*);

    /*@}*/


	/**
	 * Process the algorithm state machine.
	 */
	void Process( cMessage *msg = NULL );



    /**
     * @brief Send a control message
     */
    void SendControlMessage( int type, int destID = -1 );



    /**
     * Update neighbour data with the given message.
     */
    void UpdateNeighbour( AmacadControlMessage *m );


    /**
     * Collect CH neighbours.
     */
    void CollectClusterHeadNeighbours();


    /**
     * Calculate the F value.
     */
    double CalculateF( int id=-1, bool combined = false );


    /**
     * Compute neighbour scores and return the ID of the best scoring one.
     */
    int ComputeNeighbourScores();


    /**
     * Change the CH.
     */
    void ChangeCH( int ch = -1 );


    /**
     * Get the destination from the destination file.
     */
    void GetDestinationList();


    /**
     * @name Messages
     * @brief Messages this module sends itself.
     *
     * These are used for timed events.
     *
     **/
    /*@{*/

    cMessage *mStartMessage;				/**< Message to start processing. */
    cMessage *mAffiliationTimeoutMessage;	/**< Timeout for affiliation acknowledgements. */
    cMessage *mJoinTimeoutMessage;			/**< Timeout for JOIN acknowledgements. */
    cMessage *mInquiryTimeoutMessage;		/**< Timeout for inquiry period. */
    cMessage *mUpdateMobilityMessage;		/**< Signal to perform the reclustering. */
    cMessage *mChangeDestinationMessage;	/**< Signal to change the destination. */

    /*@}*/


    /**
     * @name Parameters
     * @brief Module parameters.
     *
     * These are uses to configure the algorithm.
     *
     **/
    /*@{*/

	int mMinimumDensity;			/**< Minimum cluster size before reclustering. */
	int mMaximumDensity;			/**< Maximum cluster size permissible. */
	int mMaximumClusterDensity;		/**< Largest number of clusters in the neighbourhood. */
	int mMaximumWarningCount;		/**< Largest number of warning messages before changing CH. */
	double mSpeedThreshold;			/**< Speed difference threshold. */
	double mBandwidthThreshold;		/**< Bandwidth difference threshold. */
	simtime_t mTimeDifference;		/**< Time to wait between poll checking. */
	simtime_t mTimeToLive;			/**< Maximum lifetime of a neighbour table entry before it is considered dead. */
	simtime_t mTimeoutPeriod;		/**< Period of acknowledgement timeout. */

    /*@}*/

};

#endif /* AMACADNETWORKLAYER_H_ */
