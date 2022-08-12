#ifndef _NEW_AES_H_
#define _NEW_AES_H_

inline int new_aes_enc_(uint8_t *src, int src_len, uint8_t *dst);
inline int new_aes_dec_(uint8_t *src, int src_len, uint8_t *dst);

inline int reorder_data(uint8_t *src, int src_len, uint8_t *dst);
inline int order_back_data(uint8_t *src, int src_len, uint8_t *dst);

void new_aes_init(void);
void new_aes_deinit(void);

#endif /*  */
