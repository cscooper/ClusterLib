#!/usr/bin/python

from VectorMath import *
from optparse import OptionParser
from matplotlib import pyplot
import numpy, os, pickle, sys, time
from OmnetReader import DataContainer


##################
# DATA COMPILING #
##################


# Parse the options passed to the data compiler.
def parseDataCompileOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-d", "--directory", dest="directory", help="Define the results directory.")
	optParser.add_option("-t", "--useTar", dest="useTar", action="store_true", help="Define whether the individual runs are compressed into tar files.")
	optParser.add_option("-o", "--outputFile", dest="outputFile", help="Specify the output file")
	(options, args) = optParser.parse_args(argv)

	if not options.directory:
		print "Please specify results directory."
		optParser.print_help()
		sys.exit()

	if not options.outputFile:
		print "Please specify output file for compiled data."
		optParser.print_help()
		sys.exit()

	return options


# Enumerate all the configurations
def enumerateConfigs( directoryName, useTar ):
	if useTar:
		configNames = list(set([f.split("-")[0] for f in os.listdir(directoryName) if "tar" in f]))
	else:
		configNames = list(set([f.split("-")[0] for f in os.listdir(directoryName) if "sca" in f]))
	dataContainers = {}
	for config in configNames:
		dataContainers[config] = DataContainer( config, directoryName, useTar )
	return dataContainers

# Collect the results from the given data container.
def collectResults( dataContainer ):
	results = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [] }

	# Get the list of scalars
	scalarList = dataContainer.getScalarList()
	for scalarDef in scalarList:
		moduleName = scalarDef[0]
		scalarName = scalarDef[1]

		if "net" not in moduleName:
			continue

		resName = scalarName.split(":")[0]
		if resName not in results:
			continue

		scalar = dataContainer.getScalar( moduleName, scalarName )
		if numpy.isnan( scalar.value ):
			continue

		results[resName].append( scalar.value )

	# Get the list of statistics
	statisticsList = dataContainer.getStatisticsList()
	for statisic in statisticsList:
		moduleName = statisic[0]
		statName = statisic[1]

		if "net" not in moduleName:
			continue

		resName = statName.split(":")[0]
		if resName not in results:
			continue

		stat = dataContainer.getStatistic( moduleName, statName )
		if 'count' not in stat.fields or 'sum' not in stat.fields or 'mean' not in stat.fields:
			continue

		if stat.fields['count'] == 0:
			continue

		if resName == 'overhead' or resName == 'helloOverhead':
			results[resName].append( stat.fields['sum'] )
		else:
			results[resName].append( stat.fields['mean'] )

	results['overhead'] = numpy.mean( results['overhead'] )
	results['helloOverhead'] = numpy.mean( results['helloOverhead'] )
	results['clusterLifetime'] = numpy.mean( results['clusterLifetime'] )
	results['clusterSize'] = numpy.mean( results['clusterSize'] )
	results['headChange'] = numpy.mean( results['headChange'] )
	return results


# Compile the data.
def dataCompile( argv ):
	options = parseDataCompileOptions(argv)
	# Enumerate all configurations.
	dataContainers = enumerateConfigs( options.directory, options.useTar )
	startTime = time.clock()
	resultSet = {}

	# Iterate through all the configurations
	for config in dataContainers:
		# Get the list of runs
		runList = sorted(dataContainers[config].getRunList())
		print "Found " + str(len(runList)) + " runs for " + config + "."

		# Iterate through the runs
		for run in runList:
			# Select the run
			print "Processing run #" + str(run)
			try:
				dataContainers[config].selectRun( run )
			except Exception as e:
				print str(e) + " Skipping."
				continue

			# Get the parameters of this run
			runAttributes = dataContainers[config].getRunAttributes()
			algorithm = runAttributes['networkType']
			location = runAttributes['launchCfg'].lstrip('xmldoc(\\"maps').rstrip('.launchd.xml\\")').lstrip('/')
			beaconInterval = runAttributes['beacon']
			initialFreshness = runAttributes['initFreshness']
			freshnessThreshold = runAttributes['freshThresh']

			# Collect the results for this run.
			results = collectResults( dataContainers[config] )

			if algorithm not in resultSet:
				resultSet[algorithm] = {}

			if location not in resultSet[algorithm]:
				resultSet[algorithm][location] = {}

			if beaconInterval not in resultSet[algorithm][location]:
				resultSet[algorithm][location][beaconInterval] = {}

			if initialFreshness not in resultSet[algorithm][location][beaconInterval]:
				resultSet[algorithm][location][beaconInterval][initialFreshness] = {}

			if freshnessThreshold not in resultSet[algorithm][location][beaconInterval][initialFreshness]:
				resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold] = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [] }

			# Add them to the set
			for metric in results:
				# If this metric has not been added to this configuration, add it.
				if metric not in resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold]:
					resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] = []
				# Append the result to the list of metrics
				resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric].append( results[metric] )

		# We've collected all the results, now we need to compute their means and stddevs
		for algorithm in resultSet:
			for location in resultSet[algorithm]:
				for beaconInterval in resultSet[algorithm][location]:
					for initialFreshness in resultSet[algorithm][location][beaconInterval]:
						for freshnessThreshold in resultSet[algorithm][location][beaconInterval][initialFreshness]:
							for metric in resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold]:
								metricMean = numpy.mean( resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] )
								metricStd  =  numpy.std( resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] )
								metricMax  =  numpy.max( resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] )
								metricMin  =  numpy.min( resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] )
								resultSet[algorithm][location][beaconInterval][initialFreshness][freshnessThreshold][metric] = (metricMean, metricStd, metricMax, metricMin)

	# We've collated all the results. Attempt to dump this to a file.
	with open(options.outputFile,"w") as f:
		pickle.dump( resultSet, f )

	# Compute the elapsed time.
	elapsedTime = time.clock() - startTime
	hours = int( elapsedTime / 3600 )
	minutes = int( ( elapsedTime - hours*3600 ) / 60 )
	seconds = elapsedTime - hours*3600 - minutes*60
	print "Analysis time: " + str( hours ) + ":" + str( minutes ) + ":" + str( seconds )

