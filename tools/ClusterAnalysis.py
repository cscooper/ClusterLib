#!/usr/bin/python

from VectorMath import *
from optparse import OptionParser
from matplotlib import pyplot,lines
from matplotlib.colors import ColorConverter
import numpy, os, pickle, sys, time, itertools, random
from OmnetReader import DataContainer

from matplotlib import rc
rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
## for Palatino and other serif fonts use:
#rc('font',**{'family':'serif','serif':['Palatino']})
rc('text', usetex=True)

#############
# BAR PLOTS #
#############

#
#	Draws a bar plot:
#	xAxis - horizontal axis labels.
#	yAxis - a list of arrays containg the data for N number of columns.
#	yErr  - same as above, but containing error bar info.
def DrawBarPlot( xAxis, yAxis, yErr, yLabel, xLabel, title, labelStrings ):
	ind = numpy.arange(len(xAxis))
	width = 0.35

	fig = pyplot.figure()
	ax = fig.add_subplot(111)

	rects = []
	yN = len(yAxis)
	#colconv = ColorConverter()
	#cols = [ colconv.to_rgb((random.random(),random.random(),random.random())) for i in range(0,yN) ]
	cols = ["b","g","r","c","m","y","k","w"]
	for i in numpy.arange(yN):
		rects.append( ax.bar( width*(ind*yN+i+ind), yAxis[i], width=width, color=cols[i], yerr=yErr[i], error_kw=dict(elinewidth=2,ecolor='black') ) )

	ax.set_xlim( -width, width*yN*( len(ind) + 2 ) )
	ax.set_xlabel( xLabel )
	ax.set_ylabel( yLabel )
	ax.set_title( title )
	ax.set_xticks( width * ( yN * ( ind + 0.5 ) + ind ) )
	xticknames = ax.set_xticklabels( xAxis )
	pyplot.setp( xticknames, fontsize=16 )
	ax.legend( ( r[0] for r in rects ), (l for l in labelStrings) )
	ax.grid( None, axis='y' )



metricPresentation = { "overhead" : "Overhead", "helloOverhead" : "Hello Overhead", "clusterLifetime" : "Cluster Lifetime" , "clusterSize" : "Cluster Size", "headChange" : "Reaffiliation", "faultAffiliation" : "Fault Affiliation", "clusterDepth" : "Cluster Depth" }
metricUnits = { "overhead" : "B", "helloOverhead" : "B", "clusterLifetime" : "s", "clusterSize" : None, "headChange" : None }
metricMultiplier = { "overhead" : 1, "helloOverhead" : 1, "clusterLifetime" : 1 , "clusterSize" : 1, "headChange" : 1 }

algorithmPresentation = { "HighestDegreeCluster" : "HD", "LowestIdCluster" : "LID", "LSUFCluster" : "LSUF", "RmacNetworkLayer" : "RMAC", "AmacadNetworkLayer" : "AMACAD", "ExtendedRmacNetworkLayer" : "E-RMAC" }
parameterAbbreviation = { "Node Density" : r"$\rho_N$", "Lane Count" : "$N_L$", "Junction Count" : "$N_J$", "Speed" : "$S_{max}$", "CBD Count" : "$N_{CBD}$", "Car Density" : r"$\rho_N$", "Time Threshold" : "$T_{TH}$", "Distance Threshold" : "$D_{TH}$", "Speed Threshold" : "$S_{TH}$", "Maximum Warning" : "$W_{max}$", "Destination Weight" : "$w_{D}$", "Time To Live" : "$TTL$", "Route Similarity Threshold" : "$S_{TH}$", "Critical Loss" : "$P_{crit}$" }
parameterUnits = { "Speed" : "km/h", "Speed Threshold" : "km/h", "Time Threshold" : "s", "Distance Threshold" : "m", "Time To Live" : "s" }

##################
# DATA COMPILING #
##################



# Parse the options passed to the data compiler.
def parseDataCompileOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-d",  "--directory",  dest="directory", help="Define the results directory.")
	optParser.add_option("-t",     "--useTar",     dest="useTar", help="Define whether the individual runs are compressed into tar files.", action="store_true")
	optParser.add_option("-p",    "--process",    dest="process", help="Specify whether to process 'location', 'grid', or 'highway'.", type='string', default='location' )
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
	resultSums = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }
	resultCounts = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }
	resultMeans = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }
	resultVar = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }
	resultMax = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }
	scalarResults = { "overhead" : [], "helloOverhead" : [], "clusterLifetime" : [] , "clusterSize" : [], "headChange" : [], "faultAffiliation" : [], "clusterDepth" : [] }

	# Get the list of scalars
	scalarList = dataContainer.getScalarList()
	for scalarDef in scalarList:
		moduleName = scalarDef[0]
		scalarName = scalarDef[1]

		if "net" not in moduleName and "manager" not in moduleName:
			continue

		resName = scalarName.split(":")[0]
		if resName not in scalarResults:
			continue

		scalar = dataContainer.getScalar( moduleName, scalarName )
		if numpy.isnan( scalar.value ):
			continue

		scalarResults[resName].append( scalar.value )

	# Get the means of these scalar results
	for key in scalarResults:
		if len(scalarResults[key]) == 0:
			continue
		resultSums[key].append( numpy.sum( scalarResults[key] ) )
		resultCounts[key].append( len( scalarResults[key] ) )
		resultMeans[key].append( numpy.mean( scalarResults[key] ) )
		resultVar[key].append( numpy.var( scalarResults[key] ) )
		resultMax[key].append( scalarResults[key] )

	# Get the list of statistics
	statisticsList = dataContainer.getStatisticsList()
	for statisic in statisticsList:
		moduleName = statisic[0]
		statName = statisic[1]

		if "net" not in moduleName and "manager" not in moduleName:
			continue

		resName = statName.split(":")[0]
		if resName not in resultMeans:
			continue

		stat = dataContainer.getStatistic( moduleName, statName )
		if 'mean' not in stat.fields or 'stddev' not in stat.fields or 'count' not in stat.fields or 'sum' not in stat.fields or 'max' not in stat.fields:
			continue

		if stat.fields['count'] == 0 or stat.fields['mean'] < 2:
			continue

		resultSums[resName].append( stat.fields['sum'] )
		resultCounts[resName].append( stat.fields['count'] )
		resultMeans[resName].append( stat.fields['mean'] )
		if stat.fields['count'] == 1:
			resultVar[resName].append( 0 )
		else:
			resultVar[resName].append( numpy.power( stat.fields['stddev'], 2 ) )
		resultMax[resName].append( stat.fields['max'] )

	return (resultMeans, resultVar, resultSums, resultCounts, resultMax)



