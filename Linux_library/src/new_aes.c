#include <string.h>
#include <stdint.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "new_aes.h"

static unsigned char key[33];
static unsigned char iv[17];

static void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}

static int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

static int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

void new_aes_deinit(void) {
	/* Clean up */
	EVP_cleanup();
	ERR_free_strings();
}

void new_aes_init(void) {
	  // This key and iv values are shared with Android-App and so never change them //
	  key[0] =  100;
	  key[1] =  56;
	  key[2] =  42;
	  key[3] =  66;
	  key[4] =  100;
	  key[5] =  50;
	  key[6] =  112;
	  key[7] =  45;
	  key[8] =  71;
	  key[9] =  99;
	  key[10] =  98;
	  key[11] =  43;
	  key[12] =  48;
	  key[13] =  57;
	  key[14] =  115;
	  key[15] =  79;
	  key[16] =  103;
	  key[17] =  56;
	  key[18] =  87;
	  key[19] =  121;
	  key[20] =  111;
	  key[21] =  118;
	  key[22] =  56;
	  key[23] =  71;
	  key[24] =  104;
	  key[25] =  109;
	  key[26] =  49;
	  key[27] =  50;
	  key[28] =  42;
	  key[29] =  45;
	  key[30] =  120;
	  key[31] =  118;
	  key[32] =  3;

	  iv[0] =  57;
	  iv[1] =  79;
	  iv[2] =  117;
	  iv[3] =  55;
	  iv[4] =  115;
	  iv[5] =  70;
	  iv[6] =  120;
	  iv[7] =  115;
	  iv[8] =  97;
	  iv[9] =  82;
	  iv[10] =  116;
	  iv[11] =  66;
	  iv[12] =  110;
	  iv[13] =  113;
	  iv[14] =  50;
	  iv[15] =  49;
	  iv[16] =  3;

	  /* Initialise the library */
	  ERR_load_crypto_strings();
	  OpenSSL_add_all_algorithms();
	  OPENSSL_config(NULL);
}

inline int new_aes_enc_(uint8_t *src, int src_len, uint8_t *dst) {
	return  encrypt (src, src_len, key, iv, dst);
}

inline int reorder_data(uint8_t *src, int src_len, uint8_t *dst) {
	dst[0] = src[12];
	dst[1] = src[14];
	dst[2] = src[11];
	dst[3] = src[10];
	dst[4] = src[3];
	dst[5] = src[7];
	dst[6] = src[2];
	dst[7] = src[5];
	dst[8] = src[4];
	dst[9] = src[6];
	dst[10] = src[8];
	dst[11] = src[1];
	dst[12] = src[9];
	dst[13] = src[0];
	dst[14] = src[13];
	dst[15] = src[15];

	return 16;
}

inline int new_aes_dec_(uint8_t *src, int src_len, uint8_t *dst) {
	return decrypt(src, src_len, key, iv, dst);
}

inline int order_back_data(uint8_t *src, int src_len, uint8_t *dst) {
	dst[12] = src[0];
	dst[14] = src[1];
	dst[11] = src[2];
	dst[10] = src[3];
	dst[3] = src[4];
	dst[7] = src[5];
	dst[2] = src[6];
	dst[5] = src[7];
	dst[4] = src[8];
	dst[6] = src[9];
	dst[8] = src[10];
	dst[1] = src[11];
	dst[9] = src[12];
	dst[0] = src[13];
	dst[13] = src[14];
	dst[15] = src[15];
	return 16;
}
