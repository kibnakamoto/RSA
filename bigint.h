#ifndef BIGINT_H
#define BIGINT_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <exception>
#include <array>
#include <type_traits>
#include <iostream>
#include <random>

// To define operations for all types instead of just multiples of 64. Calculate 2**bitsize (in 64-bit segments), every 64-bit segment is the modulo instead of UINT64_MAX, meaning replace UINT64_MAX WITH 2**bitsize

namespace BigInt
{
	// raise when wrong type
	class wrong_type_error : public std::runtime_error {
		public: explicit wrong_type_error(const char *str) : std::runtime_error(str) {}
	};
	
	// raise when integer too large
	class int_too_large_error : public std::runtime_error {
		public: explicit int_too_large_error(const char *str) : std::runtime_error(str) {}
	};
	
	
	template<typename bitsize_t>
	class SelectType {
		public:
		bool isptr = 0; // if stackoverflow, make sure to make op a ptr. Do this by assigning isptr to 1
		// custom-size integer class
		template<bitsize_t bitsize>
		// alignas((bitsize%64==0 ? bitsize/64 : bitsize/64+1)<<3)
		class BigUint {
			protected:
				// operator array
				const constexpr static bitsize_t op_size = bitsize%64==0 ? bitsize/64 : bitsize/64+1;
				uint64_t *op = new uint64_t[op_size];
				//uint64_t op[op_size]; // when iterating, start from end to start
				bitsize_t op_nonleading_i; // index of op when leading zeros end
	
				// uint128_t input to 2 uint64_t integers
				// constant mask values
			    static constexpr const __uint128_t bottom_mask_u128 = (__uint128_t{1} << 64) - 1; // 0x0000000000000000ffffffffffffffffU
		    	static constexpr const __uint128_t top_mask_u128 = ~bottom_mask_u128;                  // 0xffffffffffffffff0000000000000000U
	
				// get substring of char* - helper function
				constexpr std::string get_substring(const char* str, bitsize_t start, bitsize_t substrsize)
				{
					std::string substr = "";
				
					// Copy the characters from str[startIndex] to str[endIndex]
					const bitsize_t width = substrsize+start;
					for(bitsize_t i=start;i<width;i++) substr += str[i];
					return substr;
				}
	
			public:
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wignored-qualifiers" // silence this warning, the qualifiers are necesarry
				static const constexpr inline bitsize_t __get_op_size() { return op_size; }
				#pragma GCC diagnostic pop
				inline uint64_t* __get_op() { return op; }
		
				const constexpr static bitsize_t size = bitsize;
				template<uint8_t base=0> // type of input (int = base 10, hex = base 16)
				BigUint(std::string input);
				template<uint8_t base=0> // type of input (int = base 10, hex = base 16)
				BigUint(const char *input);
	
				// assign uint64_t compile time array to op, there is also a non-garunteed compile time function. That is a constructor
				template<bitsize_t len, std::array<uint64_t, len> tmp_op>
				consteval BigUint assign_op() noexcept;

				// destructor
				~BigUint();
	
				// the next constructor as a compile-time function
				template<typename ...Ts>
				//consteval BigUint assign_conste(const __uint128_t&&input1, Ts&&...input) noexcept; // assign consteval using the same method as the next function
				consteval BigUint assign_conste(Ts...input) noexcept; // assign consteval using the same method as the next function
		
				// numerical input. If number is 256-bit, input = left 128-bit, right 128-bit
				constexpr BigUint(const bitsize_t count, __uint128_t input...);
	
				constexpr BigUint(const uint64_t num) {
					for(bitsize_t i=0;i<op_size-1;i++) op[i] = 0;
					op[op_size-1] = num;
				}
		
				// input as operation array
				constexpr explicit BigUint(uint64_t *input, bitsize_t len);
		
				// decleration
				inline constexpr BigUint() noexcept = default;
		
				// assign uint256 to another uint256
				BigUint operator=(const BigUint &num);
				BigUint(const BigUint &num);
				constexpr BigUint operator=(const char *&num);
		
				// arithmetic operations
				constexpr BigUint operator+(const BigUint &num);
				constexpr BigUint operator+=(const BigUint &num);
				constexpr BigUint operator-(const BigUint &num);
				constexpr BigUint operator-=(const BigUint &num);
				constexpr BigUint operator*(BigUint num);
				constexpr BigUint operator*=(BigUint num);
				constexpr BigUint operator/(const BigUint &num);
				constexpr BigUint operator/=(const BigUint &num);
				constexpr BigUint operator%(const BigUint &num);
				constexpr BigUint operator%=(const BigUint &num);
	
				constexpr BigUint operator++(int);
				constexpr BigUint operator--(int);

				// if isbit=1, will return bool (bit of the number), if isbit=0, return op[index]
				bool isbit=0;
				constexpr uint64_t operator[](const bitsize_t &index) const;
		
				// bitwise operators
				constexpr BigUint operator~() const;
				constexpr BigUint operator&(const BigUint &num);
				constexpr BigUint operator&=(const BigUint &num);
				constexpr BigUint operator^(const BigUint &num);
				constexpr BigUint operator^=(const BigUint &num);
				constexpr BigUint operator>>(const bitsize_t &num); // doesn't need to be BigInt because it'll be too large if bigint
				constexpr BigUint operator>>=(const bitsize_t &num); // uint16 because it is bit-size, and bitsize var is uint16
				constexpr BigUint operator<<(const bitsize_t &num);
				constexpr BigUint operator<<=(const bitsize_t &num);
				constexpr BigUint operator|(const BigUint &num);
				constexpr BigUint operator|=(const BigUint &num);
		
				// boolean operators
				constexpr bool operator&&(BigUint num) const;
				constexpr bool operator||(BigUint num) const;
				constexpr bool operator==(const BigUint &num) const;
				constexpr bool operator!() const;
				constexpr bool operator!=(const BigUint &num) const;
				constexpr bool operator<(const BigUint &num) const;
				constexpr bool operator<=(const BigUint &num) const;
				constexpr bool operator>(const BigUint &num) const;
				constexpr bool operator>=(const BigUint &num) const;
				
				// delete operators for deleting run-time objects
				inline void operator delete(void *dat); // delete object itself
		
				inline constexpr operator uint64_t*() noexcept { return op; }
	
				constexpr operator bool() noexcept {
					return *this != "0";
				}
	
				constexpr operator uint64_t() noexcept {
					for(bitsize_t i=0;i<op_size;i++) {
						if(op[i] != 0) return op[i];
					}
					return op[0];
				}
	
				constexpr operator uint32_t() noexcept {
					for(bitsize_t i=0;i<op_size;i++) {
						if(op[i] != 0) return op[i];
					}
					return op[0];
				}
	
				constexpr operator uint16_t() noexcept {
					for(uint16_t i=0;i<op_size;i++) {
						if(op[i] != 0) return op[i];
					}
					return op[0];
				}
				constexpr operator uint8_t() noexcept {
					for(bitsize_t i=0;i<op_size;i++) {
						if(op[i] != 0) return op[i];
					}
					return op[0];
				}

				template<typename stringstream_type> // stringstream or ostringstream
				void to_ostringstream(stringstream_type &ss)
				{
					bool pad_stopped = 0; // if pad stopped, then print the rest, including zero values
					bool last_num = 0;
					uint8_t pad_size;
				
					// initialize the pad sizes based on whether the ostream is dec, hex, oct, bin.
					std::ios_base::fmtflags fmt = ss.flags();
					if(fmt & std::ios_base::dec) { // not supported
						pad_size = 16;
						ss << std::hex;
					} else if(fmt & std::ios_base::hex) pad_size = 16; // pad count: 2^64-1=16 base 16 digits
					else if(fmt & std::ios_base::oct) pad_size = 22;

					for(uint16_t i=0;i<op_size;i++) {
						if(op[i] != 0x0000000000000000ULL) pad_stopped=1;
						if(pad_stopped) {
							if(last_num)
								ss << std::setfill('0') << std::setw(pad_size) << op[i];
							else
								ss << op[i]; // no padding
							last_num = 1; // if not first print, pad
						}
					}
					if(!pad_stopped) // if zero
						ss << "0";
				}
	
				// string conversion
				operator std::string()
				{
					std::ostringstream ss;
					to_ostringstream<std::ostringstream>(ss);
					return ss.str();
				}
	
				// convert between different bigints
				template<bitsize_t n> // bitsize
				inline constexpr BigUint<n> to()
				{
					const constexpr bitsize_t new_op_size = n%64==0 ? n/64 : n/64+1;
					uint64_t *num = new uint64_t[new_op_size];
					if constexpr(new_op_size <= op_size) { // when converting to a smaller type
						const constexpr bitsize_t diff = op_size-new_op_size;
						for(bitsize_t i=new_op_size;i --> 0;) num[i] = op[i+diff]; // smallest numbers of op will be dismissed, the major segment numbers will be in num
					} else { // when converting to a bigger type
						const constexpr bitsize_t diff = new_op_size-op_size;
						for(bitsize_t i=op_size;i --> 0;) num[i+diff] = op[i];
						for(bitsize_t i=0;i<diff;i++) { // pad
							num[i] = 0;
						}
					}
					auto new_obj = BigUint<n>(num, new_op_size);
					delete[] num;
					return new_obj;
				}

		
				template<bitsize_t n> friend std::ostream& operator<<(std::ostream& cout, BigUint<n> toprint);

				// generate random number in range(from, to)
				// error is true if wrong range
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wconversion-null"
				constexpr static BigUint random(BigUint from, BigUint to, bool &error) // give range of numbers
				{
        			std::random_device randdev;
        			std::mt19937 generator(randdev() ^ time(NULL));
					
					// if range is wrong
					if(from > to) {
						//BigUint swapper = from;
						//from = to;
						//to = swapper;
						error=1; // wrong range error
						return NULL;
					}

					// check if random number is in range of UINT64_MAX
					bool u64_range=true;
					bitsize_t from_size = (uint64_t)from == 0 ? 0 : 1; // to_size is the uint64-bit size without padding
					bitsize_t to_size = (uint64_t)to == 0 ? 0 : 1; // to_size is the uint64-bit size without padding
					if (from_size || to_size) {
						for(bitsize_t i=op_size;i --> 0;) {
							if(to.__get_op()[i] != 0) {
								if(u64_range) to_size = i;
								if(i != op_size-1) u64_range=false;
							}
							if(from.__get_op()[i] != 0) {
								from_size = i;
								if(!u64_range) break;
							}
						}
					} else {
						if(to_size == 0) return "0";
					}
					to_size = op_size-to_size;
					from_size = op_size-from_size;

					// random length between op_size and to_size, the length of random data in 64-bit segments
					bitsize_t rand_len = std::uniform_int_distribution<bitsize_t>(from_size, to_size)(generator);
					rand_len = 2;

					// if to < UINT64_MAX
					if (u64_range || rand_len==1)
							return std::uniform_int_distribution<uint64_t>(from.__get_op()[op_size-1], to.__get_op()[op_size-1])(generator);

        			std::uniform_int_distribution<uint64_t>
        			                             distr_64(0, UINT64_MAX);
					uint64_t *num = new uint64_t[op_size];
					for(bitsize_t i=0;i<rand_len;i++) { // set to zero if not in range of to
						num[i] = 0x0000000000000000ULL;
					}
					for(bitsize_t i=op_size;i --> rand_len+1;) {
						num[i] = distr_64(generator);
					}
					num[rand_len] = std::uniform_int_distribution<uint64_t>(from, to)(generator);

					BigUint random_uint = BigUint(num, op_size);
					delete[] num;
					return random_uint;
				}
				#pragma GCC diagnostic pop

				// generate random number up to to
				constexpr static BigUint random(BigUint to)
				{
        			std::random_device randdev;
        			std::mt19937 generator(randdev() ^ time(NULL));

					// check if random number is in range of UINT64_MAX
					bool u64_range=true;
					bitsize_t to_size = (uint64_t)to == 0 ? 0 : 1; // to_size is the uint64-bit size without padding
					if(to_size) {
						for(bitsize_t i=op_size-1;i --> 0;) {
							if(to.__get_op()[i] != 0) {
								u64_range=false;
								to_size = i;
								break;
							}
						}
						to_size = op_size-to_size;
					} else {
						return "0";
					}

					// random length between from_size and to_size, the length of random data in 64-bit segments
					bitsize_t rand_len = std::uniform_int_distribution<bitsize_t>(1, to_size)(generator);

					// if to < UINT64_MAX
					if (u64_range || rand_len==1)
							return std::uniform_int_distribution<uint64_t>(0, to.__get_op()[op_size-1])(generator);


    				std::uniform_int_distribution<uint64_t>
    				                             distr_64(0, UINT64_MAX);
					uint64_t *num = new uint64_t[op_size];
					for(bitsize_t i=0;i<rand_len;i++) { // set to zero if not in range of to
						num[i] = 0x0000000000000000ULL;
					}
					for(bitsize_t i=op_size;i --> rand_len+1;) {
						num[i] = distr_64(generator);
					}
					num[rand_len] = std::uniform_int_distribution<uint64_t>(0, to)(generator);
					BigUint random_uint = BigUint(num, op_size);
					delete[] num;
					return random_uint;
				}

