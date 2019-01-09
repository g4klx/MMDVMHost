/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*	Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com> N2PLL
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "RS241213.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char ENCODE_MATRIX[12U][24U] = {
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 062, 044, 003, 025, 014, 016, 027, 003, 053, 004, 036, 047},
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 011, 012, 011, 011, 016, 064, 067, 055, 001, 076, 026, 073},
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 003, 001, 005, 075, 014, 006, 020, 044, 066, 006, 070, 066},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 021, 070, 027, 045, 016, 067, 023, 064, 073, 033, 044, 021},
	{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 030, 022, 003, 075, 015, 015, 033, 015, 051, 003, 053, 050},
	{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 001, 041, 027, 056, 076, 064, 021, 053, 004, 025, 001, 012},
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 061, 076, 021, 055, 076, 001, 063, 035, 030, 013, 064, 070},
	{0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 024, 022, 071, 056, 021, 035, 073, 042, 057, 074, 043, 076},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 072, 042, 005, 020, 043, 047, 033, 056, 001, 016, 013, 076},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 072, 014, 065, 054, 035, 025, 041, 016, 015, 040, 071, 026},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 073, 065, 036, 061, 042, 022, 017, 004, 044, 020, 025, 005},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 071, 005, 055, 003, 071, 034, 060, 011, 074, 002, 041, 050}};

const unsigned char ENCODE_MATRIX_24169[16U][24U] = {
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 051, 045, 067, 015, 064, 067, 052, 012 },
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 057, 025, 063, 073, 071, 022, 040, 015 },
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 005, 001, 031, 004, 016, 054, 025, 076 },
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 073, 007, 047, 014, 041, 077, 047, 011 },
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 075, 015, 051, 051, 017, 067, 017, 057 },
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 020, 032, 014, 042, 075, 042, 070, 054 },
	{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 002, 075, 043, 005, 001, 040, 012, 064 },
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 024, 074, 015, 072, 024, 026, 074, 061 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 042, 064, 007, 022, 061, 020, 040, 065 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 032, 032, 055, 041, 057, 066, 021, 077 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 065, 036, 025, 007, 050, 016, 040, 051 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 064, 006, 054, 032, 076, 046, 014, 036 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 062, 063, 074, 070, 005, 027, 037, 046 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 055, 043, 034, 071, 057, 076, 050, 064 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 024, 023, 023, 005, 050, 070, 042, 023 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 067, 075, 045, 060, 057, 024, 006, 026 } };

const unsigned char ENCODE_MATRIX_362017[20U][36U] = {
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 074, 037, 034, 006, 002, 007, 044, 064, 026, 014, 026, 044, 054, 013, 077, 005 },
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 004, 017, 050, 024, 011, 005, 030, 057, 033, 003, 002, 002, 015, 016, 025, 026 },
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 007, 023, 037, 046, 056, 075, 043, 045, 055, 021, 050, 031, 045, 027, 071, 062 },
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 026, 005, 007, 063, 063, 027, 063, 040, 006, 004, 040, 045, 047, 030, 075, 007 },
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 023, 073, 073, 041, 072, 034, 021, 051, 067, 016, 031, 074, 011, 021, 012, 021 },
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 024, 051, 025, 023, 022, 041, 074, 066, 074, 065, 070, 036, 067, 045, 064, 001 },
	{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 052, 033, 014, 002, 020, 006, 014, 025, 052, 023, 035, 074, 075, 075, 043, 027 },
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 055, 062, 056, 025, 073, 060, 015, 030, 013, 017, 020, 002, 070, 055, 014, 047 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 054, 051, 032, 065, 077, 012, 054, 013, 035, 032, 056, 012, 075, 001, 072, 063 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 074, 041, 030, 041, 043, 022, 051, 006, 064, 033, 003, 047, 027, 012, 055, 047 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 054, 070, 011, 003, 013, 022, 016, 057, 003, 045, 072, 031, 030, 056, 035, 022 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 051, 007, 072, 030, 065, 054, 006, 021, 036, 063, 050, 061, 064, 052, 001, 060 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 001, 065, 032, 070, 013, 044, 073, 024, 012, 052, 021, 055, 012, 035, 014, 072 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 011, 070, 005, 010, 065, 024, 015, 077, 022, 024, 024, 074, 007, 044, 007, 046 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 006, 002, 065, 011, 041, 020, 045, 042, 046, 054, 035, 012, 040, 064, 065, 033 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 034, 031, 001, 015, 044, 064, 016, 024, 052, 016, 006, 062, 020, 013, 055, 057 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 063, 043, 025, 044, 077, 063, 017, 017, 064, 014, 040, 074, 031, 072, 054, 006 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 071, 021, 070, 044, 056, 004, 030, 074, 004, 023, 071, 070, 063, 045, 056, 043 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 002, 001, 053, 074, 002, 014, 052, 074, 012, 057, 024, 063, 015, 042, 052, 033 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 034, 035, 002, 023, 021, 027, 022, 033, 064, 042, 005, 073, 051, 046, 073, 060 } };

