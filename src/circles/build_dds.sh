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
SKIN=CIRCLES


#
# Redline lights
#
RESOLUTION="Standard"
BG_COLORS="Red Yellow"
BG_LAYERS="BigGaugeGlow SmallGaugeGlow"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-01.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l | grep -v compressing
      done
  done
done

#
# Critical markings
#
RESOLUTION="Standard"
BG_COLORS="CriticalMarkingRed"
BG_LAYERS="BigProgress SmallProgress"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-0%d.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l | grep -v compressing
      done
  done
done

# GAUGES
#
#
RESOLUTION="Standard"
BG_COLORS="StructuredWhite1 BlueGlow1 PlainWhite1 ChromeGlow1 GreenGlow1 OrangeGlow1" # RedCritical 
BG_LAYERS="SmallProgressDual1 BigProgressDual1 BigProgress1 SmallProgress1"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f "src_images/${SKIN}-${res}-${color}-${layer}-%.4d.png" -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l | grep -v compressing
      done
  done
done


#
# BACKGROUNDS
#
RESOLUTION="640x480 800x480"
BG_COLORS="GreenGlow1 OrangeGlow1 Alum1 BlackMatte1 BlueGlow1 CarbonIce1 " #  BlackIce1
BG_LAYERS="Logo Background HiLiteLayer"

for res in $RESOLUTION
do
  for color in $BG_COLORS
  do
    for layer in $BG_LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}-${layer}-1.png -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l| grep -v compressing
      done
  done
done


#
# Logos
#
cp src_images/*-Logo-1.png ${1}_dds

exit 0