				// this print is for when stackoverflow error stops operator<<
				void print()
				{
					bool pad_stopped = 0; // if pad stopped, then print the rest, including zero values
					bool last_num = 0;
					uint8_t pad_size;
				
					// initialize the pad sizes based on whether the ostream is dec, hex, oct, bin.
					std::ios_base::fmtflags fmt = std::cout.flags();
					if(fmt & std::ios_base::dec) {
						pad_size = 16;
						std::cout << std::hex;
					}
					else if(fmt & std::ios_base::hex) pad_size = 16; // pad count: 2^64-1=16 base 16 digits
					else if(fmt & std::ios_base::oct) pad_size = 22;
					else pad_size = 64; // bin
					for(uint16_t i=0;i<op_size;i++) {
						if(op[i] != 0x0000000000000000ULL) pad_stopped=1;
						if(pad_stopped) {
							if(last_num)
								std::cout << std::setfill('0') << std::setw(pad_size) << op[i];
							else
								std::cout << op[i]; // no padding
							last_num = 1; // if not first print, pad
						}
					}
					if(!pad_stopped) // if zero
						std::cout << "0";
				}

				// log2
				constexpr static BigUint log2(BigUint n)
				{
				    return (n > "1") ? BigUint(1) + log2(n >> bitsize_t(1)) : BigUint(0);
				}

				// log2
				constexpr BigUint log2()
				{
				    return (*this > "1") ? BigUint(1) + log2(*this >> bitsize_t(1)) : BigUint(0);
				}

				constexpr BigUint factorial()
				{
      				BigUint p = 1, r = 1;
      				loop(op[op_size-1], p, r);
      				return r << bitsize_t(nminussumofbits(op[op_size-1]));

				}
		
			protected:
				constexpr bitsize_t nminussumofbits(bitsize_t v)
				{
					uint64_t w = v;
					w -= (0xaaaaaaaa & w) >> 1;
					w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
					w = (w + (w >> 4)) & 0x0f0f0f0f;
					w += w >> 8;
					w += w >> 16;
					return v - (w & 0xff);
				}

				constexpr void loop(bitsize_t n, BigUint &p, BigUint &r)
  				{
  				    if (n <= 2) return;
  				    loop(n / 2, p, r);
  				    p = p * part_product(n / 2 + 1 + ((n / 2) & 1), n - 1 + (n & 1));
  				    r = r * p;
  				}

  				constexpr BigUint part_product(int n, int m)
  				{
  				    if (m <= (n + 1)) return (BigUint) n;
  				    if (m == (n + 2)) return (BigUint) n * (BigUint)m; 
  				    int k =  (n + m) / 2;
  				    if ((k & 1) != 1) k = k - 1;
  				    return part_product(n, k) * part_product(k + 2, m);
  				}

				// remove 0x if starting with 0x
				constexpr inline bool rm_trailhex(const char *&num, size_t &input_len)
				{
					std::string_view str(const_cast<char*>(num), 2);
					if (str == "0x") {
						num += 2; // delete the 0x
						input_len-=2;
						return 1;
					}
					return 0;
				} 
		
				// check if number is hex
				constexpr bool is_hex(const char *num, size_t numlen)
				{
					for(size_t i=0;i<numlen;i++)
						if(not isxdigit(*(num+i))) return 0;
					return 1;
				}
		
				// check if input is base16
				constexpr bool input_hex(const char *&input, size_t &input_len)
				{
					// check if input is hex
					bool _is_hex = rm_trailhex(input, input_len); // remove trailing character if it exists
					if(!_is_hex) { // if no hex trail character '0x'
						_is_hex = is_hex(input, input_len); // check if input is hex
						return _is_hex;
					} else {
						return 1;
					}
				}
		