#######################
# END OF DATA COMPILE #
#######################



#################
# Data analysis #
#################


metricPresentation = { "overhead" : "Overhead", "helloOverhead" : "Hello Overhead", "clusterLifetime" : "Cluster Lifetime" , "clusterSize" : "Cluster Size", "headChange" : "Head Change" }
metricUnits = { "overhead" : "B", "helloOverhead" : "B", "clusterLifetime" : "s", "clusterSize" : None, "headChange" : None }
metricMultiplier = { "overhead" : 1, "helloOverhead" : 1, "clusterLifetime" : 1 , "clusterSize" : 1, "headChange" : 1 }

# Parse the options passed to the data analyser.
def parseDataAnalyseOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-f", "--dataFile", dest="dataFile", help="Specify the compiled data file to load.")
	(options, args) = optParser.parse_args(argv)

	if not options.dataFile:
		print "Please specify the compiled datafile to load."
		optParser.print_help()
		sys.exit()
	return options

# Get a selection
def doSelection( message, selections ):
	print message
	for i in range(0,len(selections)):
		print str(i) + ".) " + selections[i]
	print str(len(selections)) + ".) Quit"

	sels = []
	while len(sels) == 0:
		sels = raw_input( "Select (comma-separated for multiple choice): " ).split(",")

	if str(len(selections)) == sels[0]:
		return None

	retSel = []
	for s in sels:
		iSel = int(s)
		if iSel < 0 or iSel >= len(selections):
			continue
		retSel.append(selections[iSel])

	if len(retSel) == 0:
		return None

	return retSel


