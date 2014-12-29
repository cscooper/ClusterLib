//
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


// Need this for Marcum Q function
#include <boost/math/special_functions/bessel.hpp>
#include "MarcumQ.h"



// Helper function to compute factorials.
inline int factorial(int x) {
	return (x <= 1 ? 1 : x * factorial(x - 1));
}


/**
 * This computes the Marcum Q function
 */
double MarcumQ( double a, double b, int M ) {


	// Special cases.
	if ( b == 0 )
		return 1;

	if ( a == 0 ) {
		double Q=0;
		for ( int k = 0; k <= M-1; k++ )
			Q += pow(b,2*k)/(pow(2.0,k) * factorial(k));
		return Q * exp(-pow(b,2)/2);
	}

	// The basic iteration.  If a<b compute Q_M, otherwise compute 1-Q_M.
	double qSign;
	double constant;
	int k = M;
	double z = a*b;
	double t = 1, d, S = 0, x;
	if ( a < b ) {

		qSign = +1;
		constant = 0;
		x = a / b;
		d = x;
		k = 0;
		S = exp( -fabs(z) ) * boost::math::cyl_bessel_i( 0, z );

		for ( k = 1; k <= M-1; k++ ) {
			t = ( d + 1/d ) * exp( -fabs(z) ) * boost::math::cyl_bessel_i( k, z );
			S += t;
			d *= x;
		}

		k = M;

	} else {

		qSign = -1;
		constant = 1;
		x = b / a;
		d = pow( x, M );

	}

	do {
		t = d * exp( -fabs(z) ) * boost::math::cyl_bessel_i( fabs(k), z );
		S += t;
		d *= x;
		k++;
	} while ( fabs(t/S) > std::numeric_limits<double>::epsilon() );

	return constant + qSign * exp( -pow( a-b, 2 ) / 2 ) * S;

}


