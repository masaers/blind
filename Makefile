# ========================================================================
# Makefile
#
# Authors: Markus Saers <masaers@gmail.com>
# ========================================================================
#
# Settings
#

INCLUDES += 
LIBRARIES +=
CXXFLAGS += -Wall -pedantic -std=c++14 -g -O3 $(INCLUDES)
CFLAGS += $(INCLUDES)
LDFLAGS += 

PROG_NAMES = 
TEST_NAMES = blind_test

#
# Derived settings
#

# List of binaries that needs to be built
BIN_NAMES := $(PROG_NAMES) $(TEST_NAMES)

# Object files are c++ sources that do not result in stand alone binaries
OBJECTS := \
	$(patsubst %.cpp,build/obj/%.o,$(filter-out $(BIN_NAMES:%=%.cpp),$(wildcard *.cpp))) \
	$(patsubst %.c,build/obj/%.o,$(filter-out $(BIN_NAMES:%=%.c),$(wildcard *.c))) \

#
# Targets
#

# Clear default suffix rules
.SUFFIXES :
# Keep STAMPs and dependencies between calls
.PRECIOUS : %/.STAMP build/dep/%.d build/obj/%.o

all : binaries

binaries : $(BIN_NAMES:%=build/bin/%)

test : $(TEST_NAMES:%=build/test/%.out)
	@if [ -s build/test/.ERROR ]; then \
	( cat build/test/.ERROR; rm build/test/.ERROR ) \
	else echo "\n[ALL TESTS PASSED]\n"; \
	fi

build/bin/% : build/obj/%.o $(OBJECTS) build/bin/.STAMP
	$(CXX) $< $(OBJECTS) $(LDFLAGS) $(LIBRARIES) -o $@ 

build/obj/%.o : %.cpp build/obj/.STAMP build/dep/.STAMP
	@$(CXX) $(CXXFLAGS) -MM -MT '$@' $< > $(@:build/obj/%.o=build/dep/%.d)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/obj/%.o : %.c build/obj/.STAMP build/dep/.STAMP
	@$(CC) $(CFLAGS) -MM -MT '$@' $< > $(@:build/obj/%.o=build/dep/%.d)
	$(CC) $(CFLAGS) -c $< -o $@

build/test/%.out : build/bin/% build/test/.STAMP
	@echo "[TESTING: $<]"
	@if [ -e $@ ]; then \
	( cp $@ $@.old; \
          $< &> $@; \
	  diff $@ $@.old >> build/test/.ERROR \
	  || echo "REGRESSION TEST FAILED: $<" >> build/test/.ERROR \
	  ; \
	  rm $@.old ) \
	else \
	( $< &> $@; \
          echo "WARNING: No regression test: $<" >> build/test/.ERROR ) \
	fi

%/.STAMP :
	@mkdir -pv $(@D)
	@touch $@

clean :
	@rm -rf build/dep build/obj build/bin
	@rm -f *~

cleaner : clean
	@rm -rf build/*

cleanest : cleaner
	@rm -rf build

echo-% : ; @echo $($*)
print-% : ; @echo "$*='$($*)'"

-include $(wildcard build/dep/*.d)

