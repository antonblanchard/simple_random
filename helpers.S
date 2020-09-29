#include <ppc-asm.h>

#define r1 1

#define STACK_FRAME_SIZE 256

.globl prolog1_start
prolog1_start:
	mflr	r0
	std	r0,16(r1)
	stdu	r1,-STACK_FRAME_SIZE(r1)

	/* Save non volatile state onto stack */
	mfcr	r0
	std	r0,8(r1)

	std	r13,32(r1)
	std	r14,40(r1)
	std	r15,48(r1)
	std	r16,56(r1)
	std	r17,64(r1)
	std	r18,72(r1)
	std	r19,80(r1)
	std	r20,88(r1)
	std	r21,96(r1)
	std	r22,104(r1)
	std	r23,112(r1)
	std	r24,120(r1)
	std	r25,128(r1)
	std	r26,136(r1)
	std	r27,144(r1)
	std	r28,152(r1)
	std	r29,160(r1)
	std	r30,168(r1)
	std	r31,176(r1)

	/* We don't save VMX/VSX regs since we compile with -mno-altivec -mno-vsx */

	/* Save our stack pointer into the save area */
	std	r1,248(r3)
.globl prolog1_end
prolog1_end:

.globl prolog2_start
prolog2_start:
	/* Now prepare for the test case */

	/* zero r0 */
	li	r0,0

	/* zero CR */
	mtcr	r0

	/* zero LR */
	mtlr	r0

	/* zero CTR */
	mtctr	r0

	/* zero carry, overflow, etc. */
	mtxer	r0
.globl prolog2_end
prolog2_end:

.globl epilog1_start
epilog1_start:
	/* Add a nop in case the last instruction is a BC+8 */
	nop

	/* Free up r31 to use as a scratch register */
	xor	r0,r0,r31

.globl epilog1_end
epilog1_end:

.globl epilog2_start
epilog2_start:
	mfcr	r0
	std	r0, 256(r31)
	mflr	r0
	std	r0, 264(r31)
	mfctr	r0
	std	r0, 272(r31)
	mfxer	r0
	std	r0, 280(r31)

	/* Restore our stack pointer from the save area */
	ld	r1,248(r31)

	/* Restore non volatile state from stack */
	ld	r0,8(r1)
	mtcr	r0

	ld	r13,32(r1)
	ld	r14,40(r1)
	ld	r15,48(r1)
	ld	r16,56(r1)
	ld	r17,64(r1)
	ld	r18,72(r1)
	ld	r19,80(r1)
	ld	r20,88(r1)
	ld	r21,96(r1)
	ld	r22,104(r1)
	ld	r23,112(r1)
	ld	r24,120(r1)
	ld	r25,128(r1)
	ld	r26,136(r1)
	ld	r27,144(r1)
	ld	r28,152(r1)
	ld	r29,160(r1)
	ld	r30,168(r1)
	ld	r31,176(r1)

	addi	r1,r1,STACK_FRAME_SIZE

	ld	r0,16(r1)
	mtlr	r0

	blr
.globl epilog2_end
epilog2_end:
