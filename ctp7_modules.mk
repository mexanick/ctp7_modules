BUILD_HOME   := $(shell dirname `pwd`)
Project      := ctp7_modules
Package      := ctp7_modules
ShortPackage := ctp7_modules
LongPackage  := $(TargetArch)
PackageName  := $(ShortPackage)
PackagePath  := $(TargetArch)
PackageDir   := pkg/$(ShortPackage)
Packager     := "CMS GEM DAQ Project"
Arch         := $(TargetArch)

## For now, default behaviour is no soname
UseSONAMEs=no

ProjectPath:=$(BUILD_HOME)/$(Project)

ConfigDir:=$(ProjectPath)/config

include $(ConfigDir)/mfCommonDefs.mk

ifeq ($(Arch),x86_64)
include $(ConfigDir)/mfPythonDefs.mk
CFLAGS+=-Wall -fPIC
ADDFLAGS=-std=c++11 -std=gnu++11 -m64
else
include $(ConfigDir)/mfZynq.mk
ADDFLAGS=-std=c++14 -std=gnu++14
endif

CFLAGS+=-pthread
ADDFLAGS+=$(OPTFLAGS)

PackageSourceDir:=src
PackageIncludeDir:=include
PackageObjectDir:=$(PackagePath)/src/linux/$(Arch)
PackageLibraryDir:=$(PackagePath)/lib
PackageExecDir:=$(PackagePath)/bin
# PackageDocsDir:=$(PackagePath)/doc/_build/html

CTP7_MODULES_VER_MAJOR:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[1];}' | awk '{split($$0,b,":"); print b[2];}')
CTP7_MODULES_VER_MINOR:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[2];}' | awk '{split($$0,b,":"); print b[2];}')
CTP7_MODULES_VER_PATCH:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[3];}' | awk '{split($$0,b,":"); print b[2];}')

INSTALL_PREFIX?=/mnt/persistent/ctp7_modules

ifeq ($(and $(XHAL_ROOT),$(BUILD_HOME)),)
$(error "Unable to compile due to unset variables")
else
$(info XHAL_ROOT $(XHAL_ROOT))
$(info BUILD_HOME $(BUILD_HOME))
endif

IncludeDirs = $(PackageIncludeDir)
IncludeDirs+= $(XHAL_ROOT)/include
IncludeDirs+= /opt/wiscrpcsvc/include
IncludeDirs+= /opt/reedmuller/include
INC=$(IncludeDirs:%=-I%)

ifndef GEM_VARIANT
GEM_VARIANT = ge11
endif

CFLAGS+=-DGEM_VARIANT="$(GEM_VARIANT)"

LDFLAGS+=-Wl,--as-needed

ifeq ($(Arch),x86_64)
LibraryDirs+=$(XHAL_ROOT)/lib
LibraryDirs+=$(REEDMULLER_ROOT)/lib
LibraryDirs+=/opt/wiscrpcsvc/lib
else
## Need solution for system install as well as test install against specific new version
LibraryDirs+=$(PETA_STAGE)/mnt/persistent/xhal/lib
LibraryDirs+=$(PETA_STAGE)/mnt/persistent/reedmuller/lib
endif

LibraryDirs+=$(PackageLibraryDir)
Libraries=$(LibraryDirs:%=-L%)

