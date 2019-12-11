#include <stdbool.h>
#include <string.h>

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

static const char *extra_names[4] = { "CR", "LR", "CTR", "XER" };

static void run_one_test(unsigned long seed, unsigned long nr_insns)
{
	unsigned long gprs[36];

	if (nr_insns > MAX_INSNS) {
		print("Increase MAX_INSNS\r\n");
		return;
	}

	generate_testcase(insns_ptr, mem_ptr+MEM_SIZE/2, gprs, seed, nr_insns,
			  insns, false);
	memset(mem_ptr, 0, MEM_SIZE);
	execute_testcase(insns_ptr, gprs);

	/* GPR 31 was our scratch space, clear it */
	gprs[31] = 0;

	if (registers) {
		for (unsigned long i = 0; i < 36; i++) {
			if (i < 32)
				putlong(i);
			else
				print(extra_names[i-32]);
			print(" ");
			if (i != 31)
				puthex(gprs[i]);
			print("\r\n");
		}

		print("\r\n");
	} else {
		uint64_t hash = 0;

		if (hash_type == XOR) {
			for (unsigned long i = 0; i < 36; i++)
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
}

static void run_many_tests(unsigned long seed, unsigned long nr_insns,
			   unsigned long nr_tests)
{
	for (unsigned long i = 0; i < nr_tests; i++) {
		run_one_test(seed, nr_insns);
		seed++;
	}
}

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

		run_one_test(seed, nr_insns);

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

	return 0;

usage:
	usage();
	return 0;
}

int main(void)
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

	while (1)
		microrl_insert_char(prl, getchar_unbuffered());

	//free_testcase(ptr);

	return 0;
}
