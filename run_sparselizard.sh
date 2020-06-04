#!/bin/bash

# THIS SCRIPT RUNS THE SPARSELIZARD EXECUTABLE

SCRIPT=$(readlink -f "$0")
SCRIPTDIR=$(dirname "$SCRIPT")
EXTERNALLIBSDIR=$SCRIPTDIR/external_libs/libs

if [ "$(uname)" == "Linux" ]; then
# With or without the GMSH API:
if [ -d "$EXTERNALLIBSDIR/gmsh" ]; then
    LD_LIBRARY_PATH="$EXTERNALLIBSDIR/petsc/arch-linux-c-opt/lib":"$EXTERNALLIBSDIR/gmsh/lib":$LD_LIBRARY_PATH
else
    LD_LIBRARY_PATH="$EXTERNALLIBSDIR/petsc/arch-linux-c-opt/lib":$LD_LIBRARY_PATH
fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
elif [ "$(uname)" == "Darwin"  ]; then
DYLD_LIBRARY_PATH="$EXTERNALLIBSDIR/petsc/arch-darwin-c-opt/lib":$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH
fi


./sparselizard;
