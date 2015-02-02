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
override APP=jway/screen common/ad_device common/core vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man vehicle/mitsubishi_evox_man common/pi_engine common/circles common/traditional common/lifestyle 
#override APP=jway/screen common/core vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man common/pi_engine common/lifestyle

#
# Source in m1/skins/src/trunk from which we retrieve
# DDS files. 
#
override DDS_SOURCE=ff4

#
# List all plugins needed by this skin
#
override PLUGINS=touchscreen_device log ad_device



#
# Packfile spec.
PF_NEED=
PF_TARGET=ff4_rolldemo.pfl
PF_PACKID=m1@magden-auto.com/ff4_rolldemo/$(VERSION)

#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules

