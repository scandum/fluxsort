// fluxsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#define FLUX_OUT 96

void FUNC(flux_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *ptp, size_t nmemb, CMPFUNC *cmp);

// Determine whether to use mergesort or quicksort

void FUNC(flux_analyze)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	unsigned char loop, asum, bsum, csum, dsum;
	unsigned int astreaks, bstreaks, cstreaks, dstreaks;
	size_t quad1, quad2, quad3, quad4, half1, half2;
	size_t cnt, abalance, bbalance, cbalance, dbalance;
	VAR *pta, *ptb, *ptc, *ptd;

	half1 = nmemb / 2;
	quad1 = half1 / 2;
	quad2 = half1 - quad1;
	half2 = nmemb - half1;
	quad3 = half2 / 2;
	quad4 = half2 - quad3;

	pta = array;
	ptb = array + quad1;
	ptc = array + half1;
	ptd = array + half1 + quad3;

	astreaks = bstreaks = cstreaks = dstreaks = 0;
	abalance = bbalance = cbalance = dbalance = 0;

	if (quad1 < quad2) {bbalance += cmp(ptb, ptb + 1) > 0; ptb++;}
	if (quad1 < quad3) {cbalance += cmp(ptc, ptc + 1) > 0; ptc++;}
	if (quad1 < quad4) {dbalance += cmp(ptd, ptd + 1) > 0; ptd++;}

	for (cnt = nmemb ; cnt > 132 ; cnt -= 128)
	{
		for (asum = bsum = csum = dsum = 0, loop = 32 ; loop ; loop--)
		{
			asum += cmp(pta, pta + 1) > 0; pta++;
			bsum += cmp(ptb, ptb + 1) > 0; ptb++;
			csum += cmp(ptc, ptc + 1) > 0; ptc++;
			dsum += cmp(ptd, ptd + 1) > 0; ptd++;
		}
		abalance += asum; astreaks += asum = (asum == 0) | (asum == 32);
		bbalance += bsum; bstreaks += bsum = (bsum == 0) | (bsum == 32);
		cbalance += csum; cstreaks += csum = (csum == 0) | (csum == 32);
		dbalance += dsum; dstreaks += dsum = (dsum == 0) | (dsum == 32);

		if (cnt > 516 && asum + bsum + csum + dsum == 0)
		{
			abalance += 48; pta += 96;
			bbalance += 48; ptb += 96;
			cbalance += 48; ptc += 96;
			dbalance += 48; ptd += 96;
			cnt -= 384;
		}
	}

	for ( ; cnt > 7 ; cnt -= 4)
	{
		abalance += cmp(pta, pta + 1) > 0; pta++;
		bbalance += cmp(ptb, ptb + 1) > 0; ptb++;
		cbalance += cmp(ptc, ptc + 1) > 0; ptc++;
		dbalance += cmp(ptd, ptd + 1) > 0; ptd++;
	}

	cnt = abalance + bbalance + cbalance + dbalance;

	if (cnt == 0)
	{
		if (cmp(pta, pta + 1) <= 0 && cmp(ptb, ptb + 1) <= 0 && cmp(ptc, ptc + 1) <= 0)
		{
			return;
		}
	}

	asum = quad1 - abalance == 1;
	bsum = quad2 - bbalance == 1;
	csum = quad3 - cbalance == 1;
	dsum = quad4 - dbalance == 1;

	if (asum | bsum | csum | dsum)
	{
		unsigned char span1 = (asum && bsum) * (cmp(pta, pta + 1) > 0);
		unsigned char span2 = (bsum && csum) * (cmp(ptb, ptb + 1) > 0);
		unsigned char span3 = (csum && dsum) * (cmp(ptc, ptc + 1) > 0);

		switch (span1 | span2 * 2 | span3 * 4)
		{
			case 0: break;
			case 1: FUNC(quad_reversal)(array, ptb);   abalance = bbalance = 0; break;
			case 2: FUNC(quad_reversal)(pta + 1, ptc); bbalance = cbalance = 0; break;
			case 3: FUNC(quad_reversal)(array, ptc);   abalance = bbalance = cbalance = 0; break;
			case 4: FUNC(quad_reversal)(ptb + 1, ptd); cbalance = dbalance = 0; break;
			case 5: FUNC(quad_reversal)(array, ptb);
				FUNC(quad_reversal)(ptb + 1, ptd); abalance = bbalance = cbalance = dbalance = 0; break;
			case 6: FUNC(quad_reversal)(pta + 1, ptd); bbalance = cbalance = dbalance = 0; break;
			case 7: FUNC(quad_reversal)(array, ptd); return;
		}
		if (asum && abalance) {FUNC(quad_reversal)(array,   pta); abalance = 0;}
		if (bsum && bbalance) {FUNC(quad_reversal)(pta + 1, ptb); bbalance = 0;}
		if (csum && cbalance) {FUNC(quad_reversal)(ptb + 1, ptc); cbalance = 0;}
		if (dsum && dbalance) {FUNC(quad_reversal)(ptc + 1, ptd); dbalance = 0;}
	}

