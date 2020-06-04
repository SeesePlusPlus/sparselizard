mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))
external_libs_dir := $(current_dir)external_libs/libs

include $(external_libs_dir)/petsc/lib/petsc/conf/petscvariables

##### THESE ARE THE REQUIRED LIBRARIES:

UNAME := $(shell uname)
PREFIX := /usr/local

LIBS = \
	-L $(external_libs_dir)/petsc/$(PETSC_ARCH)/lib \
	-l openblas \
	-l petsc \
	-l slepc
INCL = \
	-I $(external_libs_dir)/petsc/include/petsc/mpiuni \
	-I $(external_libs_dir)/petsc/include/ \
	-I $(external_libs_dir)/petsc/$(PETSC_ARCH)/include/ \
	-I $(external_libs_dir)/petsc/$(PETSC_ARCH)/externalpackages/git.openblas \
	-I $(external_libs_dir)/petsc/$(PETSC_ARCH)/externalpackages/git.slepc/include


# With or without the GMSH API:
ifneq ("$(wildcard $(external_libs_dir)/gmsh)","")
	LIBS = $(LIBS) -L ~/SLlibs/gmsh/lib -l gmsh -D HAVE_GMSHAPI
	INCL = $(INCL) -I ~/SLlibs/gmsh/include
endif

# $@ is the filename representing the target.
# $< is the filename of the first prerequisite.
# $^ the filenames of all the prerequisites.
# $(@D) is the file path of the target file.
# D can be added to all of the above.

# Final binary name:
BIN = sparselizard
# Put all generated stuff to this build directory:
BUILD_DIR ?= ./build
# Source directories
SRC_DIRS ?= ./src
LIB_INSTALL_DIR := $(PREFIX)/lib/$(BIN)
INC_INSTALL_DIR := $(PREFIX)/include/$(BIN)

# List of all directories containing the headers:
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CXX = g++ -fopenmp # openmp is used for parallel sorting on Linux
CXX_FLAGS= $(INC_FLAGS) -std=c++11 -O3 -Wno-return-type -fpic # Do not warn when not returning anything (e.g. in virtual functions)

# List of all .cpp source files:
CPPS := $(shell find src -name \*.cpp -or -name \*.c -or -name \*.s)

# Same list as CPP but with the .o object extension:
OBJECTS=$(CPPS:%=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJECTS:.o=.d)

all: $(OBJECTS)
	# The main is always recompiled (it could have been replaced):
	$(CXX) $(CXX_FLAGS) $(LIBS) $(INCL) -c main.cpp -o $(BUILD_DIR)/main.o
	# Linking objects:
	$(CXX) $(BUILD_DIR)/main.o $(OBJECTS) $(LIBS) -o $(BIN)

# Include all .d files
-include $(DEP)

$(BUILD_DIR)/%.cpp.o: %.cpp
	# Create the folder of the current target in the build directory:
	mkdir -p $(@D)
	# Compile .cpp file. MMD creates the dependencies.
	$(CXX) $(CXX_FLAGS) $(LIBS) $(INCL) -MMD -c $< -o $@

shared: $(OBJECTS)
	$(CXX) -shared -o lib$(BIN).so $(OBJECTS)

install: lib$(BIN).so
	mkdir -p $(INC_INSTALL_DIR)
	find src -name \*.h -exec cp {} $(INC_INSTALL_DIR) \;

	mkdir -p $(LIB_INSTALL_DIR)
	cp lib$(BIN).so $(LIB_INSTALL_DIR)
	find $(external_libs_dir)/petsc/$(PETSC_ARCH)/lib -name lib\*so\* -exec cp -P {} $(LIB_INSTALL_DIR) \;

clean :
	# Removes all files created.
	rm -rf $(BUILD_DIR)
	rm -f $(BIN)
	rm -f libsparselizard.so
