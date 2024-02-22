# Libraries
LIBS=utils commons readline

# Custom libraries' paths
SHARED_LIBPATHS=
STATIC_LIBPATHS=../utils

# Compiler flags
CDEBUG=-g -Wall -DDEBUG
CRELEASE=-O3 -Wall -DNDEBUG

# Arguments when executing with start, memcheck or helgrind
ARGS=cfg/kernel.config

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes
HELGRIND_FLAGS=

# Source files (*.c) to be excluded from tests compilation
TEST_EXCLUDE=src/kernel.c
