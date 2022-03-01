/*
	Copyright (C) 2014-2022 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	fluxsort 1.1.5.2
*/

#ifndef FLUXSORT_H
#define FLUXSORT_H

#ifndef QUADSORT_H
  #include "quadsort.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

typedef int CMPFUNC (const void *a, const void *b);

//////////////////////////////////////////////////////////
//┌────────────────────────────────────────────────────┐//
//│                █████┐    ██████┐ ██████┐████████┐  │//
//│               ██┌──██┐   ██┌──██┐└─██┌─┘└──██┌──┘  │//
//│               └█████┌┘   ██████┌┘  ██│     ██│     │//
//│               ██┌──██┐   ██┌──██┐  ██│     ██│     │//
//│               └█████┌┘   ██████┌┘██████┐   ██│     │//
//│                └────┘    └─────┘ └─────┘   └─┘     │//
//└────────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////

#undef VAR
#undef FUNC
#undef STRUCT

#define VAR char
#define FUNC(NAME) NAME##8
#define STRUCT(NAME) struct NAME##8

#include "fluxsort.c"

//////////////////////////////////////////////////////////
//┌────────────────────────────────────────────────────┐//
//│           ▄██┐   █████┐    ██████┐ ██████┐████████┐│//
//│          ████│  ██┌───┘    ██┌──██┐└─██┌─┘└──██┌──┘│//
//│          └─██│  ██████┐    ██████┌┘  ██│     ██│   │//
//│            ██│  ██┌──██┐   ██┌──██┐  ██│     ██│   │//
//│          ██████┐└█████┌┘   ██████┌┘██████┐   ██│   │//
//│          └─────┘ └────┘    └─────┘ └─────┘   └─┘   │//
//└────────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////

#undef VAR
#undef FUNC
#undef STRUCT

#define VAR short
#define FUNC(NAME) NAME##16
#define STRUCT(NAME) struct NAME##16

#include "fluxsort.c"

//////////////////////////////////////////////////////////
// ┌───────────────────────────────────────────────────┐//
// │       ██████┐ ██████┐    ██████┐ ██████┐████████┐ │//
// │       └────██┐└────██┐   ██┌──██┐└─██┌─┘└──██┌──┘ │//
// │        █████┌┘ █████┌┘   ██████┌┘  ██│     ██│    │//
// │        └───██┐██┌───┘    ██┌──██┐  ██│     ██│    │//
// │       ██████┌┘███████┐   ██████┌┘██████┐   ██│    │//
// │       └─────┘ └──────┘   └─────┘ └─────┘   └─┘    │//
// └───────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////

#undef VAR
#undef FUNC
#undef STRUCT

#define VAR int
#define FUNC(NAME) NAME##32
#define STRUCT(NAME) struct NAME##32

#include "fluxsort.c"

//////////////////////////////////////////////////////////
// ┌───────────────────────────────────────────────────┐//
// │        █████┐ ██┐  ██┐   ██████┐ ██████┐████████┐ │//
// │       ██┌───┘ ██│  ██│   ██┌──██┐└─██┌─┘└──██┌──┘ │//
// │       ██████┐ ███████│   ██████┌┘  ██│     ██│    │//
// │       ██┌──██┐└────██│   ██┌──██┐  ██│     ██│    │//
// │       └█████┌┘     ██│   ██████┌┘██████┐   ██│    │//
// │        └────┘      └─┘   └─────┘ └─────┘   └─┘    │//
// └───────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////

#undef VAR
#undef FUNC
#undef STRUCT

#define VAR long long
#define FUNC(NAME) NAME##64
#define STRUCT(NAME) struct NAME##64

#include "fluxsort.c"

//////////////////////////////////////////////////////////
//┌────────────────────────────────────────────────────┐//
//│  ▄██┐  ██████┐  █████┐    ██████┐ ██████┐████████┐ │//
//│ ████│  └────██┐██┌──██┐   ██┌──██┐└─██┌─┘└──██┌──┘ │//
//│ └─██│   █████┌┘└█████┌┘   ██████┌┘  ██│     ██│    │//
//│   ██│  ██┌───┘ ██┌──██┐   ██┌──██┐  ██│     ██│    │//
//│ ██████┐███████┐└█████┌┘   ██████┌┘██████┐   ██│    │//
//│ └─────┘└──────┘ └────┘    └─────┘ └─────┘   └─┘    │//
//└────────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////

#undef VAR
#undef FUNC
#undef STRUCT

#define VAR long double
#define FUNC(NAME) NAME##128
#define STRUCT(NAME) struct NAME##128

#include "fluxsort.c"

//////////////////////////////////////////////////////////////////////////
//┌────────────────────────────────────────────────────────────────────┐//
//│███████┐██┐     ██┐   ██┐██┐  ██┐███████┐ ██████┐ ██████┐ ████████┐ │//
//│██┌────┘██│     ██│   ██│└██┐██┌┘██┌────┘██┌───██┐██┌──██┐└──██┌──┘ │//
//│█████┐  ██│     ██│   ██│ └███┌┘ ███████┐██│   ██│██████┌┘   ██│    │//
//│██┌──┘  ██│     ██│   ██│ ██┌██┐ └────██│██│   ██│██┌──██┐   ██│    │//
//│██│     ███████┐└██████┌┘██┌┘ ██┐███████│└██████┌┘██│  ██│   ██│    │//
//│└─┘     └──────┘ └─────┘ └─┘  └─┘└──────┘ └─────┘ └─┘  └─┘   └─┘    │//
//└────────────────────────────────────────────────────────────────────┘//
//////////////////////////////////////////////////////////////////////////

void fluxsort(void *array, size_t nmemb, size_t size, CMPFUNC *cmp)
{
	if (nmemb < 2)
	{
		return;
	}

	switch (size)
	{
		case sizeof(char):
			return fluxsort8(array, nmemb, cmp);

		case sizeof(short):
			return fluxsort16(array, nmemb, cmp);

		case sizeof(int):
			return fluxsort32(array, nmemb, cmp);

		case sizeof(long long):
			return fluxsort64(array, nmemb, cmp);

		case sizeof(long double):
			return fluxsort128(array, nmemb, cmp);

		default:
			return assert(size == sizeof(char) || size == sizeof(short) || size == sizeof(int) || size == sizeof(long long) || size == sizeof(long double));
	}
}

#undef VAR
#undef FUNC
#undef STRUCT

#endif
