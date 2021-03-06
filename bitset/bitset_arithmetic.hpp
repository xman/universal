//  arithmetic.cpp : bitset-based arithmetic operators
//
// Copyright (C) 2017 Stillwater Supercomputing, Inc.
//
// This file is part of the universal numbers project, which is released under an MIT Open Source license.

#include <bitset>

#include "../posit/exceptions.hpp"

namespace sw {
	namespace unum {


		// this comparison is for a two's complement number only
		template<size_t nbits>
		bool operator< (const std::bitset<nbits>& lhs, const std::bitset<nbits>& rhs) {
			// comparison of the sign bit
			if (lhs[nbits - 1] == 0 && rhs[nbits - 1] == 1)	return false;
			if (lhs[nbits - 1] == 1 && rhs[nbits - 1] == 0) return true;
			// sign is equal, compare the remaining bits
			for (int i = nbits - 2; i >= 0; --i) {
				if (lhs[i] == 0 && rhs[i] == 1)	return true;
				if (lhs[i] == 1 && rhs[i] == 0) return false;
			}
			// numbers are equal
			return false;
		}

		// add bitsets a and b and return in bitset sum. Return true if there is a carry generated.
		template<size_t nbits>
		bool add_unsigned(std::bitset<nbits> a, std::bitset<nbits> b, std::bitset<nbits + 1>& sum) {
			uint8_t carry = 0;  // ripple carry
			for (int i = 0; i < nbits; i++) {
				uint8_t _a = a[i];
				uint8_t _b = b[i];
				uint8_t _slice = _a + _b + carry;
				carry = _slice >> 1;
				sum[i] = (0x1 & _slice);
			}
			sum.set(nbits, carry);
			return carry;
		}

		template<size_t src_size, size_t tgt_size>
		void copy_into(std::bitset<src_size>& src, size_t shift, std::bitset<tgt_size>& tgt) {
			tgt.reset();
			for (size_t i = 0; i < src_size; i++)
				tgt.set(i + shift, src[i]);
		}

		// truncate right-side
		template<size_t src_size, size_t tgt_size>
		void truncate(std::bitset<src_size>& src, std::bitset<tgt_size>& tgt) {
			tgt.reset();
			for (size_t i = 0; i < tgt_size; i++)
				tgt.set(tgt_size - 1 - i, src[src_size - 1 - i]);
		}

		template<size_t from, size_t to, size_t src_size>
		std::bitset<to - from> fixed_subset(const std::bitset<src_size>& src)
		{
			static_assert(from <= to, "from cannot be larger than to.");
			static_assert(to <= src_size, "to is larger than src_size");

			std::bitset<to - from> result;
			for (size_t i = 0, end = to - from; i < end; ++i)
				result[i] = src[i + from];
			return result;
		}


		template<size_t tgt_size, size_t src_size>
		struct round_t
		{
			static std::bitset<tgt_size> eval(const std::bitset<src_size>& src, size_t n)
			{
				static_assert(src_size > 0 && tgt_size > 0, "We don't bother with empty sets.");
				if (n >= src_size)
					throw round_off_all{};

				// look for cut-off leading bits
				for (size_t leading = tgt_size + n; leading < src_size; ++leading)
					if (src[leading])
						throw cut_off_leading_bit{};

				std::bitset<tgt_size> result((src >> n).to_ullong()); // convert to size_t to deal with different sizes

				if (n > 0 && src[n - 1]) {                                // round up potentially if first cut-off bit is true
#         ifdef POSIT_ROUND_TIES_AWAY_FROM_ZERO             // TODO: Evil hack to be consistent with assign_fraction, for testing only
					result = result.to_ullong() + 1;
#         else            

					bool more_bits = false;
					for (long i = 0; i + 1 < n && !more_bits; ++i)
						more_bits |= src[i];
					if (more_bits) {
						result = result.to_ullong() + 1;                // increment_unsigned is ambiguous 
					}
					else {                                            // tie: round up odd number
#             ifndef POSIT_ROUND_TIES_TO_ZERO               // TODO: evil hack to be removed later
						if (result[0])
							result = result.to_ullong() + 1;
#             endif
					}
#         endif
				}
				return result;
			}
		};

		template<size_t src_size>
		struct round_t<0, src_size>
		{
			static std::bitset<0> eval(const std::bitset<src_size>&, size_t)
			{
				return {};
			}
		};



		/** Round off \p n last bits of bitset \p src. Round to nearest resulting in potentially smaller bitset.
		*  Doesn't return carry bit in case of overflow while rounding up! TODO: Check whether we need carry or we require an extra bit for this case.
		*/
		template<size_t tgt_size, size_t src_size>
		std::bitset<tgt_size> round(const std::bitset<src_size>& src, size_t n)
		{
			return round_t<tgt_size, src_size>::eval(src, n);
		};




		template<size_t src_size, size_t tgt_size>
		bool accumulate(const std::bitset<src_size>& addend, std::bitset<tgt_size>& accumulator) {
			uint8_t carry = 0;  // ripple carry
			for (int i = 0; i < src_size; i++) {
				uint8_t _a = addend[i];
				uint8_t _b = accumulator[i];
				uint8_t _slice = _a + _b + carry;
				carry = _slice >> 1;
				accumulator[i] = (0x1 & _slice);
			}
			accumulator.set(src_size, carry);
			return carry;
		}

		// multiply bitsets a and b and return in bitset mul. Return true if there is a carry generated.
		template<size_t nbits>
		void multiply_unsigned(std::bitset<nbits> a, std::bitset<nbits> b, std::bitset<2 * nbits + 1>& accumulator) {
			bool carry = false;
			std::bitset<2 * nbits> addend;
			accumulator.reset();
			if (a.test(0)) {
				copy_into<nbits, 2 * nbits + 1>(b, 0, accumulator);
			}
			for (int i = 1; i < nbits; i++) {
				if (a.test(i)) {
					copy_into<nbits, 2 * nbits>(b, i, addend);
					accumulate(addend, accumulator);
				}
			}
		}

		// increment the input bitset in place, and return true if there is a carry generated.
		template<size_t nbits>
		bool increment_unsigned(std::bitset<nbits>& number) {
			uint8_t carry = 1;  // ripple carry
			uint8_t _a, _slice;
			for (int i = 0; i < nbits; i++) {
				_a = number[i];
				_slice = _a + 0 + carry;
				carry = _slice >> 1;
				number[i] = (0x1 & _slice);
			}
			return carry;
		}

		// increment the input bitset in place, and return true if there is a carry generated.
		// The input number is assumed to be right adjusted starting at nbits-nrBits
		// [1 0 0 0] nrBits = 0 is a noop as there is no word to increment
		// [1 0 0 0] nrBits = 1 is the word [1]
		// [1 0 0 0] nrBits = 2 is the word [1 0]
		// [1 1 0 0] nrBits = 3 is the word [1 1 0], etc.
		template<size_t nbits>
		bool increment_unsigned(std::bitset<nbits>& number, int nrBits = nbits - 1) {
			uint8_t carry = 1;  // ripple carry
			uint8_t _a, _slice;
			int lsb = nbits - nrBits;
			for (int i = lsb; i < nbits; i++) {
				_a = number[i];
				_slice = _a + 0 + carry;
				carry = _slice >> 1;
				number[i] = (0x1 & _slice);
			}
			return carry;
		}

		// increment the input bitset in place, assuming it is representing a 2's complement number
		template<size_t nbits>
		void increment_twos_complement(std::bitset<nbits>& number) {
			uint8_t carry = 1;  // ripple carry
			uint8_t _a, _slice;
			for (size_t i = 0; i < nbits; i++) {
				_a = number[i];
				_slice = _a + 0 + carry;
				carry = _slice >> 1;
				number[i] = (0x1 & _slice);
			}
			// ignore any carry bits
		}

		// decrement the input bitset in place, assuming it is representing a 2's complement number
		template<size_t nbits>
		void decrement_twos_complement(std::bitset<nbits>& number) {
			std::bitset<nbits> minus_one;
			minus_one.set();
			uint8_t carry = 0;  // ripple carry
			uint8_t _a, _b, _slice;
			for (int i = 0; i < nbits; i++) {
				_a = number[i];
				_b = minus_one[i];
				_slice = _a + _b + carry;
				carry = _slice >> 1;
				number[i] = (0x1 & _slice);
			}
			// ignore any carry bits
		}

		template<size_t nbits>
		bool add_signed_magnitude(std::bitset<nbits> a, std::bitset<nbits> b, std::bitset<nbits>& sum) {
			uint8_t carry = 0;
			bool sign_a = a.test(nbits - 1);
			if (sign_a) {
				a = a.flip();
				carry += 1;
			}
			bool sign_b = b.test(nbits - 1);
			if (sign_b) {
				b = b.flip();
				carry += 1;
			}

			for (int i = 0; i < nbits - 2; i++) {
				uint8_t _a = a[i];
				uint8_t _b = b[i];
				uint8_t _slice = _a + _b + carry;
				carry = _slice >> 1;
				sum[i] = (0x1 & _slice);
			}

			return carry;
		}

		template<size_t nbits>
		bool subtract_signed_magnitude(std::bitset<nbits> a, std::bitset<nbits> b, std::bitset<nbits>& diff) {
			bool sign_a = a.test(nbits - 1);
			bool sign_b = b.test(nbits - 1);
			std::cerr << "subtract_signed_magnitude not implemented yet" << std::endl;
			return false;
		}

		/*
		first attempt to abstract a reusable adder structure for FP addition experiments and error-free linear algebra
		template<size_t input_bits>
		bool adder_unit(
		bool r1_sign, int r1_scale,	const std::bitset<input_bits>& r1_fraction,
		bool r2_sign, int r2_scale, const std::bitset<input_bits>& r2_fraction,
		std::bitset<input_bits + 1>& sum) {


		if (_trace_add) {
		std::cout << (r1_sign ? "sign -1" : "sign  1") << " scale " << std::setw(3) << scale_of_result << " r1  " << r1 << " diff " << diff << std::endl;
		std::cout << (r2_sign ? "sign -1" : "sign  1") << " scale " << std::setw(3) << scale_of_result << " r2  " << r2 << std::endl;
		}

		if (r1_sign != r2_sign) r2 = twos_complement(r2);
		bool carry = add_unsigned<adder_size>(r1, r2, sum);

		if (_trace_add) std::cout << (r1_sign ? "sign -1" : "sign  1") << " carry " << std::setw(3) << (carry ? 1 : 0) << " sum " << sum << std::endl;
		if (carry) {
		if (r1_sign == r2_sign) {
		// the carry implies that we have a bigger number than r1
		scale_of_result++;
		// and that the first fraction bits came after a hidden bit at the carry position in the adder result register
		for (int i = 0; i < fract_size; i++) {
		result_fraction[i] = sum[i + 1];
		}
		}
		else {
		// the carry implies that we have a smaller number than r1
		// find the hidden bit
		int shift = 0;  // shift in addition to removal of hidden bit
		for (int i = adder_size - 1; i >= 0; i--) {
		if (sum.test(i)) {
		// hidden_bit is at position i
		break;
		}
		else {
		shift++;
		}
		}
		if (shift < adder_size) {
		// adjust the scale
		scale_of_result -= shift;
		// and extract the fraction, leaving the hidden bit behind
		for (int i = fract_size - 1; i >= shift; i--) {
		result_fraction[i] = sum[i - shift];  // fract_size is already 1 smaller than adder_size so we get the implied hidden bit removal automatically
		}
		}
		else {
		// we have actual 0
		reset();
		return *this;
		}
		}
		}
		else {
		// no carry implies that the scale remains the same
		// and that the first fraction bits came after a hidden bit at nbits-2 position in the adder result register
		for (int i = 0; i < nbits - 2; i++) {
		result_fraction[i] = sum[i];
		}
		}

		if (_trace_add) std::cout << (r1_sign ? "sign -1" : "sign  1") << " scale " << std::setw(3) << scale_of_result << " sum " << sum << " fraction " << result_fraction << std::endl;

		}


		// multiply two scientific notiation values
		// new sign     = multiply signs
		// new exponent = add exponents
		// new fraction = multiply fractions
		template<size_t input_bits>
		bool multiply_unit(
		bool r1_sign, int r1_scale, const std::bitset<input_bits>& r1_fraction,
		bool r2_sign, int r2_scale, const std::bitset<input_bits>& r2_fraction,
		std::bitset<input_bits + 1>& sum) {

		if (_trace_mul) {
		std::cout << (r1_sign ? "sign -1" : "sign  1") << " scale " << std::setw(3) << scale_of_result << " r1  " << r1 << " diff " << diff << std::endl;
		std::cout << (r2_sign ? "sign -1" : "sign  1") << " scale " << std::setw(3) << scale_of_result << " r2  " << r2 << std::endl;
		}

		if (r1_sign != r2_sign) r2 = twos_complement(r2);
		bool carry = add_unsigned<adder_size>(r1, r2, sum);

		if (_trace_add) std::cout << (r1_sign ? "sign -1" : "sign  1") << " carry " << std::setw(3) << (carry ? 1 : 0) << " sum " << sum << std::endl;
		}
		*/


	} // namespace unum

} // namespace sw

