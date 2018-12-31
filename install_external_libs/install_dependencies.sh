#!/bin/bash

export PREFIX=/usr/local # default: /usr/local
export PETSC_DOWNLOAD=http://ftp.mcs.anl.gov/pub/petsc/release-snapshots/petsc-3.10.3.tar.gz
export SLEPC_DOWNLOAD=http://slepc.upv.es/download/distrib/slepc-3.10.1.tar.gz
export ENABLE_MPI=0 # Message Passing Interface; for multiprocessors. default: 0
export ENABLE_MUMPS_SERIAL=1 # default: 1
export DOWNLOAD_MUMPS=1 # default: 1
export DOWNLOAD_OPENBLAS=1 # default: 1
export ENABLE_DEBUG_SYMBOLS=1 # default: 0
export COMPILER_OPTIMIZATION_LEVEL=3 # default: 3

#./install1_petsc.sh
./install2_slepc.sh