#ifdef cmp
	cnt = nmemb / 256; // switch to quadsort if at least 50% ordered
#else
	cnt = nmemb / 512; // switch to quadsort if at least 25% ordered
#endif
	asum = astreaks > cnt;
	bsum = bstreaks > cnt;
	csum = cstreaks > cnt;
	dsum = dstreaks > cnt;

#ifndef cmp
	if (quad1 > QUAD_CACHE)
	{
		asum = bsum = csum = dsum = 1;
	}
#endif

	switch (asum + bsum * 2 + csum * 4 + dsum * 8)
	{
		case 0:
			FUNC(flux_partition)(array, swap, array, swap + nmemb, nmemb, cmp);
			return;
		case 1:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			FUNC(flux_partition)(pta + 1, swap, pta + 1, swap + quad2 + half2, quad2 + half2, cmp);
			break;
		case 2:
			FUNC(flux_partition)(array, swap, array, swap + quad1, quad1, cmp);
			if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			FUNC(flux_partition)(ptb + 1, swap, ptb + 1, swap + half2, half2, cmp);
			break;
		case 3:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			FUNC(flux_partition)(ptb + 1, swap, ptb + 1, swap + half2, half2, cmp);
			break;
		case 4:
			FUNC(flux_partition)(array, swap, array, swap + half1, half1, cmp);
			if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			FUNC(flux_partition)(ptc + 1, swap, ptc + 1, swap + quad4, quad4, cmp);
			break;
		case 8:
			FUNC(flux_partition)(array, swap, array, swap + half1 + quad3, half1 + quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 9:
			if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			FUNC(flux_partition)(pta + 1, swap, pta + 1, swap + quad2 + quad3, quad2 + quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 12:
			FUNC(flux_partition)(array, swap, array, swap + half1, half1, cmp);
			if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 5:
		case 6:
		case 7:
		case 10:
		case 11:
		case 13:
		case 14:
		case 15:
			if (asum)
			{
				if (abalance) FUNC(quadsort_swap)(array, swap, swap_size, quad1, cmp);
			}
			else FUNC(flux_partition)(array, swap, array, swap + quad1, quad1, cmp);
			if (bsum)
			{
				if (bbalance) FUNC(quadsort_swap)(pta + 1, swap, swap_size, quad2, cmp);
			}
			else FUNC(flux_partition)(pta + 1, swap, pta + 1, swap + quad2, quad2, cmp);
			if (csum)
			{
				if (cbalance) FUNC(quadsort_swap)(ptb + 1, swap, swap_size, quad3, cmp);
			}
			else FUNC(flux_partition)(ptb + 1, swap, ptb + 1, swap + quad3, quad3, cmp);
			if (dsum)
			{
				if (dbalance) FUNC(quadsort_swap)(ptc + 1, swap, swap_size, quad4, cmp);
			}
			else FUNC(flux_partition)(ptc + 1, swap, ptc + 1, swap + quad4, quad4, cmp);
			break;
	}

	if (cmp(pta, pta + 1) <= 0)
	{
		if (cmp(ptc, ptc + 1) <= 0)
		{
			if (cmp(ptb, ptb + 1) <= 0)
			{
				return;
			}
			memcpy(swap, array, nmemb * sizeof(VAR));
		}
		else
		{
			FUNC(cross_merge)(swap + half1, array + half1, quad3, quad4, cmp);
			memcpy(swap, array, half1 * sizeof(VAR));
		}
	}
	else
	{
		if (cmp(ptc, ptc + 1) <= 0)
		{
			memcpy(swap + half1, array + half1, half2 * sizeof(VAR));
			FUNC(cross_merge)(swap, array, quad1, quad2, cmp);
		}
		else
		{
			FUNC(cross_merge)(swap + half1, ptb + 1, quad3, quad4, cmp);
			FUNC(cross_merge)(swap, array, quad1, quad2, cmp);
		}
	}
	FUNC(cross_merge)(array, swap, half1, half2, cmp);
}

// The next 4 functions are used for pivot selection

VAR FUNC(binary_median)(VAR *pta, VAR *ptb, size_t len, CMPFUNC *cmp)
{
	while (len /= 2)
	{
		if (cmp(pta + len, ptb + len) <= 0) pta += len; else ptb += len;
	}
	return cmp(pta, ptb) > 0 ? *pta : *ptb;
}

void FUNC(trim_four)(VAR *pta, CMPFUNC *cmp)
{
	VAR swap;
	size_t x;

	x = cmp(pta, pta + 1)  > 0; swap = pta[!x]; pta[0] = pta[x]; pta[1] = swap; pta += 2;
	x = cmp(pta, pta + 1)  > 0; swap = pta[!x]; pta[0] = pta[x]; pta[1] = swap; pta -= 2;

	x = (cmp(pta, pta + 2) <= 0) * 2; pta[2] = pta[x]; pta++;
	x = (cmp(pta, pta + 2)  > 0) * 2; pta[0] = pta[x];
}

VAR FUNC(median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	VAR *pta, swap[9];
	size_t x, y, z;

	z = nmemb / 9;

	pta = array;

	for (x = 0 ; x < 9 ; x++)
	{
		swap[x] = *pta;

		pta += z;
	}

	FUNC(trim_four)(swap, cmp);
	FUNC(trim_four)(swap + 4, cmp);

	swap[0] = swap[5];
	swap[3] = swap[8];

	FUNC(trim_four)(swap, cmp);

	swap[0] = swap[6];

	x = cmp(swap + 0, swap + 1) > 0;
	y = cmp(swap + 0, swap + 2) > 0;
	z = cmp(swap + 1, swap + 2) > 0;

	return swap[(x == y) + (y ^ z)];
}

VAR FUNC(median_of_cbrt)(VAR *array, VAR *swap, VAR *ptx, size_t nmemb, int *generic, CMPFUNC *cmp)
{
	VAR *pta, *pts;
	size_t cnt, div, cbrt;

	for (cbrt = 32 ; nmemb > cbrt * cbrt * cbrt ; cbrt *= 2) {}

	div = nmemb / cbrt;

	pta = ptx + (size_t) &div / 16 % div;
	pts = ptx == array ? swap : array;

	for (cnt = 0 ; cnt < cbrt ; cnt++)
	{
		pts[cnt] = *pta;

		pta += div;
	}
	cbrt /= 2;

	FUNC(quadsort_swap)(pts, pts + cbrt * 2, cbrt, cbrt, cmp);
	FUNC(quadsort_swap)(pts + cbrt, pts + cbrt * 2, cbrt, cbrt, cmp);

	*generic = (cmp(pts + cbrt * 2 - 1, pts) <= 0) & (cmp(pts + cbrt - 1, pts) <= 0);

	return FUNC(binary_median)(pts, pts + cbrt, cbrt, cmp);
}

// As per suggestion by Marshall Lochbaum to improve generic data handling by mimicking dual-pivot quicksort

void FUNC(flux_reverse_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size, s_size;

#if !defined __clang__
	{
		size_t cnt, m, val;
		VAR *pts = swap;

		for (m = 0, cnt = nmemb / 8 ; cnt ; cnt--)
		{
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
		}

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
		}
		a_size = m;
		s_size = nmemb - a_size;
	}
#else
	{
		size_t cnt;
		VAR *tmp, *pta = array, *pts = swap;

		for (cnt = nmemb / 8 ; cnt ; cnt--)
		{
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
		}

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			tmp = cmp(piv, ptx) > 0 ? pta++ : pts++; *tmp = *ptx++;
		}
		a_size = pta - array;
		s_size = pts - swap;
	}
