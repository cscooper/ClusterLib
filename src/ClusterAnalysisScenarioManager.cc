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

#include "ClusterAnalysisScenarioManager.h"

#ifndef NDEBUG
#include <BaseNetwLayer.h>
#include "ChannelAccess.h"
#include "ClusterAlgorithm.h"
#endif

Define_Module(ClusterAnalysisScenarioManager);


ClusterAnalysisScenarioManager::ClusterAnalysisScenarioManager() {
	// TODO Auto-generated constructor stub

}

ClusterAnalysisScenarioManager::~ClusterAnalysisScenarioManager() {
	// TODO Auto-generated destructor stub
}



void ClusterAnalysisScenarioManager::joinMessageSent( int srcId, int destId ) {

	mAffiliationRecord[ SrcDestPair(srcId,destId) ] = simTime().dbl();

}


void ClusterAnalysisScenarioManager::joinMessageReceived( int srcId, int destId ) {

	mAffiliationRecord.erase( SrcDestPair(srcId,destId) );

}


void ClusterAnalysisScenarioManager::initialize(int stage) {

	if ( stage == 0 ) {

#ifndef NDEBUG
		// setup the visualiser
		mVisualiser = par( "visualiser" ).boolValue();
		if ( mVisualiser ) {
			mScreenDimensions.x = par(  "displayWidth" ).longValue();
			mScreenDimensions.y = par( "displayHeight" ).longValue();
			mFramePeriod = 1 / par( "framesPerSecond" ).doubleValue();
			Coord areaDims;
			areaDims.x = par( "playgroundSizeX" ).longValue();
			areaDims.y = par( "playgroundSizeY" ).longValue();
			mDrawer = new ClusterDraw( mScreenDimensions, areaDims );
			if ( !mDrawer )
				opp_error( "Could not create drawer object." );

			mUpdateMessage = new cMessage;
			scheduleAt( simTime() + mFramePeriod, mUpdateMessage );
		}
#endif

		// Load the parameters of the simulation and generate the maps.
		std::string simType = par("simType").stringValue();
		if ( simType == "highway" )
			mType = Highway;
		else if ( simType == "grid" )
			opp_error( "Grid simulations aren't implemented yet!" ); // mType = Grid;
		else
			opp_error( "Unknown simulation type '%s'!", simType.c_str() );

		mJunctionCount = par("junctionCount").longValue();
		mLaneCount = par("laneCount").longValue();
		mCarSpeed = par("carSpeed").longValue();
		mNodeDensity = par("nodeDensity").doubleValue();
		mTurnProbability = par("turnProbability").doubleValue();

		// Generate the maps.
		char cmd[2000];
		sprintf( cmd, "python ./scripts/GenerateGrid.py -d $(pwd)/maps/ -j %d -J %d -L %d -l %d -a %d -A %d -v %f -y %f -b %f -V $(pwd)/%s -S %d -t %d -w %f -p $(pwd)/scripts/ -c $(pwd)/scripts/ -B simFile %s > /dev/null",
				mJunctionCount, mJunctionCount,
				mLaneCount, mLaneCount,
				mCarSpeed, mCarSpeed,
				mNodeDensity, mNodeDensity,
				mTurnProbability,
				(const char*)par("carDefFile").stringValue(),
				par("warmupTime").longValue(),
				par("simulationTime").longValue(),
				par("laneWidth").doubleValue(),
				( mType == Highway ? "-H" : "" ) );

//		std::cerr << "Executing command: " << std::endl << cmd << std::endl;
		system(cmd);

		mCheckAffiliationRecord = new cMessage();
		scheduleAt( simTime()+1, mCheckAffiliationRecord );
		mSigFaultAffiliation = registerSignal( "sigFaultAffiliation" );

	}
	UraeScenarioManager::initialize(stage);

}