def PoolMeanVar( mean, var, count ):
	if len(mean) != len(var) or len(mean) != len(count):
		print "NOT EQUAL LENGTHS!"
		sys.exit(-1)
	newMean = 0
	newVar = 0
	sumCount = numpy.sum(count)
	for i in range(0,len(mean)):
		newMean += count[i] * mean[i]
	newMean /= sumCount
	for i in range(0,len(var)):
		newVar += count[i] * ( mean[i]*mean[i] + var[i] )
	newVar /= sumCount
	newVar -= newMean*newMean
	return newMean, newVar




class MDMACColator:

	def __init__(self):
		self.precidence = ['Beacon Interval','Initial Freshness','Freshness Threshold','Hop Count']

	def GatherResults( self, unsortedResults, runAttributes, retRes ):
		beaconInterval = float(runAttributes['beacon'])
		initialFreshness = float(runAttributes['initFreshness'])
		freshnessThreshold = float(runAttributes['freshThresh'])
		if 'hops' in runAttributes:
			hopCount = int(runAttributes['hops'])
		else:
			hopCount = 1

		if beaconInterval not in retRes:
			retRes[beaconInterval] = {}

		if initialFreshness not in retRes[beaconInterval]:
			retRes[beaconInterval][initialFreshness] = {}

		if freshnessThreshold not in retRes[beaconInterval][initialFreshness]:
			retRes[beaconInterval][initialFreshness][freshnessThreshold] = {}

		if hopCount not in retRes[beaconInterval][initialFreshness][freshnessThreshold]:
			retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount] = {}

		# Add them to the set
		for metric in unsortedResults[0]:
			# If this metric has not been added to this configuration, add it.
			if metricPresentation[metric] not in retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount]:
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]] = [ unsortedResults[0][metric], unsortedResults[1][metric], unsortedResults[2][metric], unsortedResults[3][metric], unsortedResults[4][metric], 1 ]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Mean "+metricPresentation[metric]] = 0
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Total "+metricPresentation[metric]] = 0
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Maximum "+metricPresentation[metric]] = 0
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]+" Rate"] = 0
			else:
				# Append the result to the list of metrics
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][0] += unsortedResults[0][metric]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][1] += unsortedResults[1][metric]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][2] += unsortedResults[2][metric]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][3] += unsortedResults[3][metric]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][4] += unsortedResults[4][metric]
				retRes[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metricPresentation[metric]][5] += 1

	def GetStatistics( self, resultSet ):
		removeResults = []
		for beaconInterval in resultSet:
			for initialFreshness in resultSet[beaconInterval]:
				for freshnessThreshold in resultSet[beaconInterval][initialFreshness]:
					for hopCount in resultSet[beaconInterval][initialFreshness][freshnessThreshold]:
						for metric in resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount]:
							if "Mean" in metric or "Rate" in metric or "Total" in metric or "Maximum" in metric:
								continue
							removeResults.append( [beaconInterval,initialFreshness,freshnessThreshold,hopCount,metric] )
							if len( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric] ) == 0:
								continue
							try:
								metricMean, metricStd = PoolMeanVar( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][0], resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][1], resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][3] )
								metricRateMean = numpy.sum( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][2] / 1100 ) / ( 1100 * resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][5] )
								metricRateVar = numpy.sum( numpy.power( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][2], 2 ) / numpy.power(1100,2) - metricRateMean*metricRateMean ) / resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][5]
#								metricRate = numpy.sum( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][2] ) / ( 1100 * resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][5] )
								resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Mean " + metric] = (metricMean, metricStd)
								resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Total " + metric] = (numpy.sum( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][2] )/resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][5], 0)
								resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric+ " Rate"] = (metricRateMean,metricRateVar)
								if len(resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][4]) != 0:
									resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount]["Maximum "+metric] = ( numpy.max( resultSet[beaconInterval][initialFreshness][freshnessThreshold][hopCount][metric][4] ), 0 )
							except Exception as e:
								print "Problem obtaining statistics for '" + metric + "'"
								print "Beacon interval =", beaconInterval
								print "Initial freshness =", initialFreshness
								print "Freshnes threshold =", freshnessThreshold
								print "Hop count =", hopCount
								raise e
		for r in removeResults:
			del resultSet[r[0]][r[1]][r[2]][r[3]][r[4]]



class RMACColator:

	def __init__(self):
		self.precidence = ['Missed Pings', 'Distance Threshold', 'Time Threshold']

	def GatherResults( self, unsortedResults, runAttributes, retRes ):
		# Add them to the set
		for metric in unsortedResults[0]:

			missedPings = runAttributes['missedPings']
			if missedPings not in retRes:
				retRes[missedPings] = {}

			distThresh = runAttributes['distThresh']
			if distThresh not in retRes[missedPings]:
				retRes[missedPings][distThresh] = {}

			timeThresh = runAttributes['timeThresh']
			if timeThresh not in retRes[missedPings][distThresh]:
				retRes[missedPings][distThresh][timeThresh] = {}

			# If this metric has not been added to this configuration, add it.
			if metricPresentation[metric] not in retRes[missedPings][distThresh][timeThresh]:
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]] = [ unsortedResults[0][metric], unsortedResults[1][metric], unsortedResults[2][metric], unsortedResults[3][metric], unsortedResults[4][metric], 1 ]
				retRes[missedPings][distThresh][timeThresh]["Mean "+metricPresentation[metric]] = 0
				retRes[missedPings][distThresh][timeThresh]["Total "+metricPresentation[metric]] = 0
				retRes[missedPings][distThresh][timeThresh]["Maximum "+metricPresentation[metric]] = 0
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]+" Rate"] = 0
			else:
				# Append the result to the list of metrics
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][0] += unsortedResults[0][metric]
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][1] += unsortedResults[1][metric]
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][2] += unsortedResults[2][metric]
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][3] += unsortedResults[3][metric]
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][4] += unsortedResults[4][metric]
				retRes[missedPings][distThresh][timeThresh][metricPresentation[metric]][5] += 1

	def GetStatistics( self, resultSet ):
		removeResults = []
		for missedPings in resultSet:
			for distThresh in resultSet[missedPings]:
				for timeThresh in resultSet[missedPings][distThresh]:
					for metric in resultSet[missedPings][distThresh][timeThresh]:
						if "Mean" in metric or "Rate" in metric or "Total" in metric or "Maximum" in metric:
							continue
						removeResults.append( [missedPings,distThresh,timeThresh,metric] )
						if len( resultSet[missedPings][distThresh][timeThresh][metric] ) == 0:
							continue
						try:
							metricMean, metricStd = PoolMeanVar( resultSet[missedPings][distThresh][timeThresh][metric][0], resultSet[missedPings][distThresh][timeThresh][metric][1], resultSet[missedPings][distThresh][timeThresh][metric][3] )
							metricRateMean = numpy.sum( resultSet[missedPings][distThresh][timeThresh][metric][2] ) / ( 1100 * resultSet[missedPings][distThresh][timeThresh][metric][5] )
							metricRateVar = numpy.sum( numpy.power( resultSet[missedPings][distThresh][timeThresh][metric][2], 2 ) / numpy.power(1100,2) - metricRateMean*metricRateMean ) / resultSet[missedPings][distThresh][timeThresh][metric][5]
#							metricRate = numpy.sum( resultSet[missedPings][distThresh][timeThresh][metric][2] ) / ( 1100 * resultSet[missedPings][distThresh][timeThresh][metric][5] )
							resultSet[missedPings][distThresh][timeThresh]["Mean " + metric] = (metricMean, metricStd)
							resultSet[missedPings][distThresh][timeThresh]["Total " + metric] = (numpy.sum( resultSet[missedPings][distThresh][timeThresh][metric][2] )/resultSet[missedPings][distThresh][timeThresh][metric][5], 0)
							resultSet[missedPings][distThresh][timeThresh][metric+ " Rate"] = (metricRateMean,metricRateVar)
							if len(resultSet[missedPings][distThresh][timeThresh][metric][4]) != 0:
								resultSet[missedPings][distThresh][timeThresh]["Maximum "+metric] = ( numpy.max( resultSet[missedPings][distThresh][timeThresh][metric][4] ), 0 )
						except Exception as e:
							print "Problem obtaining statistics for '" + metric + "'"
							print "Missed pings =", missedPings
							raise e
		for r in removeResults:
			del resultSet[r[0]][r[1]][r[2]][r[3]]







class AMACADColator:

	def __init__(self):
		self.precidence = ['Maximum Warning','Speed Threshold','Time To Live','Destination Weight']

	def GatherResults( self, unsortedResults, runAttributes, retRes ):
		# Add them to the set
		for metric in unsortedResults[0]:

			maxWarning = runAttributes['maxWarning']
			if maxWarning not in retRes:
				retRes[maxWarning] = {}

			speedThresh = runAttributes['speedThresh']
			if speedThresh not in retRes[maxWarning]:
				retRes[maxWarning][speedThresh] = {}

			ttl = runAttributes['ttl']
			if ttl not in retRes[maxWarning][speedThresh]:
				retRes[maxWarning][speedThresh][ttl] = {}

			destWeight = runAttributes['destWeight']
			if destWeight not in retRes[maxWarning][speedThresh][ttl]:
				retRes[maxWarning][speedThresh][ttl][destWeight] = {}

			# If this metric has not been added to this configuration, add it.
			if metricPresentation[metric] not in retRes[maxWarning][speedThresh][ttl][destWeight]:
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]] = [ unsortedResults[0][metric], unsortedResults[1][metric], unsortedResults[2][metric], unsortedResults[3][metric], unsortedResults[4][metric], 1 ]
				retRes[maxWarning][speedThresh][ttl][destWeight]["Mean "+metricPresentation[metric]] = 0
				retRes[maxWarning][speedThresh][ttl][destWeight]["Total "+metricPresentation[metric]] = 0
				retRes[maxWarning][speedThresh][ttl][destWeight]["Maximum "+metricPresentation[metric]] = 0
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]+" Rate"] = 0
			else:
				# Append the result to the list of metrics
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][0] += unsortedResults[0][metric]
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][1] += unsortedResults[1][metric]
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][2] += unsortedResults[2][metric]
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][3] += unsortedResults[3][metric]
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][4] += unsortedResults[4][metric]
				retRes[maxWarning][speedThresh][ttl][destWeight][metricPresentation[metric]][5] += 1

	def GetStatistics( self, resultSet ):
		removeResults = []
		for maxWarning in resultSet:
			for speedThresh in resultSet[maxWarning]:
				for ttl in resultSet[maxWarning][speedThresh]:
					for destWeight in resultSet[maxWarning][speedThresh][ttl]:
						for metric in resultSet[maxWarning][speedThresh][ttl][destWeight]:
							if "Mean" in metric or "Rate" in metric or "Total" in metric or "Maximum" in metric:
								continue
							removeResults.append( [maxWarning,speedThresh,ttl,destWeight,metric] )
							if len( resultSet[maxWarning][speedThresh][ttl][destWeight][metric] ) == 0:
								continue
							try:
								metricMean, metricStd = PoolMeanVar( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][0], resultSet[maxWarning][speedThresh][ttl][destWeight][metric][1], resultSet[maxWarning][speedThresh][ttl][destWeight][metric][3] )
								metricRateMean = numpy.sum( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][2] ) / ( 1100 * resultSet[maxWarning][speedThresh][ttl][destWeight][metric][5] )
								metricRateVar = numpy.sum( numpy.power( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][2], 2 ) / numpy.power(1100,2) - metricRateMean*metricRateMean ) / resultSet[maxWarning][speedThresh][ttl][destWeight][metric][5]
#								metricRate = numpy.sum( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][2] ) / ( 1100 * resultSet[maxWarning][speedThresh][ttl][destWeight][metric][5] )
								resultSet[maxWarning][speedThresh][ttl][destWeight]["Mean " + metric] = (metricMean, metricStd)
								resultSet[maxWarning][speedThresh][ttl][destWeight]["Total " + metric] = (numpy.sum( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][2] )/resultSet[maxWarning][speedThresh][ttl][destWeight][metric][5], 0)
								resultSet[maxWarning][speedThresh][ttl][destWeight][metric+ " Rate"] = (metricRateMean,metricRateVar)
								if len(resultSet[maxWarning][speedThresh][ttl][destWeight][metric][4]) != 0:
									resultSet[maxWarning][speedThresh][ttl][destWeight]["Maximum "+metric] = ( numpy.max( resultSet[maxWarning][speedThresh][ttl][destWeight][metric][4] ), 0 )
							except Exception as e:
								print "Problem obtaining statistics for '" + metric + "'"
								raise e
		for r in removeResults:
			del resultSet[r[0]][r[1]][r[2]][r[3]][r[4]]






class ExtendedRMACColator:

	def __init__(self):
		self.precidence = ['Critical Loss', 'Distance Threshold', 'Time Threshold', 'Route Similarity Threshold']

	def GatherResults( self, unsortedResults, runAttributes, retRes ):
		# Add them to the set
		for metric in unsortedResults[0]:

			criticalLoss = runAttributes['criticalLoss']
			if criticalLoss not in retRes:
				retRes[criticalLoss] = {}

			distThresh = runAttributes['distThresh']
			if distThresh not in retRes[criticalLoss]:
				retRes[criticalLoss][distThresh] = {}

			timeThresh = runAttributes['timeThresh']
			if timeThresh not in retRes[criticalLoss][distThresh]:
				retRes[criticalLoss][distThresh][timeThresh] = {}

			routeSimilarity = runAttributes['routeSimilarity']
			if routeSimilarity not in retRes[criticalLoss][distThresh][timeThresh]:
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity] = {}

			# If this metric has not been added to this configuration, add it.
			if metricPresentation[metric] not in retRes[criticalLoss][distThresh][timeThresh][routeSimilarity]:
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]] = [ unsortedResults[0][metric], unsortedResults[1][metric], unsortedResults[2][metric], unsortedResults[3][metric], unsortedResults[4][metric], 1 ]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity]["Mean "+metricPresentation[metric]] = 0
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity]["Total "+metricPresentation[metric]] = 0
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity]["Maximum "+metricPresentation[metric]] = 0
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]+" Rate"] = 0
			else:
				# Append the result to the list of metrics
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][0] += unsortedResults[0][metric]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][1] += unsortedResults[1][metric]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][2] += unsortedResults[2][metric]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][3] += unsortedResults[3][metric]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][4] += unsortedResults[4][metric]
				retRes[criticalLoss][distThresh][timeThresh][routeSimilarity][metricPresentation[metric]][5] += 1

	def GetStatistics( self, resultSet ):
		removeResults = []
		for criticalLoss in resultSet:
			for distThresh in resultSet[criticalLoss]:
				for timeThresh in resultSet[criticalLoss][distThresh]:
					for routeSimilarity in resultSet[criticalLoss][distThresh][timeThresh]:
						for metric in resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity]:
							if "Mean" in metric or "Rate" in metric or "Total" in metric or "Maximum" in metric:
								continue
							removeResults.append( [criticalLoss,distThresh,timeThresh,routeSimilarity,metric] )
							if len( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric] ) == 0:
								continue
							try:
								metricMean, metricStd = PoolMeanVar( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][0], resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][1], resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][3] )
								metricRateMean = numpy.sum( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][2] ) / ( 1100 * resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][5] )
								metricRateVar = numpy.sum( numpy.power( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][2], 2 ) / numpy.power(1100,2) - metricRateMean*metricRateMean ) / resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][5]
