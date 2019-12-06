#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "generate.h"
#include "lfsr.h"
#include "helpers.h"
#include "mystdio.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#undef DIVIDES

struct insn {
	uint32_t opcode;
	uint32_t mask;
	bool enabled;
	char *name;
};

static struct insn insns[] = {
	/* Add/sub ops */
	{ 0x38000000, 0x03ffffff, true, "addi"},
	{ 0x7c000214, 0x03fff800, true, "add"},
	{ 0x7c000215, 0x03fff800, true, "add_rc"},
	{ 0x3c000000, 0x03ffffff, true, "addis"},
	{ 0x7c000050, 0x03fff800, true, "subf"},
	{ 0x7c000051, 0x03fff800, true, "subf_rc"},
	{ 0x7c0000d0, 0x03fff800, true, "neg"},
	{ 0x7c0000d1, 0x03fff800, true, "neg_rc"},

	/* Add/sub carry ops */
	{ 0x7c000014, 0x03fff800, true, "addc"},
	{ 0x7c000015, 0x03fff800, true, "addc_rc"},
	{ 0x30000000, 0x03ffffff, true, "addic"},
	{ 0x34000000, 0x03ffffff, true, "addic_rc"},
	{ 0x7c000194, 0x03fff800, true, "addze"},
	{ 0x7c000195, 0x03fff800, true, "addze_rc"},
	{ 0x7c000114, 0x03fff800, true, "adde"},
	{ 0x7c000115, 0x03fff800, true, "adde_rc"},
	{ 0x7c0001d4, 0x03fff800, true, "addme"},
	{ 0x7c0001d5, 0x03fff800, true, "addme_rc"},
	{ 0x7c000010, 0x03fff800, true, "subfc"},
	{ 0x7c000011, 0x03fff800, true, "subfc_rc"},
	{ 0x20000000, 0x03ffffff, true, "subfic"},
	{ 0x7c000110, 0x03fff800, true, "subfe"},
	{ 0x7c000111, 0x03fff800, true, "subfe_rc"},
	{ 0x7c0001d0, 0x03fff800, true, "subfme"},
	{ 0x7c0001d1, 0x03fff800, true, "subfme_rc"},
	{ 0x7c000190, 0x03fff800, true, "subfze"},
	{ 0x7c000191, 0x03fff800, true, "subfze_rc"},
	{ 0x7c000154, 0x03fff801, false, "addex"},

	/* Logical ops */
	{ 0x70000000, 0x03ffffff, true, "andi_rc"},
	{ 0x74000000, 0x03ffffff, true, "andis_rc"},
	{ 0x60000000, 0x03ffffff, true, "ori"},
	{ 0x64000000, 0x03ffffff, true, "oris"},
	{ 0x68000000, 0x03ffffff, true, "xori"},
	{ 0x6c000000, 0x03ffffff, true, "xoris"},
	{ 0x7c000038, 0x03fff800, true, "and"},
	{ 0x7c000039, 0x03fff800, true, "and_rc"},
	{ 0x7c000278, 0x03fff800, true, "xor"},
	{ 0x7c000279, 0x03fff800, true, "xor_rc"},
	{ 0x7c0003b8, 0x03fff800, true, "nand"},
	{ 0x7c0003b9, 0x03fff800, true, "nand_rc"},
	{ 0x7c000378, 0x03fff800, true, "or"},
	{ 0x7c000379, 0x03fff800, true, "or_rc"},
	{ 0x7c0000f8, 0x03fff800, true, "nor"},
	{ 0x7c0000f9, 0x03fff800, true, "nor_rc"},
	{ 0x7c000078, 0x03fff800, true, "andc"},
	{ 0x7c000079, 0x03fff800, true, "andc_rc"},
	{ 0x7c000238, 0x03fff800, true, "eqv"},
	{ 0x7c000239, 0x03fff800, true, "eqv_rc"},
	{ 0x7c000338, 0x03fff800, true, "orc"},
	{ 0x7c000339, 0x03fff800, true, "orc_rc"},

	/* Sign extension ops */
	{ 0x7c000774, 0x03fff800, true, "extsb"},
	{ 0x7c000775, 0x03fff800, true, "extsb_rc"},
	{ 0x7c000734, 0x03fff800, true, "extsh"},
	{ 0x7c000735, 0x03fff800, true, "extsh_rc"},
	{ 0x7c0007b4, 0x03fff800, true, "extsw"},
	{ 0x7c0007b5, 0x03fff800, true, "extsw_rc"},
	{ 0x7c0006f4, 0x03fff802, false, "extswsli"},
	{ 0x7c0006f5, 0x03fff802, false, "extswsli_rc"},

	/* Count leading/trailing zeroes */
	{ 0x7c000034, 0x03fff800, true, "cntlzw"},
	{ 0x7c000035, 0x03fff800, true, "cntlzw_rc"},
	{ 0x7c000434, 0x03fff800, true, "cnttzw"},
	{ 0x7c000435, 0x03fff800, true, "cnttzw_rc"},
	{ 0x7c000074, 0x03fff800, true, "cntlzd"},
	{ 0x7c000075, 0x03fff800, true, "cntlzd_rc"},
	{ 0x7c000474, 0x03fff800, true, "cnttzd"},
	{ 0x7c000475, 0x03fff800, true, "cnttzd_rc"},

	/* Rotate and shift ops */
	{ 0x78000010, 0x03ffffe0, true, "rldcl"},
	{ 0x78000011, 0x03ffffe0, true, "rldcl_rc"},
	{ 0x78000012, 0x03ffffe0, true, "rldcr"},
	{ 0x78000013, 0x03ffffe0, true, "rldcr_rc"},
	{ 0x78000000, 0x03ffffe2, true, "rldicl"},
	{ 0x78000001, 0x03ffffe2, true, "rldicl_rc"},
	{ 0x78000004, 0x03ffffe2, true, "rldicr"},
	{ 0x78000005, 0x03ffffe2, true, "rldicr_rc"},
	{ 0x78000008, 0x03ffffe2, true, "rldic"},
	{ 0x78000009, 0x03ffffe2, true, "rldic_rc"},
	{ 0x5c000000, 0x03fffffe, true, "rlwnm"},
	{ 0x5c000001, 0x03fffffe, true, "rlwnm_rc"},
	{ 0x54000000, 0x03fffffe, true, "rlwinm"},
	{ 0x54000001, 0x03fffffe, true, "rlwinm_rc"},
	{ 0x7800000c, 0x03ffffe2, true, "rldimi"},
	{ 0x7800000d, 0x03ffffe2, true, "rldimi_rc"},
	{ 0x50000000, 0x03fffffe, true, "rlwimi"},
	{ 0x50000001, 0x03fffffe, true, "rlwimi_rc"},
	{ 0x7c000030, 0x03fff800, true, "slw"},
	{ 0x7c000031, 0x03fff800, true, "slw_rc"},
	{ 0x7c000036, 0x03fff800, true, "sld"},
	{ 0x7c000037, 0x03fff800, true, "sld_rc"},
	{ 0x7c000634, 0x03fff800, true, "srad"},
	{ 0x7c000635, 0x03fff800, true, "srad_rc"},
	{ 0x7c000674, 0x03fff802, true, "sradi"},
	{ 0x7c000675, 0x03fff802, true, "sradi_rc"},
	{ 0x7c000630, 0x03fff800, true, "sraw"},
	{ 0x7c000631, 0x03fff800, true, "sraw_rc"},
	{ 0x7c000670, 0x03fff800, true, "srawi"},
	{ 0x7c000671, 0x03fff800, true, "srawi_rc"},
	{ 0x7c000436, 0x03fff800, true, "srd"},
	{ 0x7c000437, 0x03fff800, true, "srd_rc"},
	{ 0x7c000430, 0x03fff800, true, "srw"},
	{ 0x7c000431, 0x03fff800, true, "srw_rc"},

	/* Population count ops */
	{ 0x7c0000f4, 0x03fff801, true, "popcntb"},
	{ 0x7c0003f4, 0x03fff801, true, "popcntd"},
	{ 0x7c0002f4, 0x03fff801, true, "popcntw"},

	/* Multiply ops */
	{ 0x7c000092, 0x03fffc00, true, "mulhd"},
	{ 0x7c000093, 0x03fffc00, true, "mulhd_rc"},
	{ 0x7c000012, 0x03fffc00, true, "mulhdu"},
	{ 0x7c000013, 0x03fffc00, true, "mulhdu_rc"},
	{ 0x7c000096, 0x03fffc00, true, "mulhw"},
	{ 0x7c000097, 0x03fffc00, true, "mulhw_rc"},
	{ 0x7c000016, 0x03fffc00, true, "mulhwu"},
	{ 0x7c000017, 0x03fffc00, true, "mulhwu_rc"},
	{ 0x1c000000, 0x03ffffff, true, "mulli"},
	{ 0x7c0001d2, 0x03fff800, true, "mulld"},
	{ 0x7c0001d3, 0x03fff800, true, "mulld_rc"},
	{ 0x7c0001d6, 0x03fff800, true, "mullw"},
	{ 0x7c0001d7, 0x03fff800, true, "mullw_rc"},
	{ 0x10000030, 0x03ffffc0, false, "maddhd"},
	{ 0x10000031, 0x03ffffc0, false, "maddhdu"},
	{ 0x10000033, 0x03ffffc0, false, "maddld"},

	/* SPR read/write ops */
	{ 0x7c0903a6, 0x03e00001, true, "mtspr_ctr"},
	{ 0x7c0902a6, 0x03e00001, true, "mfspr_ctr"},
	{ 0x7c0803a6, 0x03e00001, true, "mtspr_lr"},
	{ 0x7c0802a6, 0x03e00001, true, "mfspr_lr"},

	/* Compare ops */
	{ 0x7c000000, 0x03fff801, true, "cmp"},
	{ 0x2c000000, 0x03ffffff, true, "cmpi"},
	{ 0x7c000040, 0x03fff801, true, "cmpl"},
	{ 0x28000000, 0x03ffffff, true, "cmpli"},
	{ 0x7c000180, 0x03fff801, false, "cmprb"},
	{ 0x7c0001c0, 0x03fff801, false, "cmpeqb"},

	/* CR ops */
	{ 0x4c000000, 0x03fff801, true, "mcrf"},
	{ 0x7c100026, 0x03eff801, true, "mfocrf"},
	{ 0x7c100120, 0x03eff801, true, "mtocrf"},
	{ 0x7c000026, 0x03eff801, true, "mfcr"},
	{ 0x7c000120, 0x03eff801, true, "mtcrf"},
	{ 0x7c00001e, 0x03e0ffc1, true, "isel0"},
	{ 0x7c10001e, 0x03efffc1, true, "isel"},
	{ 0x7c08001e, 0x03f7ffc1, true, "isel"},
	{ 0x7c04001e, 0x03fbffc1, true, "isel"},
	{ 0x7c02001e, 0x03fdffc1, true, "isel"},
	{ 0x7c01001e, 0x03feffc1, true, "isel"},
	{ 0x7c000100, 0x03fff801, false, "setb"},

	/* BC+8 */
	{ 0x40000008, 0x03ff0001, true, "bc"},

	/* CR logical ops */
	{ 0x4c000202, 0x03fff801, false, "crand"},
	{ 0x4c000102, 0x03fff801, false, "crandc"},
	{ 0x4c000242, 0x03fff801, false, "creqv"},
	{ 0x4c0001c2, 0x03fff801, false, "crnand"},
	{ 0x4c000042, 0x03fff801, false, "crnor"},
	{ 0x4c000382, 0x03fff801, false, "cror"},
	{ 0x4c000342, 0x03fff801, false, "crorc"},
	{ 0x4c000182, 0x03fff801, false, "crxor"},

	/* Divide and mod */
	{ 0x7c0003d2, 0x03fff800, true, "divd"},
	{ 0x7c000352, 0x03fff800, true, "divde"},
	{ 0x7c000353, 0x03fff800, true, "divde_rc"},
	{ 0x7c000312, 0x03fff800, true, "divdeu"},
	{ 0x7c000313, 0x03fff800, true, "divdeu_rc"},
	{ 0x7c0003d3, 0x03fff800, true, "divd_rc"},
	{ 0x7c000392, 0x03fff800, true, "divdu"},
	{ 0x7c000393, 0x03fff800, true, "divdu_rc"},
	{ 0x7c0003d6, 0x03fff800, true, "divw"},
	{ 0x7c000356, 0x03fff800, true, "divwe"},
	{ 0x7c000357, 0x03fff800, true, "divwe_rc"},
	{ 0x7c000316, 0x03fff800, true, "divweu"},
	{ 0x7c000317, 0x03fff800, true, "divweu_rc"},
	{ 0x7c0003d7, 0x03fff800, true, "divw_rc"},
	{ 0x7c000396, 0x03fff800, true, "divwu"},
	{ 0x7c000397, 0x03fff800, true, "divwu_rc"},
	{ 0x7c000612, 0x03fff801, true, "modsd"},
	{ 0x7c000616, 0x03fff801, true, "modsw"},
	{ 0x7c000212, 0x03fff801, true, "modud"},
	{ 0x7c000216, 0x03fff801, true, "moduw"},

	/* Parity ops */
	{ 0x7c000174, 0x03fff801, true, "prtyd"},
	{ 0x7c000134, 0x03fff801, true, "prtyw"},

	/* Other misc ops */
	{ 0x7c0003f8, 0x03fff801, true, "cmpb"},
	{ 0x7c0001f8, 0x03fff801, false, "bpermd"},

	/* Cache control ops */
	{ 0x7c0007ac, 0x03fff801, false, "icbi"},
	{ 0x7c00002c, 0x03fff801, true, "icbt"},
	{ 0x4c00012c, 0x03fff801, true, "isync"},
	{ 0x7c0004ac, 0x03bff801, true, "sync"},

	/* Trap ops */
	{ 0x08000000, 0x03ffffff, false, "tdi_ti"},
	{ 0x7c000088, 0x03fff801, false, "td_ti"},
	{ 0x7c000008, 0x03fff801, false, "tw"},
	{ 0x0c000000, 0x03ffffff, false, "twi"},

};
#define NR_INSNS (sizeof(insns) / sizeof(struct insn))

enum form_t { X, D, DS };

struct ldst_insn {
	uint32_t opcode;
	uint32_t mask;
	enum form_t form;
	bool update;
	uint8_t size;
	bool enabled;
	char *name;
};

