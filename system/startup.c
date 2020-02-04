//******************************************************************************
//*
//*     Project:   Any
//*
//*     File:      Cortex-M startup file
//*
//*     Version 2.0
//*
//*     Copyright (c) 2016, emb-lib Project Team
//*
//*     Permission is hereby granted, free of charge, to any person
//*     obtaining  a copy of this software and associated documentation
//*     files (the "Software"), to deal in the Software without restriction,
//*     including without limitation the rights to use, copy, modify, merge,
//*     publish, distribute, sublicense, and/or sell copies of the Software,
//*     and to permit persons to whom the Software is furnished to do so,
//*     subject to the following conditions:
//*
//*     The above copyright notice and this permission notice shall be included
//*     in all copies or substantial portions of the Software.
//*
//*     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//*     EXPRESS  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//*     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//*     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//*     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//*     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
//*     THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*
//------------------------------------------------------------------------------

#include <string.h>
     
//------------------------------------------------------------------------------

extern unsigned char __idata_start[];
extern unsigned char __data_start[];
extern unsigned char __data_end[];
extern unsigned char __bss_start[];
extern unsigned char __bss_end[];

__attribute__ ((weak))
int  __low_level_init();
void __libc_init_array();
int  main();          

//------------------------------------------------------------------------------
void Reset_Handler()
{
#if defined DEBUG
	// remove old RTT signature
	memset((void*)0x20000000, 0x00, 64*1024);
#endif


	if( __low_level_init() )
    {
        memcpy(__data_start, __idata_start, __data_end - __data_start); // copy initialized variables
        memset(__bss_start, 0, __bss_end - __bss_start);                // zero-fill uninitialized variables
        __libc_init_array();                                            // low-level init & ctor loop
    }
    main();
}
//------------------------------------------------------------------------------
__attribute__ ((weak))
void _init()
{
}
//------------------------------------------------------------------------------
int __low_level_init()
{
    return 1;
}
//------------------------------------------------------------------------------
