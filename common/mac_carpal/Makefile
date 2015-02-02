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
BUILD_ROOT?=$(PWD)/../../..

#
# Makefile include for setting up various variables.
#
include $(BUILD_ROOT)/Makefile.inc
include ../../Version.inc


#
# Skins that we need in order to function. 
#
override APP=common/screen_x11 common/ad_device common/core  vehicle/peugeot_307hdi_man vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man vehicle/mitsubishi_evox_man common/carpal_engine common/circles common/traditional common/lifestyle 

#
# Source in m1/skins/src/trunk from which we retrieve
# DDS files. 
#
override DDS_SOURCE=

#
# List all plugins needed by this skin
#
override PLUGINS=ad_device

#
# Packfile spec.
# PF_NEED=magden@magden.us/jway_core/1.0.0 
PF_NEED=
PF_TARGET=common_rolldemo.pfl
PF_PACKID=magden@magden.us/common_rolldemo/1.0.0

#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules

