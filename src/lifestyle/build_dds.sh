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
SKIN=LIFESTYLE


#
# Warning/Red lights
#
RESOLUTION="Standard"
COLORS="Red Yellow"
LAYERS="LeftGaugeGlow"
for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}FromLeft-${layer}-01.png -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done



#
# Nozzles
#
RESOLUTION="Standard"
COLORS="Metalic1"
LAYERS="NozzleLeft NozzleMiddle"
for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}-${layer}-01.png -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done



#
# Cricital zone
#
RESOLUTION="Standard"
COLORS="CriticalMark"
LAYERS="Red"
for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}${layer}-ClipAnim1-01.png -c ${1}_dds/${SKIN}-${res}-${color}${layer}-Progress.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done



# COLORS="AlumCRITICAL AlumCYAN BeamCRITICAL BeamGREEN ElectricCRITICAL ElectricGREEN FireCRITICAL FireYELLOW PlainCRITICAL PlainORANGE"
RESOLUTION="Standard"
COLORS="Blue Green Orange Steel Yellow StructuredWhite"
LAYERS="Glow Shadow"
for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}${layer}-ClipAnim1-01.png -c ${1}_dds/${SKIN}-${res}-${color}${layer}-Progress.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done

#
# End caps
#
RESOLUTION="Standard"
COLORS="Blue CriticalRed Green Orange Steel Yellow StructuredWhite"
LAYERS="Glow-LeftCap Glow-RightCap Shadow-LeftCap Shadow-RightCap"
for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-Chrome${color}${layer}-01.png -c ${1}_dds/${SKIN}-${res}-${color}${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done


#
# background
#
RESOLUTION="640x480 800x480"
COLORS="CarbonFiber1 Alum1 BlackIce1 TwoBlackMatte1 Chrome1" #  FancyAlum1  TwBlackMatte1
LAYERS="Logo Background HiLiteLayer Overlay"

for res in $RESOLUTION
do
  for color in $COLORS
  do
    for layer in $LAYERS
      do
      $DDSCOMP -f src_images/${SKIN}-${res}-${color}-${layer}-1.png -c ${1}_dds/${SKIN}-${res}-${color}-${layer}.dds  -C${1} -i -l  | grep -v compressing
      done
  done
done

exit 0
