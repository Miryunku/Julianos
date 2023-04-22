#include "pch.h"
#include "SHA.h"

namespace {
    uint32_t hash0 = 0x6a09e667;
    uint32_t hash1 = 0xbb67ae85;
    uint32_t hash2 = 0x3c6ef372;
    uint32_t hash3 = 0xa54ff53a;
    uint32_t hash4 = 0x510e527f;
    uint32_t hash5 = 0x9b05688c;
    uint32_t hash6 = 0x1f83d9ab;
    uint32_t hash7 = 0x5be0cd19;

    uint32_t rounds[64] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    // This function converts text to its textual binary representation
    std::string str_to_bitstr(const std::wstring& text)
    {
        std::string bits;
        bits.reserve(text.size() * 2 * 8);
        for (size_t i = 0; i < text.size(); ++i)
        {
            uint16_t elem = static_cast<uint16_t>(text[i]);
            // Byte 1
            uint8_t byte1 = static_cast<uint8_t>(elem >> 8);
            uint8_t mask = 0b10000000;
            for (int32_t j = 0; j < 7; ++j)
            {
                if ((byte1 & mask) == 0) { bits.push_back(u8'0'); }
                else { bits.push_back(u8'1'); }
                mask >>= 1;
            }
            // Byte 2
            uint8_t byte2 = static_cast<uint8_t>(elem & 0b0000000011111111);
            mask <<= 7;
            for (int32_t j = 0; j < 7; ++j)
            {
                if ((byte2 & mask) == 0) { bits.push_back(u8'0'); }
                else { bits.push_back(u8'1'); }
                mask >>= 1;
            }
        }
        return bits;
    }

    std::string integer32_to_bitstr(uint32_t integer)
    {
        std::string bits;
        bits.reserve(34);
        uint32_t mask = 0b10000000000000000000000000000000;
        for (uint8_t i = 0; i < 32; ++i)
        {
            if ((integer & mask) == 0) { bits.push_back(u8'0'); }
            else { bits.push_back(u8'1'); }
            mask >>= 1;
        }
        return bits;
    }

    constexpr uint32_t bitwise_left_rotate(uint32_t number, uint8_t rotate)
    {
        return (number << rotate) | (number >> (32u - rotate));
    }

    constexpr uint32_t bitwise_right_rotate(uint32_t number, uint8_t rotate)
    {
        return (number >> rotate) | (number << (32u - rotate));
    }

    constexpr uint32_t sigma0(uint32_t number)
    {
        return bitwise_right_rotate(number, 7u) ^ bitwise_right_rotate(number, 18u) ^ (number >> 3u);
    }

    constexpr uint32_t sigma1(uint32_t number)
    {
        return bitwise_right_rotate(number, 17u) ^ bitwise_right_rotate(number, 19u) ^ (number >> 10u);
    }

    constexpr uint32_t csigma0(uint32_t number)
    {
        return bitwise_right_rotate(number, 2u) ^ bitwise_right_rotate(number, 13u) ^ bitwise_right_rotate(number, 22u);
    }

    constexpr uint32_t csigma1(uint32_t number)
    {
        return bitwise_right_rotate(number, 6u) ^ bitwise_right_rotate(number, 11u) ^ bitwise_right_rotate(number, 25u);
    }

    constexpr uint32_t ch(uint32_t e, uint32_t f, uint32_t g)
    {
        return (e & f) ^ ((~e) & g);
    }

    constexpr uint32_t maj(uint32_t a, uint32_t b, uint32_t c)
    {
        return (a & b) ^ (a & c) ^ (b & c);
    }
}

std::array<uint32_t, 8> sha2_256(const std::wstring& text)
{
    // Preprocessing stage
    uint32_t text_bit_length = text.size() * 2 * 8;
    uint32_t zero_bits_to_append = 0;
    while ((text_bit_length + 1 + zero_bits_to_append + 64) % 512 != 0) { ++zero_bits_to_append; }
    // Begin with the original bit representation of the text to hash
    std::string bit_text = str_to_bitstr(text);
    // Append 1 on bit
    bit_text.push_back(u8'1');
    // Append the necessary off bits
    for (uint32_t i = 0; i < zero_bits_to_append; ++i) { bit_text.push_back(u8'0'); }
    // Append the bit length of the original text to hash, as a 64 bit Big-Endian binary string
    for (uint8_t i = 0; i < 32; ++i) { bit_text.push_back(u8'0'); }
    bit_text.append(integer32_to_bitstr(text_bit_length));

    // Cut bit_text into 512-bit chunks
    std::vector<std::string> chunks;
    chunks.reserve(50); // small optimization, depending on how many chunks are produced
    std::string chunk;
    chunk.reserve(512);
    uint32_t sentinel = 0;
    for (size_t i = 0; i < bit_text.size(); ++i)
    {
        chunk.push_back(bit_text[i]);
        ++sentinel;
        if (sentinel == 512) // there won't be a leftover chunk because bit_text' size is a multiple of 512
        {
            chunks.emplace_back(chunk);
            chunk.clear();
            sentinel = 0;
        }
    }

    // Do things with each chunk
    for (std::string achunk : chunks)
    {
        uint32_t schedule[64] = {0};
        // Create 16 integers from the chunk and store them in the first 16 slots of schedule
        char* iter1 = achunk.data();
        char* iter2 = iter1 + 32;
        //size_t offset = 0;
        for (size_t i = 0; i < 16; ++i)
        {
            uint32_t integer;
            std::from_chars(iter1, iter2, integer, 2);
            //uint32_t integer = std::stoul(achunk.substr(offset, 32), nullptr, 2);
            schedule[i] = integer;
            //offset += 32;
            iter1 += 32;
            iter2 = iter1 + 32;
        }

        // Compute values for the remaining 48 slots
        for (size_t i = 16; i < 64; ++i)
        {
            schedule[i] = sigma1(schedule[i - 2]) + schedule[i - 7] + sigma0(schedule[i - 15]) + schedule[i - 16];
        }

        // Create 8 integers and initialize them with the current hash values
        uint32_t a = hash0;
        uint32_t b = hash1;
        uint32_t c = hash2;
        uint32_t d = hash3;
        uint32_t e = hash4;
        uint32_t f = hash5;
        uint32_t g = hash6;
        uint32_t h = hash7;

        // "Randomize" the values of the 8 integers
        for (size_t i = 0; i < 64; ++i)
        {
            uint32_t t1 = h + csigma1(e) + ch(e, f, g) + rounds[i] + schedule[i];
            uint32_t t2 = csigma0(a) + maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        // Update the final values of the hash
        hash0 += a;
        hash1 += b;
        hash2 += c;
        hash3 += d;
        hash4 += e;
        hash5 += f;
        hash6 += g;
        hash7 += h;
    }
    std::array<uint32_t, 8> thehash;
    thehash[0] = hash0;
    thehash[1] = hash1;
    thehash[2] = hash2;
    thehash[3] = hash3;
    thehash[4] = hash4;
    thehash[5] = hash5;
    thehash[6] = hash6;
    thehash[7] = hash7;
    return thehash;
}