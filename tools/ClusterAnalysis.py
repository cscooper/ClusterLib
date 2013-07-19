#!/usr/bin/python

from VectorMath import *
from optparse import OptionParser
from matplotlib import pyplot
import numpy, os
from OmnetReader import DataContainer



def ParseOptions():
	optParser = OptionParser()
	optParser.add_option("-d", "--directory", dest="directory", help="Define the results directory.")
	(options, args) = optParser.parse_args()

	if not options.directory:
		optParser.print_help()
		sys.exit()

	return options



def enumerateConfigs( directoryName ):
	configNames = [f.split("-")[0] for f in os.listdir(directoryName) if "sca" in f]
	dataContainers = {}
	for config in configNames:
		dataContainers[config] = DataContainer( config, directoryName )
	return dataContainers


def main( options ):
	dataContainers = enumerateConfigs( options.directory )
	dataContainers['MDMAC'].selectRun(0)
	runAttributes = dataContainers['MDMAC'].getRunAttributes()
	print "Config Name: " + runAttributes['configname']
	print "Run number: " + runAttributes['runnumber']
	print "Parameters:"
	print "\tAlgorithm: " + runAttributes['networkType']
	print "\tLocation: " + runAttributes['launchCfg'].lstrip('xmldoc(\\"maps').rstrip('.launchd.xml\\")').lstrip('/')
	print "\tHop Count: " + runAttributes['hops']
	print "\tBeacon Interval: " + runAttributes['beacon']
	print "\tInitial Freshness: " + runAttributes['initFreshness']
	print "\tFreshness Threshold: " + runAttributes['freshThresh']

	results = { "overhead" : 0, "helloOverhead" : 0, "clusterLifetime" : 0 , "clusterSize" : 0, "headChange" : 0 }
	meta = { "overhead" : 0, "helloOverhead" : 0, "clusterLifetime" : 0 , "clusterSize" : 0, "headChange" : 0 }

	# Get the list of scalars
	scalarList = dataContainers['MDMAC'].getScalarList()
	for scalarDef in scalarList:
		moduleName = scalarDef[0]
		scalarName = scalarDef[1]

		if "net" not in moduleName:
			continue

		resName = scalarName.split(":")[0]
		if resName not in results:
			continue

		scalar = dataContainers['MDMAC'].getScalar( moduleName, scalarName )
		if numpy.isnan( scalar.value ):
			continue

		results[resName] += scalar.value
		meta[resName] += 1 

	# Get the list of statistics
	statisticsList = dataContainers['MDMAC'].getStatisticsList()
	for statisic in statisticsList:
		moduleName = statisic[0]
		statName = statisic[1]

		if "net" not in moduleName:
			continue

		resName = statName.split(":")[0]
		if resName not in results:
			continue

		stat = dataContainers['MDMAC'].getStatistic( moduleName, statName )
		if stat.fields['count'] == 0:
			continue

		if resName == 'overhead' or resName == 'helloOverhead':
			results[resName] += stat.fields['sum']
			meta[resName] += 1
		else:
			results[resName] += stat.fields['mean']
			meta[resName] += stat.fields['count']


	results['overhead'] /= meta['overhead']
	results['helloOverhead'] /= meta['helloOverhead']
	results['clusterLifetime'] /= meta['clusterLifetime']
	results['clusterSize'] /= meta['clusterSize']
	results['headChange'] /= meta['headChange']

	print "Results: "
	print "Cluster Maintenance Overhead per node: " + str( numpy.floor( results['overhead'] / 1024 ) ) + " Kb"
	print "Mean Beaconing Overhead: " + str( numpy.floor( results['helloOverhead'] / 1024 ) ) + " Kb"
	print "Mean Cluster Lifetime: " + str( results['clusterLifetime'] )
	print "Mean Cluster Size: " + str( results['clusterSize'] )
	print "Head Changes per node: " + str( results['headChange'] )


if __name__ == "__main__":
	main( ParseOptions() )



