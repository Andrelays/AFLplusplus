ifeq ($(origin CXX),default)
  CXX = gcc
endif

CFLAGS  ?= -shared -Wall -O3
MUTATOR_DIR:= $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
AFL_DIR:= $(shell dirname $(shell dirname $(MUTATOR_DIR)))
COMMON_DIR:= $(shell dirname $(AFL_DIR))
COMMONINC = -I../../include

override CFLAGS += $(COMMONINC)

# reproducing source tree in object tree
COBJ := $(addprefix $(OUT_O_DIR)/,$(CSRC:.cpp=.o))

.PHONY: all
all: mutate_png.so test


# release:
# 	echo $(ROOT_DIR)

mutate_png.so: mutate_png.c
	$(CXX) $(CFLAGS) $^ -o $@ -lz

.PHONY: test
test: $(AFL_DIR)/afl-fuzz $(COMMON_DIR)/fpng/bin/fpng_test run_test

$(AFL_DIR)/afl-fuzz:
	cd $(AFL_DIR) && make all

$(COMMON_DIR)/fpng/bin/fpng_test:
	cd $(COMMON_DIR) && git clone https://github.com/richgel999/fpng.git
	mkdir $(COMMON_DIR)/fpng/build
	cd $(COMMON_DIR)/fpng/build && cmake -DCMAKE_C_COMPILER=$(AFL_DIR)/afl-clang-fast -DCMAKE_CXX_COMPILER=$(AFL_DIR)/afl-clang-fast++ -DSSE=1 ..
	cd $(COMMON_DIR)/fpng/build && make

.PHONY: run_test
run_test: run_custom_and_default run_custom run_default

.PHONY: run_custom_and_default
run_custom_and_default:
	unset AFL_CUSTOM_MUTATOR_ONLY AFL_CUSTOM_MUTATOR_LIBRARY
	gnome-terminal --tab --title="custom_and_default" -- bash -c 'AFL_CUSTOM_MUTATOR_LIBRARY="$(MUTATOR_DIR)/mutate_png.so" $(AFL_DIR)/afl-fuzz -i $(MUTATOR_DIR)/input_testcases/ -o $(COMMON_DIR)/output_custom_and_default -- $(COMMON_DIR)/fpng/bin/fpng_test @@'

.PHONY: run_custom
run_custom:
	unset AFL_CUSTOM_MUTATOR_ONLY AFL_CUSTOM_MUTATOR_LIBRARY
	gnome-terminal --tab --title="default_only" -- bash -c 'AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY="$(MUTATOR_DIR)/mutate_png.so" $(AFL_DIR)/afl-fuzz -i $(MUTATOR_DIR)/input_testcases/ -o $(COMMON_DIR)/output_default_only -- $(COMMON_DIR)/fpng/bin/fpng_test @@'

.PHONY: run_default
run_default:
	unset AFL_CUSTOM_MUTATOR_ONLY AFL_CUSTOM_MUTATOR_LIBRARY
	gnome-terminal --tab --title="custom_only" -- bash -c 'AFL_CUSTOM_MUTATOR_LIBRARY="$(MUTATOR_DIR)/mutate_png.so" $(AFL_DIR)/afl-fuzz -i $(MUTATOR_DIR)/input_testcases/ -o $(COMMON_DIR)/output_custom_only -- $(COMMON_DIR)/fpng/bin/fpng_test @@'

.PHONY: clean
clean:
	rm -rf mutate_png.so $(COMMON_DIR)/fpng $(COMMON_DIR)/output_default_only $(COMMON_DIR)/output_custom_only $(COMMON_DIR)/output_custom_and_default
	cd $(AFL_DIR) && make clean
