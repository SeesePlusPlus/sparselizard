#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPTDIR=$(dirname "$SCRIPT")
$SCRIPTDIR/install_petsc.sh
$SCRIPTDIR/optional_install_gmsh_api.sh
