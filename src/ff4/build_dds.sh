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
    tar xvzf images.tgz
    echo "Done."
fi 

mkdir ${1}_dds
#
# Turbine animation
#

$DDSCOMP -f "src_images/FaF-ScrambleBoost-TurboAnim-%.2d.png" -c ${1}_dds/ff4_turbine.dds  -C${1} -i -l | grep -v compressing

exit 0
