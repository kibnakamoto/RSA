#include <iostream>
#include <string>
#include <memory>
#include <stdint.h>
#include <random>
#include <ctype.h>
#include <sstream>
#include <math.h>
#include <iomanip>

#include "bigint.h"

// Rivest Shamir & Adleman
template<typename uint_type>
class Rsa
{
	public:
    uint_type gen_pub_key(uint_type eulers_totient, uint_type p, uint_type q)
    {
        // use random to have a non-const starting point
        uint_type pubkey;
		bool error;
            
        // pubkey has to be co-prime of n
        for(uint_type c=uint_type::random("2", p<<uint16_t(1u), error);c<eulers_totient;c++) {
            if(eulers_totient%c != "0") {
                if(c != q && c != p) {
                    // make sure c is prime using fermat's little theorem
                    if((pow(uint_type(2),c-"1"))%c == "1") {
                        pubkey = c;
                        break;
                    }
                }
            } else {
                // if loop ended and no public key found
                // generate new starting value
                if(c == eulers_totient-"1") {
                    c = uint_type::random("2", p<<uint16_t(1u), error);
                }
            }
        }
        
        return pubkey;
    }

    uint_type gen_priv_key(uint_type eulers_totient, uint_type pub_key)
    {
        //  e*d mod ϕ(n) = 1
        uint_type privkey = 1;
        while(privkey%pub_key != "0") {
			std::cout << std::endl << privkey << std::endl;
            privkey+=eulers_totient;
        }
        privkey/=pub_key;
        return privkey;
    }
    
    // check if private key is suitable for use
    bool verify_priv_key_use(uint_type eulers_totient, uint_type pub_key,
                             uint_type priv_key, uint_type n)
    {
        bool valid_priv_key = pub_key*priv_key %
                              eulers_totient == "1";
        return valid_priv_key;
    }
    
    bool verify_pubkey_use(uint_type pubkey, uint_type eulers_totient)
    {
        int issue_count = 0;
        // check if pubkey is bigger than 2
        if(pubkey <= "2") {
            std::cout << "\npubkey smaller than 2";
            issue_count++;
        }
        
        // check if gcd is one
        for(uint_type c=2;c<eulers_totient;c++) {
            if(eulers_totient%c=="0" && pubkey%c=="0") {
                std::cout << "\ngcd is not one";
                issue_count++;
                break;
            }
        }
        if(issue_count == 0)
            return true;
        return false;
    }

	void encrypt(std::string plaintext, uint_type n,
                        uint_type pub_key, uint_type *ct)
	{
        std::string ciphertext;
        std::stringstream ss;
        
		// encrypt data byte by byte
        for(size_t i=0;i<plaintext.length();i++) {
            ct[i] = (uint_type)pow(uint_type(plaintext[i]-48),
                                   pub_key) % n;
        }
	}
    
    std::string decrypt(std::string ciphertext, uint_type n, uint_type 
                        priv_key) {
        std::string plaintext = "";
        std::stringstream ss_plaintxt;
		auto mod = ciphertext.length()%64;
		decltype(uint_type::__get_op_size()) substr_size = uint_type::__get_op_size()<<4;
		decltype(uint_type::__get_op_size()) len = mod == 0 ? ciphertext.length()/substr_size :
														  	  ciphertext.length()/substr_size+1;
		uint_type *ct = new uint_type[len];

		// parse and decrypt ciphertext
		if(ciphertext.length() <= 64) {
			ct[0] = ciphertext;

            ss_plaintxt << (uint8_t)(pow(ct[0], (uint_type) priv_key)%n);
		} else {
        	for(decltype(uint_type::__get_op_size()) c=0;c<ciphertext.length()/substr_size;c++) {
        	    ct[c] = ciphertext.substr(c*substr_size,c*substr_size+substr_size);
        	    uint_type temp=0;

				// decrypt
            	ss_plaintxt << (uint8_t)((uint8_t)(pow(ct[0], (uint_type) priv_key)%n)+48);

        	}
		}
        plaintext = ss_plaintxt.str();
		delete[] ct;
        return plaintext;
    }
};

