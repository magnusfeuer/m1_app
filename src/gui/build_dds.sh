#!/bin/sh
#
# Generic build scripts for this skin
#
#!/bin/sh

if [ "$DDSCOMP" = "" ] 
then
    DDSCOMP=../../../bin/ddscomp
fi

if [ $# != 1 ]
then
    echo "Usage: $1 [rgb|bgr|rgba|argb|abgr|bgra]"
    exit 1
fi

#
# Check that we are not trying to compile if we already have a result
#
if [ -d ${1}_dds ]
then
    echo "$PWD/${1}_dds already exists. Will not compile. Delete this directory and run build_dds.sh again to recompile."
    exit 0
fi 



mkdir ${1}_dds > /dev/null 2>&1
SKIN=OS


#
# Dialogs
#
DIALOG_RES="497x279 600x410 357x410 497x360 410x236 410x410"

for res in $DIALOG_RES
do
  $DDSCOMP -f "${SKIN}-Dialog-BlackAlum-${res}.png" -c ${1}_dds/${SKIN}-Dialog-BlackAlum-${res}.dds  -C${1} -i -l | grep -v compressing
done


#
# Icons
#
ICONS="BackUp PowerRun TireSize GearCalibration Res640x480 Res800x480 AESensor Calibration CarKeys Resolution TimeDate"

for ico in $ICONS
do
  $DDSCOMP -f ${SKIN}-MenuIcon-${ico}-%d.png -c ${1}_dds/${SKIN}-MenuIcon-${ico}.dds  -C${1} -i -l| grep -v compressing
done

#
# Buttons
#
BUTTONS="Back Cancel FastForward FastRewind ForwardToEnd Next Ok Pause Play Record RewindToStart Stop"

for but in $BUTTONS
do
  $DDSCOMP -f ${SKIN}-Symbol-${but}-%d.png -c ${1}_dds/${SKIN}-Symbol-${but}.dds  -C${1} -i -l| grep -v compressing
done

# Shadow and overlay
$DDSCOMP -f OS-Button-Shadow.png -c${1}_dds/OS-Button-Shadow.dds  -C${1} -i -l | grep -v compressing
$DDSCOMP -f OS-Button-Overlay.png -c${1}_dds/OS-Button-Overlay.dds  -C${1} -i -l | grep -v compressing

# OK button
$DDSCOMP -f ok%.2d.png -c${1}_dds/gui_ok.dds  -C${1} -i -l | grep -v compressing

# Cancel button
$DDSCOMP -f cancel%.2d.png -c${1}_dds/gui_cancel.dds  -C${1} -i -l | grep -v compressing

# Back button
$DDSCOMP -f prev%.2d.png -c${1}_dds/gui_prev.dds  -C${1} -i -l | grep -v compressing

# Forward button
$DDSCOMP -f next%.2d.png -c${1}_dds/gui_next.dds  -C${1} -i -l | grep -v compressing

# minus button
$DDSCOMP -f minus%.2d.png -c${1}_dds/gui_minus.dds  -C${1} -i -l | grep -v compressing

# plus button
$DDSCOMP -f plus%.2d.png -c${1}_dds/gui_plus.dds  -C${1} -i -l | grep -v compressing

# Touch screen icon
$DDSCOMP -f icon_touch%.2d.png -c${1}_dds/gui_icon_touch.dds  -C${1} -i -l | grep -v compressing

# metric icon
$DDSCOMP -f icon_metric%.2d.png -c${1}_dds/gui_icon_metric.dds  -C${1} -i -l | grep -v compressing

# screen setup icon
$DDSCOMP -f icon_screen%.2d.png -c${1}_dds/gui_icon_screen.dds  -C${1} -i -l | grep -v compressing

# screen setup icon
$DDSCOMP -f icon_time%.2d.png -c${1}_dds/gui_icon_time.dds  -C${1} -i -l | grep -v compressing

# select vehicle icon
$DDSCOMP -f icon_car%.2d.png -c${1}_dds/gui_icon_car.dds  -C${1} -i -l | grep -v compressing


# Screen res 640x480 3 images
$DDSCOMP -f icon_screen_res_640x480_%.2d.png -c${1}_dds/gui_icon_screen_res_640x480.dds  -C${1} -i -l | grep -v compressing

# Screen res 800x480 3 images.
$DDSCOMP -f icon_screen_res_800x480_%.2d.png -c${1}_dds/gui_icon_screen_res_800x480.dds  -C${1} -i -l | grep -v compressing

# Copy all PNG files not part of a DDS files into target
cp gui_dialog_*.png ${1}_dds

# Copy all PNG files not part of a DDS files into target
cp gui_menu*.png ${1}_dds

# Copy record png to target
cp rec.png ${1}_dds


# Copy new arrow to target.
cp Arrow-48x48.png ${1}_dds

exit 0