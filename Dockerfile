FROM amd64/ubuntu:18.04

ENV PREFIX /usr/local
ENV PETSC_DOWNLOAD http://ftp.mcs.anl.gov/pub/petsc/release-snapshots/petsc-3.10.3.tar.gz
ENV SLEPC_DOWNLOAD http://slepc.upv.es/download/distrib/slepc-3.10.1.tar.gz
ENV ENABLE_MPI 0
ENV ENABLE_MUMPS_SERIAL 1
ENV DOWNLOAD_MUMPS 1
ENV DOWNLOAD_OPENBLAS 1
ENV ENABLE_DEBUG_SYMBOLS 1
ENV COMPILER_OPTIMIZATION_LEVEL 3

RUN apt-get update
RUN apt-get install -y wget
RUN apt-get install -y make
RUN apt-get install -y build-essential
RUN apt-get install -y python
RUN apt-get install -y gfortran
RUN apt-get install -y valgrind
RUN apt-get install -y git
RUN apt-get install -y cmake
RUN apt-get install -y unzip

COPY install_external_libs/install1_petsc.sh /app/install1_petsc.sh
COPY install_external_libs/install2_slepc.sh /app/install2_slepc.sh

RUN /app/install1_petsc.sh
RUN /app/install2_slepc.sh

COPY . /app/sparselizard
WORKDIR /app/sparselizard
RUN mkdir build
WORKDIR /app/sparselizard/build
RUN cmake ..
RUN make VERBOSE=1
RUN make install

RUN ldconfig
RUN rm -rf /app/*
RUN rm -rf ~/SLlibs
