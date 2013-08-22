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

//
// TODO auto-generated type
//
package org.mixim.modules.mobility.traci;


simple ClusterTestScenarioManager
{
    parameters:
        @display("i=block/cogwheel");
        bool debug = default(false);  // emit debug messages?
        double connectAt @unit("s") = default(0s);  // when to connect to TraCI server (must be the initial timestep of the server)
        double firstStepAt @unit("s") = default(-1s);  // when to start synchronizing with the TraCI server (-1: immediately after connecting)
        double updateInterval @unit("s") = default(1s);  // time interval of hosts' position updates
        string moduleType = default("inet.nodes.wireless.WirelessHostSimplified");  // module type to be used in the simulation for each managed vehicle
        string moduleName = default("host");  // module name to be used in the simulation for each managed vehicle
        string moduleDisplayString = default("i=misc/node2;is=vs;r=0,,#707070,1");  // module displayString to be used in the simulation for each managed vehicle
        string host = default("localhost");  // TraCI server hostname
        int port = default(9999);  // TraCI server port
        xml launchConfig; // launch configuration to send to sumo-launchd
        int seed = default(-1); // seed value to set in launch configuration, if missing (-1: current run number)
        bool autoShutdown = default(true);  // Shutdown module as soon as no more vehicles are in the simulation
        int margin = default(25);  // margin to add to all received vehicle positions
        string roiRoads = default("");  // which roads (e.g. "hwy1 hwy2") are considered to consitute the region of interest, if not empty
        string roiRects = default("");  // which rectangles (e.g. "0,0-10,10 20,20-30,30) are considered to consitute the region of interest, if not empty
        double penetrationRate = default(1); //the probability of a vehicle being equipped with Car2X technology

		// Graphics display stuff
		double displayWidth = default(640);
		double displayHeight = default(480);
		double framesPerSecond = default(1);
		int playgroundSizeX @unit("m");
		int playgroundSizeY @unit("m");

}
