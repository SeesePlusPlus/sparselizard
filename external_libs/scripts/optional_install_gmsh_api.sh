#!/bin/bash



########## ALL LIBRARIES REQUIRED BY SPARSELIZARD ARE PUT IN ~/SLlibs.

SCRIPT=$(readlink -f "$0")
SCRIPTDIR=$(dirname "$SCRIPT")
VERSION=4.5.2
BASEDIR=${SCRIPTDIR}/../libs/gmsh

mkdir $BASEDIR;
pushd $BASEDIR;

if [ ! -d ./gmsh-$VERSION ]; then
  ########## DOWNLOAD THE GMSH API :

  echo '__________________________________________';
  echo 'FETCHING THE GMSH API';
  rm gmsh*.tgz;
  wget http://gmsh.info/bin/Linux/gmsh-${VERSION}-Linux64-sdk.tgz;
  tar -xf *.tgz;
  rm gmsh*.tgz;
  mv gmsh* gmsh-${VERSION};
fi

popd
