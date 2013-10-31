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

	UraeScenarioManager::initialize(stage);
	if ( stage == 0 ) {
		mCheckAffiliationRecord = new cMessage();
		scheduleAt( simTime()+1, mCheckAffiliationRecord );
		mSigFaultAffiliation = registerSignal( "sigFaultAffiliation" );
	}

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

	} else {

		UraeScenarioManager::handleSelfMsg( m );

	}

}


void ClusterAnalysisScenarioManager::finish() {

	if ( mCheckAffiliationRecord->isScheduled() )
		cancelEvent( mCheckAffiliationRecord );
	delete mCheckAffiliationRecord;

	UraeScenarioManager::finish();

}


