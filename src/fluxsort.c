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
	fluxsort 1.1.4.4
*/

#define FLUX_OUT 24

size_t FUNC(flux_analyze)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	char loop, dist, last = -1;
	size_t cnt, balance = 0, streaks = 0;
	VAR *pta, *ptb, swap;

	pta = array;

	for (cnt = nmemb ; cnt > 16 ; cnt -= 16)
	{
		for (dist = 0, loop = 16 ; loop ; loop--)
		{
			dist += cmp(pta, pta + 1) > 0; pta++;
		}
		streaks += dist == last; last = dist;
		balance += dist;
	}

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

	if (streaks > nmemb / 20)
	{
		FUNC(quadsort)(array, nmemb, cmp);

		return 1;
	}

	if (balance <= nmemb / 6 || balance >= nmemb - nmemb / 6)
	{
		FUNC(quadsort)(array, nmemb, cmp);

		return 1;
	}

	return 0;
}

void FUNC(fluxsort)(VAR *array, size_t nmemb, CMPFUNC *cmp);

size_t FUNC(median_of_sqrt)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	VAR *pta, swap[256];
	size_t cnt, sqrt, div;

	for (sqrt = 256 ; sqrt * sqrt * 4 > nmemb ; sqrt /= 2);

	div = nmemb / sqrt;

	pta = array + rand() % sqrt;

	for (cnt = 0 ; cnt < sqrt ; cnt++)
	{
		swap[cnt] = pta[0];

		pta += div;
	}
	FUNC(fluxsort)(swap, sqrt, cmp);

	return swap[sqrt / 2];
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

VAR FUNC(median_of_twentyfive)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, v3, v4, div = nmemb / 64;

	v0 = FUNC(median_of_five)(array, div *  4, div *  1, div *  2, div *  8, div * 10, cmp);
	v1 = FUNC(median_of_five)(array, div * 16, div * 12, div * 14, div * 18, div * 20, cmp);
	v2 = FUNC(median_of_five)(array, div * 32, div * 24, div * 30, div * 34, div * 38, cmp);
	v3 = FUNC(median_of_five)(array, div * 48, div * 42, div * 44, div * 50, div * 52, cmp);
	v4 = FUNC(median_of_five)(array, div * 60, div * 54, div * 56, div * 62, div * 63, cmp);

	return array[FUNC(median_of_five)(array, v2, v0, v1, v3, v4, cmp)];
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

VAR FUNC(median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t v0, v1, v2, div = nmemb / 16;

	v0 = FUNC(median_of_three)(array, div * 2, div * 1, div * 4, cmp);
	v1 = FUNC(median_of_three)(array, div * 8, div * 6, div * 10, cmp);
	v2 = FUNC(median_of_three)(array, div * 14, div * 12, div * 15, cmp);

	return array[FUNC(median_of_three)(array, v1, v0, v2, cmp)];
}

// As per suggestion by Marshall Lochbaum to improve generic data handling

void FUNC(flux_reverse_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *ptp, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size, s_size;

	{
		size_t cnt, val, m;
		VAR *pta, *pts, piv;

		piv = *ptp;

		pta = array;
		pts = swap;

		for (m = 0, cnt = nmemb / 8 ; cnt ; cnt--)
		{
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
		}

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			val = cmp(&piv, ptx) > 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
		}
		a_size = m;
		s_size = nmemb - a_size;
	}
	memcpy(array + a_size, swap, s_size * sizeof(VAR));
	FUNC(quadsort_swap)(array, swap, a_size, cmp);
}

void FUNC(flux_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *ptp, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size, s_size;

	{
		size_t cnt, val, m;
		VAR *pta, *pts, piv;

		if (nmemb > 1024)
		{
			if (nmemb > 65536)
			{
				piv = FUNC(median_of_sqrt)(ptx, nmemb, cmp);
			}
			else
			{
				piv = FUNC(median_of_twentyfive)(ptx, nmemb, cmp);
			}
		}
		else
		{
			piv = FUNC(median_of_nine)(ptx, nmemb, cmp);
		}

		if (ptx == array && swap + nmemb < ptp && cmp(ptp, &piv) <= 0 && cmp(&piv, ptp) <= 0)
		{
			return FUNC(flux_reverse_partition)(array, swap, array, ptp, nmemb, cmp);
		}
		*--ptp = piv;

		pta = array;
		pts = swap;

		for (m = 0, cnt = nmemb / 8 ; cnt ; cnt--)
		{
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
		}

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			val = cmp(ptx, &piv) <= 0; pts[-m] = pta[m] = *ptx++; m += val; pts++;
		}

		a_size = m;
		s_size = nmemb - a_size;
	}

	if (a_size <= s_size / 16 || s_size <= FLUX_OUT)
	{
		if (s_size == 0)
		{
			return FUNC(flux_reverse_partition)(array, swap, array, ptp, a_size, cmp);
		}
		memcpy(array + a_size, swap, s_size * sizeof(VAR));
		FUNC(quadsort_swap)(array + a_size, swap, s_size, cmp);
	}
	else
	{
		FUNC(flux_partition)(array + a_size, swap, swap, ptp, s_size, cmp);
	}

	if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
	{
		FUNC(quadsort_swap)(array, swap, a_size, cmp);
	}
	else
	{
		FUNC(flux_partition)(array, swap, array, ptp, a_size, cmp);
	}
}

void FUNC(fluxsort)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (FUNC(flux_analyze)(array, nmemb, cmp) == 0)
	{
		VAR *swap = malloc(nmemb * sizeof(VAR));

		if (swap == NULL)
		{
			return FUNC(quadsort)(array, nmemb, cmp);
		}
		FUNC(flux_partition)(array, swap, array, swap + nmemb, nmemb, cmp);

		free(swap);
	}
}

void FUNC(fluxsort_swap)(VAR *array, VAR *swap, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else if (FUNC(flux_analyze)(array, nmemb, cmp) == 0)
	{
		FUNC(flux_partition)(array, swap, array, swap + nmemb, nmemb, cmp);
	}
}
