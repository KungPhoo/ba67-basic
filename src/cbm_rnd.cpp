#include <cstdint>
#include <cmath>
#include <array>
#include "cpu-6502.h"

class cbm_float {
public:
    // Input/output structure (same layout)

    // Note that this amounts to at least six bytes per floating point number,
    // but the routines provided for moving numbers between FAC, ARG and arbitrary RAM addresses
    // use a compression "trick" so that floating point numbers stored in RAM only take up five bytes:
    // Since the mantissa is always in the 0.5-to-1 range, the first binary digit will always
    // be a "1" — no need to store that. When storing a number in RAM, that "invariant 1" is
    // replaced by the sign bit, and when reading numbers from RAM, the sign bit is moved to the
    // separate sign byte in FAC or ARG, and the invariant first mantissa digit is restored to "1".

    uint8_t exp; // $61
    uint8_t mant[4]; // $62-$65 (MSB first) the high bit is always 1 - not the sign!
    uint8_t sign; // $66 the sign is stored here
    uint8_t round; // $70

    uint8_t& operator[](int i) {
        if (i == 0) {
            return exp;
        }
        if (i <= 4) {
            return mant[i - 1]; // note: mant is at indices 1–4
        }
        if (i == 5) {
            return sign;
        }
        if (i == 6) {
            return round;
        }
        return exp;
    }
    uint8_t operator[](int i) const {
        if (i == 0) {
            return exp;
        }
        if (i <= 4) {
            return mant[i - 1]; // note: mant is at indices 1–4
        }
        if (i == 5) {
            return sign;
        }
        if (i == 6) {
            return round;
        }
        return 0;
    }

    operator double() const {
        if (!exp) {
            return 0.0;
        }
        uint32_t mantissa = (((mant[0] & 0x7F) | 0x80u) << 24) + (mant[1] << 16) + (mant[2] << 8) + (mant[3] << 0);
        int32_t e         = exp;
        e -= 0x81;
        double d = std::pow(2.0, e) * double(mantissa) / double(0x80u << 24);
        if (sign) {
            d = -d;
        }
        return d;
    }

    cbm_float()
        : cbm_float(0.0) { }
    cbm_float(const cbm_float& f) { *this = f; }

    cbm_float(uint8_t e, uint8_t m1, uint8_t m2, uint8_t m3, uint8_t m4) {
        exp     = e;
        mant[0] = m1;
        mant[1] = m2;
        mant[2] = m3;
        mant[3] = m4;
        sign    = 0;
        round   = 0;
    }

    cbm_float(double d) {
        MakeCBMFloat(d);
    }
    cbm_float& operator=(const cbm_float& f) = default;

private:
    /*******************************************************************************
     * https://www.lemon64.com/forum/viewtopic.php?t=84300
     * void MakeCBMFloat(double d,Uint8_t *pCBM)
     *
     * conversion for IEEE double to CBM float.
     *
     * IEEE754 double : sign, 11 bit exponent, bias = 1023, 52 bits significand (53).
     * CBMFloat       : 8 bit exponent, bias = 128, sign, 31 bits significand (32).
     *
     * Both formats have an implied '1' bit at the start of the significand.
     * IEEE has LSB first, CBM MSB first.
     *
     * Does not handle IEEE denormals etc.
     *******************************************************************************/
    void MakeCBMFloat(double d) {
        typedef union {
            double IEEE754Double; // 8 bytes
            uint8_t Dbytes[8];
        } FLTU;
        FLTU CV;
        uint8_t Sign;
        int Exponent;
        int i;

        CV.IEEE754Double = d;
        Sign             = CV.Dbytes[7] & 0x80; // sign = MSBit
        Exponent         = (CV.Dbytes[7] & 0x7f) << 4; // 7 digits exponent ..
        Exponent |= ((CV.Dbytes[6] & 0xf0) >> 4); // .. plus 4 more
        if (Exponent != 0) // special case: exponent is zero, don't change it
        {
            Exponent -= 1022; // CBM Needs exponent offset +1, uses exp==0 for 0.000
            if ((Exponent < -128) || (Exponent > 127)) {
                // Error(E_OVERFLOW);
                Exponent = 127;
            }
            exp = Exponent + 128;
        } else {
            exp = 0;
        }

        // signif. starts at bit 3 from Dbytes[6], CBM needs it in bit 6
        // shift signicand 3 places to the left
        CV.Dbytes[6] &= 0x0f; // get rid of exponent bits
        for (i = 0; i < 3; i++) {
            CV.Dbytes[6] <<= 1;
            if (CV.Dbytes[5] & 0x80) {
                CV.Dbytes[6] |= 0x01;
            }
            CV.Dbytes[5] <<= 1;
            if (CV.Dbytes[4] & 0x80) {
                CV.Dbytes[5] |= 0x01;
            }
            CV.Dbytes[4] <<= 1;
            if (CV.Dbytes[3] & 0x80) {
                CV.Dbytes[4] |= 0x01;
            }
            CV.Dbytes[3] <<= 1;
            if (CV.Dbytes[2] & 0x80) {
                CV.Dbytes[3] |= 0x01;
            }
            CV.Dbytes[2] <<= 1;
        }
        // we don't store the sign here
        mant[0] = CV.Dbytes[6] | 0x80; // | Sign;
        mant[1] = CV.Dbytes[5];
        mant[2] = CV.Dbytes[4];
        mant[3] = CV.Dbytes[3] + ((CV.Dbytes[2] & 0x80) ? 1 : 0);
        sign    = (d >= 0) ? 0x00 : 0x80; // we keep the sign here, separately
        round   = 0;
    }

#if 0 // all this does not work
public:
    void operator*=(const cbm_float& b) {
        cbm_float::fac_mul(this, &b);
    }
    void operator+=(const cbm_float& b) {
        cbm_float::fac_add(this, &b);
    }

    cbm_float operator*(const cbm_float& b) {
        cbm_float a(*this);
        a *= b;
        return a;
    }
    cbm_float operator+(const cbm_float& b) {
        cbm_float a(*this);
        a += b;
        return a;
    }

private:
    // Pack mantissa + rounding into 40-bit value (top-aligned in 64-bit)
    static inline uint64_t pack40(const cbm_float* f) {
        return ((uint64_t)f->mant[0] << 32) | ((uint64_t)f->mant[1] << 24) | ((uint64_t)f->mant[2] << 16) | ((uint64_t)f->mant[3] << 8) | (uint64_t)f->round;
    }

    // Unpack back into bytes
    static inline void unpack40(cbm_float* f, uint64_t v) {
        f->mant[0] = (v >> 32) & 0xFF;
        f->mant[1] = (v >> 24) & 0xFF;
        f->mant[2] = (v >> 16) & 0xFF;
        f->mant[3] = (v >> 8) & 0xFF;
        f->round   = v & 0xFF;
    }

public:
    void NORMAL() {
        // zero check
        if ((mant[0] | mant[1] | mant[2] | mant[3]) == 0) {
            exp = 0;
            return;
        }

        uint8_t shift = 0;

        // --- BYTE NORMALIZATION (<< 8 each step) ---
        while (mant[0] == 0 && shift < 32) {
            mant[0] = mant[1];
            mant[1] = mant[2];
            mant[2] = mant[3];
            mant[3] = round;
            round   = 0;

            shift += 8;
        }

        // adjust exponent
        exp -= shift;

        // --- BIT NORMALIZATION ---
        while ((mant[0] & 0x80) == 0) // note: ROM works with bit7 here
        {
            uint8_t c1 = (mant[1] >> 7);
            uint8_t c2 = (mant[2] >> 7);
            uint8_t c3 = (mant[3] >> 7);
            uint8_t c4 = (round >> 7);

            mant[0] = (mant[0] << 1) | c1;
            mant[1] = (mant[1] << 1) | c2;
            mant[2] = (mant[2] << 1) | c3;
            mant[3] = (mant[3] << 1) | c4;
            round   = (round << 1);

            exp--;
        }
    }

