/*
 * ClusterAlgorithm.cc
 *
 *  Created on: Aug 20, 2013
 *      Author: craig
 */

#include <algorithm>
#include "ClusterAlgorithm.h"



ClusterAlgorithm::ClusterAlgorithm() : BaseNetwLayer() {
	// TODO Auto-generated constructor stub

}

ClusterAlgorithm::~ClusterAlgorithm() {
	// TODO Auto-generated destructor stub
}




void ClusterAlgorithm::ClusterStarted() {

	mCurrentMaximumClusterSize = mClusterMembers.size();
	mClusterStartTime = simTime();

}



void ClusterAlgorithm::ClusterMemberAdded( int id ) {

	mClusterMembers.insert( id );
	mCurrentMaximumClusterSize = std::max( (int)mCurrentMaximumClusterSize, (int)mClusterMembers.size() );

}



void ClusterAlgorithm::ClusterMemberRemoved( int id ) {

	mClusterMembers.erase(id);
	if ( mClusterMembers.size() == GetMinimumClusterSize() ) {
		mCurrentMaximumClusterSize += 1;
//		std::cerr << "Cluster died of attrition! |C|=" << mCurrentMaximumClusterSize << "\n";
		ClusterDied( CD_Attrition );
	}

}



void ClusterAlgorithm::ClusterDied( int deathType ) {

	Coord pos = mMobility->getCurrentPosition();
	emit( mSigClusterLifetime, simTime() - mClusterStartTime );
	emit( mSigClusterSize, (double)mCurrentMaximumClusterSize );
	emit( mSigClusterDeathType, (double)deathType );
	emit( mSigClusterDeathX, pos.x );
	emit( mSigClusterDeathY, pos.y );
	mCurrentMaximumClusterSize = 0;
	mClusterStartTime = 0;

}



int ClusterAlgorithm::GetMaximumClusterSize() {
	return mCurrentMaximumClusterSize;
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
	mCurrentMaximumClusterSize = 0;

	if ( state == 0 ) {

		// set up result connection
		mSigOverhead = registerSignal( "sigOverhead" );
		mSigHelloOverhead = registerSignal( "sigHelloOverhead" );
		mSigClusterLifetime = registerSignal( "sigClusterLifetime" );
		mSigClusterSize = registerSignal( "sigClusterSize" );
		mSigHeadChange = registerSignal( "sigHeadChange" );

		mSigClusterDeathType = registerSignal( "sigDeathType" );
		mSigClusterDeathX = registerSignal( "sigDeathX" );
		mSigClusterDeathY = registerSignal( "sigDeathY" );

	}

}

/** @brief Cleanup*/
void ClusterAlgorithm::finish() {
	BaseNetwLayer::finish();
}




