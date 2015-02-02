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
override APP=jway/screen common/ad_device common/core vehicle/mitsubishi_evoix_man vehicle/mazda_speed3_man vehicle/mazda_rx8_man vehicle/honda_civic_si_man vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/infiniti_g37_man vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man vehicle/mitsubishi_evox_man vehicle/acura_tsx_man vehicle/honda_accord_v6_man vehicle/honda_civic_man vehicle/scion_xd_man vehicle/scion_xb_man vehicle/scion_tc_sc_man vehicle/dodge_viper_man common/pi_engine  common/lifestyle common/circles common/traditional
#override APP=jway/screen common/core vehicle/nissan_350z_man vehicle/infinity_g35_auto vehicle/ford_mustang_gt_man vehicle/ford_focus_man vehicle/subaru_wrxsti_man vehicle/pontiac_g5gti_man common/pi_engine common/lifestyle

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
PF_NEED=-Dm1@magden-auto.com/m1_bin/1.-1.-1
PF_TARGET=jway_rolldemo.pfl
PF_PACKID=m1@magden-auto.com/m1_rolldemo/$(VERSION)
PF_POSTACTION=-as

#
# check out and build 
#
include $(BUILD_ROOT)/Makefile.rules

