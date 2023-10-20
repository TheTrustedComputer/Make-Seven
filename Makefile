MAKE7 = Make7Solver
CC = gcc
MINGW = x86_64-w64-mingw32-gcc
CC_FLAGS = -Wall -Wextra -Ofast -s -std=c2x
CC_DEBUG = -Wall -Wextra -Og -g -pg -std=c2x
CPU_ARCH = native
C_FILES = main.c
LIBS = -lm
EXTRA_FLAGS = # Place any additional flags here

release:
	${CC} ${CC_FLAGS} -march=${CPU_ARCH} ${C_FILES} -o ${MAKE7} ${LIBS} ${EXTRA_FLAGS}

release-mingw:
	${MINGW} ${CC_FLAGS} -march=${CPU_ARCH} mingw_threads.c ${C_FILES} -o ${MAKE7} ${LIBS} -pthread -static ${EXTRA_FLAGS}

debug:
	${CC} ${CC_DEBUG} -march=${CPU_ARCH} ${C_FILES} -o ${MAKE7} ${LIBS} ${EXTRA_FLAGS}

debug-mingw:
	${MINGW} ${CC_DEBUG} -march=${CPU_ARCH} mingw_threads.c ${C_FILES} -o ${MAKE7} ${LIBS} -pthread -static ${EXTRA_FLAGS}
