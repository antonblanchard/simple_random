#include <stdbool.h>
#include <string.h>

#if __STDC_HOSTED__ == 1
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "generate.h"
#include "backend.h"
#include "jenkins.h"
#include "microrl.h"
#include "lfsr.h"
#include "mystdio.h"

#define MAX_INSNS	8192
#define SLACK		512

static void microrl_print(microrl_t *pThis, const char *str)
{
	print(str);
}

static bool registers;
static bool insns;
static enum { JENKINS, XOR } hash_type = JENKINS;
static void *insns_ptr;
static void *mem_ptr;

static const char *extra_names[6] = { "CR", "LR", "CTR", "XER", "FPSCR", "VSCR" };

static long run_one_test(unsigned long seed, unsigned long nr_insns,
			 unsigned long *lenp)
{
	unsigned long gprs[NGPRS];
	long tb_diff;

	if (nr_insns > MAX_INSNS) {
		print("Increase MAX_INSNS\r\n");
		return 0;
	}

	generate_testcase(insns_ptr, mem_ptr+MEM_SIZE/2, gprs, seed, nr_insns,
			  insns, false, lenp);
	tb_diff = execute_testcase(insns_ptr, gprs, mem_ptr);

	/* GPR 31 was our scratch space, clear it */
	gprs[31] = 0;

	if (registers) {
		for (unsigned long i = 0; i < NGPRS; i++) {
			if (i < 38) {
				if (i < 32)
					putlong(i);
				else
					print(extra_names[i-32]);
				print(" ");
				if (i != 31)
					puthex(gprs[i]);
			} else {
				if (i < 102) {
					print("F");
					putlong((i - 38) >> 1);
				} else {
					print("V");
					putlong((i - 102) >> 1);
				}
				print(" ");
				puthex(gprs[i+1]);
				print(" ");
				puthex(gprs[i]);
				++i;
			}
			print("\r\n");
		}
		print("Memory @ ");
		puthex((unsigned long)mem_ptr);
		for (unsigned long i = 0; i < MEM_SIZE; i += sizeof(unsigned long)) {
			if (i % 32 == 0)
				print("\r\n");
			else
				print(" ");
			puthex(*(unsigned long *)(mem_ptr + i));
		}

		print("\r\n\r\n");
	} else {
		uint64_t hash = 0;

		if (hash_type == XOR) {
			for (unsigned long i = 0; i < NGPRS; i++)
				hash ^= gprs[i];
		} else {
			hash = jhash2((uint32_t *)gprs,
				      sizeof(gprs)/sizeof(uint32_t), 0);
		}

		putlong(seed);
		print(" ");
		puthex(hash);
		print("\r\n");
	}

	return tb_diff;
}

static void run_many_tests(unsigned long seed, unsigned long nr_insns,
			   unsigned long nr_tests)
{
	long tb_ticks = 0;
	unsigned long total_bytes = 0;

	for (unsigned long i = 0; i < nr_tests; i++) {
		tb_ticks += run_one_test(seed, nr_insns, &total_bytes);
		seed++;
	}
	print("timebase delta = ");
	putlong(tb_ticks);
	print(", # instructions = ");
	putlong(total_bytes / 4);
	print("\r\n");
}

#if __STDC_HOSTED__ == 1
static uint32_t create_branch(long offset)
{
        uint32_t instruction;

        instruction = 0x48000000 | (offset & 0x03FFFFFC);

        return instruction;
}

static void non_interactive(char *filename, unsigned long seed,
			    unsigned long nr_insns)
{
	void *end;
	char name[PATH_MAX];
	int fd;
	unsigned long gprs[NGPRS];

	if (nr_insns > MAX_INSNS) {
		print("Increase MAX_INSNS\r\n");
		return;
	}

	end = generate_testcase(insns_ptr, mem_ptr+MEM_SIZE/2, gprs,seed, nr_insns, insns, true, NULL);

	sprintf(name, "%s.bin", filename);

	fd = open(name, O_CREAT|O_TRUNC|O_RDWR, 0666);
	assert(ftruncate(fd, MEMPAGE_BASE+MEMPAGE_SIZE) == 0);

	lseek(fd, 0, SEEK_SET);
	uint32_t branch_insn = create_branch(INSNS_BASE);
	assert(write(fd, &branch_insn, sizeof(branch_insn)) == sizeof(branch_insn));

	lseek(fd, 0x100, SEEK_SET);
	branch_insn = create_branch(INSNS_BASE-0x100);
	assert(write(fd, &branch_insn, sizeof(branch_insn)) == sizeof(branch_insn));

	lseek(fd, MEMPAGE_BASE, SEEK_SET);
	assert(write(fd, insns_ptr, end-insns_ptr) == end-insns_ptr);

	close(fd);

	generate_testcase(insns_ptr, mem_ptr+MEM_SIZE/2, gprs, seed, nr_insns,
			  insns, false, NULL);
	execute_testcase(insns_ptr, gprs, mem_ptr);

	/* GPR 31 was our scratch space, clear it */
	gprs[31] = 0;

	sprintf(name, "%s.out", filename);

	FILE *f = fopen(name, "w");

	for (unsigned long i = 0; i < NGPRS; i++) {
		if (i < 32)
			fprintf(f, "GPR%lu", i);
		else
			fprintf(f, "%s", extra_names[i-32]);
		fprintf(f, " ");
		if (i != 31)
			fprintf(f, "%016lX", (gprs[i]));
		fprintf(f, "\n");
	}

	fprintf(f, "\n");
}
#endif

