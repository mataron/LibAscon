/**
 * @file
 * LibAscon internal header file.
 *
 * Common code (mostly the sponge state permutations and conversion utilities)
 * applied during encryption, decryption and hashing.
 *
 * @license Creative Commons Zero (CC0) 1.0
 * @authors see AUTHORS.md file
 */

#ifndef ASCON_INTERNAL_H
#define ASCON_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include "ascon.h"

#if DEBUG || MINSIZEREL || _MSVC
    #define ASCON_INLINE
#else
    #define ASCON_INLINE inline
#endif

/* Definitions of the initialisation vectors used to initialise the sponge
 * state for AEAD and the two types of hashing functions. */
#define PERMUTATION_12_ROUNDS 12
#define PERMUTATION_8_ROUNDS 8
#define PERMUTATION_6_ROUNDS 6
#define XOF_IV ( \
      ((uint64_t)(8 * (ASCON_RATE))     << 48U) \
    | ((uint64_t)(PERMUTATION_12_ROUNDS) << 40U) \
    )
#define AEAD128_IV ( \
       ((uint64_t)(8 * (ASCON_AEAD128_KEY_LEN)) << 56U) \
     | ((uint64_t)(8 * (ASCON_RATE))         << 48U) \
     | ((uint64_t)(PERMUTATION_12_ROUNDS)     << 40U) \
     | ((uint64_t)(PERMUTATION_6_ROUNDS)     << 32U) \
     )
#define AEAD128a_IV ( \
       ((uint64_t)(8 * (ASCON_AEAD128a_KEY_LEN)) << 56U) \
     | ((uint64_t)(8 * ASCON_DOUBLE_RATE)    << 48U) \
     | ((uint64_t)(PERMUTATION_12_ROUNDS)     << 40U) \
     | ((uint64_t)(PERMUTATION_8_ROUNDS)    << 32U) \
     )
#define AEAD80pq_IV ( \
       ((uint64_t)(8 * (ASCON_AEAD80pq_KEY_LEN)) << 56U) \
     | ((uint64_t)(8 * ASCON_RATE)               << 48U) \
     | ((uint64_t)(PERMUTATION_12_ROUNDS)        << 40U) \
     | ((uint64_t)(PERMUTATION_6_ROUNDS)         << 32U) \
     )
#define HASH_IV ( \
      ((uint64_t)(8 * (ASCON_RATE))     << 48U) \
    | ((uint64_t)(PERMUTATION_12_ROUNDS) << 40U) \
    | ((uint64_t)(8 * ASCON_HASH_DIGEST_LEN)) \
    )

/**
 * @internal
 * Applies 0b1000...000 right-side padding to a uint8_t[8] array of
 * `bytes` filled elements..
 */
#define PADDING(bytes) (0x80ULL << (56 - 8 * ((unsigned int) (bytes))))

/**
 * @internal
 * Simple minimum out of 2 arguments.
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @internal
 * States used to understand when to finalise the associated data.
 */
typedef enum
{
    ASCON_FLOW_NO_ASSOC_DATA = 0,
    ASCON_FLOW_SOME_ASSOC_DATA = 1,
    ASCON_FLOW_ASSOC_DATA_FINALISED = 2,
} ascon_flow_t;

/**
 * @internal
 * Converts an array of 8 bytes, out of which the first n are used, to a
 * uint64_t value.
 *
 * Big endian encoding.
 */
uint64_t bytes_to_u64(const uint8_t* bytes, uint_fast8_t n);

/**
 * Converts a uint64_t value to an array of n bytes, truncating the result
 * if n < 8.
 *
 * Big endian encoding.
 */
void u64_to_bytes(uint8_t* bytes, uint64_t x, uint_fast8_t n);

/**
 * @internal
 * Creates a mask to extract the n most significant bytes of a uint64_t.
 */
uint64_t byte_mask(uint_fast8_t n);

/**
 * @internal
 * Ascon sponge permutation with 12 rounds, known as permutation-a.
 */
void ascon_permutation_a12(ascon_sponge_t* sponge);

/**
 * @internal
 * Ascon sponge permutation with 8 rounds.
 */
void ascon_permutation_b8(ascon_sponge_t* const sponge);

/**
 * @internal
 * Ascon sponge permutation with 6 rounds, known as permutation-b.
 */
void ascon_permutation_b6(ascon_sponge_t* sponge);

/**
 * @internal
 * Initialises the AEAD128 or AEAD128a online processing.
 */
void ascon_aead_init(ascon_aead_ctx_t* ctx,
                     const uint8_t* key,
                     const uint8_t* nonce,
                     uint64_t iv);

/**
 * @internal
 * Handles the finalisation of the associated data before any plaintext or
 * ciphertext is being processed for Ascon128 and Ascon80pq
 *
 * MUST be called ONLY once! In other words, when
 * ctx->bufstate.assoc_data_state == ASCON_FLOW_ASSOC_DATA_FINALISED
 * then it MUST NOT be called anymore.
 *
 * It handles both the case when some or none associated data was given.
 */
void ascon_aead128_80pq_finalise_assoc_data(ascon_aead_ctx_t* ctx);

/**
 * @internal
 * Generates an arbitrary-length tag from a finalised state for all AEAD
 * ciphers.
 *
 * MUST be called ONLY when all AD and PT/CT is absorbed and state is
 * prepared for tag generation.
 */
void ascon_aead_generate_tag(ascon_aead_ctx_t* ctx,
                             uint8_t* tag,
                             uint8_t tag_len);

/**
 * @internal
 * Function pointer representing the operation run by the
 * buffered_accumulation() when ASCON_RATE bytes ara available in the buffer to
 * be absorbed.
 *
 * @param[in, out] sponge the sponge state to absorb data into.
 * @param[out] data_out optional outgoing data from the sponge, which happens
 *       during encryption or decryption, but not during hashing.
 * @param[in] data_in the input data to be absorbed by the sponge.
 */
typedef void (* absorb_fptr)(ascon_sponge_t* sponge,
                             uint8_t* data_out,
                             const uint8_t* data_in);

/**
 * @internal
 * Buffers any input data into the bufstate and on accumulation of ASCON_RATE
 * bytes, runs the absorb function to process them.
 *
 * This function is used by the AEAD and hash implementations to enable
 * the Init-Update-Final paradigm. The update functions pass the absorb_fptr
 * function specific to them, while this function is the framework handling the
 * accumulation of data until the proper amount is reached.
 *
 * It is not used during the Final step, as that requires padding and special
 * additional operations such as tag/digest generation.
 *
 * @param[in, out] ctx the sponge and the buffer to accumulate data in
 * @param[out] data_out optional output data squeezed from the sponge
 * @param[in] data_in input data to be absorbed by the sponge
 * @param[in] absorb function that handles the absorption and optional squeezing
 *        of the sponge
 * @param[in] data_in_len length of the \p data_in in bytes
 * @param[in] rate buffer size, i.e. number of accumulated bytes after which
 *        an absorption is required
 * @return number of bytes written into \p data_out
 */
size_t buffered_accumulation(ascon_bufstate_t* ctx,
                             uint8_t* data_out,
                             const uint8_t* data_in,
                             absorb_fptr absorb,
                             size_t data_in_len,
                             uint8_t rate);

#ifdef __cplusplus
}
#endif

#endif  /* ASCON_INTERNAL_H */
