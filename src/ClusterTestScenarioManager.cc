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


#include <fstream>
#include <queue>
#include <algorithm>
#include "ClusterTestScenarioManager.h"

#include "RmacControlMessage_m.h"
#include "RmacNetworkLayer.h"

Define_Module(ClusterTestScenarioManager);

ClusterTestScenarioManager::ClusterTestScenarioManager() {
	// TODO Auto-generated constructor stub
}

ClusterTestScenarioManager::~ClusterTestScenarioManager() {
	// TODO Auto-generated destructor stub
}


void ClusterTestScenarioManager::initialize(int stage) {

	TraCIScenarioManagerLaunchd::initialize( stage );

	if( stage == 0 ) {

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

}


void ClusterTestScenarioManager::handleSelfMsg( cMessage *m ) {

	if ( m == mUpdateMessage ) {

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
			int numStates = mod->GetStateCount();
			int currState = mod->GetClusterState();

			int colourCode = currState * 300 / numStates;
			int r, g, b;
			r = colourCode % 10;
			g = ( ( colourCode - r ) % 100 ) / 10;
			b = ( colourCode - r - g ) / 100;
			col = ClusterDraw::Colour( r, g, b );

			if ( isHead ) {
				mDrawer->drawCircle( mob->getCurrentPosition(), 2, col, 3 );
			} else {
				Coord pos = mob->getCurrentPosition();
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

			}

		}
		mDrawer->update( mFramePeriod );
		scheduleAt( simTime() + mFramePeriod, mUpdateMessage );

	} else {

		TraCIScenarioManagerLaunchd::handleSelfMsg(m);

	}

}



void ClusterTestScenarioManager::finish() {

	if ( mUpdateMessage->isScheduled() )
		cancelEvent( mUpdateMessage );
	delete mUpdateMessage;

	if ( mDrawer )
		delete mDrawer;

	TraCIScenarioManagerLaunchd::finish();

}






