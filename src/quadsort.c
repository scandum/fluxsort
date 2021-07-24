/*
	Copyright (C) 2014-2021 Igor van den Hoven ivdhoven@gmail.com
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
	quadsort 1.1.4.3
*/

void FUNC(tail_swap)(VAR *array, unsigned char nmemb, CMPFUNC *cmp)
{
	VAR *pta, *end, *ptt, tmp;
	unsigned char i, top;

	switch (nmemb)
	{
		case 0:
		case 1:
			return;

		case 2:
			swap_two(array, tmp);
			return;

		case 3:
			swap_three(array, tmp);
			return;

		case 4:
			swap_four(array, tmp);
			return;

		case 5:
			swap_four(array, tmp);
			swap_five(array, pta, ptt, end, tmp, cmp);
			return;

		case 6:
			swap_four(array, tmp);
			swap_six(array, pta, ptt, end, tmp, cmp);
			return;

		case 7:
			swap_four(array, tmp);
			swap_seven(array, pta, ptt, end, tmp, cmp);
			return;
		case 8:
			swap_four(array, tmp);
			swap_eight(array, pta, ptt, end, tmp, cmp);
			return;
	}
	swap_four(array, tmp);
	swap_eight(array, pta, ptt, end, tmp, cmp);

	for (i = 8 ; i < nmemb ; i++)
	{
		pta = end = array + i;

		if (cmp(--pta, end) <= 0)
		{
			continue;
		}

		tmp = *end;

		if (cmp(array, &tmp) > 0)
		{
			top = i;

			do
			{
				*end-- = *pta--;
			}
			while (--top);

			*end = tmp;
		}
		else
		{
			do
			{
				*end-- = *pta--;
			}
			while (cmp(pta, &tmp) > 0);

			*end = tmp;
		}
	}
}

void FUNC(tail_merge)(VAR *array, VAR *swap, unsigned int nmemb, unsigned int block, CMPFUNC *cmp);
void FUNC(quad_merge_block)(VAR *array, VAR *swap, unsigned int block, CMPFUNC *cmp);
void FUNC(parity_merge_thirtytwo)(VAR *array, VAR *swap, CMPFUNC *cmp);