static struct ldst_insn ldst_insns[] = {
	{ 0x88000000, 0x03ffffff, D,  false, 1, true, "lbz"},
	{ 0x7c0000ae, 0x03fff801, X,  false, 1, true, "lbzx"},
	{ 0xe8000000, 0x03fffffc, DS, false, 8, true, "ld"},
	{ 0x7c000428, 0x03fff801, X,  false, 8, true, "ldbrx"},
	{ 0x7c00002a, 0x03fff801, X,  false, 8, true, "ldx"},
	{ 0x7c00062c, 0x03fff801, X,  false, 2, true, "lhbrx"},
	{ 0xa0000000, 0x03ffffff, D,  false, 2, true, "lhz"},
	{ 0x7c00022e, 0x03fff801, X,  false, 2, true, "lhzx"},
	{ 0x7c00042c, 0x03fff801, X,  false, 4, true, "lwbrx"},
	{ 0x80000000, 0x03ffffff, D,  false, 4, true, "lwz"},
	{ 0x7c00002e, 0x03fff801, X,  false, 4, true, "lwzx"},
	{ 0x98000000, 0x03ffffff, D,  false, 1, true, "stb"},
	{ 0x7c0001ae, 0x03fff801, X,  false, 1, true, "stbx"},
	{ 0xf8000000, 0x03fffffc, DS, false, 8, true, "std"},
	{ 0x7c000528, 0x03fff801, X,  false, 8, true, "stdbrx"},
	{ 0x7c00012a, 0x03fff801, X,  false, 8, true, "stdx"},
	{ 0xb0000000, 0x03ffffff, D,  false, 2, true, "sth"},
	{ 0x7c00072c, 0x03fff801, X,  false, 2, true, "sthbrx"},
	{ 0x7c00032e, 0x03fff801, X,  false, 2, true, "sthx"},
	{ 0x90000000, 0x03ffffff, D,  false, 4, true, "stw"},
	{ 0x7c00052c, 0x03fff801, X,  false, 4, true, "stwbrx"},
	{ 0x7c00012e, 0x03fff801, X,  false, 4, true, "stwx"},
	{ 0x7c000068, 0x03fff801, X,  false, 1, true, "lbarx"},
	{ 0x7c0000e8, 0x03fff801, X,  false, 2, true, "lharx"},
	{ 0x7c0000a8, 0x03fff801, X,  false, 8, true, "ldarx"},
	{ 0xe8000001, 0x03fffffc, DS, true,  8, true, "ldu"},
	{ 0x7c00006a, 0x03fff801, X,  true,  8, true, "ldux"},
	{ 0xa8000000, 0x03ffffff, D,  false, 2, true, "lha"},
	{ 0x7c0002ae, 0x03fff801, X,  false, 2, true, "lhax"},
	{ 0xe8000002, 0x03fffffc, DS, false, 4, true, "lwa"},
	{ 0x7c0002aa, 0x03fff801, X,  false, 4, true, "lwax"},
	{ 0x94000000, 0x03ffffff, D,  true,  4, true, "stwu"},
	{ 0x7c00016e, 0x03fff801, X,  true,  4, true, "stwux"},
	{ 0x9c000000, 0x03ffffff, D,  true,  1, true, "stbu"},
	{ 0x7c0001ee, 0x03fff801, X,  true,  1, true, "stbux"},
	{ 0xb4000000, 0x03ffffff, D,  true,  2, true, "sthu"},
	{ 0x7c00036e, 0x03fff801, X,  true,  2, true, "sthux"},
	{ 0xf8000001, 0x03fffffc, DS, true,  8, true, "stdu"},
	{ 0x7c00016a, 0x03fff801, X,  true,  8, true, "stdux"},
	{ 0x8c000000, 0x03ffffff, D,  true,  1, true, "lbzu"},
	{ 0x7c0000ee, 0x03fff801, X,  true,  1, true, "lbzux"},
	{ 0xa4000000, 0x03ffffff, D,  true,  2, true, "lhzu"},
	{ 0x7c00026e, 0x03fff801, X,  true,  2, true, "lhzux"},
	{ 0x84000000, 0x03ffffff, D,  true,  4, true, "lwzu"},
	{ 0x7c00006e, 0x03fff801, X,  true,  4, true, "lwzux"},
	{ 0xac000000, 0x03ffffff, D,  true,  2, true, "lhau"},
	{ 0x7c0002ee, 0x03fff801, X,  true,  2, true, "lhaux"},
	{ 0x7c0002ea, 0x03fff801, X,  true,  4, true, "lwaux"},
};
#define NR_LDST_INSNS (sizeof(ldst_insns) / sizeof(struct ldst_insn))

static unsigned long fxvalues[] = {
	0x0000000000000000,	/* all zeros */
	0xFFFFFFFFFFFFFFFF,	/* all ones */
	0x0000000000000001,	/* one */
	0x1111111111111111,	/* low bits */
	0x8888888888888888,	/* high bits */
	0x00000000FFFFFFFF,	/* 32 bit all ones */
	0xFFFFFFFF00000000,	/* high 32 bits all ones */
	0x0000000100000000,	/* 2 ^ 32 */
	0x00000000ffffff80,	/* signed char min */
	0x000000000000007f,	/* signed char max */
	0x00000000000000ff,	/* unsigned char max */
	0x00000000ffff8000,	/* short min */
	0x0000000000007fff,	/* short max */
	0x000000000000ffff,	/* unsigned short max */
	0x0000000080000000,	/* int min */
	0x000000007fffffff,	/* int max */
	0x00000000ffffffff,	/* unsigned int max */
	0x8000000000000000,	/* long max */
	0x7fffffffffffffff,	/* long min */
	0xffffffffffffffff,	/* unsigned long max */
	0x0001020304050607,
	0x0706050403020100,
	0x00FF00FF00FF00FF,
	0xFF00FF00FF00FF00,
	0xa5a5a5a5a5a5a5a5,
};
#define NR_FXVALUES (sizeof(fxvalues)/sizeof(fxvalues[0]))

