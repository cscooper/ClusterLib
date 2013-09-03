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

package clusterlib;

import org.mixim.base.modules.BaseNetwLayer;

//
// This module implements the clustering mechanism for
// Robust Mobility-Aware Clustering (RMAC).
//
simple RmacNetworkLayer extends BaseNetwLayer
{
   	parameters:
        @class(RmacNetworkLayer);

        int connectionLimits;          			  // Maximum number of connections this node can have. This is basically an upper limit on the size of the cluster.
        double distanceThreshold @unit("m");      // Distance threshold.
        double timeThreshold @unit("s");          // Time threshold.
        double inquiryPeriod @unit("s");          // Period for INQ broadcasts.
        double inquiryResponsePeriod @unit("s");  // Period during which INQ responses are collated.
        double joinTimeoutPeriod @unit("s");      // Period for JOIN timeout.

}

