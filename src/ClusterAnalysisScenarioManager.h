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

#ifndef CLUSTERANALYSISSCENARIOMANAGER_H_
#define CLUSTERANALYSISSCENARIOMANAGER_H_

#include <UraeScenarioManager.h>

#ifndef NDEBUG
#include "ClusterDraw.h"
#endif


class ClusterAnalysisScenarioManager: public UraeScenarioManager {

public:
	ClusterAnalysisScenarioManager();
	virtual ~ClusterAnalysisScenarioManager();

	void joinMessageSent( int srcId, int destId );
	void joinMessageReceived( int srcId, int destId );

	virtual void initialize(int stage);
	virtual void handleSelfMsg( cMessage *m );
	virtual void finish();

private:
	typedef std::pair<int,int> SrcDestPair;
	typedef std::map<SrcDestPair,float> AffiliationMap;

	AffiliationMap mAffiliationRecord;
	cMessage *mCheckAffiliationRecord;
	simsignal_t mSigFaultAffiliation;

	// simulation parameters
	enum SimulationType {
		Highway = 0,
		Grid
	};

	SimulationType mType;
	int mJunctionCount;
	int mLaneCount;
	int mCarSpeed;
	double mNodeDensity;
	double mTurnProbability;

#ifndef NDEBUG
	bool mVisualiser;
	ClusterDraw *mDrawer;
	Coord mScreenDimensions;
	double mFramePeriod;
	cMessage *mUpdateMessage;
#endif

};



class ClusterAnalysisScenarioManagerAccess
{
	public:
	static ClusterAnalysisScenarioManager* get() {
			return FindModule<ClusterAnalysisScenarioManager*>::findGlobalModule();
		};
};


#endif /* CLUSTERANALYSISSCENARIOMANAGER_H_ */
