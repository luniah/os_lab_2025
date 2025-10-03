CC=gcc
CFLAGS=-I.

# Default target - builds all programs
all: sequential_min_max parallel_min_max exec_sequential

# Build sequential_min_max executable
sequential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

# Build parallel_min_max executable
parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Build exec_sequential executable
exec_sequential: exec_sequential.c
	$(CC) -o exec_sequential exec_sequential.c $(CFLAGS)

# Compile utils.o object file
utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Compile find_min_max.o object file
find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Clean up built files
clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max exec_sequential