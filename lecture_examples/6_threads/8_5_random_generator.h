#pragma once

/**
 * ---------------------------------------------------------------
 * Mersenne Twister
 * A thread-safe random number generator with good randomness in a
 * small number of instructions. It is used it to introduce random
 * timing delays. The built-in generator, random() function, gives
 * strange results, it makes reordering happening much less
 * frequent. Perhaps, the generator below is just faster.
 * ---------------------------------------------------------------
 */
#define MT_IA  397
#define MT_LEN 624

struct mersenne_twister
{
	unsigned int buffer[MT_LEN];
	int index;
};

static unsigned int
mersenne_twister_generate(struct mersenne_twister *twister)
{
	int i = twister->index;
	int i2 = twister->index + 1;
	int j = twister->index + MT_IA;
	if (i2 >= MT_LEN)
		i2 = 0;
	if (j >= MT_LEN)
		j -= MT_LEN;

	unsigned int s = (twister->buffer[i] & 0x80000000) |
			 (twister->buffer[i2] & 0x7fffffff);
	unsigned int r = twister->buffer[j] ^ (s >> 1) ^ ((s & 1) * 0x9908B0DF);
	twister->buffer[twister->index] = r;
	twister->index = i2;

	r ^= (r >> 11);
	r ^= (r << 7) & 0x9d2c5680UL;
	r ^= (r << 15) & 0xefc60000UL;
	r ^= (r >> 18);
	return r;
}

static void
mersenne_twister_create(struct mersenne_twister *twister, unsigned int seed)
{
	for (int i = 0; i < MT_LEN; i++)
		twister->buffer[i] = seed;
	twister->index = 0;
	for (int i = 0; i < MT_LEN * 100; i++)
		mersenne_twister_generate(twister);
}