static microrl_t rl;
static microrl_t *prl = &rl;

static void sigint(microrl_t *pThis)
{
}

#define _CMD_HELP		"help"
#define _CMD_VER		"version"
#define _CMD_SET		"set"
#define   _CMD_SET_REGISTERS	"registers"
#define   _CMD_SET_INSNS	"insns"
#define   _CMD_SET_CHECKSUM	"checksum"
#define _CMD_SHOW		"show"
#define _CMD_TEST		"test"
#define _CMD_TEST_MANY		"test_many"
#define _CMD_ENABLE		"enable"
#define _CMD_DISABLE		"disable"
#define _CMD_READ		"read"
#define _CMD_MEMTEST		"memtest"
#define _CMD_QUIT		"quit"

#define _NUM_OF_VER_SCMD 2

static char *cmds[] = { _CMD_HELP, _CMD_VER, _CMD_SET, _CMD_SHOW, _CMD_TEST,
		    _CMD_TEST_MANY, _CMD_ENABLE, _CMD_DISABLE, _CMD_READ,
		    _CMD_MEMTEST };

#define NUM_CMDS (sizeof(cmds) / sizeof(cmds[0]))

#ifdef _USE_COMPLETE
static char *compl_world[NUM_CMDS + 1];

static char **microrl_complete(microrl_t *pThis, int argc, const char* const* argv)
{
	unsigned long j = 0;

	compl_world[0] = NULL;

	if (argc == 1) {
		const char *bit = argv[0];

		/* iterate through our available token and match it */
		for (unsigned long i = 0; i < NUM_CMDS; i++) {
			if (strstr(cmds[i], bit) == cmds[i]) {
				/* add it to completion set */
				compl_world[j++] = cmds[i];
			}
		}
	}

	/* NULL terminate */
	compl_world[j] = NULL;

	return compl_world;
}
#endif

void usage(void)
{
	print("Help:\r\n");
	print("\t\thelp\r\n");
	print("\t\tversion\r\n");
	print("\t\tset  [variable] [value]\r\n");
	print("\t\tshow [variable] [value]\r\n");
	print("\t\ttest [seed] [nr_insns]\r\n");
	print("\t\ttest_many [first_seed] [nr_insns] [nr_tests]\r\n");
	print("\t\tenable [insn]\r\n");
	print("\t\tdisable [insn]\r\n");
	print("\t\tmemtest [start_addr] [end_addr]\r\n");
#if __STDC_HOSTED__ == 1
	print("\t\tquit\r\n");
#endif
}

static void set_variable(const char *var, const char *val)
{
	if (!strcmp(var, _CMD_SET_REGISTERS)) {
		if (!strcmp(val, "0"))
			registers = false;
		else if (!strcmp(val, "1"))
			registers = true;
	} else if (!strcmp(var, _CMD_SET_INSNS)) {
		if (!strcmp(val, "0"))
			insns = false;
		else if (!strcmp(val, "1"))
			insns = true;
	} else if (!strcmp(var, _CMD_SET_CHECKSUM)) {
		if (!strcmp(val, "xor"))
			hash_type = XOR;
		else if (!strcmp(val, "jenkins"))
			hash_type = JENKINS;
		else
			usage();
	}
}

static void show_variable(const char *var)
{
	if (!strcmp(var, _CMD_SET_REGISTERS)) {
		print("registers ");

		if (registers == true)
			print("1\r\n");
		else
			print("0\r\n");
	} else if (!strcmp(var, _CMD_SET_INSNS)) {
		print("insns ");

		if (insns == true)
			print("1\r\n");
		else
			print("0\r\n");
	} else if (!strcmp(var, _CMD_SET_CHECKSUM)) {
		print("checksum ");
		if (hash_type == XOR)
			print("xor\r\n");
		else
			print("jenkins\r\n");
	}
}

static uint8_t __atoi_one(uint8_t x)
{
	uint8_t v;

	if (x >= 'a')
		v = x - 'a' + 10;
	else if (x >= 'A')
		v = x - 'A' + 10;
	else
		v = x - '0';

	return v;
}

