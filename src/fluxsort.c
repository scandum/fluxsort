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
	fluxsort 1.1.4.3
*/

#define FLUX_OUT 24

size_t FUNC(flux_analyze)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t cnt, balance = 0;
	VAR *pta, *ptb, swap;

	pta = array;
	cnt = nmemb;

	while (--cnt)
	{
		balance += cmp(pta, pta + 1) > 0;
		pta++;
	}

	if (balance == 0)
	{
		return 1;
	}

	if (balance == nmemb - 1)
	{
		pta = array;
		ptb = array + nmemb;

		cnt = nmemb / 2;

		do
		{
			swap = *pta; *pta++ = *--ptb; *ptb = swap;
		}
		while (--cnt);

		return 1;
	}

	if (balance <= nmemb / 6 || balance >= nmemb / 6 * 5)
	{
		quadsort32(array, nmemb, cmp);

		return 1;
	}
	return 0;
}

size_t FUNC(median_of_five)(VAR *array, size_t v0, size_t v1, size_t v2, size_t v3, size_t v4, CMPFUNC *cmp)
{
	unsigned char t[4], val;

	val = cmp(&array[v0], &array[v1]) > 0; t[0]  = val; t[1] = !val;
	val = cmp(&array[v0], &array[v2]) > 0; t[0] += val; t[2] = !val;
	val = cmp(&array[v0], &array[v3]) > 0; t[0] += val; t[3] = !val;
	val = cmp(&array[v0], &array[v4]) > 0; t[0] += val;

	if (t[0] == 2) return v0;

	val = cmp(&array[v1], &array[v2]) > 0; t[1] += val; t[2] += !val;
	val = cmp(&array[v1], &array[v3]) > 0; t[1] += val; t[3] += !val;
	val = cmp(&array[v1], &array[v4]) > 0; t[1] += val;

	if (t[1] == 2) return v1;

	val = cmp(&array[v2], &array[v3]) > 0; t[2] += val; t[3] += !val;
	val = cmp(&array[v2], &array[v4]) > 0; t[2] += val;

	if (t[2] == 2) return v2;

	val = cmp(&array[v3], &array[v4]) > 0; t[3] += val;

	return t[3] == 2 ? v3 : v4;
}

size_t FUNC(median_of_three)(VAR *array, size_t v0, size_t v1, size_t v2, CMPFUNC *cmp)
{
	unsigned char t[2], val;

	val = cmp(&array[v0], &array[v1]) > 0; t[0]  = val; t[1] = !val;
	val = cmp(&array[v0], &array[v2]) > 0; t[0] += val;

	if (t[0] == 1) return v0;

	val = cmp(&array[v1], &array[v2]) > 0; t[1] += val;

	return t[1] == 1 ? v1 : v2;
}

VAR FUNC(median_of_fifteen)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, v3, v4, div = nmemb / 16;

	v0 = FUNC(median_of_three)(array, div * 2, div * 1, div * 3, cmp);
	v1 = FUNC(median_of_three)(array, div * 5, div * 4, div * 6, cmp);
	v2 = FUNC(median_of_three)(array, div * 8, div * 7, div * 9, cmp);
	v3 = FUNC(median_of_three)(array, div * 11, div * 10, div * 12, cmp);
	v4 = FUNC(median_of_three)(array, div * 14, div * 13, div * 15, cmp);

	return array[FUNC(median_of_five)(array, v2, v0, v1, v3, v4, cmp)];
}

VAR FUNC(median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, div = nmemb / 16;

	v0 = FUNC(median_of_three)(array, div * 2, div * 1, div * 4, cmp);
	v1 = FUNC(median_of_three)(array, div * 8, div * 6, div * 10, cmp);
	v2 = FUNC(median_of_three)(array, div * 14, div * 12, div * 15, cmp);

	return array[FUNC(median_of_three)(array, v1, v0, v2, cmp)];
}

void FUNC(flux_partition)(VAR *array, VAR *swap, VAR *ptx, size_t nmemb, CMPFUNC *cmp)
{
	unsigned char val;
	size_t a_size, s_size;
	VAR *pta, *pts, *pte, piv;

	if (nmemb > 1024)
	{
		piv = FUNC(median_of_fifteen)(ptx, nmemb, cmp);
	}
	else
	{
		piv = FUNC(median_of_nine)(ptx, nmemb, cmp);
	}

	pte = ptx + nmemb;

	pta = array;
	pts = swap;

	while (ptx + 8 < pte)
	{
		val = cmp(ptx + 0, &piv) <= 0; *pta = ptx[0]; pta += val; *pts = ptx[0]; pts += !val;
		val = cmp(ptx + 1, &piv) <= 0; *pta = ptx[1]; pta += val; *pts = ptx[1]; pts += !val;
		val = cmp(ptx + 2, &piv) <= 0; *pta = ptx[2]; pta += val; *pts = ptx[2]; pts += !val;
		val = cmp(ptx + 3, &piv) <= 0; *pta = ptx[3]; pta += val; *pts = ptx[3]; pts += !val;
		val = cmp(ptx + 4, &piv) <= 0; *pta = ptx[4]; pta += val; *pts = ptx[4]; pts += !val;
		val = cmp(ptx + 5, &piv) <= 0; *pta = ptx[5]; pta += val; *pts = ptx[5]; pts += !val;
		val = cmp(ptx + 6, &piv) <= 0; *pta = ptx[6]; pta += val; *pts = ptx[6]; pts += !val;
		val = cmp(ptx + 7, &piv) <= 0; *pta = ptx[7]; pta += val; *pts = ptx[7]; pts += !val;

		ptx += 8;
	}

	while (ptx < pte)
	{
		val = cmp(ptx, &piv) <= 0;
		*pta = *ptx; pta += val;
		*pts = *ptx; pts += !val;

		ptx++;
	}

	s_size = pts - swap;
	a_size = nmemb - s_size;

	if (a_size <= s_size / 16 || s_size <= FLUX_OUT)
	{
		memcpy(pta, swap, s_size * sizeof(VAR));
		FUNC(quadsort_swap)(pta, swap, s_size, cmp);
	}
	else
	{
		FUNC(flux_partition)(pta, swap, swap, s_size, cmp);
	}

	if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
	{
		FUNC(quadsort_swap)(array, swap, a_size, cmp);
	}
	else
	{
		FUNC(flux_partition)(array, swap, array, a_size, cmp);
	}
}

void FUNC(fluxsort)(void *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (FUNC(flux_analyze)(array, nmemb, cmp) == 0)
	{
		VAR *swap = malloc(nmemb * sizeof(VAR));

		FUNC(flux_partition)(array, swap, array, nmemb, cmp);

		free(swap);
	}
}
