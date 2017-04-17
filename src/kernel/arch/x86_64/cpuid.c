/*_
 * Copyright (c) 2017 Hirochika Asai <asai@jar.jp>
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

#include <aos/const.h>
#include "../../kernel.h"
#include "arch.h"

int
cpuid_parse(void)
{
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;

    /* CPUID.0H */
    rax = cpuid(0x00, &rbx, &rcx, &rdx);
    if ( 0x756e6547 != rbx || 0x6c65746e != rcx || 0x49656e69 != rdx ) {
        /* Not Intel processor (Genu ntel inel) */
        return -1;
    }

    /* CPUID.01H */
    rax = cpuid(0x01, &rbx, &rcx, &rdx);
    /*
      EAX[3:0]          Stepping ID
      EAX[7:4]          Model
      EAX[11:8]         Family ID
      EAX[13:12]        Processor Type
      EAX[19:16]        Extended Model ID
      EAX[27:20]        Extended Family ID

      EBX[7:0]          Brand Index
      EBX[15:8]         CLFLUSH line size (Value * 8 in bytes)
      EBX[23:16]        Maximum number of addressable IDs
      EBX[31:24]        Initial APIC ID
      ECX/EDX           Feature Information
    */
    /*
      Family = 0x06
      Model
      IvyBridge         0x3a
      SandyBridge       0x2a
      SandyBridge-E     0x2d
      Westmere          0x25 / 0x2c / 0x2f
      Nehalem           0x1e / 0x1a / 0x2e
      Penryn            0x17 / 0x1d
     */

    /* Processor Brand String: CPUID.80000002H, CPUID.80000003H,
       CPUID.80000004H */
    char str[4 * 4 * 3];
    rax = cpuid(0x80000002, &rbx, &rcx, &rdx);
    *(u32 *)(str + 0) = rax;
    *(u32 *)(str + 4) = rbx;
    *(u32 *)(str + 8) = rcx;
    *(u32 *)(str + 12) = rdx;
    rax = cpuid(0x80000003, &rbx, &rcx, &rdx);
    *(u32 *)(str + 16) = rax;
    *(u32 *)(str + 20) = rbx;
    *(u32 *)(str + 24) = rcx;
    *(u32 *)(str + 28) = rdx;
    rax = cpuid(0x80000004, &rbx, &rcx, &rdx);
    *(u32 *)(str + 32) = rax;
    *(u32 *)(str + 36) = rbx;
    *(u32 *)(str + 40) = rcx;
    *(u32 *)(str + 44) = rdx;

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
