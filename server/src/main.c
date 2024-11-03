/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Lukas Grützmacher <lg2@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <dlfcn.h>

void swap(long * a, long * b);
void print_values(long * values, size_t length, size_t start, size_t stop, long current);
void _qsort(long * values, size_t start, size_t stop);
void qsort(long * values, size_t length);
void random_values(long * values, size_t length);
bool is_sorted(long * values, size_t length);
long fib2(long n);
long fib1(long n);

#define FIB_INPUT		(1l << 30)
#define FIB_REPS		(1l <<  3)
#define VALUES_LENGTH	(1l << 14)
#define QSORT_REPS		(1l <<  5)
long VALUES[VALUES_LENGTH];

void swap(long * a, long * b) {
	long temp = *a;
	*a = *b;
	*b = temp;
}

#if DEBUG
void print_values(long * values, size_t length, size_t start, size_t stop, long current) {
	for (size_t i = 0; i < length; i++) {
		if (start <= i && i < stop) {
			printf("%3ld ", values[i]);
		} else {
			printf("    ");
		}
	}
	printf("curr = %3ld\n", current);
}
#else
void print_values(long *, size_t, size_t, size_t, long) {}
#endif

void _qsort(long * values, size_t start, size_t stop) {
	if (stop - start <= 1)
		return;
	if (stop - start == 2) {
		if (values[start] <= values[start+1])
			return;
		swap(&values[start], &values[start + 1]);
		return;
	}

	long pivot = values[start];
	long current = values[start + 1];
	print_values(values, VALUES_LENGTH, start, stop, current);

	// where to put to-be-sorted elements
	size_t front = start;		// when they're <= pivot, they go to a backwards-moving front end
	size_t back  = stop  - 1;	// when they're >  pivot, they go to a  forwards-moving back end

	/** memory layout of values from start to stop at this point in the program
	 * start
	 * |     front reads from here (front + 2)
	 * |     |        back
	 * |     |        |
	 * v  a  l  u  e  s
	 * |  |           |
	 * |  |           stop
	 * |  original was moved into current first
	 * |  thus writable
	 * |
	 * original stored in pivot
	 * thus, writable as front
	 */
	while (back - front > 1) {
		print_values(values, VALUES_LENGTH, start, stop, current);
		if (current > pivot) {
			swap(&current, &values[back]);
			back--;
		} else {
			values[front] = current;
			current = values[front + 2];
			front++;
		}
	}

	/** memory layout of values from start to stop at this point in the program
	 *
	 *       back points here
	 *       thus writable
	 *          |
	 * v  a  l  u  e  s
	 * |     |
	 * |     |
	 * |  front points here
	 * |  thus writable
	 * |
	 * original stored in pivot
	 * must be placed at front or front + 1
	 */

	if (current > pivot) {
		values[front + 1] = current;
	} else {
		values[front] = current;
		// make sure front points to the value that's already sorted
		front++;
	}
	values[front] = pivot;

	print_values(values, VALUES_LENGTH, start, stop, current);
	_qsort(values, start, front);
	_qsort(values, front + 1, stop);
	print_values(values, VALUES_LENGTH, start, stop, current);
}

void qsort(long * values, size_t length) {
	_qsort(values, 0, length);
}

void random_values(long* values, size_t length) {
	static long current = 0x283745034;
	for (size_t i = 0; i < length; i++) {
		current = current >> 8;
		current = current * current;
		current = current >> 8;
		values[i] = current & 0xff;
	}
}
bool is_sorted(long* values, size_t length) {
	for (size_t i = 1; i < length; i++) {
		if (values[i + 1] < values[i])
			return false;
	}
	return true;
}

long fib2(long n) {
	long a = 0;
	long b = 1;

	for (long i = 0; i < n; i++) {
		long d = a + b;
		a = b;
		b = d;
	}
	return a;
}

long fib1(long n) {
	return fib2(n);
}

int
main(void) {
	char * error;

	dlerror(); // clear error
	printf("started\n");
	void * main_program_handle = dlopen(NULL, RTLD_LAZY);
	if (error = dlerror()) { printf("error (main): %s\n", error); } else {
		printf("main program handle: %p\n", main_program_handle);
		void * dl_printf = dlsym(main_program_handle, "printf");
		if (error = dlerror()) { printf("error (printf): %s\n", error); } else {
			printf("printf is at %p, dl finds it at %p\n", &printf, dl_printf);
		}
	}

	dlerror();
	const char * lib_name = "libdl.so";
	const char * function_name = "dlerror";
	void * handle = dlopen(lib_name, RTLD_LAZY);
	if (error = dlerror()) { printf("error (%s): %s\n", lib_name, error); } else {
		printf("handle: %p\n", handle);
		void * dl_function = dlsym(handle, function_name);
		if (error = dlerror()) { printf("error (%s): %s\n", function_name, error); } else {
			printf("%s is at ?, dl finds it at %p\n", function_name, dl_function);
		}
	}
	for (int i = 0;; i++) {

		#if 0
		// some testing around with user space mapping,
		// Insert_ok does not yet yield a readable user side experience :/
		if (i > 1) {
			const long unsigned * const BTB = 0x0000000ffee0000;
			const long unsigned * btb = BTB;
			for (int b = 0; b < 20; b++) {
				printf("btb @%p: %16lx\n", btb, *(btb++));
			}
		}
		#endif

		bool sorted;
		for (int i = 0; i < QSORT_REPS; i++) {
			random_values		(&(VALUES[0]), VALUES_LENGTH);
			qsort				(&(VALUES[0]), VALUES_LENGTH);
			sorted = is_sorted	(&(VALUES[0]), VALUES_LENGTH);
		}
 		if (sorted) {
			printf(
				">>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
				">>>>>>>>>>>> successfully sorted a few values <<<<<<<<<<<<<<\n"
				">>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
			);
		} else {
			printf(
				">>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
				">>>>>>>>> shamefully failed sorting a few values <<<<<<<<<<<\n"
				">>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
			);
		}

		long f;
		for (int i = 0; i < FIB_REPS; i++) {
			f = fib1(FIB_INPUT);
		}
		printf(
			">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
			">>>>>>>>>>>> fib(%11ld) = %20ld! <<<<<<<<<<<<<<\n"
			">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",
			FIB_INPUT, f
		);
	}
}
