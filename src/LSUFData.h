/*
 * LSUFData.h
 *
 *  Created on: Jun 18, 2013
 *      Author: craig
 */

#ifndef LSUFDATA_H_
#define LSUFDATA_H_

#include <map>

class LSUFData {

protected:

    /**
     * @brief Contains the weight of the lane.
     */
    typedef struct LaneWeight {
        float mWeight;          /**< The weight of this lane. */
        unsigned char mFlowID;  /**< The ID of the flow to which this lane belongs. */
    } LaneWeight;

    typedef std::map<std::string,LaneWeight> LaneWeightData;

    LaneWeightData mLaneWeights;    /**< The lane weights. */
    unsigned int mReferenceCount;   /**< The number of modules currently referencing this data container. */

public:
    /** Default constructor */
    LSUFData( const char *filename );

    /** Default destructor */
    virtual ~LSUFData();

    /** Get the number of references to this data container. */
    unsigned int getReferenceCount();

    /** Increment the reference counter. */
    void addRef();

    /** Decrement the reference counter. */
    void release();

    /** Get the weight and flow ID of the given lane. */
    bool getLaneWeight( std::string strLane, float *weight, unsigned char *flow );

};

#endif /* LSUFDATA_H_ */