void ClusterAnalysisScenarioManager::handleSelfMsg( cMessage *m ) {

	if ( m == mCheckAffiliationRecord ) {

		std::vector<SrcDestPair> pairsToErase;
		for ( AffiliationMap::iterator it = mAffiliationRecord.begin(); it != mAffiliationRecord.end(); it++ ) {
			if ( fabs( it->second - simTime().dbl() ) > 1 ) {
				emit( mSigFaultAffiliation, 1 );
				pairsToErase.push_back( it->first );
			}
		}

		for ( std::vector<SrcDestPair>::iterator it = pairsToErase.begin(); it != pairsToErase.end(); it++ )
			mAffiliationRecord.erase( *it );

		scheduleAt( simTime()+1, mCheckAffiliationRecord );

#ifndef NDEBUG
	} else if ( mVisualiser && m == mUpdateMessage ) {

		for ( std::map<std::string,cModule*>::iterator it = hosts.begin(); it != hosts.end(); it++ ) {

			cModule *clusterAlgorithm = it->second->getSubmodule("net",-1);
			ClusterAlgorithm *mod = dynamic_cast<ClusterAlgorithm*>(clusterAlgorithm);
			if ( !mod )
				continue;

			BaseMobility *mob = dynamic_cast<BaseMobility*>(it->second->getSubmodule("mobility",-1));
			if ( !mob )
				continue;

			ClusterDraw::Colour col;
			bool isHead = mod->IsClusterHead();
			int currState = mod->GetClusterState();

			if ( mClusterStateColours.empty() )
				for ( int i = 0; i < mod->GetStateCount(); i++ )
					mClusterStateColours.push_back( ClusterDraw::Colour( rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX ) );

			col = mClusterStateColours[currState];

			Coord pos = mob->getCurrentPosition();
			if ( isHead ) {

				mDrawer->drawCircle( pos, 2, col, 3 );
				if ( mod->IsSubclusterHead() ) {
	                ClusterAlgorithm *p = dynamic_cast<ClusterAlgorithm*>( cSimulation::getActiveSimulation()->getModule( mod->GetClusterHead() ) );
	                if ( p && p != mod ) {
	                    float w;
	                    col = ClusterDraw::Colour(1,0,1);
	                    if ( p->NodeIsMember( mod->getId() ) ) {
	                        col = ClusterDraw::Colour(0,0,0);
	                        w = 2.5;
	                    } else {
	                        mDrawer->drawCircle( pos, 4, col );
	                        w = 1;
	                    }
	                    mDrawer->drawLine( pos, p->GetMobilityModule()->getCurrentPosition(), col, w );
	                }
				}

				// Draw lines to nodes this node thinks are part of it's cluster.
				ClusterAlgorithm::NodeIdSet n;
				mod->GetClusterMemberList(&n);
				for ( ClusterAlgorithm::NodeIdSet::iterator it = n.begin(); it != n.end(); it++ ) {
	                ClusterAlgorithm *p = dynamic_cast<ClusterAlgorithm*>( cSimulation::getActiveSimulation()->getModule( *it ) );
	                if ( p->GetClusterHead() != mod->getId() )
	                	mDrawer->drawLine( pos, p->GetMobilityModule()->getCurrentPosition(), ClusterDraw::Colour(1,0,0), 2 );
				}

			} else {

				mDrawer->drawCircle( pos, 2, col );
                ClusterAlgorithm *p = dynamic_cast<ClusterAlgorithm*>( cSimulation::getActiveSimulation()->getModule( mod->GetClusterHead() ) );
                if ( p && p != mod ) {
                    float w;
                    col = ClusterDraw::Colour(1,0,1);
                    if ( p->NodeIsMember( mod->getId() ) ) {
                        col = ClusterDraw::Colour(0,0,0);
                        w = 2.5;
                    } else {
                        mDrawer->drawCircle( pos, 4, col );
                        w = 1;
                    }
                    mDrawer->drawLine( pos, p->GetMobilityModule()->getCurrentPosition(), col, w );
                }
				mDrawer->drawString( pos + Coord(0,6), mod->GetMessageString(), ClusterDraw::Colour(0,0,0) );

			}

// 			ChannelAccess *channelAccess = FindModule<ChannelAccess*>::findSubModule(it->second);
// 			float radius = channelAccess->getConnectionManager( channelAccess->getParentModule() )->getMaxInterferenceDistance();
// 			mDrawer->drawCircle( pos, radius, ClusterDraw::Colour(0,0,0) );

		}
		mDrawer->update( mFramePeriod );
		scheduleAt( simTime() + mFramePeriod, mUpdateMessage );

#endif

	} else {

		UraeScenarioManager::handleSelfMsg( m );

	}

}


void ClusterAnalysisScenarioManager::finish() {

	// clean up all the files, except the launchd file, which needs to be preserved
	system( "rm ./maps/simFile.net*" );
	system( "rm ./maps/simFile.rou*" );
	system( "rm ./maps/simFile.corner.*" );
	system( "rm ./maps/simFile.lsuf" );
	system( "rm ./maps/simFile.sumo.*" );

	if ( mCheckAffiliationRecord->isScheduled() )
		cancelEvent( mCheckAffiliationRecord );
	delete mCheckAffiliationRecord;

#ifndef NDEBUG
	if ( mVisualiser ) {
		if ( mUpdateMessage->isScheduled() )
			cancelEvent( mUpdateMessage );
		delete mUpdateMessage;
		if ( mDrawer )
			delete mDrawer;
	}
#endif

	UraeScenarioManager::finish();

}


