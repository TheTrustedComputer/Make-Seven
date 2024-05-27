## NOTE: Requires GCC 14+ and Clang 18+ to compile with the C23 standard.

MAKE7                = Make7Solver
CPU_ARCH             = native
CC                   = gcc
MINGW                = x86_64-w64-mingw32-gcc
CC_FLAGS             = -Wall -Wextra -Ofast -s -std=c23 -funroll-loops -fmerge-all-constants
CC_DEBUG             = -Wall -Wextra -Og -g -pg -std=c23
CC_LTO               = -flto # May cause a segmentation fault when static linking; comment to disable
C_FILES              = main.c
LIBS                 = -lm
CC_RELEASE_TARGET    = -march=$(CPU_ARCH) $(C_FILES) -o $(MAKE7) $(LIBS) $(EXTRA_FLAGS)
CC_DEBUG_TARGET      = -march=$(CPU_ARCH) $(C_FILES) -o $(MAKE7) $(LIBS) $(EXTRA_FLAGS)
MINGW_RELEASE_TARGET = -march=$(CPU_ARCH) mingw_threads.c $(C_FILES) -o $(MAKE7) $(LIBS) -pthread -static $(EXTRA_FLAGS)
MINGW_DEBUG_TARGET   = -march=$(CPU_ARCH) mingw_threads.c $(C_FILES) -o $(MAKE7) $(LIBS) -pthread -static $(EXTRA_FLAGS)
EXTRA_FLAGS          = # Place any additional compiler flags here

release:
	$(CC) $(CC_FLAGS) $(CC_RELEASE_TARGET) $(CC_LTO)

release-mingw:
	$(MINGW) $(CC_FLAGS) $(MINGW_RELEASE_TARGET) $(CC_LTO)

release-pgo: clean
ifeq ($(CC), gcc)
	$(CC) $(CC_FLAGS) $(CC_RELEASE_TARGET) $(CC_LTO) -fprofile-generate
else ifeq ($(CC), clang)
	$(CC) $(CC_FLAGS) $(CC_RELEASE_TARGET) $(CC_LTO) -fprofile-instr-generate
endif
	./Make7Solver -g -t 1
ifeq ($(CC), gcc)
	$(CC) $(CC_FLAGS) $(CC_RELEASE_TARGET) $(CC_LTO) -fprofile-use
else ifeq ($(CC), clang)
	llvm-profdata merge -sparse default.profraw -o Make7Solver.profdata
	$(CC) $(CC_FLAGS) $(CC_RELEASE_TARGET) $(CC_LTO) -fprofile-instr-use=Make7Solver.profdata
endif

release-mingw-pgo: clean
	$(MINGW) $(CC_FLAGS) $(MINGW_RELEASE_TARGET) $(CC_LTO) -fprofile-generate
	./Make7Solver.exe -g -t 1
	$(MINGW) $(CC_FLAGS) $(MINGW_RELEASE_TARGET) $(CC_LTO) -fprofile-use

debug:
	$(CC) $(CC_DEBUG) $(CC_DEBUG_TARGET) $(CC_LTO)

debug-mingw:
	$(MINGW) $(CC_DEBUG) $(MINGW_DEBUG_TARGET) $(CC_LTO)

clean:
	rm -f *.gcda *.profraw *.profdata
