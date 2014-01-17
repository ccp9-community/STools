#!/bin/bash

#set -x

# Required library to perform manipulation of castep input files
source castep_manip.sh

if [ "$#" -ne "2" ]
then
  echo "Usage: $0 [settings_file] [structure_file]"
  exit 1
fi

## SETTINGS ##
# The following variables should be set before running this script
# (change values to suit your needs):
#
# export SEED=graphene (the castep seed)
# export REMOTE_HOST=muhrincp@login.hector.ac.uk (the remote host string)
# export REMOTE_WORK_DIR=work/runs/remote (the remote host working directory)
# export JOB_FILE=graphene.job (the submission script)

# INPUTS ##
declare -r SETTINGS=$1
declare -r STRUCTURE=$2

filename=$(basename "$STRUCTURE")
extension="${filename##*.}"
filename="${filename%.*}"

declare -r PARAM_FILE=${filename}.param
declare -r CELL_FILE=${filename}.cell
declare -r NEW_JOB_FILE=${filename}.job

##################################
# Set up the cell file for the run
if [ "$extension" != "cell" ]
then
  sconvert $STRUCTURE $CELL_FILE
fi

# Copy over any stub cell contents
if [ -e "${SEED}.cell" ]
then
  cat ${SEED}.cell >> $CELL_FILE
fi

cp ${SEED}.param $PARAM_FILE

#################################
# Set up the job file for the run
sed "s/REPLACE_SEED/${filename}/" $JOB_FILE > ${NEW_JOB_FILE}

#################################################
# Set up the param file using the input settings
iter=$(castep_param_get_value $SETTINGS maxIter)
if [ -n "$iter" ]
then
  castep_param_set_value $PARAM_FILE GEOM_MAX_ITER $iter
fi

qsub_remote $REMOTE_HOST $REMOTE_WORK_DIR $NEW_JOB_FILE $PARAM_FILE $CELL_FILE > /dev/null 2>&1
if [ "$?" -ne "0" ]
then
  exit 1
fi

## Did we get the castep file?
declare -r CASTEP=${filename}.castep
if [ -e "$CASTEP" ]
then
  sconvert ${filename}.castep\#last $STRUCTURE
  echo -e "\n\n#  OUTCOME" >> $SETTINGS
  echo "successful: true" >> $SETTINGS
  # Get information from the castep file
  sinfo graphene.castep\#last -f -n -i 'finalEnthalpy: $h$\nfinalInternalEnergy: $u$ \nfinalPressure: $p$ \n' >> $SETTINGS
  iters=`grep -oE "finished iteration .*" $CASTEP | tail -n 1 | sed -r 's/finished iteration[[:blank:]]+([[:digit:]]+)[[:blank:]]+.*/\1/'`
  if [ -n "$iters" ]
  then
    echo "finalIters: $iters" >> $SETTINGS
  fi
else
  echo "successful: false" >> $SETTINGS
  exit 1
fi

exit 0