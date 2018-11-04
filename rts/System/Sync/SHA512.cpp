/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cstring>

#include "SHA512.hpp"


static uint8_t hex2dec(uint8_t c) {
	if (c >= '0' && c <= '9') return (     (c - '0'));
	if (c >= 'a' && c <= 'f') return (10 + (c - 'a'));
	if (c >= 'A' && c <= 'F') return (10 + (c - 'A'));
	return 0;
}

static uint64_t rotr64(uint64_t x, uint64_t i) {
	assert(i >=  1);
	assert(i <= 63);
	return ((x << (64 - i)) | (x >> i));
}


void sha512::read_digest(const hex_digest& hex_chars, raw_digest& sha_bytes) {
	for (uint8_t i = 0; i < SHA_LEN; i++) {
		const uint8_t c0 = hex2dec(hex_chars[i * 2 + 0]);
		const uint8_t c1 = hex2dec(hex_chars[i * 2 + 1]);
		sha_bytes[i] = (c0 << 4) | c1;
	}
}

void sha512::dump_digest(const raw_digest& sha_bytes, hex_digest& hex_chars) {
	for (uint8_t i = 0; i < SHA_LEN; i++) {
		snprintf(hex_chars.data() + (i * 2), hex_chars.size() - (i * 2), "%02x", sha_bytes[i]);
	}

	hex_chars[hex_chars.size() - 1] = 0;
}


void sha512::calc_digest(const msg_vector& msg_bytes, raw_digest& sha_bytes) {
	calc_digest(msg_bytes.data(), msg_bytes.size(), sha_bytes.data());
}

void sha512::calc_digest(const uint8_t msg_bytes[], size_t len, uint8_t sha_bytes[SHA_LEN]) {
	uint8_t block[BLK_LEN] = {0};
	uint64_t state[NUM_STATE_CONSTS] = {0};

	size_t ofs = len & (~static_cast<size_t>(BLK_LEN - 1));

	static_assert(sizeof(STATE_CONSTS) == (NUM_STATE_CONSTS * sizeof(uint64_t)), "");
	std::memcpy(&state[0], &STATE_CONSTS[0], sizeof(STATE_CONSTS));
	dm_compress(state, msg_bytes, ofs);

	// handle final blocks
	if ((len - ofs) > 0)
		std::memmove(block, &msg_bytes[ofs], len - ofs);

	ofs  = len & (BLK_LEN - 1);
	ofs += 1;

	block[ofs - 1] = 0x80;

	// apply padding
	if ((ofs + 16) > BLK_LEN) {
		dm_compress(state, block, BLK_LEN);
		std::memset(block, 0, BLK_LEN);
	}

	block[BLK_LEN - 1] = static_cast<uint8_t>((len & 0x1Fu) << 3);
	len >>= 5;

	// write lengths
	for (uint8_t i = 1; i < 16; i++, len >>= 8) {
		block[BLK_LEN - 1 - i] = static_cast<uint8_t>(len);
	}

	dm_compress(state, block, BLK_LEN);

	// convert state to digest bytes; big-endian order
	for (uint8_t i = 0; i < SHA_LEN; i++) {
		sha_bytes[i] = static_cast<uint8_t>(state[i >> 3] >> ((7 - (i & 7)) << 3));
	}
}


void sha512::dm_compress(uint64_t state[NUM_STATE_CONSTS], const uint8_t blocks[], size_t len) {
	assert(len == 0 || (len % BLK_LEN) == 0);

	uint64_t schedule[NUM_ROUND_CONSTS] = {0};

	for (size_t i = 0; i < len; ) {
		// initialize schedule
		for (uint8_t j = 0; j < 16; j++, i += 8) {
			schedule[j]  = 0;
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 0]) << 56);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 1]) << 48);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 2]) << 40);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 3]) << 32);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 4]) << 24);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 5]) << 16);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 6]) <<  8);
			schedule[j] |= (static_cast<uint64_t>(blocks[i + 7]) <<  0);
		}

		for (uint8_t j = 16; j < NUM_ROUND_CONSTS; j++) {
			schedule[j]  = schedule[j - 16] + schedule[j - 7];
			schedule[j] += (rotr64(schedule[j - 15],  1) ^ rotr64(schedule[j - 15],  8) ^ (schedule[j - 15] >> 7));
			schedule[j] += (rotr64(schedule[j -  2], 19) ^ rotr64(schedule[j -  2], 61) ^ (schedule[j -  2] >> 6));
		}

		// round constants
		uint64_t a = state[0];
		uint64_t b = state[1];
		uint64_t c = state[2];
		uint64_t d = state[3];
		uint64_t e = state[4];
		uint64_t f = state[5];
		uint64_t g = state[6];
		uint64_t h = state[7];

		for (uint8_t j = 0; j < NUM_ROUND_CONSTS; j++) {
			const uint64_t t1 = h + (rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41)) + (g ^ (e & (f ^ g))) + ROUND_CONSTS[j] + schedule[j];
			const uint64_t t2 =     (rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39)) + ((a & (b | c)) | (b & c));

			h = g;
			g = f;
			f = e;
			e = d + t1;
			d = c;
			c = b;
			b = a;
			a = t1 + t2;
		}

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}
}


bool sha512::unit_test(const char* msg_str, const char* sha_str) {
	msg_vector msg_bytes = {};
	raw_digest sha_bytes = {0};

	if (msg_str[0] != 0) {
		msg_bytes.resize(std::strlen(msg_str), 0);
		std::memcpy(msg_bytes.data(), msg_str, msg_bytes.size());
	}

	calc_digest(msg_bytes, sha_bytes);

	size_t k = 0;

	for (size_t n = 0; n < SHA_LEN; n++) {
		const uint8_t a = hex2dec(sha_str[n * 2 + 0]);
		const uint8_t b = hex2dec(sha_str[n * 2 + 1]);

		k += (sha_bytes[n] == ((a << 4) | b));
	}

	return (k == SHA_LEN);
}