static unsigned long __atoi(const char *str, uint8_t base)
{
	unsigned long ret = 0;

	for (unsigned long i = 0; str[i] != 0x0; i++)
		ret = ret * base + __atoi_one(str[i]);

	return ret;
}

static void read_data(const char *addr)
{
	unsigned long x = __atoi(addr, 16);

	puthex(*(unsigned long *)x);
	print("\r\n");
}

static void memtest(const char *start, const char *end)
{
	unsigned long s = __atoi(start, 16);
	unsigned long e = __atoi(end, 16);
	unsigned long lfsr = 1;

	print("Writing\r\n");
	for (unsigned long i = s; i < e; i += sizeof(unsigned long)) {
		unsigned long val;

		lfsr = mylfsr(32, lfsr);
		val = lfsr;
		lfsr = mylfsr(32, lfsr);
		val |= lfsr << 32;

		*(unsigned long *)i = val;
	}

	print("Checking\r\n");
	lfsr = 1;
	for (unsigned long i = s; i < e; i += sizeof(unsigned long)) {
		unsigned long val, tmp;

		lfsr = mylfsr(32, lfsr);
		val = lfsr;
		lfsr = mylfsr(32, lfsr);
		val |= lfsr << 32;

		tmp = *(unsigned long *)i;

		if (tmp != val) {
			print("Bad data at ");
			puthex(i);
			print(" expected ");
			puthex(val);
			print(" got ");
			puthex(tmp);
			print("\r\n");
		}
	}

	print("Done\r\n");
}

static int execute(microrl_t *pThis, int argc, const char *const *argv)
{
	if (!strcmp(argv[0], _CMD_HELP)) {
		usage();
	} else if (!strcmp(argv[0], _CMD_VER)) {
		if (argc != 1)
			goto usage;

		print("Version: ");
		print(VERSION);
		print("\r\n");
	} else if (!strcmp(argv[0], _CMD_TEST)) {
		unsigned long seed;
		unsigned long nr_insns;

		if (argc != 3)
			goto usage;

		seed = __atoi(argv[1], 10);
		nr_insns = __atoi(argv[2], 10);

		run_one_test(seed, nr_insns, NULL);

	} else if (!strcmp(argv[0], _CMD_TEST_MANY)) {
		unsigned long seed;
		unsigned long nr_insns;
		unsigned long nr_tests;

		if (argc != 4)
			goto usage;

		seed = __atoi(argv[1], 10);
		nr_insns = __atoi(argv[2], 10);
		nr_tests = __atoi(argv[3], 10);

		run_many_tests(seed, nr_insns, nr_tests);

	} else if (!strcmp(argv[0], _CMD_SET)) {
		if (argc != 3)
			goto usage;

		set_variable(argv[1], argv[2]);
	} else if (!strcmp(argv[0], _CMD_SHOW)) {
		if (argc != 2)
			goto usage;

		show_variable(argv[1]);
	} else if (!strcmp(argv[0], _CMD_ENABLE)) {
		enable_insn(argv[1]);
	} else if (!strcmp(argv[0], _CMD_DISABLE)) {
		disable_insn(argv[1]);
	} else if (!strcmp(argv[0], _CMD_READ)) {
		if (argc != 2)
			goto usage;

		read_data(argv[1]);
	} else if (!strcmp(argv[0], _CMD_MEMTEST)) {
		if (argc != 3)
			goto usage;

		memtest(argv[1], argv[2]);
	}
#if __STDC_HOSTED__ == 1
	else if (!strcmp(argv[0], _CMD_QUIT)) {
		exit(0);
	}
#endif

	return 0;

usage:
	usage();
	return 0;
}

int main(int argc, char *argv[])
{
	init_console();

	microrl_init(prl, microrl_print);
	microrl_set_execute_callback(prl, execute);
#ifdef _USE_COMPLETE
	microrl_set_complete_callback (prl, microrl_complete);
#endif
	microrl_set_sigint_callback(prl, sigint);

	insns_ptr = init_testcase(MAX_INSNS + SLACK);
	mem_ptr = init_memory();

	if (argc == 4 || argc == 3) {
		unsigned long seed = strtoul(argv[1], NULL, 10);
		unsigned long nr_insns = strtoul(argv[2], NULL, 10);
		unsigned long nr_tests = argc > 3? strtoul(argv[3], NULL, 10): 1;

		run_many_tests(seed, nr_insns, nr_tests);
		exit(0);
	}

#if __STDC_HOSTED__ == 1
	if (argc == 5 && argv[1][0] == '-') {
		char *filename = argv[2];
		unsigned long seed = strtoul(argv[3], NULL, 10);
		unsigned long nr_insns = strtoul(argv[4], NULL, 10);

		non_interactive(filename, seed, nr_insns);

		exit(0);
	}
#endif

	while (1)
		microrl_insert_char(prl, getchar_unbuffered());

	//free_testcase(ptr);

	return 0;
}