unsigned int FUNC(quad_swap)(VAR *array, unsigned int nmemb, CMPFUNC *cmp)
{
	VAR swap[32];
	unsigned int count, reverse;
	VAR *pta, *pts, *ptt, *pte, tmp;

	pta = array;

	count = nmemb / 4;

	count &= ~1;

	while (count--)
	{
		if (cmp(&pta[0], &pta[1]) > 0)
		{
			if (cmp(&pta[2], &pta[3]) > 0)
			{
				if (cmp(&pta[1], &pta[2]) > 0)
				{
					pts = pta;
					pta += 4;
					goto swapper;
				}
				tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}
			tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
		}
		else if (cmp(&pta[2], &pta[3]) > 0)
		{
			tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
		}

		if (cmp(&pta[1], &pta[2]) > 0)
		{
			if (cmp(&pta[0], &pta[2]) <= 0)
			{
				if (cmp(&pta[1], &pta[3]) <= 0)
				{
					tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
				}
				else
				{
					tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
			}
			else if (cmp(&pta[0], &pta[3]) > 0)
			{
				tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp; tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
			}
			else if (cmp(&pta[1], &pta[3]) <= 0)
			{
				tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
			}
			else
			{
				tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}
		}
		count--;

		pts = pta;

		swap_eight(pts, ptt, pte, pta, tmp, cmp);

		continue;

		swapper:

		if (count--)
		{
			if (cmp(&pta[0], &pta[1]) > 0)
			{
				if (cmp(&pta[2], &pta[3]) > 0)
				{
					if (cmp(&pta[1], &pta[2]) > 0)
					{
						if (cmp(&pta[-1], &pta[0]) > 0)
						{
							pta += 4;

							goto swapper;
						}
					}
					tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
				tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
			}
			else if (cmp(&pta[2], &pta[3]) > 0)
			{
				tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}

			if (cmp(&pta[1], &pta[2]) > 0)
			{
				if (cmp(&pta[0], &pta[2]) <= 0)
				{
					if (cmp(&pta[1], &pta[3]) <= 0)
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
					}
					else
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
					}
				}
				else if (cmp(&pta[0], &pta[3]) > 0)
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp; tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp;
				}
				else if (cmp(&pta[1], &pta[3]) <= 0)
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = pta[1]; pta[1] = tmp;
				}
				else
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = pta[3]; pta[3] = pta[1]; pta[1] = tmp;
				}
			}
			ptt = pta - 1;

			reverse = (ptt - pts) / 2;

			do
			{
				tmp = *pts; *pts++ = *ptt; *ptt-- = tmp;
			}
			while (reverse--);

			pts = count & 1 ? pta : pta - 4;

			count &= ~1;

			swap_eight(pts, ptt, pte, pta, tmp, cmp);

			continue;
		}

		if (pts == array)
		{
			switch (nmemb & 7)
			{
				case 7:
					if (cmp(&pta[5], &pta[6]) <= 0)
					{
						break;
					}
				case 6:
					if (cmp(&pta[4], &pta[5]) <= 0)
					{
						break;
					}
				case 5:
					if (cmp(&pta[3], &pta[4]) <= 0)
					{
						break;
					}
				case 4:
					if (cmp(&pta[2], &pta[3]) <= 0)
					{
						break;
					}					
				case 3:
					if (cmp(&pta[1], &pta[2]) <= 0)
					{
						break;
					}
				case 2:
					if (cmp(&pta[0], &pta[1]) <= 0)
					{
						break;
					}
				case 1:
					if (cmp(&pta[-1], &pta[0]) <= 0)
					{
						break;
					}
				case 0:
					ptt = pts + nmemb - 1;

					reverse = (ptt - pts) / 2;

					do
					{
						tmp = *pts; *pts++ = *ptt; *ptt-- = tmp;
					}
					while (reverse--);

					return 1;
			}
		}
		ptt = pta - 1;

		reverse = (ptt - pts) / 2;

		do
		{
			tmp = *pts; *pts++ = *ptt; *ptt-- = tmp;
		}
		while (reverse--);

		break;
	}

	FUNC(tail_swap)(pta, nmemb & 7, cmp);

	pta = array;

	count = nmemb / 32;

	while (count--)
	{
		FUNC(parity_merge_thirtytwo)(pta, swap, cmp);

		pta += 32;
	}

	if ((nmemb & 31) > 8)
	{
		FUNC(tail_merge)(pta, swap, nmemb & 31, 8, cmp);
	}

	return 0;
}

void FUNC(parity_merge_eight)(VAR *dest, VAR *from, CMPFUNC *cmp)
{
	VAR *ptl, *ptr;

	ptl = from;
	ptr = from + 8;

	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest = cmp(ptl, ptr) <= 0 ? *ptl : *ptr;

	ptl = from + 7;
	ptr = from + 15;

	dest += 8;

	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest = cmp(ptl, ptr) > 0 ? *ptl : *ptr;
}

void FUNC(parity_merge_sixteen)(VAR *dest, VAR *from, CMPFUNC *cmp)
{
	VAR *ptl, *ptr;

	ptl = from;
	ptr = from + 16;

	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest++ = cmp(ptl, ptr) <= 0 ? *ptl++ : *ptr++;
	*dest = cmp(ptl, ptr) <= 0 ? *ptl : *ptr;

	ptl = from + 15;
	ptr = from + 31;

	dest += 16;

	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest-- = cmp(ptl, ptr) > 0 ? *ptl-- : *ptr--;
	*dest = cmp(ptl, ptr) > 0 ? *ptl : *ptr;
}

