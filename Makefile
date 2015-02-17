# The location of the Celera Assember installation
CELERA_DIR=celera_assembler/wgs-8.1

# A bunch of stuff from the Celera Assembler Makefile
CELERA_LDFLAGS= -D_GLIBCXX_PARALLEL -fopenmp -pthread -lm -lz -L $(CELERA_DIR)/Linux-amd64/lib -lCA

CELERA_CXXFLAGS:= -D_GLIBCXX_PARALLEL -fopenmp
CELERA_CXXFLAGS+= -pthread
CELERA_CXXFLAGS+= -Wno-sign-compare -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DX86_GCC_LINUX

CA_SUBDIRS=AS_RUN AS_UTL AS_UID AS_MSG AS_PER \
           AS_GKP AS_OBT AS_MER AS_OVL AS_OVM \
           AS_OVS AS_ALN AS_CGB AS_BOG AS_BAT \
           AS_PBR AS_REZ AS_CNS AS_LIN AS_CGW \
           AS_TER AS_ENV AS_REF

INC_IMPORT_DIRS+=$(CELERA_DIR)/src $(patsubst %, $(CELERA_DIR)/src/%, $(strip $(CA_SUBDIRS)))
INC_DIRS=$(patsubst %, -I%, \ $(strip $(INC_IMPORT_DIRS)))
CELERA_CXXFLAGS+=$(INC_DIRS)
# and that's the end of the celera-specific stuff

CXXFLAGS=-std=c++11 -g -O1

# In the daligner C code, there's a bunch of code c++ would call a string
# constant to char* conversion, but whatever 
CXXFLAGS+= -Wno-write-strings
# The code that prints warnings, etc in celera assembler is missing some spaces
# that C++11 complains about, but again, whatever
CXXFLAGS+= -Wno-literal-suffix

LDFLAGS=-L/home/UNIXHOME/mkinsella/local/lib -lprotobuf

# The protocol buffer files and how to make them
PROTOS=$(wildcard *.proto)
PROTO_SRCS=$(patsubst %.proto,%.pb.cc,$(PROTOS))

%.pb.cc: %.proto
	~/local/bin/protoc --cpp_out=$(dir $<) $<

# Source files that need Celera Assembler
CELERA_DEPENDENT_SRCS= OVBWriter.cc
# Source files that don't need Celera Assembler
CELERA_INDEPENDENT_SRCS= LASReader.cc dalign/DB.cc dalign/QV.cc dalign/align.cc Trimmer.cc
CELERA_INDEPENDENT_SRCS+=$(PROTO_SRCS)

EXE_SRCS=read_from_las.cc write_to_ovb.cc
EXE_OBJS=$(patsubst %.cc,%.o,$(EXE_SRCS))
EXES=$(patsubst %.o,%,$(EXE_OBJS))

CELERA_DEPENDENT_OBJS=$(patsubst %.cc,%.o,$(CELERA_DEPENDENT_SRCS))
CELERA_INDEPENDENT_OBJS=$(patsubst %.cc,%.o,$(CELERA_INDEPENDENT_SRCS))

.PHONY: clean all celera_assembler

all: $(EXES)

celera_assembler:
	$(MAKE) -C celera_assembler

$(EXE_OBJS): CXXFLAGS+=$(CELERA_CXXFLAGS)

write_to_ovb: $(CELERA_INDEPENDENT_OBJS) $(CELERA_DEPENDENT_OBJS) write_to_ovb.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(CELERA_LDFLAGS)

read_from_las: $(CELERA_INDEPENDENT_OBJS) read_from_las.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(CELERA_DEPENDENT_OBJS): CXXFLAGS+=$(CELERA_CXXFLAGS)
## Turn this off for now so we don't have to keep verifying that CA has been made
#$(CELERA_DEPENDENT_OBJS): celera_assembler

$(CELERA_INDEPENDENT_OBJS): $(PROTO_SRCS)

STATICLIB=libdconvert.a
$(STATICLIB): $(CELERA_DEPENDENT_OBJS) $(CELERA_INDEPENDENT_OBJS)
	ar rcs $@ $^

clean:
	rm -f $(CELERA_DEPENDENT_OBJS) $(CELERA_INDEPENDENT_OBJS) las_to_ovb
	rm -f $(PROTO_SRCS)
	rm -f $(EXE_OBJS) $(EXES)
	rm -f $(TESTS) $(STATICLIB)


TESTS_SRCS=$(wildcard tests/*_tests.cc)
TESTS=$(patsubst %.cc,%,$(TESTS_SRCS))

test: LDLIBS+=-lgtest -lgtest_main -lpthread -L. -ldconvert -lprotobuf
test: CXXFLAGS+=-I. -O0
test: $(STATICLIB) $(TESTS)
