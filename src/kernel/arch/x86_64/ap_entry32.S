/*_
 * Copyright (c) 2015-2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "const.h"

	.text

	.code32
	.globl	ap_entry32

	.set	APIC_LAPIC_ID,0x020
	.set	MSR_APIC_BASE,0x1b

/* Entry point */
ap_entry32:
	cli

	/* %cs is automatically set after the long jump operation */
	/* Setup other segment registers */
	movl	$AP_GDT_DATA64_SEL,%eax
	movl	%eax,%ss
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%fs
	movl	%eax,%gs

	/* Obtain APIC ID */
	movl	$MSR_APIC_BASE,%ecx
	rdmsr
	andl	$0xfffffffffffff000,%eax	/* APIC Base */
	movl	APIC_LAPIC_ID(%eax),%eax
	shrl	$24,%eax

	/* Setup stack with 16 byte guard */
	addl	$1,%eax
	movl	$CPU_DATA_SIZE,%ebx
	mull	%ebx		/* [%edx:%eax] = %eax * P_DATA_SIZE */
	addl	$CPU_DATA_BASE,%eax
	subl	$CPU_STACK_GUARD,%eax
	movl	%eax,%esp

	/* Enable PAE */
	movl	$0x40220,%eax	/* CR4[bit 5] = PAE */
	movl	%eax,%cr4	/* CR4[bit 9] = OSFXSR */
				/* CR4[bit 18] = OSXSAVE */

	/* Setup page table register */
	movl	$KERNEL_PGT,%ebx
	movl	%ebx,%cr3

	/* Enable long mode */
	movl	$0xc0000080,%ecx	/* EFER MSR number */
	rdmsr			/* Read from 64bit-specific register */
	btsl	$8,%eax		/* LME bit = 1 */
	wrmsr			/* Write to 64bit-specific register */

	/* Activate page translation and long mode */
	movl	$0x80000001,%eax
	movl	%eax,%cr0

	/* Load code64 descriptor */
	pushl	$AP_GDT_CODE64_SEL
	pushl	$ap_entry64
	lret
