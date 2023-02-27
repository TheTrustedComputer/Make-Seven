CC = gcc
MINGW = x86_64-w64-mingw32-gcc
CC_FLAGS = -Ofast -s
CC_DEBUG = -Og -g -pg -Wall -Wextra
CPU_ARCH = native
MAKE7 = MakeSeven

release:
	${CC} ${CC_FLAGS} -march=${CPU_ARCH} main.c -o ${MAKE7}

release-mingw:
	${MINGW} ${CC_FLAGS} -march=${CPU_ARCH} -static main.c -o ${MAKE7}

debug:
	${CC} ${CC_DEBUG} -march=${CPU_ARCH} main.c -o ${MAKE7}
