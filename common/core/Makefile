#
# All rights reserved. Reproduction, modification, use or disclosure
# to third parties without express authority is forbidden.
# Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
#

#
# Build all necessary DDS files and other stuff.
#

UNAME := $(shell uname)
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
# Source in m1/skins/src/trunk from which we retrieve
# DDS files.
#
override DDS_SOURCE=gui

#
# Which subskins do we want to include here.
# If this is non-nil the included files will be a part of the packfile.
#
override APP=

#
# List all plugins needed by this skin
#
override PLUGINS=log csv_store

# override FONTS=bankgothic:10 bankgothic:12 bankgothic:20 


#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules
