#ifndef BIGINT_CPP
#define BIGINT_CPP

#include <iostream>
#include <string>
#include <stdint.h>
#include <cstdarg>
#include <array>
#include <utility>
#include <bitset>
#include <sstream>
#include <cassert>
#include <vector>
#include <limits>


#include "bigint.h"

// NOTE: all operators work for the same op size. Maybe remove the conditions that define it otherwise

namespace BigInt
{
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	template<uint8_t base> // type of input (oct = base 8, hex = base 16)
	SelectType<bitsize_t>::BigUint<bitsize>::BigUint(std::string input)
	{
		strtobigint<base>(input.c_str());
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	template<uint8_t base> // type of input (oct = base 8, hex = base 16)
	SelectType<bitsize_t>::BigUint<bitsize>::BigUint(const char* input)
	{
		strtobigint<base>(input); // to avoid annoying C++ conversion error
	}
	
	// numerical input constructor. If number is 256-bit, input = left 128-bit, right 128-bit
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpragmas"
	#pragma GCC diagnostic ignored "-Wc++2b-extensions"
	#pragma GCC diagnostic ignored "-Wc++17-extensions"
	#pragma GCC diagnostic ignored "-Wvarargs"
	template<typename bitsize_t>
	template<bitsize_t bitsize> 
	constexpr SelectType<bitsize_t>::BigUint<bitsize>::BigUint(const bitsize_t count, __uint128_t ...) {
		std::va_list args;
		va_start(args, count);
	
		// pad the operator array
		const bitsize_t count64 = count << 1; // count if input is 64-bits
		op_nonleading_i = count64 < op_size? op_size-count64 : 0;
		for(bitsize_t i=0;i<=op_nonleading_i;i++) op[i] = 0x0000000000000000ULL;
	
		// add the inputs to the operator array
	    for(size_t i=0;i<count;i++) {
	        __uint128_t num = va_arg(args, __uint128_t);
	        op[op_size-i*2-1] = num >> 64; ////////////////////// RECENT CHANGE - maybe change indexes if problem
	        op[op_size-i*2-2] = num&bottom_mask_u128;
	    }
		va_end(args);
	}

	// numerical input. If number is 256-bit, input = left 128-bit, right 128-bit, same as the function above except it's compile-time
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	template<typename ...Ts> // all uint128_t, must specify
	consteval SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::assign_conste(Ts... input) noexcept
	{
		// pad the operator array
		constexpr const size_t count = sizeof...(Ts);
		const constexpr bitsize_t count64 = count << 1; // count if input is 64-bits
		if constexpr(count64 < op_size)
				op_nonleading_i = op_size-count64;
		else
			op_nonleading_i = 0;

		assert(op_nonleading_i < 262144); // if called, that means that bitsize is too large. Padding is too much for compile time. Try using non-compile-time function
		for(bitsize_t i=0;i<op_nonleading_i;i++) op[i] = 0x0000000000000000ULL;
	
		// add the inputs to the operator array
		bitsize_t i=0;
	    for(const auto num : {input...}) {
	        op[op_size-i*2-1] = num >> 64; ////////////////////// RECENT CHANGE - maybe change indexes if problem
	        op[op_size-i*2-2] = num&bottom_mask_u128;
			i++;
	    }
		return *this;
	}
	#pragma GCC diagnostic pop

	// assignment to operator array of known length
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize>::BigUint(uint64_t *input, bitsize_t len) // input order has to be: input[0] = most left 64-bit
	{
		// calculate pad count
		bitsize_t pad_count;
		if(len > op_size) { // if bigger integer type, and input array has non-zero number at the height of op
			bool found = false;
			for(bitsize_t i=0;i<=op_size;i++) { // iterate to op_size because input array order is backwards
				if(input[i] != 0)
					found = true;
			}
			if(found) { // maybe input[op_size] = 0 but input[op_size+1] !=0, therefore use the found defined above
				throw int_too_large_error(("given integer is too large for the defined BigUint<" + std::to_string(bitsize) +
										  "> which is not enough to hold a value of BigUint<" + std::to_string(len*64) + ">").c_str());
			} else {
				bitsize_t nonzeroi; // index of non-zero input element
				for(bitsize_t i=op_size+1;i<len;i++) { // basically continuation of previous loop
					if(input[i] != 0) {
						nonzeroi=i;
						break;
					}
				}
	    		pad_count = nonzeroi;
			}
		} else {
			pad_count = len-op_size;
	
		}
	    // pad the operator array
	    for(bitsize_t i=0;i<pad_count;i++) op[i] = 0x0000000000000000ULL;
	
	    // add input to operator array
	    for(bitsize_t i=op_size;i --> pad_count;) op[i] = input[i];
		op_nonleading_i = pad_count;
	}

	// helper function to assign compile time array so that it can be assigned to op as compile time
	// Basically the above constructor as a constexpr function
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wc++20-extensions" // not initialized on decleration on constexpr function
	#pragma GCC diagnostic ignored "-Wc++17-extensions"
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	template<bitsize_t len, std::array<uint64_t, len> input>
	consteval SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::assign_op() noexcept
	{
		// calculate pad count
		if(len > op_size) { // if bigger integer type, and input array has non-zero number at the height of op
			bool found = false;

			for(bitsize_t i=0;i<=op_size;i++) { // iterate to op_size because input array order is backwards
				if constexpr(input[i] != 0)
					found = true;
			}
			if (found) { // maybe input[op_size] = 0 but input[op_size+1] !=0, therefore use the found defined above
				throw int_too_large_error("given integer is too large for the defined BigUint");
			} else {
				bitsize_t nonzeroi; // index of non-zero input element
				for(bitsize_t i=op_size+1;i<len;i++) { // basically continuation of previous loop
					if constexpr(input[i] != 0) {
						nonzeroi=i;
						break;
					}
				}
	    		// pad the operator array
	    		for(bitsize_t i=0;i<nonzeroi;i++) op[i] = 0x0000000000000000ULL;
				op_nonleading_i = nonzeroi;

	    		// add input to operator array
	    		for(bitsize_t i=op_size;i --> nonzeroi;) op[i] = input[i];
			}
		} else {
			const bitsize_t constexpr pad_count = len-op_size;

	    	// pad the operator array
	    	for(bitsize_t i=0;i<pad_count;i++) op[i] = 0x0000000000000000ULL;
	
	    	// add input to operator array
	    	for(bitsize_t i=op_size;i --> pad_count;) op[i] = input[i];
		}
		return *this;
	}
	#pragma GCC diagnostic pop

	// copy assignment operator
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator=(const BigUint &num)
	{
		// assign non-leading-zero elements of the operator array
		if(num.op_size <= op_size) {
			for(bitsize_t i=0;i<num.op_size;i++) op[i] = num.op[i];
			for(bitsize_t i=num.op_size;i<op_size;i++) op[i] = num.op[i]; // op_size is bigger than num.op_size, pad op
		} else {
			for(bitsize_t i=0;i<op_size;i++) op[i] = num.op[i]; // num.op won't fit in op
		}
		return *this;
	}

	// copy constructor
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	SelectType<bitsize_t>::BigUint<bitsize>::BigUint(const BigUint &num)
	{
		// assign non-leading-zero elements of the operator array
		if(num.op_size <= op_size) {
			for(bitsize_t i=0;i<num.op_size;i++) op[i] = num.op[i];
			for(bitsize_t i=num.op_size;i<op_size;i++) op[i] = num.op[i]; // op_size is bigger than num.op_size, pad op
		} else {
			for(bitsize_t i=0;i<op_size;i++) op[i] = num.op[i]; // num.op won't fit in op
		}
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint assignment operator")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator=(const char* &num)
	{
		*this = BigUint<bitsize>(num);
		return *this; // reconstruct as new object
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean and operator&&")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator&&(BigUint num) const
	{
		if (*this == "0" or num == "0") return 0;
		return 1;
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean or operator||")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator||(BigUint num) const
	{
		if (*this == "0" and num == "0") return 0;
		return 1;
	}
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wc++17-extensions"
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean equal to operator==")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator==(const BigUint &num) const
	{
		bool equal = 0;
		for(bitsize_t i=0;i<op_size;i++) {
			equal = op[i] == num.op[i];
			if (!equal) return 0;
		}
		return equal;
	}
	#pragma GCC diagnostic pop
	
	// check if initialized and not zero
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean not operator!")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator!() const
	{
		bool notzero = 0;
		for(bitsize_t i=0;i<op_size;i++) {
			if(op[i] != 0) {
				notzero = 1;
				break;
			}
		}
		return notzero;
	}

	// boolean operator, check if not equal to
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wc++17-extensions"
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean not equal to operator!=")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator!=(const BigUint &num) const
	{
		bool notequal = 0;
		for(bitsize_t i=0;i<op_size;i++) {
			notequal |= op[i] != num.op[i];
			if(notequal) return notequal;
		}
		return notequal;
	}
	#pragma GCC diagnostic pop

	// TODO: make sure that all of the following comparision operators work with different op-sizes, they are currently not designed to.
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint boolean less than operator<")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator<(const BigUint &num) const
	{
		bool less = 0;
	
		// condition to avoid iterating over non-existing members of op
		const constexpr bitsize_t iterator = op_size < num.op_size ? op_size : num.op_size;
		for(bitsize_t i=0;i<iterator;i++) {
			if(op[i] < num.op[i]) {
				less = 1;
				break;
			} else {
				if(op[i] == num.op[i]) continue;
				break;
			}
		}
		return less;
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator<=(const BigUint &num) const
	{
		bool less = 0; // or equal
	
		// condition to avoid iterating over non-existing members of op
		const bitsize_t constexpr iterator = op_size < num.op_size ? op_size : num.op_size;
		for(bitsize_t i=iterator;i --> 0;) {
			if(op[i] <= num.op[i]) {
				less = 1;
				break;
			} else {
				break;
			}
		}
		return less;
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint greater operator>")]]
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator>(const BigUint &num) const
	{
		bool greater = 0; // or equal
	
		// condition to avoid iterating over non-existing members of op
		const constexpr bitsize_t iterator = op_size < num.op_size ? op_size : num.op_size;
		for(bitsize_t i=0;i<iterator;i++) {
			if(op[i] > num.op[i]) {
				greater = 1;
				break;
			} else {
				if(op[i] == num.op[i]) continue;
				break;
			}
		}
		return greater;
	}
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr bool SelectType<bitsize_t>::BigUint<bitsize>::operator>=(const BigUint &num) const
	{
		bool greater = 0; // or equal
	
		// condition to avoid iterating over non-existing members of op
		const constexpr bitsize_t iterator = op_size < num.op_size ? op_size : num.op_size;
		for(bitsize_t i=iterator;i --> 0;) {
			if(op[i] >= num.op[i]) {
				greater = 1;
				break;
			} else {
				break;
			}
		}
		return greater;
	}
	
	
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator+")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator+(const BigUint &num)
	{
		uint64_t *new_op = (uint64_t*)calloc(8, op_size);
		uint64_t *tmp_op = new uint64_t[op_size*8];
		memcpy(tmp_op, op, 8*op_size); // if ptr: set to op
		//std::copy(std::begin(op), std::end(op), std::begin(tmp_op)); // if array: set to op
		//for(bitsize_t i=0;i<op_size;i++) new_op[i] = 0; // for debugging valgrind error, initialize new_op to zero first
		
		for(bitsize_t i=op_size;i --> 0;) {
			__uint128_t tmp = tmp_op[i];
			tmp += num.op[i];
			if(tmp > UINT64_MAX) {
	    		new_op[i] += tmp & UINT64_MAX; // assign the main value to assign value with no carry (only no carry because of bit-shifting)
				bitsize_t j = 1;
				if(j <= i) {
					while(new_op[i-j] == UINT64_MAX) {
	    				tmp_op[i-j]++; // carry
						j++;
					}
	    			tmp_op[i-j]++;
				}
			} else {
				new_op[i] += tmp;
			}
		}
	
		auto newint = BigUint<bitsize>(new_op, op_size);
		delete[] tmp_op;
		free(new_op);
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator+=(const BigUint &num)
	{
		uint64_t *tmp_op = new uint64_t[op_size];
		//uint64_t tmp_op[op_size];
		memcpy(tmp_op, op, op_size*8); // if ptr: set to op
		// std::copy(std::begin(op), std::end(op), std::begin(tmp_op)); // if array: set to op
		for(bitsize_t i=op_size;i --> 0;) {
			op[i] = 0;
			__uint128_t tmp = tmp_op[i];
			tmp += num.op[i];
			if(tmp > UINT64_MAX) {
	    		op[i] = tmp & UINT64_MAX; // assign the main value to assign value with no carry (only no carry because of bit-shifting)
				bitsize_t j = 1;
				if(j <= i) {
					while(op[i-j] == UINT64_MAX) { // carry index
	    				tmp_op[i-j]++; // carry
						j++;
					}
	    			tmp_op[i-j]++; // carry
				}
			} else {
				op[i] += tmp;
			}
		}
		delete[] tmp_op;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator-")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator-(const BigUint &num)
	{
		uint64_t *ret = new uint64_t[op_size];
		uint64_t *new_op = new uint64_t[op_size];
		memcpy(new_op, op, op_size*8); // if ptr: set to op
		// std::copy(std::begin(op), std::end(op), std::begin(new_op)); // if array: set to op
		for(bitsize_t i=op_size;i --> 0;) {
			ret[i] = 0;
			if (new_op[i] < num.op[i]) {
				ret[i] = new_op[i]-num.op[i];
				bitsize_t j=1;
				while(i-j > 0 and new_op[i-j] == 0) {
					new_op[i-j]--; // carry
					j++;
				}
				new_op[i-j]--;
			} else {
				ret[i] = new_op[i] - num.op[i];
			}
		}
		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] new_op;
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator-=(const BigUint &num)
	{
		for(bitsize_t i=op_size;i --> 0;) {
			if (op[i] < num.op[i]) {
				op[i] -= num.op[i];
				bitsize_t j=1;
				while(i-j > 0 and op[i-j] == 0) {
					op[i-j]--; // carry
					j++;
				}
				op[i-j]--;
			} else {
				op[i] -= num.op[i];
			}
		}
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator*")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator*(BigUint num)
	{
		// Russian Peasant Algorithm
		uint64_t *o = new uint64_t[op_size];
		memcpy(o, op, 8*op_size); // for ptr
		// for(bitsize_t i=0;i<op_size;i++) o[i] = op[i]; // for array
		BigUint<bitsize> new_op = BigUint<bitsize>(o, op_size);
		BigUint<bitsize> ret = 0;
		while(num > "0") {
			if(num & "1") ret += new_op;
			new_op <<= 1; // try replacing with += new_op
			num >>= 1; // try replacing with div
		}
		delete[] o;
		return ret;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator*=(BigUint num)
	{
		*this = *this * num;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator/")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator/(const BigUint &num)
	{
			BigUint<bitsize> d = num;
			BigUint<bitsize> current = 1;
			BigUint<bitsize> ret = 0;

			// make copy of *this
			uint64_t *o = new uint64_t[op_size];
			memcpy(o, op, 8*op_size);
			// for(bitsize_t i=0;i<op_size;i++) o[i] = op[i];
			BigUint<bitsize> new_op = BigUint<bitsize>(o, op_size);
			delete[] o;

			if (d > new_op) {
				return 0;
			}

			if (d == new_op) {
				return 1;
			}

			while (d < new_op) {
				d <<= 1;
				current <<= 1;
			}

			d >>= 1;
			current >>= 1;

			while(current != "0") {
				if(new_op > d) {
					new_op -= d;
					ret |= current;
				}
				current >>= 1;
				d >>= 1;
			}
			return ret;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator/=(const BigUint &num)
	{
		*this = *this / num;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator%")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator%(const BigUint &num)
	{
		return *this - (*this / num) * num;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator%=(const BigUint &num)
	{
		*this = *this % num;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator++(int)
	{
		*this += 1;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator--(int)
	{
		*this -= 1;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator[] for accessing bit")]]
	constexpr uint64_t SelectType<bitsize_t>::BigUint<bitsize>::operator[](const bitsize_t &index) const
	{
		if(isbit) {
			const uint64_t mod = index%64;
			return op[!mod ? index/64 : index/64+1] & (1 << mod);
		} else {
		return op[index];
		}
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator~")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator~() const
	{
		uint64_t *ret = new uint64_t[op_size];
		for(bitsize_t i=0;i<op_size;i++)  ret[i] = ~op[i];
		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator&")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator&(const BigUint &num)
	{
		uint64_t *ret = new uint64_t[op_size];
		for(bitsize_t i=0;i<op_size;i++)  ret[i] = op[i] & num.op[i];
		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator&=(const BigUint &num)
	{
		// assuming they are the same size. Which should be enforced by compiler by default
		for(bitsize_t i=0;i<op_size;i++)  op[i] &= num.op[i];
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator^")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator^(const BigUint &num)
	{
		// assuming they are the same size. Which should be enforced by compiler by default
		uint64_t *ret = new uint64_t[op_size];
		for(bitsize_t i=0;i<op_size;i++)  ret[i] = op[i] ^ num.op[i];
		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator^=(const BigUint &num)
	{
		// assuming they are the same size. Which should be enforced by compiler by default
		for(bitsize_t i=0;i<op_size;i++)  op[i] ^= num.op[i];
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator>>")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator>>(const bitsize_t &num)
	{
		if(num >= bitsize) {
			uint64_t *ret = (uint64_t*)calloc(8, op_size);
			auto num = BigUint<bitsize>(ret, op_size);
			free(ret);
			return num;
		}
		uint64_t *ret = new uint64_t[op_size];
		memcpy(ret, op, 8*op_size);

		bitsize_t shift = num;
		bitsize_t div = shift/64;
		if(shift>=64) {
		 	for(bitsize_t i=div;i<op_size;i++) ret[i] = op[i-div];
		 	for(bitsize_t i=0;i<div;i++) ret[i] = 0;
		}
		shift %= 64;
		if(shift != 0) {
			for(bitsize_t i=div;i<op_size;i++) {
				ret[i] >>= shift;
				if(i != div) {
					// hex((0x5520466034583347 >> 2) | ((0x2337616833<<62) & (2**64-1)))
					// std::cout << std::endl << "solution:" << (uint64_t)(ret[i]  |  ( ((__uint128_t)op[i-1] << 62) & UINT64_MAX)) << std::endl;
					ret[i] |= op[i-1] << (64-shift);
				}
			}
		}

		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator>>=(const bitsize_t &num)
	{
		if(num >= bitsize) {
			for(bitsize_t i=0;i<op_size;i++) op[i] = 0;
			return *this;
		}

		uint64_t *_copy = new uint64_t[op_size];
		memcpy(_copy, op, 8*op_size);

		bitsize_t shift = num;
		bitsize_t div = shift/64;
		if(shift>=64) {
		 	for(bitsize_t i=div;i<op_size;i++) op[i] = op[i-div];
		 	for(bitsize_t i=0;i<div;i++) op[i] = 0;
		}

		shift %= 64;
		if(shift != 0) {
			for(bitsize_t i=div;i<op_size;i++) {
				op[i] >>= shift;
				if(i != div) {
					op[i] |= _copy[i-1] << (64-shift);
				}
			}
		}

		delete[] _copy;
		return *this;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator<<")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator<<(const bitsize_t &num)
	{
		if(num >= bitsize) {
			uint64_t *ret = (uint64_t*)calloc(8, op_size);
			auto num = BigUint<bitsize>(ret, op_size);
			free(ret);
			return num;
		}
		uint64_t *ret = new uint64_t[op_size];
		memcpy(ret, op, 8*op_size);

		bitsize_t shift = num;
		bitsize_t div = shift/64;
		bitsize_t ind = op_size-div;
		if(shift>=64) {
		 	for(bitsize_t i=0;i<ind;i++) ret[i] = ret[i+div];
		 	for(bitsize_t i=ind;i<op_size;i++) ret[i] = 0;
		}
		shift %= 64;
		if(shift != 0) {
			for(bitsize_t i=0;i<ind;i++) {
				__uint128_t tmp = (__uint128_t)ret[i] << shift;
				if(tmp > UINT64_MAX and i != 0) { // move overflow to next index
					ret[i-1] |= tmp >> 64;
					ret[i] = tmp;
				} else {
					ret[i] = tmp;
				}
			}
		}

		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator<<=(const bitsize_t &num)
	{
		if(num >= bitsize) {
			for(bitsize_t i=0;i<op_size;i++) op[i] = 0;
			return *this;
		}

		bitsize_t shift = num;
		bitsize_t div = shift/64;
		bitsize_t ind = op_size-div;
		if(shift>=64) {
		 	for(bitsize_t i=0;i<ind;i++) op[i] = op[i+div];
		 	for(bitsize_t i=ind;i<op_size;i++) op[i] = 0;
		}
		shift %= 64;
		if(shift != 0) {
			for(bitsize_t i=0;i<ind;i++) {
				__uint128_t tmp = (__uint128_t)op[i] << shift;
				if(tmp > UINT64_MAX and i != 0) { // move overflow to next index
					op[i-1] |= tmp >> 64;
					op[i] = tmp;
				} else {
					op[i] = tmp;
				}
			}
		}

		return *this;
	}

	 
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	[[nodiscard("discarded BigUint operator|")]]
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator|(const BigUint &num)
	{
		// assuming they are the same size. Which should be enforced by compiler by default
		uint64_t *ret = new uint64_t[op_size];
		for(bitsize_t i=0;i<op_size;i++)  ret[i] = op[i] | num.op[i];
		auto newint = BigUint<bitsize>(ret, op_size);
		delete[] ret;
		return newint;
	}

	template<typename bitsize_t>
	template<bitsize_t bitsize>
	constexpr SelectType<bitsize_t>::BigUint<bitsize> SelectType<bitsize_t>::BigUint<bitsize>::operator|=(const BigUint &num)
	{
		// assuming they are the same size. Which should be enforced by compiler by default
		for(bitsize_t i=0;i<op_size;i++)  op[i] |= num.op[i];
		return *this;
	}

	template<uint16_t bitsize>
	SelectType<uint16_t>::BigUint<bitsize> pow(SelectType<uint16_t>::BigUint<bitsize> base, SelectType<uint16_t>::BigUint<bitsize> exp)
	{

		SelectType<uint16_t>::BigUint<bitsize> ret = 1;
		for(SelectType<uint16_t>::BigUint<bitsize> i=0;i<exp;i++) {
			ret *= base;
		}
		return ret;
		//BigUint<bitsize> q = exp;
		//BigUint<bitsize> prod = 1;
		//BigUint<bitsize> current = base;
		//while(q > "0") {
		//	if(q & "1") {
		//		prod *= current;
		//		q-=1;
		//	}
		//	current *= current;
		//	q /= 2;
		//}
		//
		//return prod;
	}
	template<uint32_t bitsize>
	SelectType<uint32_t>::BigUint<bitsize> pow(SelectType<uint32_t>::BigUint<bitsize> base, SelectType<uint32_t>::BigUint<bitsize> exp)
	{

		SelectType<uint32_t>::BigUint<bitsize> ret = 1;
		for(SelectType<uint32_t>::BigUint<bitsize> i=0;i<exp;i++) {
			ret *= base;
		}
		return ret;
	}

	template<uint64_t bitsize>
	SelectType<uint64_t>::BigUint<bitsize> pow(SelectType<uint64_t>::BigUint<bitsize> base, SelectType<uint64_t>::BigUint<bitsize> exp)
	{

		SelectType<uint64_t>::BigUint<bitsize> ret = 1;
		for(SelectType<uint64_t>::BigUint<bitsize> i=0;i<exp;i++) {
			ret *= base;
		}
		return ret;
	}

	template<__uint128_t bitsize>
	SelectType<__uint128_t>::BigUint<bitsize> pow(SelectType<__uint128_t>::BigUint<bitsize> base, SelectType<__uint128_t>::BigUint<bitsize> exp)
	{

		SelectType<__uint128_t>::BigUint<bitsize> ret = 1;
		for(SelectType<__uint128_t>::BigUint<bitsize> i=0;i<exp;i++) {
			ret *= base;
		}
		return ret;
	}

	// define destructor
	template<typename bitsize_t>
	template<bitsize_t bitsize>
	SelectType<bitsize_t>::BigUint<bitsize>::~BigUint()
	{
		delete[] op;
	}
	

	// use the following types.

	// Use BigUint when you need numbers in range (0, 2^32768)
	template<uint16_t bitsize>
	using BigUint = SelectType<uint16_t>::BigUint<bitsize>;

	// Use LargeUint when you need numbers in range (0, 2^2147483648)
	template<uint32_t bitsize>
	using LargeUint = SelectType<uint32_t>::BigUint<bitsize>;

	// Use HugeUint when you need numbers in range (0, 2^9223372036854775808)
	template<uint64_t bitsize>
	using HugeUint = SelectType<uint64_t>::BigUint<bitsize>;

	// Use YugeUint when you need numbers in range (0, 2^170141183460469231731687303715884105728)
	template<__uint128_t bitsize>
	using YugeUint = SelectType<__uint128_t>::BigUint<bitsize>;
}; /* NAMESPACE BIGINT */

#endif /* BIGINT_CPP */