    static uint64_t shr40_round(uint64_t m) {
        uint8_t round = m & 0xFF;
        uint8_t lsb   = (m >> 8) & 1;

        // shift entire 40-bit value
        m >>= 1;

        // sticky behavior: once any bit is lost, round != 0
        if (round & 1) {
            m |= 1ULL; // set sticky bit
        }

        return m & 0xFFFFFFFFFFULL;
    }
    static void fac_add(cbm_float* a, const cbm_float* b) {
        // Handle zero shortcuts
        if (a->exp == 0) {
            *a = *b;
            return;
        }
        if (b->exp == 0) {
            return;
        }

        uint64_t ma = pack40(a);
        uint64_t mb = pack40(b);

        ma &= 0xFFFFFFFFFFULL;
        mb &= 0xFFFFFFFFFFULL;

        uint8_t ea = a->exp;
        uint8_t eb = b->exp;

        // --------------------------------------------------
        // Align exponents (shift smaller one RIGHT)
        // --------------------------------------------------
        while (ea < eb) {
            ma = shr40_round(ma);
            ea++;
        }

        while (eb < ea) {
            mb = shr40_round(mb);
            eb++;
        }


        // --------------------------------------------------
        // Add or subtract mantissas depending on sign
        // --------------------------------------------------
        uint64_t result;
        uint8_t sign = 0;

        if (a->sign == b->sign) {
            sign = a->sign;

            result = ma + mb;

            if (result & (1ULL << 40)) {
                uint64_t carry = result & 1ULL;

                result >>= 1;

                if (carry) {
                    result |= (1ULL << 39); // correct bit position
                }
                ea++;
            }
            result &= 0xFFFFFFFFFFULL;
        } else {
            // subtraction
            if (ma >= mb) {
                result = ma - mb;
                sign   = a->sign;
            } else {
                result = mb - ma;
                sign   = b->sign;
            }

            if (result == 0) {
                a->exp     = 0;
                a->sign    = 0;
                a->mant[0] = a->mant[1] = a->mant[2] = a->mant[3] = a->round = 0;
                return;
            }
        }

        // Store back
        a->exp  = ea;
        a->sign = sign;

        unpack40(a, result);

        double test(*a);

        // Normalize (critical!)
        a->NORMAL();
        // normalizeFAC1_bitexact(a);
    }


    typedef struct {
        uint64_t lo; // lower 40 bits used
        uint64_t hi; // upper 40 bits used
    } u80;

    // shift left (80-bit)
    static inline void shl80(u80* v) {
        v->hi = ((v->hi << 1) | (v->lo >> 39)) & 0xFFFFFFFFFFULL;
        v->lo = (v->lo << 1) & 0xFFFFFFFFFFULL;
    }

    // add 40-bit value into 80-bit
    static inline void add80(u80* v, uint64_t x) {
        uint64_t old_lo = v->lo;
        v->lo           = (v->lo + x) & 0xFFFFFFFFFFULL;

        // carry from low → high
        if (v->lo < old_lo) {
            v->hi = (v->hi + 1) & 0xFFFFFFFFFFULL;
        }
    }

    // 40x40 multiply → 80-bit
    static u80 mul40(uint64_t a, uint64_t b) {
        u80 result = { 0, 0 };

        for (int i = 0; i < 40; i++) {
            if (b & 1) {
                add80(&result, a);
            }

            b >>= 1;
            shl80(&result);
        }

        return result;
    }

