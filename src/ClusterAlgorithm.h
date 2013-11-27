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
	ClusterAlgorithm();
	virtual ~ClusterAlgorithm();

	typedef std::set<unsigned int> NodeIdSet;

	virtual int GetStateCount() = 0;
	virtual int GetClusterState() = 0;
	virtual bool IsClusterHead() = 0;
	virtual bool IsSubclusterHead() = 0;

	virtual int GetClusterHead();
	virtual bool NodeIsMember( unsigned int );
	virtual void GetClusterMemberList( NodeIdSet* );
	virtual BaseMobility *GetMobilityModule();

	std::string &GetMessageString() { return mMessageString; }

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

    /** @brief Cleanup*/
    virtual void finish();

protected:
    int mClusterHead;               /**< ID of the CH we're associated with (initialised to -1). */
    NodeIdSet mClusterMembers;      /**< Set of CMs associated with this node (if it is a CH) */
    BaseMobility *mMobility;		/**< Mobility module for this node. */

    std::string mMessageString;		/**< Message to show in visualiser. */

};

#endif /* CLUSTERALGORITHM_H_ */