				// convert string to bigint
				template<uint8_t base=0> // type of input (int = base 8, hex = base 16)
				constexpr void strtobigint(const char *input)
				{
					constexpr const bool base8 = base==8;
					constexpr const bool base16 = base==16;
		   			size_t len = strlen(input);
					// TODO: check if input length is in range of operator array length, if it's not generate error
					constexpr const static __uint128_t hex_len_op = op_size*16;
					constexpr const static __uint128_t oct_len_op = op_size*8;
		
					if constexpr(base16 or base8) {
						if constexpr(base16) {
							rm_trailhex(input); // remove trailing character if it exists
							if(len >= hex_len_op) {
								auto num = std::string(input).erase(hex_len_op, len-hex_len_op); // prune to hex_len_op
								input = num.c_str();
								len = strlen(input);
							} // compare byte size
							hexoct_to_bigint(input, len, 16);
						} else { // base 8
							if(len >= oct_len_op) {
								auto num = std::string(input).erase(oct_len_op, len-oct_len_op); // prune to hex_len_op
								input = num.c_str();
								len = strlen(input);
							} // compare byte size
							hexoct_to_bigint(input, len, 8);
						}
					} else {
		   				bool hex_input = input_hex(input, len);
						if(hex_input) {
							if(len >= hex_len_op) {
								auto num = std::string(input).erase(hex_len_op, len-hex_len_op); // prune to hex_len_op
								input = num.c_str();
								len = strlen(input);
								
							} // compare byte size
							hexoct_to_bigint(input, len, 16);
						}
						else throw wrong_type_error("string or const char* input has to be hex");
					}
				}
				
				// helper algorithm to convert hex or oct values to big integer
				constexpr void hexoct_to_bigint(const char *input, bitsize_t len, const unsigned char part_size)
				{
		   			// convert oct/hex input to op elements
		   			const uint8_t ind = len%part_size;
		   			const bitsize_t multiple16_count = (len-ind)/part_size;
					uint64_t *tmp;
					if(multiple16_count != 0) {
		   				tmp = new uint64_t[multiple16_count];
						// get's the first multiple of part_size values of the integer
		   				for(bitsize_t i=0;i<multiple16_count;i++) {
							std::stringstream ss;
							ss << std::hex << get_substring(input, i*part_size+ind,part_size);
							ss >> tmp[i];
						}
		   			   	if(ind!=0) { // if len not a multiple of part_size
		   			   		op_nonleading_i = op_size-multiple16_count-1; // includes the final value
							std::stringstream ss;
							ss << std::hex << get_substring(input, 0,ind);
							ss >> op[op_size-multiple16_count-1];
		   			   	 	for(bitsize_t i=multiple16_count+1;i --> 1;) op[op_size-i] = tmp[multiple16_count-i];
		   			   	} else {
		   			   		op_nonleading_i = op_size-multiple16_count; // if length is a multiple of part_size
		   			   	 	for(bitsize_t i=multiple16_count+1;i --> 1;) op[op_size-i] = tmp[multiple16_count-i];
		   			   	}
					} else { // length < part_size
		   				tmp = new uint64_t[1];
						std::stringstream ss;
						ss << std::hex << input;
						ss >> *tmp;
		   			   op_nonleading_i = op_size-1;
					   op[op_nonleading_i] = *tmp;
					}
		   			// pad the operator array
		   			for(bitsize_t i=0;i<op_nonleading_i;i++) op[i] = 0x0000000000000000ULL;
		   			delete[] tmp;
				}
		};

		template<bitsize_t bitsize>
		BigUint<bitsize> pow(BigUint<bitsize> base, BigUint<bitsize> exp);
		
	};
	//using uint192_t  = SelectType<uint16_t>::BigUint<192>; // remove until division algorithm works for non power of 2.
	using uint256_t  = SelectType<uint16_t>::BigUint<256>;
	//using uint384_t  = SelectType<uint16_t>::BigUint<384>; // remove until division algorithm works for non power of 2.
	using uint512_t  = SelectType<uint16_t>::BigUint<512>;
	using uint1024_t = SelectType<uint16_t>::BigUint<1024>;

