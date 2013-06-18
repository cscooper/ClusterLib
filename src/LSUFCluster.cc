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

#include "BaseNetwLayer.h"

#include <cassert>

#include "NetwControlInfo.h"
#include "BaseMacLayer.h"
#include "AddressingInterface.h"
#include "SimpleAddress.h"
#include "FindModule.h"
#include "ClusterControlMessage_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"
#include "BaseMobility.h"
#include "BaseConnectionManager.h"
#include "ChannelAccess.h"

#include "LSUFCluster.h"

Define_Module(LSUFCluster);


/** @brief Compute the CH weight for this node. */
double LSUFCluster::calculateWeight() {

	/*
	 * Overview of LSUF:
	 * This is based on Utility-Function clustering.
	 * The metric is a weight computed from statistics of
	 * node placement around the node computing its own weight,
	 * as well as the lane it is in.
	 */

	return 0;

}
