#ifndef __CRYPTO_SIZES_H__
#define __CRYPTO_SIZES_H__

#define PSA_BITS_TO_BYTES(bits) (((bits) + 7u) / 8u)

#define PSA_ECDSA_SIGNATURE_SIZE(curve_bits)    \
    (PSA_BITS_TO_BYTES(curve_bits) * 2u)

#endif /* __CRYPTO_SIZES_H__ */
