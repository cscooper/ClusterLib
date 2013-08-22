/*
 * LSUFData.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: craig
 */

#include <fstream>
#include "LSUFData.h"




/** Default constructor */
LSUFData::LSUFData( const char *filename ) {

    std::ifstream ins;
    ins.open( filename );

    if ( ins.fail() )
    	throw "Could not load LSUF datafile";

    int count;
    ins >> std::dec >> count;

    for ( int i = 0; i < count; i++ ) {

        std::string laneName;
        float weight;
        unsigned char flow;

        ins >> laneName >> weight >> flow;

        mLaneWeights[laneName].mWeight = weight;
        mLaneWeights[laneName].mFlowID = flow;

    }

    ins.close();

}


/** Default destructor */
LSUFData::~LSUFData() {

    mLaneWeights.clear();

}



/** Get the number of references to this data container. */
unsigned int LSUFData::getReferenceCount() {

    return mReferenceCount;

}


/** Increment the reference counter. */
void LSUFData::addRef() {

    mReferenceCount++;

}


/** Decrement the reference counter. */
void LSUFData::release() {

    mReferenceCount--;

}


/** Get the weight and flow ID of the given lane. */
bool LSUFData::getLaneWeight( std::string strLane, float *weight, unsigned char *flow ) {

    if ( mLaneWeights.find( strLane ) == mLaneWeights.end() ) {
        *flow = 0;
	*weight = 1;
        return true;
    }

    if ( weight )
    	*weight = mLaneWeights[strLane].mWeight;

    if ( flow )
    	*flow = mLaneWeights[strLane].mFlowID;

    return true;

}




