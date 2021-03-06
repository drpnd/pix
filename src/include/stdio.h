/*_
 * Copyright (c) 2015-2017 Hirochika Asai <asai@jar.jp>
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

#ifndef _STDIO_H
#define _STDIO_H

#include <aos/types.h>

#define EOF     -1

typedef struct {
    /* File descriptor */
    int fd;
    /* Mode */
    int mode;
    /* Stream buffer */
    struct {
        char *buf;
        size_t pos;
        size_t sz;
    } ibuf;
    struct {
        char *buf;
        size_t pos;
        size_t sz;
    } obuf;
    /* EOF */
    int eof;
    /* Error */
    int error;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int snprintf(char *__restrict__, size_t, const char *__restrict__, ...);
int fprintf(FILE * __restrict__, const char * __restrict__, ...);
int printf(const char * __restrict__, ...);

FILE * fdopen(int, const char *);
int fclose(FILE *);
int ferror(FILE *);
void clearerr(FILE *);
char * fgets(char * __restrict__, int size, FILE * __restrict__);
int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
size_t fwrite(const void *__restrict__, size_t, size_t, FILE *__restrict__);
int fputs(const char *__restrict__, FILE *__restrict__);
int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);
int fileno(FILE *);

#endif /* _STDIO_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
