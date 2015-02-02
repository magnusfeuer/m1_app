#
# Master makefile for all plugins
#
PLUGINS=mut3_device canusb_device zt2_device lc1_device carpal_device ad_device consult_device log apex_device pi_sys3_device touchscreen_device csv_store

.PHONY: all clean spotless

all: 
	for i in $(PLUGINS); do (cd $${i}; make); done

clean:
	for i in $(PLUGINS); do (cd $${i}; make clean); done

spotless:
	for i in $(PLUGINS); do (cd $${i}; make spotless); done