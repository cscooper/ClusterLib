/*
 * LSUFData.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: craig
 */

#include <cmath>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include "ClusterDraw.h"



/** Default constructor */
ClusterDraw::ClusterDraw( Coord dims, Coord pgSize ) {

	al_init();
	al_install_keyboard();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();

	mDisplay = al_create_display( dims.x, dims.y );
	mPlaygroundSize = pgSize;

	mScreenDimensions = dims;
	mCameraPosition = pgSize / 2;
	mFieldOfView = M_PI / 4;
	mAspectRatio = dims.y / dims.x;
	mHeight = mCameraPosition.x / ( 2 * tan( mFieldOfView ) );
	mHalfViewDimensions.x = mCameraPosition.x;
	mHalfViewDimensions.y = mAspectRatio * mCameraPosition.x;

	mTextFont = al_load_font( "ARIALUNI.TTF", 16, 0 );

}


/** Default destructor */
ClusterDraw::~ClusterDraw() {

	al_destroy_display( mDisplay );

	al_shutdown_primitives_addon();
	al_uninstall_keyboard();
	al_uninstall_system();

}





/** Draw a circle. */
void ClusterDraw::drawCircle( Coord p, double r, Colour c, double weight, bool filled ) {

	Circle circle;
	circle.mOrigin = p;
	circle.mRadius = r;
	circle.mColour = c;
	circle.mWeight = weight;
	circle.mFilled = filled;
	mCircleList.push_back( circle );

}




/** Draw a line. */
void ClusterDraw::drawLine( Coord p1, Coord p2, Colour c, double thickness ) {

	Line l;
	l.mStart = p1;
	l.mEnd = p2;
	l.mColour = c;
	l.mThickness = thickness;
	mLineList.push_back( l );

}





/** Get the current camera height. */
double ClusterDraw::getCameraHeight() {

	return mHeight;

}



/** Set the current camera height. */
void ClusterDraw::setCameraHeight( double h ) {
	mHeight = h;
}