#endif
	memcpy(array + a_size, swap, s_size * sizeof(VAR));

	if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
	{
		FUNC(quadsort_swap)(array, swap, a_size, a_size, cmp);
		return;
	}
	FUNC(flux_partition)(array, swap, array, piv, a_size, cmp);
}

size_t FUNC(flux_default_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t run = 0, a = 0, m = 0;

#if !defined __clang__
	size_t val;

	for (a = 8 ; a <= nmemb ; a += 8)
	{
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;

		if (m == a) run = a;
	}

	for (a = nmemb % 8 ; a ; a--)
	{
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
	}
	swap -= nmemb;
#else
	VAR *tmp, *pta = array, *pts = swap;

	for (a = 8 ; a <= nmemb ; a += 8)
	{
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;

		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;

		if (pta == array || pts == swap) run = a;
	}

	for (a = nmemb % 8 ; a ; a--)
	{
		tmp = cmp(ptx, piv) <= 0 ? pta++ : pts++; *tmp = *ptx++;
	}
	m = pta - array;
#endif

	if (run <= nmemb / 4)
	{
		return m;
	}

	if (m == nmemb)
	{
		return m;
	}

	a = nmemb - m;

	memcpy(array + m, swap, a * sizeof(VAR));

	FUNC(quadsort_swap)(array + m, swap, a, a, cmp);
	FUNC(quadsort_swap)(array, swap, m, m, cmp);

	return 0;
}