#define PPC_OPCODE(OPC)		((OPC) << 26)
#define PPC_RT(RT)		((RT) << 21)
#define PPC_RS(RS)		((RS) << 21)
#define PPC_RA(RA)		((RA) << 16)
#define PPC_RB(RB)		((RB) << 11)
#define PPC_SH(SH)		((((SH) >> 5) << 1) | (((SH) & 0x1f) << 11))
#define PPC_ME(ME)		((((ME) >> 5) << 5) | (((ME) & 0x1f) << 6))

#define ADDIS(RT, RA, UI)	(PPC_OPCODE(15) | PPC_RS(RT) | PPC_RA(RA) | ((UI) & 0xffff))
#define ORIS(RS, RA, UI)	(PPC_OPCODE(25) | PPC_RS(RS) | PPC_RA(RA) | ((UI) & 0xffff))
#define ORI(RS, RA, UI)		(PPC_OPCODE(24) | PPC_RS(RS) | PPC_RA(RA) | ((UI) & 0xffff))
#define STD(RS, RA, DS)		(PPC_OPCODE(62) | PPC_RS(RS) | PPC_RA(RA) | DS)
#define RLDICR(RA, RS, SH, ME)	(PPC_OPCODE(30) | PPC_RA(RA) | PPC_RS(RS) | PPC_SH(SH) | PPC_ME(ME) | 4)
#define NOP			0x60000000

static void *load_64bit_imm(uint32_t *p, int gpr, uint64_t val)
{
	*p++ = ADDIS(gpr, 0, (val >> 48) & 0xffff);
	*p++ = ORI(gpr, gpr, (val >> 32) & 0xffff);
	*p++ = RLDICR(gpr, gpr, 32, 31);
	*p++ = ORIS(gpr, gpr, (val >> 16) & 0xffff);
	*p++ = ORI(gpr, gpr, val & 0xffff);

	return p;
}

static void *do_one_loadstore(uint32_t *p, void *mem, struct ldst_insn *insnp,
			      uint32_t *lfsr, bool print_insns)
{
	uint32_t insn = insnp->opcode;
	uint64_t off;

	/*
	 * The preceding instruction might have been a BC+8, put a NOP here
	 * just in case.
	 */
	*p++ = NOP;

	/* Form a small positive or negative offset */
	*lfsr = mylfsr(32, *lfsr);
	off = *lfsr % 32;

	/* We don't support unaligned accesses yet, so align the offset */
	off &= ~(insnp->size-1);

	/* Use the high bit for the sign of the offset */
	if (*lfsr & 0x80000000)
		off = -off;

	if (insnp->form == X) {
		uint8_t ra, rb, rt;

		*lfsr = mylfsr(32, *lfsr);
		rb = *lfsr % 32;
		ra = (rb + 1) % 32;
		/* RA=0 is invalid for update forms */
		if (insnp->update) {
			if (ra == 0)
				ra = 1;
		}
		rt = (ra + 1) % 32;

		/* if RA=R0 the hardware uses 0, so put the base in RB */
		p = load_64bit_imm(p, rb, (unsigned long)mem);

		p = load_64bit_imm(p, ra, off);
		insn |= PPC_RT(rt) | PPC_RA(ra) | PPC_RB(rb);
	} else {
		uint8_t ra, rt;

		*lfsr = mylfsr(32, *lfsr);
		ra = *lfsr % 32;
		/*
		 * if RA=R0 the hardware uses 0. Since the offset isn't
		 * big enough to reach our data page, we can't test RA=R0
		 */
		if (ra == 0)
			ra = 1;

		rt = (ra + 1) % 32;

		p = load_64bit_imm(p, ra, (unsigned long)mem);

		insn |= PPC_RT(rt) | PPC_RA(ra) | (off & 0xffff);
	}

	if (print_insns) {
		puthex(insn);
		print(" ");
		print(insnp->name);
		print("\r\n");
	}

	*p++ = insn;

	return p;
}