Sources      := $(wildcard $(PackageSourceDir)/*.cpp) $(wildcard $(PackageSourceDir)/*/*.cpp)
TestSources  := $(wildcard $(PackageTestSourceDir)/*.cxx) $(wildcard $(PackageTestSourceDir)/*.cxx)
Dependencies := $(patsubst $(PackageSourceDir)/%.cpp, $(PackageObjectDir)/%.d, $(Sources))
TargetObjects:= $(patsubst %.d,%.o,$(Dependencies))

TargetLibraries:=memhub memory utils extras
ifeq ($(Arch),x86_64)
else
TargetLibraries+=optical
endif

include $(ConfigDir)/mfRPMRules.mk

$(PackageSpecFile): $(ProjectPath)/$(PackageName)/spec.template

.PHONY: rpc

default: build
	$(MakeDir) $(PackageDir)

## @ctp7_modules Prepare the package for building the RPM
rpmprep: build doc

# Define as dependency everything that should cause a rebuild
TarballDependencies = $(TargetLibraries) Makefile ctp7_modules.mk spec.template $(PackageIncludeDir)/packageinfo.h
ifeq ($(Arch),x86_64)
else
endif

## this needs to reproduce the compiled tree because... wrong headed
## either do the make in the spec file, or don't make up your mind!
$(PackageSourceTarball): $(TarballDependencies)
	$(MakeDir) $(PackageDir)
	@cp -rf lib $(PackageDir)
ifeq ($(Arch),x86_64)
	@echo nothing to do
else
	$(MakeDir) $(PackagePath)/$(PackageDir)/gem-peta-stage/ctp7/$(INSTALL_PATH)/lib
	@cp -rfp $(PackageLibraryDir)/* $(PackagePath)/$(PackageDir)/gem-peta-stage/ctp7/$(INSTALL_PATH)/lib
endif
	$(MakeDir) $(RPM_DIR)
	@cp -rfp spec.template $(PackagePath)
	$(MakeDir) $(PackagePath)/$(PackageDir)/$(PackageName)/$(LongPackage)
	@cp -rfp --parents $(PackageObjectDir) $(PackagePath)/$(PackageDir)/$(PackageName)
	@cp -rfp --parents $(PackageLibraryDir) $(PackagePath)/$(PackageDir)/$(PackageName)
	-cp -rfp --parents $(PackageExecDir) $(PackagePath)/$(PackageDir)/$(PackageName)
	@cp -rfp $(PackageSourceDir) $(PackagePath)/$(PackageDir)/$(PackageName)
	@cp -rfp $(PackageIncludeDir) $(PackagePath)/$(PackageDir)/$(PackageName)
	@cp -rfp ctp7_modules.mk $(PackagePath)/$(PackageDir)/$(PackageName)/Makefile
	@cp -rfp $(ProjectPath)/config $(PackagePath)/$(PackageDir)
	cd $(PackagePath)/$(PackageDir)/..; \
	    tar cjf $(PackageSourceTarball) . ;
#	$(RM) $(PackagePath)/$(PackageDir)

# Everything links against these libraries
BASE_LINKS = -lxhal-base -llmdb -lwisci2c -llog4cplus

# Generic shared object creation rule, need to accomodate cases where we have lib.o lib/sub.o
pc:=%
.SECONDEXPANSION:
$(PackageLibraryDir)/%.so: $$(filter $(PackageObjectDir)/$$*$$(pc).o, $(TargetObjects))
	$(MakeDir) $(@D)
	$(CXX) $(CFLAGS) $(ADDFLAGS) $(LDFLAGS) $(Libraries) -shared -Wl,-soname,$(*F).so -o $@ $^ $(EXTRA_LINKS) $(BASE_LINKS)

## adapted from http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
## Generic object creation rule, generate dependencies and use them later
$(PackageObjectDir)/%.o: $(PackageSourceDir)/%.cpp
	$(MakeDir) $(@D)
	$(CXX) $(CFLAGS) $(ADDFLAGS) -c $(INC) -MT $@ -MMD -MP -MF $(@D)/$(*F).Td -o $@ $<
	mv $(@D)/$(*F).Td $(@D)/$(*F).d
# this was to prevent an older object than dependency file (for some versions of gcc)
	touch $@

# dummy rule for dependencies
$(PackageObjectDir)/%.d:

# mark dependencies and objects as not auto-removed
.PRECIOUS: $(PackageObjectDir)/%.d
.PRECIOUS: $(PackageObjectDir)/%.o

# Force rule for all target library names
$(TargetLibraries):

# Define the target library dependencies
memhub:
	$(eval export EXTRA_LINKS=-lmemsvc)
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/memhub.so EXTRA_LINKS="$(EXTRA_LINKS)"

memory: memhub
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/memory.so EXTRA_LINKS="$(EXTRA_LINKS)"

optical: memhub
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/optical.so EXTRA_LINKS="$(EXTRA_LINKS)"

utils: memhub
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/utils.so EXTRA_LINKS="$(EXTRA_LINKS)"

extras: memhub utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/extras.so EXTRA_LINKS="$(EXTRA_LINKS)"

amc: extras utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/amc.so EXTRA_LINKS="$(EXTRA_LINKS)"

daq_monitor: amc extras utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/daq_monitor.so EXTRA_LINKS="$(EXTRA_LINKS)"

vfat3: optohybrid amc extras utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so) -lreedmuller)
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/vfat3.so EXTRA_LINKS="$(EXTRA_LINKS)"

optohybrid: amc extras utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/optohybrid.so EXTRA_LINKS="$(EXTRA_LINKS)"

calibration_routines: optohybrid vfat3 amc extras utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/calibration_routines.so EXTRA_LINKS="$(EXTRA_LINKS)"

gbt: utils
	$(eval export EXTRA_LINKS=$(^:%=-l:%.so))
	TargetArch=$(TargetArch) $(MAKE) -f ctp7_modules.mk $(PackageLibraryDir)/gbt.so EXTRA_LINKS="$(EXTRA_LINKS)"

build: $(TargetLibraries)
	@echo Executing build stage

_all: build
	@echo Executing _all stage

### local (PC) test functions, need standard gcc toolchain, dirs, and flags
.PHONY: test
# test: test/tester.cpp
TestExecs := $(patsubst $(PackageTestSourceDir)/%.cxx, $(PackageExecDir)/%, $(TestSources))
$(TestExecs):

$(PackageExecDir)/%: $(PackageTestSourceDir)/%.cxx
	$(MakeDir) $(@D)
	g++ -O0 -g3 -fno-inline -std=c++11 -c $(INC) -MT $@ -MMD -MP -MF $(@D)/$(*F).Td -o $@ $<
	mv $(@D)/$(*F).Td $(@D)/$(*F).d
	touch $@
	g++ -O0 -g3 -fno-inline -std=c++11 -o $@ $< $(INC) $(LDFLAGS) -L/opt/wiscrpcsvc/lib -lwiscrpcsvc

test: $(TestExecs)

.PHONY: cleanall
cleanall: clean cleanrpm
	-rm -rf $(Dependencies)
	-rm -rf $(PackageDir)
	-rm -rf $(PackageObjectDir)

clean:
	@echo Cleaning up all generated files
	-rm -rf $(TargetObjects)
	-rm -rf $(PackageLibraryDir)

cleandoc:
	@echo "TO DO"

-include $(Dependencies)
