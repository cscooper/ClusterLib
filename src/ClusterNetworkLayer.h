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

#ifndef __CLUSTERLIB_CLUSTERNETWORKLAYER_H_
#define __CLUSTERLIB_CLUSTERNETWORKLAYER_H_

#include <omnetpp.h>

#include <MiXiMDefs.h>
#include <BaseNetwLayer.h>

#include <fstream>
#include <set>
#include <map>


#define BEAT_LENGTH	0.25	// measured in second.

/**
 * This module implements the clustering mechanism for
 * Modified Distributed Mobility-Aware Clustering (M-DMAC).
 * There is a pure virtual function that must be implemented by
 * inheriting cluster modules, which provides the weighting function
 * for the particular cluster algorithm being tested.
 */
class ClusterNetworkLayer : public BaseNetwLayer
{

public:

	/**
	 * @brief Messages used by the clustering mechanism.
	 */
	enum ClusterMessageKinds {
		HELLO_MESSAGE = LAST_BASE_NETW_MESSAGE_KIND+1,	/**< Periodic beacon used to update node weights. */
		CH_MESSAGE,										/**< Announcement that a node has become a Cluster Head (CH). */
		JOIN_MESSAGE,									/**< Announcement of a node's intention to join a CH. */
		DATA,											/**< A datagram. */
		LAST_CLUSTER_MESSAGE_KIND
	};


	struct Neighbour {
		double mWeight;						/**< Weight of this node. */
		Coord mPosition;					/**< Position of the Neighbour. */
		Coord mVelocity;					/**< Velocity of the Neighbour. */
		bool mIsClusterHead;				/**< Is this node a CH? */
		unsigned int mFreshness;			/**< How long this node will stay in range of this neighbour. Measured in beats. */
	};

protected:

	typedef std::set<unsigned int> NodeIdSet;
	typedef std::map<unsigned int,Neighbour> NeighbourSet;
	typedef std::map<unsigned int,Neighbour>::iterator NeighbourIterator;

	unsigned int mID;						/**< Node's unique ID. */
	double mWeight;							/**< Weight of this node. */
	BaseMobility *mMobility;				/**< Pointer to the vehicle's mobility module (to get location and velocity). */
	int mClusterHead;						/**< ID of the CH we're associated with (initialised to -1). */
	bool mIsClusterHead;					/**< Is this node a CH? */
	NodeIdSet mClusterMembers;				/**< Set of CMs associated with this node (if it is a CH) */
	NeighbourSet mNeighbours;				/**< The set of neighbours near this node. */
	simtime_t mClusterStartTime;			/**< The time at which this node last became a CH. */

	double mTransmitRangeSq;				/**< Required for the freshness calculation. Obtained from the PhyLayer module. */

    /**
     * @name Messages
     * @brief Messages this module sends itself.
     *
     * These are used for timed events.
     *
     **/
    /*@{*/

	cMessage *mFirstInitMessage;			/**< Run the cluster init function for the first time. */
	cMessage *mSendHelloMessage;			/**< Send a HELLO message. */
	cMessage *mBeatMessage;					/**< Process the neighbour table for out-of-date node entries. */

    /*@}*/


    /**
     * @name Parameters
     * @brief Configurations for the module.
     *
     * These variables are set in the simulation configuration.
     *
     **/
    /*@{*/

	unsigned int mInitialFreshness;			/**< The initial node freshness (measured in beats). */
	unsigned int mFreshnessThreshold;		/**< The minimum freshness for which a node is eligible to be a CH. */
	double mAngleThreshold;					/**< The maximum angle between the directions of this node and another node for it to be considered for CH. */
	unsigned int mHopCount;					/**< The number of hops for cluster control messages. */
	double mBeaconInterval;					/**< The interval between each HELLO message. */

    /*@}*/


    /**
     * @name Results
     * @brief Results to be recorded.
     *
     * These members record the performance of the algorithm.
     * The following performance metrics are recorded:
     * 		1. Overhead per node:
     * 		   This is the amount of traffic dedicated to cluster control,
     * 		   measured as total bytes transmitted.
     * 		   Type: Scalar
     * 		2. Overhead per node (HELLO messages only):
     * 		   This is the amount of traffic dedicated HELLO messages,
     * 		   measured as total bytes transmitted.
     * 		   Type: Scalar
     * 		3. Cluster Lifetime:
     * 		   A cluster's life begins when a node becomes a CH and obtains
     * 		   its first CM. It dies either when the CH becomes a CM of another
     * 		   cluster, or the CH loses all its members. The lifetime is the
     * 		   time, in seconds, between each of these events.
     * 		   Type: Scalar
     *		4. Cluster Size:
     *		   Larger clusters mean more stability and lighter load on RSU
     *		   Backbone.
     *		   Type: Scalar
     *		5. CH changes per node:
     *		   How often do CMs change CH?
     *		   Type: Scalar
     *
     **/
    /*@{*/

	simsignal_t mSigOverhead;			/**< Overhead per node (not including HELLO messages). */
	simsignal_t mSigHelloOverhead;		/**< Overhead per node (HELLO messages only). */
	simsignal_t mSigClusterLifetime;	/**< Lifetime of a cluster for which this node is CH. */
	simsignal_t mSigClusterSize;		/**< Size of the cluster for which this node is CH. */
	simsignal_t mSigHeadChange;			/**< Changes in CH for this node. */

	std::ofstream mClusterDeathOutput;	/**< The handle for the file where locations of cluster death will be logged. */

	/*@}*/

public:
    //Module_Class_Members(BaseNetwLayer,BaseLayer,0);
	ClusterNetworkLayer() : BaseNetwLayer() {}


    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

    /** @brief Cleanup*/
    virtual void finish();

protected:

    /** @brief Handle messages from upper layer */
    virtual void handleUpperMsg(cMessage* msg);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage* msg);

    /** @brief Handle self messages */
    virtual void handleSelfMsg(cMessage* msg);

    /** @brief decapsulate higher layer message from ClusterControlMessage */
    virtual cMessage* decapsMsg(ClusterControlMessage*);

    /** @brief Encapsulate higher layer packet into a ClusterControlMessage */
    virtual NetwPkt* encapsMsg(cPacket*);

    /** @brief Compute the CH weight for this node. */
    virtual double calculateWeight();

    /**
     * @name Cluster Methods
     * @brief Methods handling the formation and maintenance of clusters.
     *
     * These methods are implementations of the M-DMAC functions as specified
     * in the paper "Modified DMAC Clustering Algorithm for VANETs" by G. Wolny
     *
     **/
    /*@{*/

    /** @brief Initiate clustering. */
    void init();

    /** @brief Process the neighbour table in one beat. Also, update the node's weight. */
    void processBeat();

    /** @brief Select a CH from the neighbour table. */
    int chooseClusterHead();

    /** @brief Handle a link failure. Link failure is detected when a CMs freshness reaches 0. */
    void linkFailure( unsigned int );

    /** @brief Calculate the freshness of the given neighbour. */
    void calculateFreshness( unsigned int );

    /** @brief Determine whether the given node is a suitable CH. */
    bool testClusterHeadChange( unsigned int );

    /** @brief Handle a HELLO message. */
    void receiveHelloMessage( ClusterControlMessage* );

    /** @brief Handle a CH message. */
    void receiveChMessage( ClusterControlMessage* );

    /** @brief Handle a JOIN message. */
    void receiveJoinMessage( ClusterControlMessage* );

    /** @brief Update a neighbour's data. */
    void updateNeighbour( ClusterControlMessage* );

    /** @brief Sends a given cluster message. */
    void sendClusterMessage( int, int = -1, int = -1 );

    /*@}*/

};

#endif
