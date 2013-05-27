#
# OMNeT++/OMNEST Makefile for libClusterLib
#
# This file was generated with the command:
#  opp_makemake -f --deep --make-so -I/home/craig/Documents/PhD/simulations/ClusterLib/URAE/include -I../../veins-2.0/src/base/utils -I../../veins-2.0/src/base/phyLayer -I../../veins-2.0/src/base/connectionManager -I../../veins-2.0/src/base/modules -I../../veins-2.0/src/base/messages -L/home/craig/Documents/PhD/simulations/ClusterLib/URAE/lib -L../../veins-2.0/out/$(CONFIGNAME)/tests/testUtils -L../../veins-2.0/out/$(CONFIGNAME)/src/modules -L../../veins-2.0/out/$(CONFIGNAME)/src/base -lmiximtestUtils -lmiximmodules -lmiximbase -KVEINS_2_0_PROJ=../../veins-2.0
#

# Name of target to be created (-o option)
TARGET = libClusterLib$(SHARED_LIB_SUFFIX)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I../URAE/include \
    -I../../veins-2.0/src/base/utils \
    -I../../veins-2.0/src/base/phyLayer \
    -I../../veins-2.0/src/base/connectionManager \
    -I../../veins-2.0/src/base/modules \
    -I../../veins-2.0/src/base/messages \
    -I.

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = -L../URAE/lib -L../../veins-2.0/out/$(CONFIGNAME)/tests/testUtils -L../../veins-2.0/out/$(CONFIGNAME)/src/modules -L../../veins-2.0/out/$(CONFIGNAME)/src/base  -lmiximtestUtils -lmiximmodules -lmiximbase
LIBS += -Wl,-rpath,`abspath ../URAE/lib` -Wl,-rpath,`abspath ../../veins-2.0/out/$(CONFIGNAME)/tests/testUtils` -Wl,-rpath,`abspath ../../veins-2.0/out/$(CONFIGNAME)/src/modules` -Wl,-rpath,`abspath ../../veins-2.0/out/$(CONFIGNAME)/src/base`

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = $O/ClusterNetworkLayer.o $O/LowestIdCluster.o $O/HighestDegreeCluster.o $O/ClusterControlMessage_m.o

# Message files
MSGFILES = \
    ClusterControlMessage.msg

# Other makefile variables (-K)
VEINS_2_0_PROJ=../../veins-2.0

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ($(MAKECMDGOALS),depend)
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET)

$(TARGET) : $O/$(TARGET)
	$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	$(SHLIB_LD) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(LIBS) $(OMNETPP_LIBS) $(LDFLAGS)
	$(SHLIB_POSTPROCESS) $O/$(TARGET)

.PHONY:

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE)
	@$(MKPATH) $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f ClusterLib ClusterLib.exe libClusterLib.so libClusterLib.a libClusterLib.dll libClusterLib.dylib
	-rm -f ./*_m.cc ./*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/ClusterControlMessage_m.o: ClusterControlMessage_m.cc \
	ClusterControlMessage_m.h \
	$(VEINS_2_0_PROJ)/src/base/messages/NetwPkt_m.h \
	$(VEINS_2_0_PROJ)/src/base/utils/MiXiMDefs.h \
	$(VEINS_2_0_PROJ)/src/base/utils/SimpleAddress.h \
	$(VEINS_2_0_PROJ)/src/base/utils/miximkerneldefs.h
$O/ClusterNetworkLayer.o: ClusterNetworkLayer.cc \
	ClusterControlMessage_m.h \
	ClusterNetworkLayer.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/BaseConnectionManager.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/ChannelAccess.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/NicEntry.h \
	$(VEINS_2_0_PROJ)/src/base/messages/NetwPkt_m.h \
	$(VEINS_2_0_PROJ)/src/base/modules/AddressingInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/ArpInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseBattery.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMacLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMobility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseModule.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseNetwLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseWorldUtility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BatteryAccess.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/Interpolation.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/MappingBase.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Coord.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FWMath.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FindModule.h \
	$(VEINS_2_0_PROJ)/src/base/utils/HostState.h \
	$(VEINS_2_0_PROJ)/src/base/utils/MiXiMDefs.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Move.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwToMacControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/PassedMessage.h \
	$(VEINS_2_0_PROJ)/src/base/utils/SimpleAddress.h \
	$(VEINS_2_0_PROJ)/src/base/utils/miximkerneldefs.h
$O/HighestDegreeCluster.o: HighestDegreeCluster.cc \
	ClusterControlMessage_m.h \
	ClusterNetworkLayer.h \
	HighestDegreeCluster.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/BaseConnectionManager.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/ChannelAccess.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/NicEntry.h \
	$(VEINS_2_0_PROJ)/src/base/messages/NetwPkt_m.h \
	$(VEINS_2_0_PROJ)/src/base/modules/AddressingInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/ArpInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseBattery.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMacLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMobility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseModule.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseNetwLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseWorldUtility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BatteryAccess.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/Interpolation.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/MappingBase.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Coord.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FWMath.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FindModule.h \
	$(VEINS_2_0_PROJ)/src/base/utils/HostState.h \
	$(VEINS_2_0_PROJ)/src/base/utils/MiXiMDefs.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Move.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwToMacControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/PassedMessage.h \
	$(VEINS_2_0_PROJ)/src/base/utils/SimpleAddress.h \
	$(VEINS_2_0_PROJ)/src/base/utils/miximkerneldefs.h
$O/LowestIdCluster.o: LowestIdCluster.cc \
	ClusterControlMessage_m.h \
	ClusterNetworkLayer.h \
	LowestIdCluster.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/BaseConnectionManager.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/ChannelAccess.h \
	$(VEINS_2_0_PROJ)/src/base/connectionManager/NicEntry.h \
	$(VEINS_2_0_PROJ)/src/base/messages/NetwPkt_m.h \
	$(VEINS_2_0_PROJ)/src/base/modules/AddressingInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/ArpInterface.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseBattery.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMacLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseMobility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseModule.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseNetwLayer.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BaseWorldUtility.h \
	$(VEINS_2_0_PROJ)/src/base/modules/BatteryAccess.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/Interpolation.h \
	$(VEINS_2_0_PROJ)/src/base/phyLayer/MappingBase.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Coord.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FWMath.h \
	$(VEINS_2_0_PROJ)/src/base/utils/FindModule.h \
	$(VEINS_2_0_PROJ)/src/base/utils/HostState.h \
	$(VEINS_2_0_PROJ)/src/base/utils/MiXiMDefs.h \
	$(VEINS_2_0_PROJ)/src/base/utils/Move.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/NetwToMacControlInfo.h \
	$(VEINS_2_0_PROJ)/src/base/utils/PassedMessage.h \
	$(VEINS_2_0_PROJ)/src/base/utils/SimpleAddress.h \
	$(VEINS_2_0_PROJ)/src/base/utils/miximkerneldefs.h

