#include <common.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

typedef struct maes_s
{
    uint8_t key[512];
    uint32_t key_len;
    EVP_CIPHER_CTX e_ctx;
    EVP_CIPHER_CTX d_ctx;
} maes_t;

static maes_t _maes;

/**************************************************************************
 name	: aes_init
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
int8_t aes_init(uint8_t *key_data, uint32_t key_data_len)
{
    int32_t i, nrounds = 5;
    uint8_t key[32], iv[32];
    uint32_t salt[] = { 12345, 54321 };

    memset((uint8_t *)&_maes, 0, sizeof(maes_t));

    memcpy(_maes.key, key_data, key_data_len);
    _maes.key_len = key_data_len;

    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), (uint8_t *)salt, key_data,
        key_data_len, nrounds, key, iv);
    if (i != 32) {
        return -1;
    }

    EVP_CIPHER_CTX_init(&_maes.e_ctx);
    EVP_EncryptInit_ex(&_maes.e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(&_maes.d_ctx);
    EVP_DecryptInit_ex(&_maes.d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

    return 0;
}

/**************************************************************************
 name	: aes_encrypt
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint32_t aes_encrypt(uint8_t *data, uint32_t len)
{
    int32_t c_len = len + AES_BLOCK_SIZE, f_len = 0;
    uint8_t *cipher = calloc(1, c_len);
    EVP_EncryptInit_ex(&_maes.e_ctx, NULL, NULL, NULL, NULL);
    EVP_EncryptUpdate(&_maes.e_ctx, cipher, &c_len, data, len);
    EVP_EncryptFinal_ex(&_maes.e_ctx, cipher + c_len, &f_len);
    len = c_len + f_len;

    memcpy(data, cipher, len);
    free(cipher);
    return len;
}

/**************************************************************************
 name	: aes_decrypt
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
uint32_t aes_decrypt(uint8_t *data, uint32_t len)
{
    int32_t p_len = len, f_len = 0;
    uint8_t *plain = calloc(1, p_len + AES_BLOCK_SIZE);
    EVP_DecryptInit_ex(&_maes.d_ctx, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(&_maes.d_ctx, plain, &p_len,
        (const unsigned char *)data, len);
    EVP_DecryptFinal_ex(&_maes.d_ctx, plain + p_len, &f_len);
    len = p_len + f_len;
    memcpy(data, plain, len);
    free(plain);
    return len;
}
