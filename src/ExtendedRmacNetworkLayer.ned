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
// Robust Mobility-Aware Clustering (RMAC). I've made some
// of my own modifications.
//
simple ExtendedRmacNetworkLayer extends BaseNetwLayer
{
   	parameters:
        @class(ExtendedRmacNetworkLayer);

        int connectionLimits;          			  // Maximum number of connections this node can have. This is basically an upper limit on the size of the cluster.
        double distanceThreshold @unit("m");      // Distance threshold.
        double timeThreshold @unit("s");          // Time threshold.
        double inquiryPeriod @unit("s");          // Period for INQ broadcasts.
        double inquiryResponsePeriod @unit("s");  // Period during which INQ responses are collated.
        double joinTimeoutPeriod @unit("s");      // Period for JOIN timeout.
        double pollInterval @unit("s");      	  // Polling interval.
        double pollTimeout @unit("s");      	  // Polling timeout.
        int missedPingThreshold;				  // Maximum number of missed pings before a CM is declared dead.
        int routeSimilarityThreshold;			  // Number of links in a route that will be compared.
        double criticalLossProbability;			  // The highest loss probability before a CM connection is considered dead.

		// signals
		@signal[sigOverhead](type="int");
		@signal[sigHelloOverhead](type="int");
		@signal[sigClusterLifetime](type="double");
		@signal[sigClusterSize](type="int");
		@signal[sigHeadChange](type="int");
		@signal[sigDeathType](type="int");
		@signal[sigDeathX](type="double");
		@signal[sigDeathY](type="double");
		@signal[sigClusterDepth](type="int");

		// statistics
		@statistic[       overhead]( source =            sigOverhead; record =  stats; title =                  "Overhead per node"; unit = "bytes" );
		@statistic[  helloOverhead]( source =       sigHelloOverhead; record =  stats; title = "Overhead per node (HELLO messages)"; unit = "bytes" );
		@statistic[clusterLifetime]( source =     sigClusterLifetime; record =  stats; title =                   "Cluster Lifetime"; unit =     "s" );
		@statistic[    clusterSize]( source =         sigClusterSize; record =  stats; title =                       "Cluster Size"; 				);
		@statistic[     headChange]( source = "count(sigHeadChange)"; record =   last; title =                         "CH changes"; 				);
		@statistic[      deathType]( source =           sigDeathType; record = vector; title =                "Cluster Death Types"; 				);
		@statistic[         deathX]( source =              sigDeathX; record = vector; title =           "Cluster Death Position X"; 				);
		@statistic[         deathY]( source =              sigDeathY; record = vector; title =           "Cluster Death Position Y"; 				);
		@statistic[   clusterDepth]( source =        sigClusterDepth; record =  stats; title =                      "Cluster Depth"; 				);


}

