#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../src/zmalloc.h"
#include "./test-harness-c/src/harness.h"

// num_memory_allocations * (max_memory_size + sizeof(ZRegion))
// must be less than z_memory_size for this test to always succeed
static const int max_memory_size = 400;
static const int num_memory_allocations = 100;

void test_zmalloc (TestResult* result, int seed)
{
	int ii, jj;
	int* mem[num_memory_allocations];

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		mem[ii] = (int*) zmalloc(size * sizeof(int));

		// all memory allocations should succeed
		affirm(result, size == 0 || mem[ii] != NULL, "Memory allocation failed");

		for (jj = 0; jj < size; jj += 1) {
			mem[ii][jj] = rand();
		}

	}

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		for (jj = 0; jj < size; jj += 1) {
			// check that the value was not overwritten
			affirm(result, mem[ii][jj] == rand(), "Memory checking failed");
		}
		zfree(mem[ii]);
	}

}

void test_zcalloc (TestResult* result, int seed)
{
	int ii, jj;
	int* mem[num_memory_allocations];

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		mem[ii] = (int*) zcalloc(size, sizeof(int));

		// all memory allocations should succeed
		affirm(result, size == 0 || mem[ii] != NULL, "Memory allocation failed");
	}

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		for (jj = 0; jj < size; jj += 1) {
			// check that the value is 0
			affirm(result, mem[ii][jj] == 0, "Memory checking failed");
		}
		zfree(mem[ii]);
	}
}

void test_zfree (TestResult* result, int seed)
{
	// zprintMemory();
	// printf("\n");
	int num_allocations = 0;

	int** allocations = (int**) malloc(num_memory_allocations * sizeof(int*));
	int length = num_memory_allocations;

	srand(seed);
	while (true) {

		int size = rand() % max_memory_size;
		int* mem = (int*) zmalloc(size * sizeof(int));

		if (mem == NULL) {
			break;
		}

		if (length == num_allocations) {
			length *= 2;
			allocations = (int**) realloc(allocations, length * sizeof(int*));
		}

		allocations[num_allocations] = mem;
		num_allocations += 1;
	}
	// zprintMemory();
	// printf("\n");

	int ii;
	for (ii = 0; ii < num_allocations; ii += 1) {
		zfree(allocations[ii]);
	}

	// zprintMemory();
	// printf("\n");

	// the previous block of code found the maximum number of
	// allocations before zmalloc ran out of memory
	// this block tests that freeing that memory then allocating it again works
	srand(seed);
	for (ii = 0; ii < num_allocations; ii += 1) {
		int size = rand() % max_memory_size;
		allocations[ii] = (int*) zmalloc(size * sizeof(int));
		affirm(result, allocations[ii] != NULL, "Memory allocation after free failed");
	}
	for (ii = 0; ii < num_allocations; ii += 1) {
		zfree(allocations[ii]);
	}

	free(allocations);
}

void test_zrealloc (TestResult* result, int seed)
{
	int ii, jj;
	int* mem[num_memory_allocations];
	int old_size[num_memory_allocations];
	int new_size[num_memory_allocations];

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		mem[ii] = (int*) zmalloc(size * sizeof(int));
		old_size[ii] = size;

		// all memory allocations should succeed
		affirm(result, size == 0 || mem[ii] != NULL, "Memory allocation failed");

		for (jj = 0; jj < old_size[ii]; jj += 1) {
			mem[ii][jj] = rand();
		}
	}

	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		int size = rand() % max_memory_size;
		if (mem[ii] != NULL) {
			mem[ii] = (int*) zrealloc(mem[ii], size * sizeof(int));
			new_size[ii] = size;

			// all memory allocations should succeed
			affirm(result, size == 0 || mem[ii] != NULL, "Memory reallocation failed");

			if (new_size[ii] > old_size[ii]) {
				for (jj = old_size[ii]; jj < new_size[ii]; jj += 1) {
					mem[ii][jj] = rand();
				}
			}
		}
	}

	srand(seed);
	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		rand(); // old size

		for (jj = 0; jj < old_size[ii]; jj += 1) {
			if (jj < new_size[ii]) {
				affirm(result, mem[ii][jj] == rand(), "Memory checking from allocation failed");
			} else {
				// if the new size is less than the old size we still need to call
				// rand to verify correctly, but we don't want to access the memory
				rand();
			}
		}
	}

	for (ii = 0; ii < num_memory_allocations; ii += 1) {

		rand(); // new size
		if (mem[ii] != NULL) {


			if (new_size[ii] > old_size[ii]) {
				for (jj = old_size[ii]; jj < new_size[ii]; jj += 1) {
					affirm(result, mem[ii][jj] == rand(), "Memory checking from reallocation failed");
				}
			}
		}
	}

}


int main (int argc, char** argv)
{
	TestHarness* harness = (TestHarness*) malloc(sizeof(TestHarness));
	createTestHarness(harness,
			"test memory allocator",
			(TestFunc[]) {
				{ test_zmalloc, "test zmalloc" },
				{ test_zcalloc, "test zcalloc" },
				{ test_zfree, "test zfree" },
				{ test_zrealloc, "test zrealloc" },
			},
			4);

	if (argc == 1) { // if no arguments are passed in, run the test once
		runTestHarness(harness, 1115438165, BASIC_INFO);
	} else if (argc == 2) { // if a number is passed in, run it that many times
		int num = atoi(argv[1]);
		int valid = true;
		int ii;
		for (ii = 0; ii < num; ii += 1) {
			int seed = rand();
			valid = runTestHarness(harness, seed, MINIMAL_INFO) && valid;
		}
		if (valid) {
			printf("[passed] All tests passed!\n");
		} else {
			printf("[FAILED] One or more tests have failed\n");
		}
	}
}