int main()
{
	typedef BigInt::BigUint<256> uint_type;
    uint_type pubkey, q, p, priv_key,n;
    std::string plaintext, ciphertext;
	auto rsa = Rsa<uint_type>();
    bool pubkey_usable = false;
    bool privkey_usable = false;
    bool q_prime = true;
    bool p_prime = true;
	std::cout << "\nIMPORTANT: ALL NUMBERS ARE HEX";

	// test
	// q = 7
	// p = 0x11
	// n = 119;
	// priv_key = 0x23;
	// pubkey = 11;
	// plaintext =  21+48;
	// uint_type *out = new uint_type[plaintext.length()];
    // std::cout << "\nplaintext before:\t " << plaintext << std::endl;
    // rsa.encrypt(plaintext, n, pubkey, out);
    // rsa.decrypt(ciphertext, n, priv_key);
    // std::cout << "\nplaintext:\t " << std::dec << plaintext[0]+0 << std::endl;
    // std::cout << "\nciphertext:\t " << (uint64_t)out[0] << std::endl;
	// delete[] out;
	// exit(0);
	// test end

    // get user input for RSA parameters
     do {
	 	wrong_inp:
         std::cout << std::endl << "input q (prime number):\t";
	 	try {
         	std::cin >> q;
	 	} catch(BigInt::wrong_type_error e) {
	 		std::cout << std::flush << "wrong input, error: BigInt::wrong_type_error, try again..." << std::endl;
	 		goto wrong_inp;
	 	}

         // use fermat's little theorem to find if q is a prime number
         uint_type a = 2;
         q_prime = pow(a,q-uint_type("1"))%q == "1";
	 	std::cout << std::endl << "is " << q << " prime: " << q_prime;
     } while(!q_prime);
     do {
	 	wrong_inp_p:
         std::cout << "\ninput p (prime number):\t";
	 	try {
         	std::cin >> p;
	 	} catch(BigInt::wrong_type_error e) {
	 		std::cout << std::flush << "wrong input, error: BigInt::wrong_type_error, try again..." << std::endl;
	 		goto wrong_inp_p;
	 	}

         // use fermat's little theorem to find if q is a prime number
         uint_type a = 2;
         p_prime = (uint_type)(pow(a,p-"1"))%p == "1";
	 	std::cout << std::endl << "is " << p << " prime: " << p_prime;
     } while(!p_prime);

     // calculate Euler's totient since p and q are defined
     uint_type eulers_totient = (p-"1")*(q-"1");
     n = q*p;

     // get public key
     do {
         std::cout << "\ninput public key:\t";
	 	bool catched = false;
	 	try {
         	std::cin >> pubkey;
	 	} catch(BigInt::wrong_type_error e) {
         	// if non digit public key inputted
             std::cin.clear();
             std::cin.ignore(std::numeric_limits
                             <std::streamsize>::max(), '\n');
             pubkey = rsa.gen_pub_key(eulers_totient,p,q);
             std::cout << "\npubkey:\t" << std::dec << pubkey
                       << std::endl;
             pubkey_usable = true;
	 		std::cout << std::endl << "is " << p << " usable public key: " << pubkey_usable;
	 		catched=true;
         } 
	 	if (!catched) {
             // verify public key for RSA use
             pubkey_usable = rsa.verify_pubkey_use(pubkey,
                                                    eulers_totient);
             if(!pubkey_usable)
                 std::cout << "\npublic key not usable";
         }
     } while(!pubkey_usable);

     // get private key
     do {
         std::cout << "\ninput private key:\t";
	 	bool catched = false;

	 	try {
         	std::cin >> priv_key;
	 	} catch(BigInt::wrong_type_error e) {
         	// if non digit private key inputted
             std::cin.clear();
             std::cin.ignore(std::numeric_limits
                             <std::streamsize>::max(), '\n');
             priv_key = rsa.gen_priv_key(eulers_totient, pubkey);
             std::cout << "\nprivkey:\t" << std::dec << priv_key
                       << std::endl;
             privkey_usable = true;
	 		catched=true;
         } 
	     if (!catched) {
              privkey_usable = rsa.verify_priv_key_use(eulers_totient, 
                                                        pubkey,
                                                        priv_key, n);
              if(!privkey_usable)
                  std::cout << "\nprivate key not usable:" << priv_key;
          }
     } while (!privkey_usable);
    
    // encrypt of decrypt
    std::string choice;
    std::cout << "\n(e)ncrypt or (d)ecrypt:\t";
    std::cin >> choice;
    if(choice == "e" || choice == "encrypt") {
        std::cout << "\ninput plaintext:\t";
        std::cin.ignore (std::numeric_limits<std::streamsize>::max(),
                         '\n'); 

        std::getline(std::cin, plaintext);
		uint_type ct[plaintext.length()];
        rsa.encrypt(plaintext, n, pubkey, ct);
		std::cout << "\nciphertext:\t" ;
		for(size_t i=0;i<plaintext.length();i++)
        	std::cout << std::setfill('0') << std::setw(uint_type::__get_op_size()<<4) << ct[i];
       	std::cout << std::endl;
    } else if(choice == "d" || choice == "decrypt") {
		std::string ciphertext;
        std::string plaintext;
        std::cout << "\ninput ciphertext:\t";
        std::cin >> ciphertext;
        plaintext = rsa.decrypt(ciphertext, n, priv_key);
        std::cout << "\nplaintext:\t " << std::dec << plaintext << std::endl;
    }
}
