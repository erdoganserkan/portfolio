#ifndef AES_H_
#define AES_H_

#include <stdio.h>
#include <stdint.h>

int8_t aes_init(uint8_t *key_data, uint32_t key_data_len);
uint32_t aes_encrypt(uint8_t *data, uint32_t len);
uint32_t aes_decrypt(uint8_t *data, uint32_t len);

#endif /* AES_H_ */
