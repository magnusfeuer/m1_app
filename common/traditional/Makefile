#
# All rights reserved. Reproduction, modification, use or disclosure
# to third parties without express authority is forbidden.
# Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
#

#
# Build all necessary DDS files and other stuff.
#


#
# Root directory (where m1/dist/[platform]/[version] is checked out)
#
BUILD_ROOT ?=$(PWD)/../../..

#
# Makefile include for setting up various variables.
#
include $(BUILD_ROOT)/Makefile.inc
include ../../Version.inc

#
# Skins that we need in order to function. 
#
override APP=

#
# Source in m1/skins/src/trunk from which we retrieve
# DDS files. (Not used here)
#
override DDS_SOURCE=traditional

#
# List all plugins needed by this skin
#
override PLUGINS=


#
# Packfile spec.
#
PF_NEED=-Dm1@magden-auto.com/m1_core/1.-1.-1 
PF_TARGET=traditional.pfl
PF_PACKID=m1@magden-auto.com/traditional/$(VERSION)
PF_POSTACTION=-as

#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules

