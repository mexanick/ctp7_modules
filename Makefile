TARGETS := arm \
	x86_64

TARGETS.RPM        := $(patsubst %,%.rpm,         $(TARGETS))
TARGETS.CLEAN      := $(patsubst %,%.clean,       $(TARGETS))
TARGETS.CLEANRPM   := $(patsubst %,%.cleanrpm,    $(TARGETS))
TARGETS.CLEANALLRPM:= $(patsubst %,%.cleanallrpm, $(TARGETS))
TARGETS.CLEANALL   := $(patsubst %,%.cleanall,    $(TARGETS))
TARGETS.CHECKABI   := $(patsubst %,%.checkabi,    $(TARGETS))
TARGETS.INSTALL    := $(patsubst %,%.install,     $(TARGETS))
TARGETS.UNINSTALL  := $(patsubst %,%.uninstall,   $(TARGETS))
TARGETS.RELEASE    := $(patsubst %,%.release,     $(TARGETS))

.PHONY: $(TARGETS) \
	$(TARGETS.RPM) \
	$(TARGETS.CLEAN) \
	$(TARGETS.CLEANRPM) \
	$(TARGETS.CLEANALLRPM) \
	$(TARGETS.CLEANALL) \
	$(TARGETS.CHECKABI) \
	$(TARGETS.INSTALL) \
	$(TARGETS.UNINSTALL) \
	$(TARGETS.RELEASE)

.PHONY: all default install uninstall rpm release
.PHONY: clean cleanall cleanrpm cleanallrpm

default: all

all: $(TARGETS) doc

rpm: $(TARGETS) $(TARGETS.RPM)

clean: $(TARGETS.CLEAN)

cleanall: $(TARGETS.CLEANALL) cleandoc

cleanrpm: $(TARGETS.CLEANRPM)

cleanallrpm: $(TARGETS.CLEANALLRPM)

checkabi: $(TARGETS.CHECKABI)

install: $(TARGETS.INSTALL)

uninstall: $(TARGETS.UNINSTALL)

release: $(TARGETS.RELEASE)

$(TARGETS):
	TargetArch=$@ $(MAKE) -f ctp7_modules.mk build

$(TARGETS.RPM):
	TargetArch=$(patsubst %.rpm,%,$@) $(MAKE) -f ctp7_modules.mk rpm

$(TARGETS.CLEAN):
	TargetArch=$(patsubst %.clean,%,$@) $(MAKE) -f ctp7_modules.mk clean

$(TARGETS.CLEANRPM):
	TargetArch=$(patsubst %.cleanrpm,%,$@) $(MAKE) -f ctp7_modules.mk cleanrpm

$(TARGETS.CLEANALLRPM):
	TargetArch=$(patsubst %.cleanallrpm,%,$@) $(MAKE) -f ctp7_modules.mk cleanallrpm

$(TARGETS.CLEANALL):
	TargetArch=$(patsubst %.cleanall,%,$@) $(MAKE) -f ctp7_modules.mk cleanall

$(TARGETS.CHECKABI):
	TargetArch=$(patsubst %.checkabi,%,$@) $(MAKE) -f ctp7_modules.mk checkabi

$(TARGETS.INSTALL):
	TargetArch=$(patsubst %.install,%,$@) $(MAKE) -f ctp7_modules.mk install

$(TARGETS.UNINSTALL):
	TargetArch=$(patsubst %.uninstall,%,$@) $(MAKE) -f ctp7_modules.mk uninstall

$(TARGETS.RELEASE):
	TargetArch=$(patsubst %.release,%,$@) $(MAKE) -f ctp7_modules.mk release

doc:
	@echo "TO DO"

cleandoc: 
	@echo "TO DO"

arm:

x86_64:

ctp7:

apx:
