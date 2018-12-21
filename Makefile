##### THESE ARE THE REQUIRED LIBRARIES:

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LIBS = -L ~/SLlibs/petsc/arch-linux2-c-opt/lib -l openblas -l petsc -l slepc
INCL = -I ~/SLlibs/petsc/include/petsc/mpiuni -I ~/SLlibs/petsc/arch-linux2-c-opt/externalpackages/git.openblas -I ~/SLlibs/petsc/include/ -I ~/SLlibs/petsc/arch-linux2-c-opt/include/
endif
ifeq ($(UNAME), Darwin)
LIBS = -L ~/SLlibs/petsc/arch-darwin-c-opt/lib -l openblas -l petsc -l slepc
INCL = -I ~/SLlibs/petsc/include/petsc/mpiuni -I ~/SLlibs/petsc/arch-darwin-c-opt/externalpackages/git.openblas -I ~/SLlibs/petsc/include/ -I ~/SLlibs/petsc/arch-darwin-c-opt/include/
endif


# $@ is the filename representing the target.
# $< is the filename of the first prerequisite.
# $^ the filenames of all the prerequisites.
# $(@D) is the file path of the target file. 
# D can be added to all of the above.

CXX = g++ -fopenmp # openmp is used for parallel sorting on Linux
CXX_FLAGS= -std=c++11 -O3 -fPIC -Wno-return-type

# List of all directories containing the headers:
INCLUDES = -I src -I src/field -I src/expression -I src/expression/operation -I src/shapefunction -I src/formulation -I src/shapefunction/hierarchical -I src/shapefunction/hierarchical/h1 -I src/shapefunction/hierarchical/hcurl -I src/shapefunction/hierarchical/meca -I src/gausspoint -I src/shapefunction/lagrange -I src/mesh -I src/io -I src/io/gmsh -I src/io/paraview -I src/io/nastran -I src/resolution -I src/geometry
# List of all .cpp source files:
CPPS= $(wildcard src/**/*.cpp)
# List of all .h header files:
INCLUDE_FILES = $(wildcard src/**/*.h)
# Final binary name:
BIN = sparselizard
# Put all generated stuff to this build directory:
BUILD_DIR = ./build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

PREFIX = /usr/local
INSTALL_NAME = sparselizard
INCLUDE_INSTALL_DIR = $(PREFIX)/include/$(INSTALL_NAME)
LIB_INSTALL_DIR = $(PREFIX)/lib/$(INSTALL_NAME)

# Same list as CPP but with the .o object extension:
OBJECTS=$(CPPS:%.cpp=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJECTS:%.o=%.d)

all: $(OBJECTS)
	# The main is always recompiled (it could have been replaced):
	$(CXX) $(CXX_FLAGS) $(LIBS) $(INCL) $(INCLUDES) -c main.cpp -o $(BUILD_DIR)/main.o
	# Linking objects:
	mkdir -p $(BIN_DIR)
	$(CXX) $(BUILD_DIR)/main.o $(OBJECTS) $(LIBS) -o $(BIN_DIR)/$(BIN)
	make shared-lib

include-files:
	echo $(INCLUDE_FILES)

# Include all .d files
-include $(DEP)

$(BUILD_DIR)/%.o: %.cpp
	# Create the folder of the current target in the build directory:
	mkdir -p $(@D)
	# Compile .cpp file. MMD creates the dependencies.
	$(CXX) $(CXX_FLAGS) $(LIBS) $(INCL) $(INCLUDES) -MMD -c $< -o $@

shared-lib:
	mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $(LIB_DIR)/lib$(BIN).so $(OBJECTS)

install:
	mkdir -p $(INCLUDE_INSTALL_DIR)
	mkdir -p $(LIB_INSTALL_DIR)
	cp $(INCLUDE_FILES) $(INCLUDE_INSTALL_DIR)
	cp $(LIB_DIR)/* $(LIB_INSTALL_DIR)

clean :
	# Removes all files created.
	rm -rf $(BUILD_DIR)
