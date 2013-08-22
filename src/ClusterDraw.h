/*
 * ClusterDraw.h
 *
 *  Created on: Aug 13, 2013
 *      Author: craig
 */

#ifndef CLUSTERDRAW_H_
#define CLUSTERDRAW_H_

#include <map>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include "BaseMobility.h"

class ClusterDraw {

public:

    struct Colour {
    	Colour( float _r=0, float _g=0, float _b=0 ) : r(_r), g(_g), b(_b) {}
    	float r, g, b;
    };

    struct Line {
    	Coord mStart, mEnd;		/**< Start and end points. */
    	Colour mColour;			/**< Colour of the line. */
    	double mThickness;		/**< Thickness of the line. */
    };

    struct Circle {
    	Coord mOrigin;			/**< Centre of the circle. */
    	double mRadius;			/**< Radius of the circle. */
    	Colour mColour;			/**< Colour of the circle */
    	double mWeight;			/**< Thickness of the line. */
    	bool mFilled;			/**< Is the circle filled? */
    };

    typedef std::vector<Line> LineList;
    typedef std::vector<Circle> CircleList;

    /** Default constructor */
    ClusterDraw( Coord dims, Coord pgSize );

    /** Default destructor */
    virtual ~ClusterDraw();

    /** Draw a circle. */
    void drawCircle( Coord p, double r, Colour c, double weight = 1, bool filled=false );

    /** Draw a line. */
    void drawLine( Coord p1, Coord p2, Colour c, double thickness = 1 );

    /** Get the current camera height. */
    double getCameraHeight();

    /** Set the current camera height. */
    void setCameraHeight( double h );

    /** Update the view and flip the display. */
    void update( double elapsedTime );

protected:
    ALLEGRO_DISPLAY *mDisplay;			/**< Pointer to the Allegro display. */
    ALLEGRO_FONT *mTextFont;			/**< Font to use for drawing. */

    LineList mLineList;					/**< List of lines to draw. */
    CircleList mCircleList;				/**< List of circles to draw. */

    Coord mScreenDimensions;			/**< Dimensions of the display in pixels. */
    Coord mPlaygroundSize;				/**< Size of the display's playground. */
    Coord mCameraPosition;				/**< Location of the camera's centre point. */
    double mFieldOfView;				/**< Field of View */
    double mAspectRatio;				/**< Aspect ratio of display. */
    Coord mHalfViewDimensions;			/**< Coord containing half the width and height of the view. */
    double mHeight;						/**< Height of the camera above the playing field. */

    /** Print the position and height of the camera. */
    void printStatus();

};

#endif /* LSUFDATA_H_ */
