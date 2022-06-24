#include <iostream>
#include <string>
#include <memory>
#include <stdint.h>
#include <random>
#include <ctype.h>
#include <sstream>
#include <math.h>

#if !defined(UINT64_MAX)
    using uint64_t = unsigned long long;
#endif

// use GNU Multiple Precision Arithmetic Library for implementing 
// 2048-bit encryption

// Rivest Shamir & Adleman
namespace rsa
{
    uint64_t gen_pub_key(uint64_t eulers_totient, uint64_t p, uint64_t q)
    {
        // use random to have a non-const starting point
        std::random_device randdev;
        std::mt19937 generator(randdev() ^ time(NULL));
        std::uniform_int_distribution<uint64_t>
                                     distr(2, p/4);
        uint64_t pubkey;
            
        // pubkey has to be co-prime of n
        for(uint64_t c=distr(generator);c<eulers_totient;c++) {
            if(eulers_totient%c != 0) {
                if(c != q && c != p) {
                    // make sure c is prime using fermat's little theorem
                    if((uint64_t)(pow(2,c-1) - 1)%c == 0) {
                        pubkey = c;
                        break;
                    }
                }
            }
        }
        
        return pubkey;
    }

    uint64_t gen_priv_key(uint64_t eulers_totient, uint64_t pub_key)
    {
        //  e*d mod ϕ(n) = 1
        uint64_t privkey = 1;
        while(privkey%pub_key != 0) {
            privkey+=eulers_totient;
        }
        privkey/=pub_key;
        return privkey;
    }
    
    // check if private key is suitable for use
    bool verify_priv_key_use(uint64_t eulers_totient, uint64_t pub_key,
                             uint64_t priv_key, uint64_t n)
    {
        bool valid_priv_key = pub_key*priv_key %
                              eulers_totient == 1;
        return valid_priv_key;
    }
    
    bool verify_pubkey_use(uint64_t pubkey, uint64_t eulers_totient)
    {
        int issue_count = 0;
        // check if pubkey is bigger than 2
        if(pubkey <= 2) {
            std::cout << "\npubkey smaller than 2";
            issue_count++;
        }
        
        // check if gcd is one
        for(int c=2;c<eulers_totient;c++) {
            if(eulers_totient%c==0 && pubkey%c==0) {
                std::cout << "\ngcd is not one";
                issue_count++;
                break;
            }
        }
        if(issue_count == 0)
            return true;
        return false;
    }

	std::string encrypt(std::string plaintext, uint64_t n,
                        uint64_t pub_key)
	{
        std::string ciphertext;
        std::stringstream ss;
        for(uint64_t c=0;c<plaintext.length();c++) {
            ss << (uint64_t)pow(plaintext[c]-48,
                                pub_key) % n
               << " "; // space as delimeter
        }
        ss >> ciphertext;
        return ciphertext;
	}
    
    std::string decrypt(std::string ciphertext, uint64_t n, uint64_t 
                        priv_key) {
        std::string plaintext = "";
        std::stringstream ss;
        size_t pos = 0;
        while((pos = ciphertext.find(" ")) != std::string::npos) {
            // ciphertext of single char
            uint64_t char_ct;
            ss << ciphertext.substr(0, pos);
            ss >> char_ct;
            plaintext += (uint64_t)pow(char_ct, priv_key) % n;
            
            // reset ss and delete used value from ciphertext
            ss.clear();
            ciphertext.erase(0, pos+1);
        }
        return plaintext;
    }
};

int main() {
    uint64_t pubkey, q, p, priv_key,n;
    std::string plaintext, ciphertext;
    bool pubkey_usable = false;
    bool privkey_usable = false;
    bool q_prime = true;
    bool p_prime = true;

    // get user input for RSA parameters
    do {
        std::cout << "input q (prime number):\t";
        std::cin >> q;
        // use fermat's little theorem to find if q is a prime number
        uint64_t a = 2;
        q_prime = (uint64_t)(pow(a,q-1) - 1)%q == 0;
    } while(!q_prime);
    do {
        std::cout << "\ninput p (prime number):\t";
        std::cin >> p;

        // use fermat's little theorem to find if q is a prime number
        uint64_t a = 2;
        p_prime = (uint64_t)(pow(a,p-1) - 1)%p == 0;
    } while(!p_prime);
    // calculate Euler's totient since p and q are defined
    uint64_t eulers_totient = (p-1)*(q-1);
    n = q*p;
    do {
        std::cout << "\ninput public key:\t";
        std::cin >> pubkey;
    
        // if non digit public key inputted
        if(std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits
                            <std::streamsize>::max(), '\n');
            pubkey = rsa::gen_pub_key(eulers_totient,p,q);
            std::cout << "\npubkey:\t" << std::dec << pubkey
                      << std::endl;
            pubkey_usable = true;
        } else {
            // verify public key for RSA use
            pubkey_usable = rsa::verify_pubkey_use(pubkey,
                                                   eulers_totient);
            if(!pubkey_usable)
                std::cout << "\npublic key not usable";
        }
    } while(!pubkey_usable);
    do {
        std::cout << "\ninput private key:\t";
        std::cin >> priv_key;

        // if non digit private key inputted
        if(std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits
                            <std::streamsize>::max(), '\n');
            priv_key = rsa::gen_priv_key(eulers_totient, pubkey);
            std::cout << "\nprivkey:\t" << std::dec << priv_key
                      << std::endl;
            privkey_usable = true;
        } else {
            privkey_usable = rsa::verify_priv_key_use(eulers_totient, 
                                                      pubkey,
                                                      priv_key, n);
            if(!privkey_usable)
                std::cout << "\nprivate key not usable";
        }
    } while (!privkey_usable);
    std::cout << "\ninput plaintext:\t";
    std::cin >> plaintext;
    ciphertext = rsa::encrypt(plaintext, n, pubkey);
    std::cout << "ciphertext:\t" << ciphertext;
}
