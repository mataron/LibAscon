/**
 * @file
 */

#ifndef VECTORS_H
#define VECTORS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ascon.h"

// The awk commands retrieve the longest line of a given type in the file.
// Of that line, one has to count the hexstring length and divide by 2.

// awk '{print length, $0}' hash.txt |grep 'Msg ='|sort -nr|head -1
#define VECS_MAX_HASH_MESSAGE_LEN 1024
// awk '{print length, $0}' aead128.txt |grep 'PT ='|sort -nr|head -1
#define VECS_MAX_AEAD_PLAINTEXT_LEN 32
// awk '{print length, $0}' aead128.txt |grep 'AD ='|sort -nr|head -1
#define VECS_MAX_AEAD_ASSOC_DATA_LEN 32
// awk '{print length, $0}' aead128.txt |grep 'CT ='|sort -nr|head -1
#define VECS_MAX_AEAD_CIPHERTEXT_LEN 48

#define VECS_MAX_HEXBYTES_LEN 1024

typedef enum
{
    VECS_OK = 0,
    VECS_EOF = -1,
    VECS_IO_CANNOT_OPEN_FILE = 1,
    VECS_FORMAT_INCORRECT_COUNT_HDR = 2,
    VECS_FORMAT_INCORRECT_MESSAGE_HDR = 3,
    VECS_FORMAT_INCORRECT_DIGEST_HDR = 4,
    VECS_FORMAT_INCORRECT_KEY_HDR = 5,
    VECS_FORMAT_INCORRECT_NONCE_HDR = 6,
    VECS_FORMAT_INCORRECT_PLAINTEXT_HDR = 7,
    VECS_FORMAT_INCORRECT_ASSOC_DATA_HDR = 8,
    VECS_FORMAT_INCORRECT_CIPHERTEXT_HDR = 9,
    VECS_FORMAT_TOO_SHORT_HEXBYTES = 10,
    VECS_FORMAT_TOO_LARGE_HEXBYTES = 11,
    VECS_FORMAT_TOO_SHORT_PLAINTEXT = 12,
    VECS_FORMAT_TOO_LARGE_PLAINTEXT = 13,
    VECS_FORMAT_TOO_SHORT_DIGEST = 14,
    VECS_FORMAT_TOO_SHORT_KEY = 15,
    VECS_FORMAT_TOO_SHORT_NONCE = 16,
    VECS_FORMAT_TOO_SHORT_CIPHERTEXT = 17,
} vecs_err_t;

typedef struct
{
    uint8_t message[VECS_MAX_HASH_MESSAGE_LEN];
    uint8_t expected_digest[ASCON_HASH_DIGEST_LEN];
    size_t message_len;
} vecs_hash_t;

typedef struct
{
    uint8_t plaintext[VECS_MAX_AEAD_PLAINTEXT_LEN];
    uint8_t assoc_data[VECS_MAX_AEAD_ASSOC_DATA_LEN];
    uint8_t expected_ciphertext[VECS_MAX_AEAD_CIPHERTEXT_LEN];
    uint8_t key[ASCON_AEAD_KEY_LEN];
    uint8_t nonce[ASCON_AEAD_NONCE_LEN];
    uint8_t expected_tag[ASCON_AEAD_TAG_LEN];
    size_t plaintext_len;
    size_t assoc_data_len;
    size_t expected_ciphertext_len;
} vecs_aead_t;

typedef struct
{
    FILE* handle;
} vecs_ctx_t;

vecs_err_t vecs_init(vecs_ctx_t* ctx, const char* file_name);

vecs_err_t vecs_hash_next(vecs_ctx_t* ctx, vecs_hash_t* testcase);

vecs_err_t vecs_aead_next(vecs_ctx_t* ctx, vecs_aead_t* testcase);

void vecs_hash_log(const vecs_hash_t* testcase,
                   const uint8_t* obtained_digest);

void vecs_aead_log(const vecs_aead_t* testcase,
                   const uint8_t* obtained_ciphertext,
                   const uint8_t* obtained_tag,
                   uint64_t obtained_ciphertext_len);

#ifdef __cplusplus
}
#endif

#endif  /* VECTORS_H */
