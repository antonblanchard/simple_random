void *generate_testcase(void *ptr, void *mem, void *save, unsigned long seed, unsigned long nr_insns,
			bool print_insns, bool sim, unsigned long *lenp);
void enable_insn(const char *insn);
void disable_insn(const char *insn);
