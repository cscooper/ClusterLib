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
// Modified Distributed Mobility-Aware Clustering (M-DMAC).
// There is a pure virtual function that must be implemented by
// inheriting cluster modules, which provides the weighting function
// for the particular cluster algorithm being tested.
//
simple ClusterNetworkLayer extends BaseNetwLayer
{
   	parameters:
        @class(ClusterNetworkLayer);

    	int initialFreshness = default(10);
    	int freshnessThreshold = default(4);
    	double angleThreshold = default(0.25);	// multiple of PI
    	int hopCount = default(1);
    	double beaconInterval @unit(s) = default(20s);

		// signals
		@signal[sigOverhead](type="int");
		@signal[sigClusterLifetime](type="double");
		@signal[sigClusterSize](type="int");
		@signal[sigHeadChange](type="int");

		// statistics
		@statistic[       overhead]( source =            sigOverhead; record =      stats; title = "Overhead per node"; unit = "bytes" );
		@statistic[clusterLifetime]( source =     sigClusterLifetime; record =      stats; title =  "Cluster Lifetime"; unit =     "s" );
		@statistic[    clusterSize]( source =         sigClusterSize; record =     vector; title =      "Cluster Size"; 			   );
		@statistic[     headChange]( source = "count(sigHeadChange)"; record = last,stats; title =        "CH changes"; 			   );

}