const unsigned int rsGFexp[64] = {
	1, 2, 4, 8, 16, 32, 3, 6, 12, 24, 48, 35, 5, 10, 20, 40,
	19, 38, 15, 30, 60, 59, 53, 41, 17, 34, 7, 14, 28, 56, 51, 37,
	9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
	13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33, 0 };

const unsigned int rsGFlog[64] = {
	63, 0, 1, 6, 2, 12, 7, 26, 3, 32, 13, 35, 8, 48, 27, 18,
	4, 24, 33, 16, 14, 52, 36, 54, 9, 45, 49, 38, 28, 41, 19, 56,
	5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
	10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58 };

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

static unsigned char bin2Hex(const unsigned char* input, unsigned int offset)
{
	unsigned char output = 0x00U;

	output |= READ_BIT(input, offset + 0U) ? 0x20U : 0x00U;
	output |= READ_BIT(input, offset + 1U) ? 0x10U : 0x00U;
	output |= READ_BIT(input, offset + 2U) ? 0x08U : 0x00U;
	output |= READ_BIT(input, offset + 3U) ? 0x04U : 0x00U;
	output |= READ_BIT(input, offset + 4U) ? 0x02U : 0x00U;
	output |= READ_BIT(input, offset + 5U) ? 0x01U : 0x00U;

	return output;
}

static void hex2Bin(unsigned char input, unsigned char* output, unsigned int offset)
{
	WRITE_BIT(output, offset + 0U, input & 0x20U);
	WRITE_BIT(output, offset + 1U, input & 0x10U);
	WRITE_BIT(output, offset + 2U, input & 0x08U);
	WRITE_BIT(output, offset + 3U, input & 0x04U);
	WRITE_BIT(output, offset + 4U, input & 0x02U);
	WRITE_BIT(output, offset + 5U, input & 0x01U);
}

CRS241213::CRS241213()
{
}

CRS241213::~CRS241213()
{
}

bool CRS241213::decode(unsigned char* data)
{
	return decode(data, 24U, 39, 12);
}

void CRS241213::encode(unsigned char* data)
{
	assert(data != NULL);

	unsigned char codeword[24U];

	for (unsigned int i = 0U; i < 24U; i++) {
		codeword[i] = 0x00U;

		unsigned int offset = 0U;
		for (unsigned int j = 0U; j < 12U; j++, offset += 6U) {
			unsigned char hexbit = bin2Hex(data, offset);
			codeword[i] ^= gf6Mult(hexbit, ENCODE_MATRIX[j][i]);
		}
	}

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 24U; i++, offset += 6U)
		hex2Bin(codeword[i], data, offset);
}

bool CRS241213::decode24169(unsigned char* data)
{
	return decode(data, 24U, 39, 8);
}

void CRS241213::encode24169(unsigned char* data)
{
	assert(data != NULL);

	unsigned char codeword[24U];

	for (unsigned int i = 0U; i < 24U; i++) {
		codeword[i] = 0x00U;

		unsigned int offset = 0U;
		for (unsigned int j = 0U; j < 16U; j++, offset += 6U) {
			unsigned char hexbit = bin2Hex(data, offset);
			codeword[i] ^= gf6Mult(hexbit, ENCODE_MATRIX_24169[j][i]);
		}
	}

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 24U; i++, offset += 6U)
		hex2Bin(codeword[i], data, offset);
}

bool CRS241213::decode362017(unsigned char* data)
{
	return decode(data, 36U, 27, 16);
}