void FUNC(parity_merge_thirtytwo)(VAR *array, VAR *swap, CMPFUNC *cmp)
{
	if (cmp(array + 7, array + 8) <= 0 && cmp(array + 15, array + 16) <= 0 && cmp(array + 23, array + 24) <= 0)
	{
		return;
	}
	FUNC(parity_merge_eight)(swap, array, cmp);
	FUNC(parity_merge_eight)(swap + 16, array + 16, cmp);

	FUNC(parity_merge_sixteen)(array, swap, cmp);
}

void FUNC(forward_merge)(VAR *dest, VAR *from, size_t block, CMPFUNC *cmp)
{
	VAR *l, *r, *m, *e; // left, right, middle, end

	l = from;
	r = from + block;
	m = r;
	e = r + block;

	if (cmp(m - 1, e - 1) <= 0)
	{
		do
		{
			if (cmp(l, r) <= 0)
			{
				*dest++ = *l++;
				continue;
			}
			*dest++ = *r++;
			if (cmp(l, r) <= 0)
			{
				*dest++ = *l++;
				continue;
			}
			*dest++ = *r++;
			if (cmp(l, r) <= 0)
			{
				*dest++ = *l++;
				continue;
			}
			*dest++ = *r++;
		}
		while (l < m);

		do *dest++ = *r++; while (r < e);
	}
	else
	{
		do
		{
			if (cmp(l, r) > 0)
			{
				*dest++ = *r++;
				continue;
			}
			*dest++ = *l++;
			if (cmp(l, r) > 0)
			{
				*dest++ = *r++;
				continue;
			}
			*dest++ = *l++;
			if (cmp(l, r) > 0)
			{
				*dest++ = *r++;
				continue;
			}
			*dest++ = *l++;
		}
		while (r < e);

		do *dest++ = *l++; while (l < m);
	}
}

// main memory: [A][B][C][D]
// swap memory: [A  B]       step 1
// swap memory: [A  B][C  D] step 2
// main memory: [A  B  C  D] step 3

void FUNC(quad_merge_block)(VAR *array, VAR *swap, unsigned int block, CMPFUNC *cmp)
{
	register VAR *pts, *c, *c_max;
	unsigned int block_x_2 = block * 2;

	c_max = array + block;

	if (cmp(c_max - 1, c_max) <= 0)
	{
		c_max += block_x_2;

		if (cmp(c_max - 1, c_max) <= 0)
		{
			c_max -= block;

			if (cmp(c_max - 1, c_max) <= 0)
			{
				return;
			}
			pts = swap;

			c = array;

			do *pts++ = *c++; while (c < c_max); // step 1

			c_max = c + block_x_2;

			do *pts++ = *c++; while (c < c_max); // step 2

			return FUNC(forward_merge)(array, swap, block_x_2, cmp); // step 3
		}
		pts = swap;

		c = array;
		c_max = array + block_x_2;

		do *pts++ = *c++; while (c < c_max); // step 1
	}
	else
	{
		FUNC(forward_merge)(swap, array, block, cmp); // step 1
	}
	FUNC(forward_merge)(swap + block_x_2, array + block_x_2, block, cmp); // step 2

	FUNC(forward_merge)(array, swap, block_x_2, cmp); // step 3
}

void FUNC(quad_merge)(VAR *array, VAR *swap, unsigned int nmemb, unsigned int block, CMPFUNC *cmp)
{
	register VAR *pta, *pte;

	pte = array + nmemb;

	block *= 4;

	while (block * 2 <= nmemb)
	{
		pta = array;

		do
		{
			FUNC(quad_merge_block)(pta, swap, block / 4, cmp);

			pta += block;
		}
		while (pta + block <= pte);

		FUNC(tail_merge)(pta, swap, pte - pta, block / 4, cmp);

		block *= 4;
	}
	FUNC(tail_merge)(array, swap, nmemb, block / 4, cmp);
}

