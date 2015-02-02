#!/bin/sh
#
# Generic build scripts for this skin
#
#!/bin/sh


#
# Buttons
#
BUTTONS="Back Cancel FastForward FastRewind ForwardToEnd Next Ok Pause Play Record RewindToStart Stop"
STATES="Faded Normal Selected"

for but in $BUTTONS
do
  i=1;
  for stat in $STATES
  do
    mv OS-Symbol-${but}-${stat}.png OS-Symbol-${but}-$i.png 
    i=`expr $i + 1`
  done
done

exit 0

