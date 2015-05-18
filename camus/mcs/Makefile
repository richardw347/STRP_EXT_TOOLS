##
##  Makefile for Standard, Profile, Debug, 32-bit, and Static versions of camus_mcs
##

CXX ?= g++

CSRCS     = $(wildcard *.C) $(wildcard *.cpp)
CHDRS     = $(wildcard *.h)
COBJS     = $(addsuffix .o, $(basename $(CSRCS)))

PCOBJS    = $(addsuffix p,  $(COBJS))
DCOBJS    = $(addsuffix d,  $(COBJS))
R32COBJS  = $(addsuffix 32,  $(COBJS))

EXEC      = camus_mcs

CFLAGS    = -Wall -pipe -fno-strict-aliasing
COPTIMIZE = -O3 #-mtune=native


.PHONY : s p d r build clean depend

s:	WAY=standard
p:	WAY=profile
d:	WAY=debug
r32: WAY=32 bit
rs:	WAY=static

s:	CFLAGS+=$(COPTIMIZE) -D NDEBUG
p:	CFLAGS+=$(COPTIMIZE) -pg -ggdb -D DEBUG
d:	CFLAGS+=-O0 -ggdb -D DEBUG
r32: CFLAGS+=-O3 -D NDEBUG -m32
rs:	CFLAGS+=-O3 -D NDEBUG -m32

s:	build $(EXEC)
p:	build $(EXEC)_profile
d:	build $(EXEC)_debug
r32: build $(EXEC)_i386
rs:	build $(EXEC)_static_i386

build:
	@echo Building $(EXEC) "("$(WAY)")"

clean:
	@rm -f $(EXEC) $(EXEC)_profile $(EXEC)_debug $(EXEC)_i386 $(EXEC)_static_i386 \
	  $(COBJS) $(PCOBJS) $(DCOBJS) $(R32COBJS) depend.mak

## Build rule
%.o %.op %.od %.o32:	%.C
	@echo Compiling: $<
	@$(CXX) $(CFLAGS) -c -o $@ $<

%.o %.op %.od %.o32:	%.cpp
	@echo Compiling: $<
	@$(CXX) $(CFLAGS) -c -o $@ $<

## Linking rules (standard/profile/debug/release)
$(EXEC): $(COBJS)
	@echo Linking $(EXEC)
	@$(CXX) $(COBJS) -pipe -O2 -ggdb -Wall -o $@ 

$(EXEC)_profile: $(PCOBJS)
	@echo Linking $@
	@$(CXX) $(PCOBJS) -pipe -O2 -ggdb -Wall -pg -o $@

$(EXEC)_debug:	$(DCOBJS)
	@echo Linking $@
	@$(CXX) $(DCOBJS) -pipe -O2 -ggdb -Wall -o $@

$(EXEC)_i386: $(R32COBJS)
	@echo Linking $@
	@$(CXX) $(R32COBJS) -pipe -m32 -O2 -Wall -o $@

$(EXEC)_static_i386: $(R32COBJS)
	@echo Linking $@
	@$(CXX) --static $(R32COBJS) -pipe -m32 -O2 -Wall -o $@


## Make dependencies
depend:	depend.mak
depend.mak:	$(CSRCS) $(CHDRS)
	@echo Making dependencies ...
	@$(CXX) -MM $(CSRCS) > depend.mak
	@cp depend.mak /tmp/depend.mak.tmp
	@sed "s/o:/op:/" /tmp/depend.mak.tmp >> depend.mak
	@sed "s/o:/od:/" /tmp/depend.mak.tmp >> depend.mak
	@sed "s/o:/o32:/" /tmp/depend.mak.tmp >> depend.mak
	@rm /tmp/depend.mak.tmp

include depend.mak