#								metricRate = numpy.sum( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][2] ) / ( 1100 * resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][5] )
								resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity]["Mean " + metric] = (metricMean, metricStd)
								resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity]["Total " + metric] = (numpy.sum( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][2] )/resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][5], 0)
								resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric+ " Rate"] = (metricRateMean,metricRateVar)
								if len(resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][4]) != 0:
									resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity]["Maximum "+metric] = ( numpy.max( resultSet[criticalLoss][distThresh][timeThresh][routeSimilarity][metric][4] ), 0 )
							except Exception as e:
								print "Problem obtaining statistics for '" + metric + "'"
								raise e
		for r in removeResults:
			del resultSet[r[0]][r[1]][r[2]][r[3]][r[4]]





Colators = { "HighestDegreeCluster" : MDMACColator, "LowestIdCluster" : MDMACColator, "LSUFCluster" : MDMACColator, "RmacNetworkLayer" : RMACColator, "AmacadNetworkLayer" : AMACADColator, "AmacadWeightCluster" : AMACADColator, "ExtendedRmacNetworkLayer" : ExtendedRMACColator }



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

		if ( ( "grid" in config ) != ( options.process == 'grid' ) ) or ( ( "highway" in config ) != ( options.process == 'highway' ) ):
			if options.process == 'grid':
				print "We've been asked to only process grid sims, and " + config + " is not one. Skipping."
			elif options.process == 'highway':
				print "We've been asked to only process highway sims, and " + config + " is not one. Skipping."
			elif options.process == 'location':
				print "We've been asked to only process location sims, and " + config + " is not one. Skipping."
			else:
				print "Invalid process identifier '" + options.process + "'"
				sys.exit(-1)
			continue

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
			try:
				channelModel = runAttributes['channelModel']
				algorithm = runAttributes['networkType']
				#print "Process " + options.process
				if options.process == 'grid':
					carDensity = float(runAttributes['cars'])
					cbdCount = float(runAttributes['cbds'])
				elif options.process == 'highway':
					laneCount = int(runAttributes['laneCount'])
					junctionCount = int(runAttributes['junctionCount'])
					speed = float(runAttributes['speed'])
					carDensity = float(runAttributes['cars'])
				elif options.process == 'location':
					location = runAttributes['launchCfg'].lstrip('xmldoc(\\"maps').rstrip('.launchd.xml\\")').lstrip('/')
			except KeyError as e:
				print "Run #" + str(run) + " is missing run attributes. Skipping. (Error: " + str(e) + ")"
				continue

			# Collect the results for this run.
			results = collectResults( dataContainers[config] )

			# Create the colator
			colator = Colators[algorithm]()

			if options.process == 'grid':
				# The location data is specified by the lane count and the road length.

				if cbdCount not in resultSet:
					resultSet[cbdCount] = {}

				if carDensity not in resultSet[cbdCount]:
					resultSet[cbdCount][carDensity] = {}

				if channelModel not in resultSet[cbdCount][carDensity]:
					resultSet[cbdCount][carDensity][channelModel] = {}

				if algorithm not in resultSet[cbdCount][carDensity][channelModel]:
					resultSet[cbdCount][carDensity][channelModel][algorithm] = {}

				colator.GatherResults( results, runAttributes, resultSet[cbdCount][carDensity][channelModel][algorithm] )

			elif options.process == 'highway':
				# The location data is specified by the lane count, the road length, speed, and node density.
				if laneCount not in resultSet:
					resultSet[laneCount] = {}

				if junctionCount not in resultSet[laneCount]:
					resultSet[laneCount][junctionCount] = {}

				if speed not in resultSet[laneCount][junctionCount]:
					resultSet[laneCount][junctionCount][speed] = {}

				if carDensity not in resultSet[laneCount][junctionCount][speed]:
					resultSet[laneCount][junctionCount][speed][carDensity] = {}

				if channelModel not in resultSet[laneCount][junctionCount][speed][carDensity]:
					resultSet[laneCount][junctionCount][speed][carDensity][channelModel] = {}

				if algorithm not in resultSet[laneCount][junctionCount][speed][carDensity][channelModel]:
					resultSet[laneCount][junctionCount][speed][carDensity][channelModel][algorithm] = {}

				colator.GatherResults( results, runAttributes, resultSet[laneCount][junctionCount][speed][carDensity][channelModel][algorithm] )

			elif options.process == 'location':
				# The location data is specified by the name of the map
				if location not in resultSet:
					resultSet[location] = {}

				if junctionCount not in resultSet[location]:
					resultSet[location] = {}

				if channelModel not in resultSet[location]:
					resultSet[location][channelModel] = {}

				if algorithm not in resultSet[location][channelModel]:
					resultSet[location][channelModel][algorithm] = {}

				colator.GatherResults( results, runAttributes, resultSet[location][channelModel][algorithm] )

		# We've collected all the results, now we need to compute their means and stddevs
		if options.process == 'grid':
			for cbdCount in resultSet:
				for carDensity in resultSet[cbdCount]:
					for channelModel in resultSet[cbdCount][carDensity]:
						for algorithm in resultSet[cbdCount][carDensity][channelModel]:
							colator.GetStatistics( resultSet[cbdCount][carDensity][channelModel][algorithm] )

		elif options.process == 'highway':
			for laneCount in resultSet:
				for junctionCount in resultSet[laneCount]:
					for speed in resultSet[laneCount][junctionCount]:
						for carDensity in resultSet[laneCount][junctionCount][speed]:
							for channelModel in resultSet[laneCount][junctionCount][speed][carDensity]:
								for algorithm in resultSet[laneCount][junctionCount][speed][carDensity][channelModel]:
									colator.GetStatistics( resultSet[laneCount][junctionCount][speed][carDensity][channelModel][algorithm] )

		elif options.process == 'location':
			for location in resultSet:
				for channelModel in resultSet[location]:
					for algorithm in resultSet[location][channelModel]:
						colator.GetStatistics( resultSet[location][algorithm] )

	resultSet['settings'] = {}
	resultSet['settings']['process'] = options.process
	if options.process == 'grid':
		resultSet['settings']['precidence'] = ['CBD Count','Car Density','Channel Model','Algorithm']
	elif options.process == 'highway':
		resultSet['settings']['precidence'] = ['Lane Count','Junction Count','Speed','Node Density','Channel Model','Algorithm']
	elif options.process == 'location':
		resultSet['settings']['precidence'] = ['Location','Channel Model','Algorithm']

	resultSet['settings']['precidence'] += colator.precidence

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



# Parse the options passed to the data analyser.
def parseDataAnalyseOptions( argv ):
	optParser = OptionParser()
	optParser.add_option("-f", "--dataFile", dest="dataFile", help="Specify the compiled data file to load.")
	optParser.add_option("-p", "--plotType", dest="plotType", help="Specify the kind of plot to use.", default="line")
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
		print str(i) + ".) " + str( selections[i] )
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



def locationAnalyse( resultData ):

	if 'settings' in resultData.keys():
		precidence = resultData['settings']['precidence']
		resultData.pop('settings',None)

	while True:
		# Pick a location (NOTE THIS ASSUMES THE LOCATIONS ARE THE SAME REGARDLESS OF THE ALGORITHM IN USE!)
		location = []
		while len(location) != 1:
			location = doSelection( "Locations (Choose one!):", resultData.keys() )
			if not location:
				break
		location = location[0]

		# Pick an algorithm
		algorithm = doSelection( "Algorithms:", resultData[location].keys() )
		if not algorithm:
			break

		# Now we go through the parameters one by one.
		# We pick the value we want to use at each point, but at least one of the variables 
		# needs to be selected as an x-axis variable.
		xAxis = None

		# Start with beacon interval
		beaconIntervals = sorted( resultData[location][algorithm[0]].keys(), key=lambda x: float(x) )
		beaconInterval = doSelection( "Beacon Interval:", beaconIntervals + ["Use as x-axis"] )
		if not beaconInterval:
			break
		beaconInterval = beaconInterval[0]
		if "Use as x-axis" == beaconInterval:
			xAxis = "Beacon Interval"
			beaconInterval = beaconIntervals[0]

		# Now go onto Initial Freshness
		initFreshnesses = sorted( resultData[location][algorithm[0]][beaconInterval].keys(), key=lambda x: float(x) )
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
		freshnessThresholds = sorted( resultData[location][algorithm[0]][beaconInterval][initialFreshness].keys(), key=lambda x: float(x) )
		freshnessThreshold = None
		while True:
			freshnessThreshold = doSelection( "Freshness Threshold:", fresultsreshnessThresholds + ["Use as x-axis"] )
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
			metrics = doSelection( "Metric:", sorted(resultData[location][algorithm[0]][beaconInterval][initialFreshness][freshnessThreshold].keys()) )
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
						for v in sorted( resultData[location][al].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[location][al][v][initialFreshness][freshnessThreshold][metric][0] )
							yAxisError.append( resultData[location][al][v][initialFreshness][freshnessThreshold][metric][1] )
					elif xAxis == "Initial Freshness":
						for v in sorted( resultData[location][al][beaconInterval].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[location][al][beaconInterval][v][freshnessThreshold][metric][0] )
							yAxisError.append( resultData[location][al][beaconInterval][v][freshnessThreshold][metric][1] )
					elif xAxis == "Freshness Threshold":
						for v in sorted( resultData[location][al][beaconInterval][initialFreshness].keys(), key = lambda x: float(x) ):
							xAxisVals.append( float(v) )
							yAxisVals.append( resultData[location][al][beaconInterval][initialFreshness][v][metric][0] )
							yAxisError.append( resultData[location][al][beaconInterval][initialFreshness][v][metric][1] )

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



def gridAnalyse(resultData):
	if 'settings' in resultData.keys():
		precidence = resultData['settings']['precidence']
		resultData.pop('settings',None)

	def acquireData( dat, axis, precidence, **kwargs ):
		hVals = []
		vVals = []
		loc = precidence.index(axis)
		if loc == 0:
			hVals = sorted( dat.keys() )
			vVals = [ dat[val][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]][kwargs[precidence[5]]][kwargs[precidence[6]]][kwargs['metric']] for val in hVals ]
		elif loc == 1:
			hVals = sorted( dat[kwargs[precidence[0]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][val][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]][kwargs[precidence[5]]][kwargs[precidence[6]]][kwargs['metric']] for val in hVals ]
		elif loc == 2:
			hVals = sorted( dat[kwargs[precidence[0]]][kwargs[precidence[1]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][kwargs[precidence[1]]][val][kwargs[precidence[3]]][kwargs[precidence[4]]][kwargs[precidence[5]]][kwargs[precidence[6]]][kwargs['metric']] for val in hVals ]
		elif loc == 3:
			hVals = sorted( dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][val][kwargs[precidence[4]]][kwargs[precidence[5]]][kwargs[precidence[6]]][kwargs['metric']] for val in hVals ]
		elif loc == 4:
			hVals = sorted( dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][val][kwargs[precidence[5]]][kwargs[precidence[6]]][kwargs['metric']] for val in hVals ]
		elif loc == 5:
			hVals = sorted( dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]][val][kwargs['metric']][kwargs[precidence[6]]] for val in hVals ]
		elif loc == 6:
			hVals = sorted( dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]][kwargs[precidence[5]]].keys() )
			vVals = [ dat[kwargs[precidence[0]]][kwargs[precidence[1]]][kwargs[precidence[2]]][kwargs[precidence[3]]][kwargs[precidence[4]]][kwargs[precidence[5]]][val][kwargs['metric']] for val in hVals ]
		return (hVals,vVals)

	markers = list(lines.Line2D.markers.keys())[5:] 

	while True:
		# Analyse grid 
		res = {}
		setAxes = []

		endThis = False
		xAxis = None
		currVals = resultData
		for n in precidence:
			res[n] = doSelection( 'Select a ' + n + ':', sorted(currVals.keys()) + ['Use as axis'] )
			if not res[n]:
				endThis = True
				break
			nextN = res[n][0]
			if nextN == 'Use as axis':
				res[n] = sorted(currVals.keys())
			nextN = res[n][0]
			if len(res[n]) > 1:
				setAxes.append(n)
				nextN = res[n][0]
				strMsg = "Use as horizontal axis"
				if xAxis:
					strMsg += " (Current: " + xAxis + ")"
				strMsg += "?"
				if raw_input( strMsg ).lower() == "y":
					xAxis = n
			currVals = currVals[nextN]

		if endThis:
			break

		permutations = list( itertools.product(*[ res[n] for n in precidence if n is not xAxis ]) )
		kwargSet = [dict( zip( [a for a in precidence if a is not xAxis], p ) ) for p in permutations]

		while True:
			# Now pick a metric:
			metrics = None
			metrics = doSelection( "Metric:", sorted(currVals) )
			if not metrics:
				break

			markerIndex = 0
			# Get the data
			for kw in kwargSet:
				conf = kw;
				conf['metric'] = metrics[0]
				hVals,vVals = acquireData( resultData, xAxis, precidence, **conf )
				labelStr = ""
				s = False
				for k in kw:
					if k in setAxes and k is not xAxis:
						if s:
							labelStr += "; "
						labelStr += k + "=" + str(kw[k])
						s = True
				v = [val[0] for val in vVals]
				pyplot.plot( hVals, v, label=labelStr, marker=markers[markerIndex] )
				markerIndex+=1
				if markerIndex == len(markers):
					markerIndex = 0
			pyplot.legend(loc='best')
			pyplot.title( metrics[0] + " vs. " + xAxis )
			pyplot.xlabel( xAxis )
			pyplot.ylabel( metrics[0] )
			pyplot.show()


def isNumber(s):
	try:
		float(s)
		return True
	except ValueError:
		return False
	return False

def convertIfNumeric(s):
	if isNumber(s):
		return float(s)
	return s