void generate_testcase(void *ptr, void *mem, void *save, unsigned long seed,
		       unsigned long nr_insns, bool print_insns)
{
	uint32_t *p;
	uint32_t lfsr = seed;

	/* LFSR needs a non zero value to work */
	if (!lfsr)
		lfsr = 0xffffffff;

	/* Prolog */
	memcpy(ptr, prolog1_start, prolog1_end-prolog1_start);
	ptr += prolog1_end-prolog1_start;

	/* Initialize GPRs */
	for (unsigned long i = 0; i < 32; i++) {
		uint64_t val;

		lfsr = mylfsr(32, lfsr);

		val = fxvalues[lfsr % NR_FXVALUES];
		ptr = load_64bit_imm(ptr, i, val);
	}

	/* At this point we can start the test */
	for (unsigned long i = 0; i < nr_insns; i++) {
		uint32_t j;
		uint32_t insn;

		if (!(i & 0x1f)) {
			do {
				lfsr = mylfsr(32, lfsr);
				j = lfsr % NR_LDST_INSNS;
			} while (ldst_insns[j].enabled == false);

			ptr = do_one_loadstore(ptr, mem, &ldst_insns[j],
					       &lfsr, print_insns);
		} else {
			do {
				lfsr = mylfsr(32, lfsr);
				j = lfsr % NR_INSNS;
			} while (insns[j].enabled == false);

			lfsr = mylfsr(32, lfsr);
			insn = insns[j].opcode | (lfsr & insns[j].mask);

			if (print_insns) {
				puthex(insn);
				print(" ");
				print(insns[j].name);
				print("\r\n");
			}

			*(uint32_t *)ptr = insn;
			ptr += sizeof(uint32_t);
		}
	}

	/* First epilog */
	memcpy(ptr, epilog1_start, epilog1_end-epilog1_start);
	ptr += sizeof(epilog1_end-epilog1_start);

	/*
	 * At this point r31 is free, create a pointer to our
	 * save area and write the GPRs out. Assume address is in
	 * the low 32 bits.
	 */
	ptr = load_64bit_imm(ptr, 31, (uint64_t)save);

	p = ptr;
	/* Save GPR 0-31 to our save area */
	for (unsigned long i = 0; i < 31; i++)
		*p++ = STD(i, 31, i*sizeof(uint64_t));
	ptr = p;

	/* Second epilog */
	memcpy(ptr, epilog2_start, epilog2_end-epilog2_start);
	ptr += sizeof(epilog2_end-epilog2_start);

	asm volatile("sync; icbi 0,%0; sync; isync": : "r"(ptr));
}

void enable_insn(const char *insn)
{
	size_t l;
	bool wild = false;

	l = strlen(insn);
	if (l > 0 && insn[l-1] == '*') {
		--l;
		wild = true;
	}
	for (unsigned long i = 0; i < NR_INSNS; i++) {
		if (!insns[i].enabled && !strncmp(insns[i].name, insn, l) &&
		    (wild || insn[l] == 0)) {
			print("Enabling ");
			print(insns[i].name);
			print("\r\n");
			insns[i].enabled = true;
		}
	}
}

void disable_insn(const char *insn)
{
	size_t l;
	bool wild = false;

	l = strlen(insn);
	if (l > 0 && insn[l-1] == '*') {
		--l;
		wild = true;
	}
	for (unsigned long i = 0; i < NR_INSNS; i++) {
		if (insns[i].enabled && !strncmp(insns[i].name, insn, l) &&
		    (wild || insn[l] == 0)) {
			print("Disabling ");
			print(insns[i].name);
			print("\r\n");
			insns[i].enabled = false;
		}
	}
}
