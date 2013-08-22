/*
 * ClusterAlgorithm.cc
 *
 *  Created on: Aug 20, 2013
 *      Author: craig
 */

#include "ClusterAlgorithm.h"



ClusterAlgorithm::ClusterAlgorithm() : BaseNetwLayer() {
	// TODO Auto-generated constructor stub

}

ClusterAlgorithm::~ClusterAlgorithm() {
	// TODO Auto-generated destructor stub
}


int ClusterAlgorithm::GetClusterHead() {
    return mClusterHead;
}


bool ClusterAlgorithm::NodeIsMember( unsigned int nodeId ) {
    return mClusterMembers.find( nodeId ) != mClusterMembers.end();
}


void ClusterAlgorithm::GetClusterMemberList( NodeIdSet *cm ) {
	NodeIdSet::iterator it = mClusterMembers.begin();
	for ( ; it != mClusterMembers.end(); it++ )
		cm->insert( *it );
}


BaseMobility *ClusterAlgorithm::GetMobilityModule() {
	return mMobility;
}



/** @brief Initialization of the module and some variables*/
void ClusterAlgorithm::initialize( int state ) {
	BaseNetwLayer::initialize( state );
}

/** @brief Cleanup*/
void ClusterAlgorithm::finish() {
	BaseNetwLayer::finish();
}




