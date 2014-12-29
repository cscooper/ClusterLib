ClusterLib
==========

Implementations of a selection of clustering algorithms for VANETs, written in C++ for OMNeT++

Supported clustering algorithms:
    - Modified Distributed Mobility-Aware Clustering (MDMAC), by Wolny et al.
      Class name: MdmacNetworkLayer
    - Adaptive Mobility-Aware Clustering Algorithm with Destination (AMACAD), by Morales et al.
      Class name: AmacadNetworkLayer
    - Robust Mobility Adaptive Clustering (RMAC), by Goonewardene et al.
      Class name: RmacNetworkLayer
    - Channel and Route Aware Clustering (CRAC), by Cooper et al.
      Class name: ExtendedRmacNetworkLayer

MDMAC also implements a number of clustering metrics:
    - Lowest ID; Class name: LowestIdCluster
    - Highest Degree; Class name: HighestDegreeCluster
    - Lane-Sense Utility Function, by Almalag et al.; Class name: LSUFCluster
    - Route Similarity, which counts the consecutive common route links between two nodes, by Cooper et al
      Class name: RouteSimilarityCluster
    - Destination Analysis, similar to AMACAD's weight; Class name: AmacadWeightCluster

To compile these you need OMNeT++ 4.2 or later (http://omnetpp.org/), VEINS 2.0 (http://veins.car2x.org/), and Urban Radio Channel (https://github.com/cscooper/URC).

Email me at andor734@gmail.com or craigsco@bigpond.net.au if you have questions.

Publications:

G. Wolny, “Modified dmac clustering algorithm for vanets,” in Systems
and Networks Communications, 2008. ICSNC ’08. 3rd International
Conference on, 2008, pp. 268–273

M. Morales, C. seon Hong, and Y. C. Bang, “An adaptable mobility-
aware clustering algorithm in vehicular networks,” in Network Opera-
tions and Management Symposium (APNOMS), 2011 13th Asia-Pacific,
Sept 2011, pp. 1–6.

M. Morales, E. J. Cho, C. seon Hong, and S. Lee, “An adaptable
mobility-aware clustering algorithm in vehicular networks,” Journal of
Computing Science and Engineering, vol. 6, pp. 227–242, Sept 2012.

R. Goonewardene, F. Ali, and E. Stipidis, “Robust mobility adaptive
clustering scheme with support for geographic routing for vehicular ad
hoc networks,” IET Intell. Transp. Syst., vol. 3, no. 2, p. 148, 2009.

C. Cooper , M. Ros, F. Safaei, D. Franklin, M. Abolhasan, "Simulation
of Contrasting Clustering Paradigms under an Experimentally-derived
Channel Model", IEEE Vehicular Technology Conference (VTC2014-Fall),
Vancouver, Canada, 2014

M. Almalag and M. Weigle, “Using traffic flow for cluster formation in
vehicular ad-hoc networks,” in Local Computer Networks (LCN), 2010
IEEE 35th Conference on, 2010, pp. 631–636.

C. Cooper and A. Mukunthan, “Urban radio channel,” 2014. [Online].
Available: https://github.com/cscooper/URC