void FUNC(flux_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size = 0, s_size;
	int generic = 0;

	while (1)
	{
		--piv;

		if (nmemb <= 2048)
		{
			*piv = FUNC(median_of_nine)(ptx, nmemb, cmp);
		}
		else
		{
			*piv = FUNC(median_of_cbrt)(array, swap, ptx, nmemb, &generic, cmp);

			if (generic)
			{
				if (ptx == swap)
				{
					memcpy(array, swap, nmemb * sizeof(VAR));
				}
				FUNC(quadsort_swap)(array, swap, nmemb, nmemb, cmp);
				return;
			}
		}

		if (a_size && cmp(piv + 1, piv) <= 0)
		{
			FUNC(flux_reverse_partition)(array, swap, array, piv, nmemb, cmp);
			return;
		}
		a_size = FUNC(flux_default_partition)(array, swap, ptx, piv, nmemb, cmp);
		s_size = nmemb - a_size;

		if (a_size <= s_size / 32 || s_size <= FLUX_OUT)
		{
			if (a_size == 0)
			{
				return;
			}
			if (s_size == 0)
			{
				FUNC(flux_reverse_partition)(array, swap, array, piv, a_size, cmp);
				return;
			}
			memcpy(array + a_size, swap, s_size * sizeof(VAR));
			FUNC(quadsort_swap)(array + a_size, swap, s_size, s_size, cmp);
		}
		else
		{
			FUNC(flux_partition)(array + a_size, swap, swap, piv, s_size, cmp);
		}

		if (s_size <= a_size / 32 || a_size <= FLUX_OUT)
		{
			if (a_size <= FLUX_OUT)
			{
				FUNC(quadsort_swap)(array, swap, a_size, a_size, cmp);
			}
			else
			{
				FUNC(flux_reverse_partition)(array, swap, array, piv, a_size, cmp);
			}
			return;
		}
		nmemb = a_size;
		ptx = array;
	}
}

void FUNC(fluxsort)(void *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb <= 132)
	{
		FUNC(quadsort)(array, nmemb, cmp);
	}
	else
	{
		VAR *pta = (VAR *) array;
		VAR *swap = (VAR *) malloc(nmemb * sizeof(VAR));

		if (swap == NULL)
		{
			FUNC(quadsort)(array, nmemb, cmp);
			return;
		}
		FUNC(flux_analyze)(pta, swap, nmemb, nmemb, cmp);

		free(swap);
	}
}

void FUNC(fluxsort_swap)(void *array, void *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb <= 132)
	{
		FUNC(quadsort_swap)(array, swap, swap_size, nmemb, cmp);
	}
	else
	{
		VAR *pta = (VAR *) array;
		VAR *pts = (VAR *) swap;

		FUNC(flux_analyze)(pta, pts, swap_size, nmemb, cmp);
	}
}