	// types of bitsize
	typedef SelectType<uint16_t> selected_type16;
	typedef SelectType<uint32_t> selected_type32;
	typedef SelectType<uint64_t> selected_type64;
	typedef SelectType<__uint128_t> selected_type128;


	// stringstream
	template<uint16_t bitsize>
	std::stringstream& operator<<(std::stringstream& ss, selected_type16::BigUint<bitsize> num)
	{
		num.to_ostringstream(ss);
		return ss;
	}
	
	// stringstream
	template<uint32_t bitsize>
	std::stringstream& operator<<(std::stringstream& ss, selected_type32::BigUint<bitsize> num)
	{
		num.to_ostringstream(ss);
		return ss;
	}
	
	// stringstream
	template<uint64_t bitsize>
	std::stringstream& operator<<(std::stringstream& ss, selected_type64::BigUint<bitsize> num)
	{
		num.to_ostringstream(ss);
		return ss;
	}
	
	// stringstream
	template<__uint128_t bitsize>
	std::stringstream& operator<<(std::stringstream& ss, selected_type128::BigUint<bitsize> num)
	{
		num.to_ostringstream(ss);
		return ss;
	}

	// output stream operator
	template<uint16_t bitsize>
	std::ostream& operator<<(std::ostream& cout, selected_type16::BigUint<bitsize> toprint)
	{
		toprint.to_ostringstream(cout);
		return cout;
	}

	// output stream operator
	template<uint32_t bitsize>
	std::ostream& operator<<(std::ostream& cout, selected_type32::BigUint<bitsize> toprint)
	{
		toprint.to_ostringstream(cout);
		return cout;
	}
	 
	// output stream operator
	template<uint64_t bitsize>
	std::ostream& operator<<(std::ostream& cout, selected_type64::BigUint<bitsize> toprint)
	{
		toprint.to_ostringstream(cout);
		return cout;
	}
	 
	// output stream operator
	template<__uint128_t bitsize>
	std::ostream& operator<<(std::ostream& cout, selected_type128::BigUint<bitsize> toprint)
	{
		toprint.to_ostringstream(cout);
		return cout;
	}
	 
	 
	// input stream operator
	template<uint16_t bitsize>
	std::istream& operator>>(std::istream& cin, selected_type16::BigUint<bitsize> &input)
	{
		std::string num;
		cin >> num;
		input = num;
		return cin;
	}

	// input stream operator
	template<uint32_t bitsize>
	std::istream& operator>>(std::istream& cin, selected_type32::BigUint<bitsize> &input)
	{
		std::string num;
		cin >> num;
		input = num;
		return cin;
	}

	// input stream operator
	template<uint64_t bitsize>
	std::istream& operator>>(std::istream& cin, selected_type64::BigUint<bitsize> &input)
	{
		std::string num;
		cin >> num;
		input = num;
		return cin;
	}

	// input stream operator
	template<__uint128_t bitsize>
	std::istream& operator>>(std::istream& cin, selected_type128::BigUint<bitsize> &input)
	{
		std::string num;
		cin >> num;
		input = num;
		return cin;
	}

	// stringstream operator
	template<uint16_t bitsize>
	std::stringstream& operator>>(std::stringstream& ss, selected_type16::BigUint<bitsize> &input)
	{
		std::string num;
		ss >> num;
		input = num;
		return ss;
	}

	// stringstream operator
	template<uint32_t bitsize>
	std::stringstream& operator>>(std::stringstream& ss, selected_type32::BigUint<bitsize> &input)
	{
		std::string num;
		ss >> num;
		input = num;
		return ss;
	}

	// stringstream operator
	template<uint64_t bitsize>
	std::stringstream& operator>>(std::stringstream& ss, selected_type64::BigUint<bitsize> &input)
	{
		std::string num;
		ss >> num;
		input = num;
		return ss;
	}

	// stringstream operator
	template<__uint128_t bitsize>
	std::stringstream& operator>>(std::stringstream& ss, selected_type128::BigUint<bitsize> &input)
	{
		std::string num;
		ss >> num;
		input = num;
		return ss;
	}

}; /* NAMESPACE BIGINT */

// include here because of template class and function
#include "bigint.cpp"

#endif /* BIGINT_H */