void CRS241213::encode362017(unsigned char* data)
{
	assert(data != NULL);

	unsigned char codeword[36U];

	for (unsigned int i = 0U; i < 36U; i++) {
		codeword[i] = 0x00U;

		unsigned int offset = 0U;
		for (unsigned int j = 0U; j < 20U; j++, offset += 6U) {
			unsigned char hexbit = bin2Hex(data, offset);
			codeword[i] ^= gf6Mult(hexbit, ENCODE_MATRIX_362017[j][i]);
		}
	}

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 36U; i++, offset += 6U)
		hex2Bin(codeword[i], data, offset);
}

// GF(2 ^ 6) multiply(for Reed - Solomon encoder)
unsigned char CRS241213::gf6Mult(unsigned char a, unsigned char b) const
{
	unsigned char p = 0x00U;

	for (unsigned int i = 0U; i < 6U; i++) {
		if ((b & 0x01U) == 0x01U)
			p ^= a;

		a <<= 1;

		if ((a & 0x40U) == 0x40U)
			a ^= 0x43U;			// primitive polynomial : x ^ 6 + x + 1

		b >>= 1;
	}

	return p;
}

bool CRS241213::decode(unsigned char* data, const unsigned int bitLength, const int firstData, const int roots)
{
	assert(data != NULL);

	//unsigned char HB[24U];
	unsigned char HB[63U];
	::memset(HB, 0x00U, 63U);

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < bitLength; i++, offset += 6)
		HB[i] = bin2Hex(data, offset);

	//RS (63,63-nroots,nroots+1) decoder where nroots = number of parity bits
	// rsDec(8, 39) rsDec(16, 27) rsDec(12, 39)

	const int nroots = roots;
	int lambda[18];	// Err+Eras Locator poly
	int S[17];		// syndrome poly
	int b[18];
	int t[18];
	int omega[18];
	int root[17];
	int reg[18];
	int locn[17];

	int i, j, count, r, el, SynError, DiscrR, q, DegOmega, tmp, num1, num2, den, DegLambda;

	// form the syndromes; i.e., evaluate HB(x) at roots of g(x)
	for (i = 0; i <= nroots - 1; i++) {
		S[i] = HB[0];
	}

	//for (j = 1; j <= 24; j++) {		// XXX was 62
	//for (j = 1; j <= (int)(bitLength - 1); j++) {
	for (j = 1; j <= 62; j++) {
		for (i = 0; i <= nroots - 1; i++) {
			if (S[i] == 0) {
				S[i] = HB[j];
			}
			else {
				S[i] = HB[j] ^ rsGFexp[(rsGFlog[S[i]] + i + 1) % 63];
			}
		}
	}

	// convert syndromes to index form, checking for nonzero condition
	SynError = 0;

	for (i = 0; i <= nroots - 1; i++) {
		SynError = SynError | S[i];
		S[i] = rsGFlog[S[i]];
	}

	if (SynError == 0) {
		// if syndrome is zero, rsData[] is a codeword and there are
		// no errors to correct. So return rsData[] unmodified
		count = 0;
		return true;
	}

	for (i = 1; i <= nroots; i++) {
		lambda[i] = 0;
	}

	lambda[0] = 1;

	for (i = 0; i <= nroots; i++) {
		b[i] = rsGFlog[lambda[i]];
	}

	// begin Berlekamp-Massey algorithm to determine error+erasure
	// locator polynomial
	r = 0;
	el = 0;
	while (++r <= nroots) {
		// r is the step number
		//r = r + 1;
		// compute discrepancy at the r-th step in poly-form
		DiscrR = 0;

		for (i = 0; i <= r - 1; i++) {
			if ((lambda[i] != 0) && (S[r - i - 1] != 63)) {
				DiscrR = DiscrR ^ rsGFexp[(rsGFlog[lambda[i]] + S[r - i - 1]) % 63];
			}
		}

		DiscrR = rsGFlog[DiscrR];	// index form

		if (DiscrR == 63) {
			// shift elements upward one step
			for (i = nroots; i >= 1; i += -1) {
				b[i] = b[i - 1];
			}

			b[0] = 63;
		}
		else {
			// t(x) <-- lambda(x) - DiscrR*x*b(x)
			t[0] = lambda[0];

			for (i = 0; i <= nroots - 1; i++) {
				if (b[i] != 63) {
					t[i + 1] = lambda[i + 1] ^ rsGFexp[(DiscrR + b[i]) % 63];
				}
				else {
					t[i + 1] = lambda[i + 1];
				}
			}

			if (2 * el <= r - 1) {
				el = r - el;
				// b(x) <-- inv(DiscrR) * lambda(x)

				for (i = 0; i <= nroots; i++) {
					if (lambda[i]) {
						b[i] = (rsGFlog[lambda[i]] - DiscrR + 63) % 63;
					}
					else {
						b[i] = 63;
					}
				}
			}
			else {
				// shift elements upward one step
				for (i = nroots; i >= 1; i += -1) {
					b[i] = b[i - 1];
				}

				b[0] = 63;
			}

			for (i = 0; i <= nroots; i++) {
				lambda[i] = t[i];
			}
		}
	} /* end while() */

	  // convert lambda to index form and compute deg(lambda(x))
	DegLambda = 0;
	for (i = 0; i <= nroots; i++) {
		lambda[i] = rsGFlog[lambda[i]];

		if (lambda[i] != 63) {
			DegLambda = i;
		}
	}

	// Find roots of the error+erasure locator polynomial by Chien search
	for (i = 1; i <= nroots; i++) {
		reg[i] = lambda[i];
	}

	count = 0;// number of roots of lambda(x)

	for (i = 1; i <= 63; i++) {
		q = 1;// lambda[0] is always 0

		for (j = DegLambda; j >= 1; j += -1) {
			if (reg[j] != 63) {
				reg[j] = (reg[j] + j) % 63;
				q = q ^ rsGFexp[reg[j]];
			}
		}

		// it is a root
		if (q == 0) {
			// store root (index-form) and error location number
			root[count] = i;
			locn[count] = i - 40;
			// if we have max possible roots, abort search to save time
			count = count + 1;

			if (count == DegLambda) {
				break;
			}
		}
	}

	if (DegLambda != count) {
		// deg(lambda) unequal to number of roots => uncorrectable error detected
		return false;
	}

	// compute err+eras evaluator poly omega(x)
	// = s(x)*lambda(x) (modulo x**nroots). in index form. Also find deg(omega).
	DegOmega = 0;
	for (i = 0; i <= nroots - 1; i++) {
		tmp = 0;
		if (DegLambda < i) {
			j = DegLambda;
		}
		else {
			j = i;
		}

		for ( /* j = j */; j >= 0; j += -1) {
			if ((S[i - j] != 63) && (lambda[j] != 63)) {
				tmp = tmp ^ rsGFexp[(S[i - j] + lambda[j]) % 63];
			}
		}

		if (tmp) {
			DegOmega = i;
		}

		omega[i] = rsGFlog[tmp];
	}

	omega[nroots] = 63;

	//compute error values in poly-form:
	// num1 = omega(inv(X(l)))
	// num2 = inv(X(l))**(FCR - 1)
	// den = lambda_pr(inv(X(l)))
	for (j = count - 1; j >= 0; j += -1) {
		num1 = 0;

		for (i = DegOmega; i >= 0; i += -1) {
			if (omega[i] != 63) {
				num1 = num1 ^ rsGFexp[(omega[i] + i * root[j]) % 63];
			}
		}

		num2 = rsGFexp[0];
		den = 0;

		// lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i]
		if (DegLambda < nroots) {
			i = DegLambda;
		}
		else {
			i = nroots;
		}

		for (i = i & ~1; i >= 0; i += -2) {
			if (lambda[i + 1] != 63) {
				den = den ^ rsGFexp[(lambda[i + 1] + i * root[j]) % 63];
			}
		}

		if (den == 0) {
			return false;
		}

		// apply error to data
		if (num1 != 0) {
			if (locn[j] < firstData)
				return false;
			HB[locn[j]] = HB[locn[j]] ^ (rsGFexp[(rsGFlog[num1] + rsGFlog[num2] + 63 - rsGFlog[den]) % 63]);
		}
	}

	offset = 0U;
	for (unsigned int i = 0U; i < (unsigned int)nroots; i++, offset += 6)
		hex2Bin(HB[i], data, offset);

	return true;
}