/** Update the view and flip the display. */
void ClusterDraw::update( double elapsedTime ) {

	bool heightChanged = false;
	ALLEGRO_KEYBOARD_STATE s;
	al_get_keyboard_state( &s );

	if ( al_key_down( &s, ALLEGRO_KEY_DOWN ) ) {
		mCameraPosition.y += 100 * elapsedTime;
	} else if ( al_key_down( &s, ALLEGRO_KEY_UP ) ) {
		mCameraPosition.y -= 100 * elapsedTime;
	}

	if ( al_key_down( &s, ALLEGRO_KEY_RIGHT ) ) {
		mCameraPosition.x += 100 * elapsedTime;
	} else if ( al_key_down( &s, ALLEGRO_KEY_LEFT ) ) {
		mCameraPosition.x -= 100 * elapsedTime;
	}

	if ( al_key_down( &s, ALLEGRO_KEY_S ) ) {
		mHeight += 100 * elapsedTime;
		heightChanged = true;
	} else if ( al_key_down( &s, ALLEGRO_KEY_W ) ) {
		mHeight -= 100 * elapsedTime;
		heightChanged = true;
	}

	if ( mHeight < 1 )
		mHeight = 1;

	// update the view
	if ( heightChanged ) {
		mHalfViewDimensions.x = mHeight * tan( mFieldOfView );
		mHalfViewDimensions.y = mAspectRatio * mHalfViewDimensions.x;
	}

	Coord topLeft = mCameraPosition - mHalfViewDimensions;
	Coord bottomRight = mCameraPosition + mHalfViewDimensions;

	if ( mHalfViewDimensions.x * 2 > mPlaygroundSize.x ) {
		mCameraPosition.x = mHalfViewDimensions.x;
	} else if ( bottomRight.x > mPlaygroundSize.x ) {
		mCameraPosition.x = mPlaygroundSize.x/2 - mHalfViewDimensions.x;
	} else if ( topLeft.x < 0 ) {
		mCameraPosition.x = mHalfViewDimensions.x;
	}

	if ( mHalfViewDimensions.y * 2 > mPlaygroundSize.y ) {
		mCameraPosition.y = mHalfViewDimensions.y;
	} else if ( bottomRight.y > mPlaygroundSize.y ) {
		mCameraPosition.y = mPlaygroundSize.y/2 - mHalfViewDimensions.y;
	} else if ( topLeft.y < 0 ) {
		mCameraPosition.y = mHalfViewDimensions.y;
	}

	topLeft = mCameraPosition - mHalfViewDimensions;
	Coord dims = mHalfViewDimensions * 2;
	double multiplier = mAspectRatio * mScreenDimensions.x * 0.5 / mHalfViewDimensions.y;

#define OutsideScreen(p) ( p.x < 0 || p.y < 0 || p.x > mScreenDimensions.x || p.y > mScreenDimensions.y )

	// clear the screen.
	al_clear_to_color( al_map_rgb_f(1,1,1) );

	// Now iterate through all the lines, correct their positions, and draw them.
	for ( LineList::iterator lineIt = mLineList.begin(); lineIt != mLineList.end(); lineIt++ ) {

		// Transform the coordinates to screen.
		Coord newStart, newEnd;
		newStart = ( lineIt->mStart - topLeft ) * multiplier;
		newEnd   = (   lineIt->mEnd - topLeft ) * multiplier;

		// Make sure we're on the screen
		if ( OutsideScreen( newStart ) && OutsideScreen( newEnd ) )
			continue;	// We're not on the screen, so skip drawing the line.

		al_draw_line( newStart.x, newStart.y, newEnd.x, newEnd.y, al_map_rgb_f( lineIt->mColour.r, lineIt->mColour.g, lineIt->mColour.b ), lineIt->mThickness );

	}

	// Now iterate through all the circles, correct their positions, and draw them.
	for ( CircleList::iterator circleIt = mCircleList.begin(); circleIt != mCircleList.end(); circleIt++ ) {

		// Transform the coordinates to screen.
		Coord newOrigin;
		newOrigin = ( circleIt->mOrigin - topLeft ) * multiplier;

		// Make sure we're on the screen
		if ( OutsideScreen( newOrigin ) )
			continue;	// We're not on the screen, so skip drawing the circle.

		if ( circleIt->mFilled )
			al_draw_filled_circle( newOrigin.x, newOrigin.y, circleIt->mRadius * multiplier, al_map_rgb_f( circleIt->mColour.r, circleIt->mColour.g, circleIt->mColour.b ) );
		else
			al_draw_circle( newOrigin.x, newOrigin.y, circleIt->mRadius * multiplier, al_map_rgb_f( circleIt->mColour.r, circleIt->mColour.g, circleIt->mColour.b ), circleIt->mWeight );

	}

	printStatus();

	al_flip_display();

	mLineList.clear();
	mCircleList.clear();

}




/** Print the position and height of the camera. */
void ClusterDraw::printStatus() {

	Coord topLeft = mCameraPosition - mHalfViewDimensions;
	double h = al_get_font_line_height( mTextFont )+2;
	double mul = mAspectRatio * mScreenDimensions.x * 0.5 / mHalfViewDimensions.y;
	al_draw_textf( mTextFont, al_map_rgb_f(0,0,0), 0,   0, 0, "Position: %f,%f", mCameraPosition.x, mCameraPosition.y );
	al_draw_textf( mTextFont, al_map_rgb_f(0,0,0), 0,   h, 0, "Height: %f", mHeight );
	al_draw_textf( mTextFont, al_map_rgb_f(0,0,0), 0, 2*h, 0, "Half-view: %f,%f", mHalfViewDimensions.x, mHalfViewDimensions.y );
	al_draw_textf( mTextFont, al_map_rgb_f(0,0,0), 0, 3*h, 0, "Multiplier: %f", mul );
	al_draw_textf( mTextFont, al_map_rgb_f(0,0,0), 0, 4*h, 0, "top-left: %f,%f", topLeft.x, topLeft.y );

}



