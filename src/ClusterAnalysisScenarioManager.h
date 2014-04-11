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
	std::string mRunPrefix;
	double mNodeDensity;		// Node density is common to both simulation types.

	// Highway simulations.
	int mJunctionCount;
	int mLaneCount;
	int mCarSpeed;
	double mCarSpeedVariance;
	double mTurnProbability;

	// Grid simulations
	std::string mBaseMap;
	int mNumberOfCBDs;

#ifndef NDEBUG
	bool mVisualiser;
	ClusterDraw *mDrawer;
	Coord mScreenDimensions;
	double mFramePeriod;
	cMessage *mUpdateMessage;
	std::vector<ClusterDraw::Colour> mClusterStateColours;
#endif

};



class ClusterAnalysisScenarioManagerAccess
{
	public:
	static ClusterAnalysisScenarioManager* get() {
			return FindModule<ClusterAnalysisScenarioManager*>::findGlobalModule();
		};
};


inline bool fileExists(const char *filename);

#endif /* CLUSTERANALYSISSCENARIOMANAGER_H_ */
