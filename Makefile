MAKE7 = MakeSeven
CC = gcc
MINGW = x86_64-w64-mingw32-gcc
CC_FLAGS = -Wall -Wextra -Ofast -s -lm -std=c2x
CC_DEBUG = -Wall -Wextra -Og -g -pg -lm -std=c2x
CPU_ARCH = native
C_FILES = main.c

release:
	${CC} ${CC_FLAGS} -march=${CPU_ARCH} ${EXTRA_FLAGS} ${C_FILES} -o ${MAKE7}

release-mingw:
	${MINGW} ${CC_FLAGS} -march=${CPU_ARCH} ${EXTRA_FLAGS} ${C_FILES} -o ${MAKE7}

debug:
	${CC} ${CC_DEBUG} -march=${CPU_ARCH} ${EXTRA_FLAGS} ${C_FILES} -o ${MAKE7}
