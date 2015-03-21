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
override APP=jway/screen common/ad_device  common/core common/consult_engine common/circles common/traditional common/lifestyle vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man vehicle/peugeot_307hdi_man
# override SKINS=jway/screen common/core common/apex_engine  vehicle/nissan_350z_man  vehicle/peugeot_307hdi_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man common/lifestyle

#
# Source in m1/skins/src/trunk from which we retrieve
# DDS files. 
#
override DDS_SOURCE=

#
# List all plugins needed by this skin
#
override PLUGINS=touchscreen_device log ad_device

#
# Packfile spec.
#
PF_NEED=-Dm1@magden-auto.comn/m1_bin/1.-1.-1 
PF_TARGET=ff4jway_core.pfl
PF_PACKID=m1@magden-auto.com/m1_core/$(VERSION)
PF_POSTACTION=-as

#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules
