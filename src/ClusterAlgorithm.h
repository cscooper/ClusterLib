/*
 * ClusterAlgorithm.h
 *
 *  Created on: Aug 20, 2013
 *      Author: craig
 */

#ifndef CLUSTERALGORITHM_H_
#define CLUSTERALGORITHM_H_

#include <omnetpp.h>
#include <MiXiMDefs.h>
#include <BaseNetwLayer.h>
#include <BaseMobility.h>

#include <set>


class ClusterAlgorithm : public BaseNetwLayer {
public:

	/**
	 * @brief Types of cluster death.
	 */
	enum ClusterDeath {
		CD_Attrition = 0,
		CD_Cannibal,
		CD_Suicide
	};

	ClusterAlgorithm();
	virtual ~ClusterAlgorithm();

	typedef std::pair<unsigned int,unsigned int> NodePair;
	typedef std::set<unsigned int> NodeIdSet;
	typedef std::vector<unsigned int> NodeIdList;

	virtual int GetStateCount() = 0;
	virtual int GetClusterState() = 0;
	virtual bool IsClusterHead() = 0;
	virtual bool IsSubclusterHead() = 0;
	virtual bool IsHierarchical() = 0;
	virtual int GetMinimumClusterSize() = 0;

	virtual void UpdateMessageString() = 0;

	virtual void ClusterStarted();
	virtual void ClusterMemberAdded( int id );
	virtual void ClusterMemberRemoved( int id );
	virtual void ClusterDied( int deathType );

	int GetMaximumClusterSize();

	virtual int GetClusterHead();
	virtual bool NodeIsMember( unsigned int );
	virtual void GetClusterMemberList( NodeIdSet* );
	virtual BaseMobility *GetMobilityModule();

	std::string &GetMessageString() { return mMessageString; }

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

    /** @brief Cleanup*/
    virtual void finish();

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

	simsignal_t mSigClusterDeathType;	/**< Type of cluster death this node goes through. */
	simsignal_t mSigClusterDeathX;		/**< X position of cluster deaths. */
	simsignal_t mSigClusterDeathY;		/**< Y position of cluster deaths. */


	/*@}*/

protected:
    unsigned int mId;				/**< ID of the node. */
    int mClusterHead;               /**< ID of the CH we're associated with (initialised to -1). */
    NodeIdSet mClusterMembers;      /**< Set of CMs associated with this node (if it is a CH) */
    BaseMobility *mMobility;		/**< Mobility module for this node. */

    simtime_t mClusterStartTime;	/**< The time at which this node last became a CH. */
	int mCurrentMaximumClusterSize; /**< The highest number of nodes in the cluster of which we are currently head. */

    std::string mMessageString;		/**< Message to show in visualiser. */

};

#endif /* CLUSTERALGORITHM_H_ */
