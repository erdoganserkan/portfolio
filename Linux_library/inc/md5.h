#ifndef MD5_H_
#define MD5_H_

typedef union uwb
{
    unsigned w;
    unsigned char b[4];
} MD5union;

unsigned *md5_init(char *msg, int *msg_len);

#endif /* MD5_H_ */