# Analyse the data
def dataAnalyse( argv ):
	options = parseDataAnalyseOptions( argv )

	try:
		with open( options.dataFile, "r" ) as f:
			resultData = pickle.load( f )
	except Exception as e:
		print str(e)
		sys.exit(0)

	# We have the data, now let's get to analysing it.
	while True:
		# Pick an algorithm
		algorithm = doSelection( "Algorithms:", resultData.keys() )
		if not algorithm:
			break

		# Pick a location (NOTE THIS ASSUMES THE LOCATIONS ARE THE SAME REGARDLESS OF THE ALGORITHM IN USE!)
		location = []
		while len(location) != 1:
			location = doSelection( "Locations (Choose one!):", resultData[algorithm[0]].keys() )
			if not location:
				break
		location = location[0]

		# Now we go through the parameters one by one.
		# We pick the value we want to use at each point, but at least one of the variables 
		# needs to be selected as an x-axis variable.
		xAxis = None

		# Start with beacon interval
		beaconIntervals = sorted( resultData[algorithm[0]][location].keys(), key=lambda x: float(x) )
		beaconInterval = doSelection( "Beacon Interval:", beaconIntervals + ["Use as x-axis"] )
		if not beaconInterval:
			break
		beaconInterval = beaconInterval[0]
		if "Use as x-axis" == beaconInterval:
			xAxis = "Beacon Interval"
			beaconInterval = beaconIntervals[0]

		# Now go onto Initial Freshness
		initFreshnesses = sorted( resultData[algorithm[0]][location][beaconInterval].keys(), key=lambda x: float(x) )
		initialFreshness = None
		while True:
			initialFreshness = doSelection( "Initial Freshness:", initFreshnesses + ["Use as x-axis"] )
			if not initialFreshness:
				break
			initialFreshness = initialFreshness[0]
			if "Use as x-axis" == initialFreshness:
				if not xAxis:
					xAxis = "Initial Freshness"
					initialFreshness = initFreshnesses[0]
				else:
					print xAxis + " already chosen as x-axis."
					continue
			break
		if not initialFreshness:
			break

		# Now go to Freshness Threshold
		freshnessThresholds = sorted( resultData[algorithm[0]][location][beaconInterval][initialFreshness].keys(), key=lambda x: float(x) )
		freshnessThreshold = None
		while True:
			freshnessThreshold = doSelection( "Freshness Threshold:", freshnessThresholds + ["Use as x-axis"] )
			if not freshnessThreshold:
				break
			freshnessThreshold = freshnessThreshold[0]
			if "Use as x-axis" == freshnessThreshold:
				if not xAxis:
					xAxis = "Freshness Threshold"
					freshnessThreshold = freshnessThresholds[0]
				else:
					print xAxis + " already chosen as x-axis."
					continue
			break
		if not freshnessThreshold:
			break

		if not xAxis:
			print "You didn't choose an x-axis variable! Start again!"
			continue

		while True:
			# Now pick a metric:
			metrics = None
			metrics = doSelection( "Metric:", sorted(resultData[algorithm[0]][location][beaconInterval][initialFreshness][freshnessThreshold].keys()) )
			if not metrics:
				break

			saveToFiles = len(metrics) > 1

			for metric in metrics:
				# Now collect the data!
				for al in algorithm:
					xAxisVals = []
					yAxisVals = []
					yAxisError = []
					if xAxis == "Beacon Interval":
						for v in sorted( resultData[al][location].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[al][location][v][initialFreshness][freshnessThreshold][metric][0] )
							yAxisError.append( resultData[al][location][v][initialFreshness][freshnessThreshold][metric][1] )
					elif xAxis == "Initial Freshness":
						for v in sorted( resultData[al][location][beaconInterval].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[al][location][beaconInterval][v][freshnessThreshold][metric][0] )
							yAxisError.append( resultData[al][location][beaconInterval][v][freshnessThreshold][metric][1] )
					elif xAxis == "Freshness Threshold":
						for v in sorted( resultData[al][location][beaconInterval][initialFreshness].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[al][location][beaconInterval][initialFreshness][v][metric][0] )
							yAxisError.append( resultData[al][location][beaconInterval][initialFreshness][v][metric][1] )

					pyplot.errorbar( xAxisVals, yAxisVals, yerr = yAxisError, label=al )
					pyplot.xlim( [0,max(xAxisVals)+0.1] )

				pyplot.legend(loc='best')
				pyplot.xlabel(xAxis + " (s)")
				yl = metricPresentation[metric]
				if metricUnits[metric]:
					yl += " (" + metricUnits[metric] + ")"
				pyplot.ylabel(yl)

				titleStr = metricPresentation[metric] + " vs. " + xAxis
				titleStr += "\n(" + location + "; "
				if xAxis == "Beacon Interval":
					titleStr += "Initial Freshness: " + str(initialFreshness) + "; Freshness Threshold: " + str(freshnessThreshold) + ")"
				elif xAxis == "Initial Freshness":
					titleStr += "Beacon Interval: " + str(beaconInterval) + "; Freshness Threshold: " + str(freshnessThreshold) + ")"
				elif xAxis == "Freshness Threshold":
					titleStr += "Beacon Interval: " + str(beaconInterval) + "; Threshold Initial: " + str(initialFreshness) + ")"

				pyplot.title( titleStr )

				if saveToFiles:
					titleStr = titleStr.split("\n")
					pyplot.savefig( titleStr[0] + " " + titleStr[1] + ".pdf" )
					pyplot.close()

				else:
					pyplot.show()
					if raw_input( "Do you want to try a different metric? (Y/n) " ).lower() == 'n':
						break

	print "Good bye!"

########################
# End of Data analysis #
########################




if __name__ == "__main__":
	if sys.argv[1] == "compile":
		dataCompile( sys.argv[2:] )
	elif sys.argv[1] == "analyse":
		dataAnalyse( sys.argv[2:] )
	else:
		print "Unknown command: " + sys.argv[1]



