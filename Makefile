MAKE7 = MakeSeven
CC = gcc
MINGW = x86_64-w64-mingw32-gcc
CC_FLAGS = -Wall -Wextra -Ofast -g -std=c2x
CC_DEBUG = -Wall -Wextra -Og -g -pg -std=c2x
CPU_ARCH = native
C_FILES = main.c
LIBS = -lm -pthread

release:
	${CC} ${CC_FLAGS} -march=${CPU_ARCH} ${C_FILES} -o ${MAKE7} ${LIBS} ${EXTRA_FLAGS}

release-mingw:
	${MINGW} ${CC_FLAGS} -march=${CPU_ARCH} ${EXTRA_FLAGS} ${C_FILES} -o ${MAKE7} ${LIBS} ${EXTRA_FLAGS}

debug:
	${CC} ${CC_DEBUG} -march=${CPU_ARCH} ${EXTRA_FLAGS} ${C_FILES} -o ${MAKE7} ${LIBS} ${EXTRA_FLAGS}
