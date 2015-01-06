CA_DIR=../../celera_assembler_replacement/wgs-8.1

# A bunch of stuff from the Celera Assembler Makefile
LDFLAGS= -D_GLIBCXX_PARALLEL -fopenmp -pthread -lm -lz -L $(CA_DIR)/Linux-amd64/lib -lCA
CXXFLAGS:= -D_GLIBCXX_PARALLEL -fopenmp
CXXFLAGS+= -pthread -Wall -Wextra -Wno-write-strings -Wno-unused -Wno-char-subscripts -Wno-sign-compare -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DX86_GCC_LINUX
CA_SUBDIRS = AS_RUN \
             AS_UTL \
             AS_UID \
             AS_MSG \
             AS_PER \
             AS_GKP \
             AS_OBT \
             AS_MER \
             AS_OVL \
             AS_OVM \
             AS_OVS \
             AS_ALN \
             AS_CGB \
             AS_BOG \
             AS_BAT \
             AS_PBR \
             AS_REZ \
             AS_CNS \
             AS_LIN \
             AS_CGW \
             AS_TER \
             AS_ENV \
             AS_REF

INC_IMPORT_DIRS += $(CA_DIR)/src $(patsubst %, $(CA_DIR)/src/%, $(strip $(CA_SUBDIRS)))
INC_DIRS = $(patsubst %, -I%, \ $(strip $(INC_IMPORT_DIRS)))

CXXFLAGS+=$(INC_DIRS)

LIB_SOURCES = GKPReader.cpp LASReader.cpp OVBWriter.cpp dalign/DB.cpp dalign/QV.cpp dalign/align.cpp
EXE_SOURCES = gkp_to_fasta.cpp las_to_ovb.cpp

LIB_OBJECTS = $(patsubst %.cpp,%.o,$(strip $(LIB_SOURCES))) 
EXE_OBJECTS = $(patsubst %.cpp,%.o,$(strip $(EXE_SOURCES))) 

TARGETS = $(patsubst %.o,%,$(EXE_OBJECTS))

all: $(LIB_OBJECTS) $(EXE_OBJECTS) $(TARGETS)

gkp_to_fasta: $(LIB_OBJECTS) gkp_to_fasta.o
	$(CXX) $^ -o $@ $(LDFLAGS)

las_to_ovb: $(LIB_OBJECTS) las_to_ovb.o
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(EXE_OBJECTS)
	rm -f $(LIB_OBJECTS)
	rm -f $(TARGETS)
