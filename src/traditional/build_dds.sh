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


#
# Check that we have unpacked the tar file
#
if [ ! -d src_images ] 
then
    echo "Unpacking tar file with images. Plz wait."
    tar xzf images.tgz
    echo "Done."
fi 


mkdir ${1}_dds > /dev/null 2>&1
SKIN=TRADITIONAL

#
# Warning ligts
#
RESOLUTION="Standard"
BG_COLORS="Socket1 Red1 Yellow1"
BG_LAYERS="BigFaceHighContrast1 SmallFaceHighContrast1 BigFaceAlum1  BigFaceCarbonFiber1 BigFaceBlackPlastic1 BigFaceWhiteGauge1 SmallFaceAlum1 SmallFaceCarbonFiber1 SmallFaceBlackPlastic1 SmallFaceWhiteGauge1"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done


#
# Critical marks
#
RESOLUTION="Standard"
BG_COLORS="CriticalRedMark"
BG_LAYERS="SmallFace- BigFace"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}%.4d.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done


#
# Center knobs
#
RESOLUTION="Standard"
BG_COLORS="HighContrast1 Alum1 CarbonFiber1 BlackPlastic1 WhiteGauge1"
BG_LAYERS="CenterKnob CenterKnobSmall"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-1.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done


#
# TICKS
#
RESOLUTION="Standard"
BG_COLORS="White"
BG_LAYERS="Big_3_1_1_3_1  Big_4_1_1_4_1 Big_5_1_1_5_1 Big_6_1_1_6_1 Big_7_1_1_7_1 Small_3_1_1_3_1  Small_4_1_1_4_1 Small_5_1_1_5_1 Small_6_1_1_6_1 Small_7_1_1_7_1"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-1.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l | grep -v compressing
      done
  done
done

#
# BACKGROUNDS
#
# Original BG_COLORS="CarbonFiber1 Alum1 BlackPlastic1 BmwNight1 VintageLeather1 Walnut1"
#           BG_COLORS="CarbonFiber1 BlackPlastic1 Alum1 VintageLeather1 Walnut1"
RESOLUTION="640x480 800x480"
BG_COLORS="HighContrast1 CarbonFiber1 BlackPlastic1 Alum1 WhiteGauge1"
BG_LAYERS="Logo Background HiLiteLayer"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}-${layer}-1.png -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done



#
# GAUGES
#
RESOLUTION="Standard"
BG_COLORS="Green1 Blue1 Red1 White1 PlainOrange"
BG_LAYERS="SmallNeedle1 BigNeedle1"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-%.4d.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done


exit 0