def highwayAnalyse(resultData):
	if 'settings' in resultData.keys():
		precidence = resultData['settings']['precidence']
		plotType = resultData['settings']['plotType']
		resultData.pop('settings',None)

	def obtainKeys( dat, loc, level, precidence, **kwargs ):
		if loc > len(precidence)+1:
			return None
		if level == loc:
			return sorted(dat.keys(),key=convertIfNumeric)
		return obtainKeys( dat[kwargs[precidence[level]]], loc, level+1, precidence, **kwargs )

	def obtainMetrics( dat, val, loc, level, precidence, **kwargs ):
		if loc > len(precidence)+1:
			return None
		if level == len(precidence):
			return dat[kwargs['metric']]
		if level == loc:
			return obtainMetrics( dat[val], None, loc, level+1, precidence, **kwargs )
		return obtainMetrics( dat[kwargs[precidence[level]]], val, loc, level+1, precidence, **kwargs )

	def acquireData( dat, axis, precidence, **kwargs ):
		hVals = []
		vVals = []
		loc = precidence.index(axis)
		hVals = obtainKeys( dat, loc, 0, precidence, **kwargs )
		vVals = [ obtainMetrics( dat, val, loc, 0, precidence, **kwargs ) for val in hVals ]
		return (hVals,vVals)

	markers = list(lines.Line2D.markers.keys())[5:] 

	while True:
		# Analyse grid 
		res = {}
		setAxes = []

		endThis = False
		xAxis = None
		currVals = resultData
		for n in precidence:
			if len(currVals) == 1:
				print "Only one value of " + n + "; Selecting '" + n + "' = " + str(currVals.keys()[0])
				res[n] = [currVals.keys()[0]]
				currVals = currVals[res[n][0]]
				continue

			res[n] = doSelection( 'Select a ' + n + ':', sorted(currVals.keys(),key=convertIfNumeric) + ['Use as axis'] )
			if not res[n]:
				endThis = True
				break
			nextN = res[n][0]
			if nextN == 'Use as axis':
				res[n] = sorted(currVals.keys(),key=convertIfNumeric)
			nextN = res[n][0]
			if len(res[n]) > 1:
				setAxes.append(n)
				nextN = res[n][0]
				strMsg = "Use as horizontal axis"
				if xAxis:
					strMsg += " (Current: " + xAxis + ")"
				strMsg += "?"
				if raw_input( strMsg ).lower() == "y":
					xAxis = n
			currVals = currVals[nextN]

		if endThis:
			break

		permutations = list( itertools.product(*[ res[n] for n in precidence if n is not xAxis ]) )
		kwargSet = [dict( zip( [a for a in precidence if a is not xAxis], p ) ) for p in permutations]

		while True:
			# Now pick a metric:
			metrics = None
			metrics = doSelection( "Metric:", sorted(currVals) )
			if not metrics:
				break

			markerIndex = 0

			if plotType == "column" or plotType == "export":
				hAxis = []
				yAxis = []
				yErr = []
				labelStrings = []

			yMax = 0
			# Get the data
			for kw in kwargSet:
				conf = kw;
				conf['metric'] = metrics[0]
				try:
					hVals,vVals = acquireData( resultData, xAxis, precidence, **conf )
				except KeyError:
					print "Skipping " + str(conf)
					continue
				labelStr = ""
				s = False
				for k in kw:
					if k in setAxes and k is not xAxis:
						if s:
							labelStr += "; "
						paramName = k
						if k in parameterAbbreviation:
							paramName = parameterAbbreviation[k]
						units = ""
						if k in parameterUnits:
							units = parameterUnits[k]
						if paramName == "Algorithm":
							labelStr += algorithmPresentation[kw[k]]
						else:
							labelStr += paramName + "=" + str(kw[k]) + units
						s = True

				v = [val[0] for val in vVals]
				verr = numpy.sqrt( [val[1] for val in vVals] ) / 2
				yMax = max( yMax, max( [ v[i]+verr[i] for i in range(0,len(v)) ] ) )

				if plotType == "column" or plotType == "export":
					hAxis = hVals
					yAxis.append( v )
					yErr.append( verr )
					labelStrings.append( labelStr )
				elif plotType == "line":
					pyplot.errorbar( hVals, v, verr, label=labelStr, marker=markers[markerIndex] )

				markerIndex+=1
				if markerIndex == len(markers):
					markerIndex = 0

			if plotType == "column":
				DrawBarPlot( hAxis, yAxis, yErr, metrics[0], xAxis, metrics[0] + " vs. " + xAxis, labelStrings )
			elif plotType == "line":
				pyplot.legend(loc='best')
				pyplot.title( metrics[0] + " vs. " + xAxis )
				pyplot.xlabel( xAxis )
				pyplot.ylabel( metrics[0] )
				pyplot.ylim(0,yMax+0.5)
				pyplot.grid( None, axis='y')
			elif plotType == "export":
				filename = metrics[0] + " vs. " + xAxis + ".dat"
				with open(filename,"w") as f:
					# Write the label strings
					f.write("Labels: "+",".join(labelStrings)+"\n")
					# Write the axis names
					f.write("xAxis: "+xAxis+"\n")
					f.write("yAxis: "+metrics[0]+"\n")
					# Write the data
					f.write("x: "+",".join([str(a) for a in hAxis])+"\n")
					for i in range(0,len(yAxis)):
						f.write("y"+str(i)+": "+",".join([str(a) for a in yAxis[i]])+"\n")
						f.write("yerr"+str(i)+": "+",".join([str(a) for a in yErr[i]])+"\n")

			pyplot.show()


# Analyse the data
def dataAnalyse( argv ):
	options = parseDataAnalyseOptions( argv )

	try:
		with open( options.dataFile, "r" ) as f:
			resultData = pickle.load( f )
	except Exception as e:
		print str(e)
		sys.exit(0)

	if 'settings' in resultData.keys():
		process = resultData['settings']['process']
	else:
		process = 'location'

	resultData['settings']['plotType'] = options.plotType

	# We have the data, now let's get to analysing it.
	if process == 'grid':
		highwayAnalyse(resultData)
	elif process == 'highway':
		highwayAnalyse(resultData)
	elif process == 'location':
		locationAnalyse(resultData)
	else:
		print "Unknown process identifier '" + process + "'"

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



