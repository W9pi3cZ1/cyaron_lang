ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[Build]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES := $(wildcard ./src/*.c ) $(wildcard ./src/**/*.c )
HEADER_SOURCES := $(wildcard ./include/*.h) $(wildcard ./include/**/*.h)

OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS           := $(OBJS:%.o=%.d)
BUILD_DIR      := ./build

CC := clang
ifneq ($(DEBUG),)
C_CONFIG += -DNO_DEBUG
endif
ifneq ($(CLOCK),)
C_CONFIG += -DNO_CLOCK
endif
ifneq ($(SANITIZER),)
C_CONFIG += -fsanitize=leak
endif
ifneq ($(CODEGEN),)
C_CONFIG += -DCODEGEN
endif
C_FLAGS := -Iinclude -MMD -O2 -g3 $(C_CONFIG)
LD := $(CC)
LD_FLAGS := $(C_FLAGS) -fuse-linker-plugin -fuse-ld=lld

.PHONY: all build clean build_once gen_min_impl run run_once

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

all: build

info:
	$(Q)printf "DEBUG=$(DEBUG) CLOCK=$(CLOCK) C_FLAGS=$(C_FLAGS)"

gen_min_impl:
	$(Q)sed -n '/#ifndef NO_STD_INC/{:a;n;/#endif/!{p;ba};}' $(C_SOURCES) $(HEADER_SOURCES) | awk '!seen[$$0]++'
	$(Q)$(CC) -E -P -DNO_STD_INC -DNO_CUSTOM_INC -DNO_DEBUG -DNO_CLOCK $(C_FLAGS) $(HEADER_SOURCES)
	$(Q)$(CC) -E -P -DNO_STD_INC -DNO_CUSTOM_INC -DNO_DEBUG -DNO_CLOCK $(C_FLAGS) $(C_SOURCES)

$(BUILD_DIR)/cyaron_once: 
	$(V)$(CC) $(C_FLAGS) $(LD_FLAGS) -o $@ $(C_SOURCES)

build_once:  $(BUILD_DIR)/cyaron_once
	$(Q)printf "\033[1;32m[Done]\033[0m Build(once) completed.\n\n"

$(BUILD_DIR)/cyaron: $(OBJS)
	$(V)$(LD) -o $@ $(OBJS) $(LD_FLAGS)

build: $(BUILD_DIR)/cyaron
	$(Q)printf "\033[1;32m[Done]\033[0m Build completed.\n\n"

run: build
	$(Q)$(BUILD_DIR)/cyaron

run_once: build_once
	$(Q)$(BUILD_DIR)/cyaron_once

clean:
	$(Q)$(RM) $(OBJS) $(DEPS) $(wildcard $(BUILD_DIR)/*)
	$(Q)printf "\033[1;32m[Done]\033[0m Clean completed.\n\n"