void FUNC(partial_backward_merge)(VAR *array, VAR *swap, size_t nmemb, size_t block, CMPFUNC *cmp)
{
	VAR *r, *m, *e, *s; // right, middle, end, swap

	m = array + block;
	e = array + nmemb - 1;
	r = m--;

	if (cmp(m, r) <= 0)
	{
		return;
	}

	while (cmp(m, e) <= 0)
	{
		e--;
	}

	s = swap;

	do *s++ = *r++; while (r <= e);

	s--;

	*e-- = *m--;

	if (cmp(array, swap) <= 0)
	{
		do
		{
			while (cmp(m, s) > 0)
			{
				*e-- = *m--;
			}
			*e-- = *s--;
		}
		while (s >= swap);
	}
	else
	{
		do
		{
			if (cmp(m, s) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
			if (cmp(m, s) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
			if (cmp(m, s) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
		}
		while (m >= array);

		do *e-- = *s--; while (s >= swap);
	}
}

void FUNC(tail_merge)(VAR *array, VAR *swap, unsigned int nmemb, unsigned int block, CMPFUNC *cmp)
{
	register VAR *pta, *pte;

	pte = array + nmemb;

	while (block < nmemb)
	{
		pta = array;

		for (pta = array ; pta + block < pte ; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				FUNC(partial_backward_merge)(pta, swap, block * 2, block, cmp);

				continue;
			}
			FUNC(partial_backward_merge)(pta, swap, pte - pta, block, cmp);

			break;
		}
		block *= 2;
	}
}

///////////////////////////////////////////////////////////////////////////////
//┌─────────────────────────────────────────────────────────────────────────┐//
//│    ██████┐ ██┐   ██┐ █████┐ ██████┐ ███████┐ ██████┐ ██████┐ ████████┐  │//
//│   ██┌───██┐██│   ██│██┌──██┐██┌──██┐██┌────┘██┌───██┐██┌──██┐└──██┌──┘  │//
//│   ██│   ██│██│   ██│███████│██│  ██│███████┐██│   ██│██████┌┘   ██│     │//
//│   ██│▄▄ ██│██│   ██│██┌──██│██│  ██│└────██│██│   ██│██┌──██┐   ██│     │//
//│   └██████┌┘└██████┌┘██│  ██│██████┌┘███████│└██████┌┘██│  ██│   ██│     │//
//│    └──▀▀─┘  └─────┘ └─┘  └─┘└─────┘ └──────┘ └─────┘ └─┘  └─┘   └─┘     │//
//└─────────────────────────────────────────────────────────────────────────┘//
///////////////////////////////////////////////////////////////////////////////

void FUNC(quadsort)(void *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (nmemb < 256)
	{
		if (FUNC(quad_swap)(array, nmemb, cmp) == 0)
		{
			VAR swap[128];

			FUNC(tail_merge)(array, swap, nmemb, 32, cmp);
		}
	}
	else if (FUNC(quad_swap)(array, nmemb, cmp) == 0)
	{
		VAR *swap = malloc(nmemb * sizeof(VAR) / 2);

		if (swap == NULL)
		{
			fprintf(stderr, "quadsort(%p,%zu,%p): malloc() failed: %s\n", array, nmemb, cmp, strerror(errno));

			return;
		}

		FUNC(quad_merge)(array, swap, nmemb, 32, cmp);

		free(swap);
	}
}

void FUNC(quadsort_swap)(VAR *array, VAR *swap, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (nmemb < 256)
	{
		if (FUNC(quad_swap)(array, nmemb, cmp) == 0)
		{
			FUNC(tail_merge)(array, swap, nmemb, 32, cmp);
		}
	}
	else if (FUNC(quad_swap)(array, nmemb, cmp) == 0)
	{
		FUNC(quad_merge)(array, swap, nmemb, 32, cmp);
	}
}