    static void fac_mul(cbm_float* a, const cbm_float* b) {

        *a = ((double)*a * (double)*b);
        return;



        // Zero shortcut
        if (a->exp == 0 || b->exp == 0) {
            a->exp     = 0;
            a->sign    = 0;
            a->mant[0] = a->mant[1] = a->mant[2] = a->mant[3] = a->round = 0;
            return;
        }

        uint64_t ma = pack40(a) & 0xFFFFFFFFFFULL;
        uint64_t mb = pack40(b) & 0xFFFFFFFFFFULL;

        // --------------------------------------------------
        // 40-bit × 40-bit -> 80-bit (we keep top 48 bits)
        // --------------------------------------------------
        u80 p = mul40(ma, mb);

        // Take the top 40 bits (like original routine expects)
        uint64_t result = p.hi;


        // --------------------------------------------------
        // Exponent addition (with bias handling)
        // --------------------------------------------------
        uint8_t exp = (uint8_t)(a->exp + b->exp);

        // Normalize if top bit not set
        if (!(result & (1ULL << 39))) {
            result <<= 1;
            exp--;
        }

        // Sign
        uint8_t sign = a->sign ^ b->sign;

        // Store
        a->exp  = exp;
        a->sign = sign;

        unpack40(a, result);

        a->NORMAL();
        // normalizeFAC1_bitexact(a);
    }
#endif
};




double cbm_rnd(double x) {
#if 1
    // use the 6502 CPU to actually perform the CBM BASIC RND() routine
    cbm_float fx(x);
    static CPU6502 cpu {};

    for (int i = 0; i < 7; ++i) {
        cpu.RAM[0x61 + i] = fx[i];
    }

    cpu.sys(0xE097);
    while (cpu.PC != 0xBBFB && !cpu.cpuJam) {
        cpu.executeNext();
    }
    for (int i = 0; i < 7; ++i) {
        fx[i] = cpu.RAM[0x61 + i];
    }
    return double(fx);

#else // simple implementation - not bit perfect
    static cbm_float seed(0x80, 0x4F, 0xC7, 0x52, 0x58);
    if (x < 0.0) {
        seed = cbm_float(x); // -1 is 81 80 00 00 00
    } else if (x > 0.0) {
        x = (double)seed; // this part is not bit perfect and I could not get it converted
        x *= 11879546.0;
        x += 3.927677739E-8;
        seed = x;
        // const cbm_float RMULZC(0x98, 0x35, 0x44, 0x7A, 0x00);
        // const cbm_float RADDZC(0x68, 0x28, 0xB1, 0x46, 0x00);
        // seed *= RMULZC; // $0061: 7f b5 fb 29 02 s 35 $0070 f4
        // seed += RADDZC; // $0061: 7f b5 fb 2a 54 s 35 $0070 56
    }

    // .E0D3 byte swap
    std::swap(seed[1], seed[4]);
    std::swap(seed[2], seed[3]); // -1 goes 81 00 00 00 80

    // .E0E3 LDA #$00; STA $66
    seed.sign = 0;
    // .E0E7 LDA $61; STA $70
    seed.round = seed.exp;
    // .E0EB LDA #$80; STA $61
    seed.exp = 0x80;

    // .E0EF JSR B8D7 NORMAL
    seed.NORMAL(); // 80 00 00 00 80 -> 68 80 81 00 00

    // seed[1] &= (~0x80);
    seed.sign     = 0;
    double normal = seed; // $008B: 68 00 81 00 00

    // after rnd(-1);rnd(1): $008B IS 7F 28 55 F7 6B (0.328780872)

    // .E0F2 LDX #$8B; LDY #$00; JMP BBD4 MOVMF
    // break at BBFB yields 68 80 81 00 00

    return (double)seed;
#endif
}


/*
FOR I=1 TO 10:PRINT RND(-I);RND(1):NEXT
 2.99196472e-08  .328780872
 2.99205567e-08  .865554613
 4.48217179e-08  .931279155
 2.99214662e-08  .402343613
 3.73720468e-08  .898884767
 4.48226274e-08  .199681314
 5.2273208e-08  .451873339
 2.99223757e-08  .935211164
 3.3647666e-08  .18740319
 3.73729563e-08  .435673767

READY.

10 PRINT RND(-1)
20 FOR I=1 TO 10: PRINT RND(1);RND(1)
30 NEXT
RUN
 2.99196472E-08
 .328780872  .978964086
 .895758909  .161031701
 .0224078245  .0365292135
 .128172149  .504270478
 .675061268  .784099338
 .155153367  .513626032
 .140994413  .957709718
 .644601505  .45621782
 .160543778  .596096987
 .156785717  .1307581

READY.
*/