#include <common.h>
#include <string.h>
#include <math.h>
#include <md5.h>

typedef unsigned DigestArray[4];

unsigned func0(unsigned abcd[])
{
    return (abcd[1] & abcd[2]) | (~abcd[1] & abcd[3]);
}

unsigned func1(unsigned abcd[])
{
    return (abcd[3] & abcd[1]) | (~abcd[3] & abcd[2]);
}

unsigned func2(unsigned abcd[])
{
    return abcd[1] ^ abcd[2] ^ abcd[3];
}

unsigned func3(unsigned abcd[])
{
    return abcd[2] ^ (abcd[1] | ~abcd[3]);
}

typedef unsigned (*DgstFctn)(unsigned a[]);

/**************************************************************************
 name	:
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
unsigned *md5_calc_table(unsigned *k)
{
    double s, pwr;
    int i;

    pwr = pow(2, 32);
    for (i = 0; i < 64; i++) {
        s = fabs(sin(1 + i));
        k[i] = (unsigned)(s * pwr);
    }
    return k;
}

/**************************************************************************
 name	: rol
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
unsigned rol(unsigned r, short N)
{
    unsigned mask1 = (1 << N) - 1;
    return ((r >> (32 - N)) & mask1) | ((r << N) & ~mask1);
}

/**************************************************************************
 name	: md5_init
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
unsigned *md5_init(char *msg, int *msg_len)
{
    /*Initialize Digest Array as A , B, C, D */
    static DigestArray h0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
    static DgstFctn ff[] = { &func0, &func1, &func2, &func3 };
    static short M[] = { 1, 5, 3, 7 };
    static short O[] = { 0, 1, 5, 0 };
#if 1
    static short rot0[] = { 7, 12, 17, 22 };
    static short rot1[] = { 5, 9, 14, 20 };
    static short rot2[] = { 4, 11, 16, 23 };
    static short rot3[] = { 6, 10, 15, 21 };
#else
    static short rot0[] = {65,23,123,3};
    static short rot1[] = {76,87,12,231};
    static short rot2[] = {1,0,43,175};
    static short rot3[] = {134,241,254,13};
#endif
    static short *rots[] = { rot0, rot1, rot2, rot3 };
    static unsigned kspace[64];
    static unsigned *k;
    int mlen = *msg_len;
    MD5union u;
    static DigestArray h;
    DigestArray abcd;
    DgstFctn fctn;
    short m, o, g;
    unsigned f;
    short *rotn;
    union
    {
        unsigned w[16];
        char b[64];
    } mm;
    int os = 0;
    int grp, grps, q, p;
    unsigned char *msg2;

    if (k == NULL)
        k = md5_calc_table(kspace);

    for (q = 0; q < 4; q++)
        h[q] = h0[q];   // initialize

    {
        grps = 1 + (mlen + 8) / 64;
        msg2 = ctech_malloc(64 * grps);
        memcpy(msg2, msg, mlen);
        msg2[mlen] = (unsigned char)0x80;
        q = mlen + 1;
        while (q < 64 * grps) {
            msg2[q] = 0;
            q++;
        }
        {
            MD5union u;
            u.w = 8 * mlen;
            q -= 8;
            memcpy(msg2 + q, &u.w, 4);
        }
    }

    for (grp = 0; grp < grps; grp++)
        {
        memcpy(mm.b, msg2 + os, 64);
        for (q = 0; q < 4; q++)
            abcd[q] = h[q];
        for (p = 0; p < 4; p++) {
            fctn = ff[p];
            rotn = rots[p];
            m = M[p];
            o = O[p];
            for (q = 0; q < 16; q++) {
                g = (m * q + o) % 16;
                f = abcd[1]
                    + rol(abcd[0] + fctn(abcd) + k[q + 16 * p] + mm.w[g],
                        rotn[q % 4]);

                abcd[0] = abcd[3];
                abcd[3] = abcd[2];
                abcd[2] = abcd[1];
                abcd[1] = f;
            }
        }
        for (p = 0; p < 4; p++)
            h[p] += abcd[p];
        os += 64;
    }
    ctech_free(msg2);
#if 0
    o=0;
//	printf("md5:");
    for (m=0; m<4; m++) {
        u.w = h[m];
        for (g=0;g<4;g++) {
            msg[o++]=u.b[g];
//			printf("%.2X", u.b[g]);
        }
    }
//	printf("\n");
#endif
    *msg_len = 16;
    return h;
}
/*
 int main( int argc, char *argv[] )
 {
 int j,k;
 const char *msg = "This code has been provided by C codechamp";
 printf("----------------------------------------------------\n");
 printf("-------------Made by C codechamp--------------------\n");
 printf("----------------------------------------------------\n\n");
 printf("\t MD5 ENCRYPTION ALGORITHM IN C \n\n");
 printf("Input String to be Encrypted using MD5 : \n\t%s",msg);
 unsigned *d = md5(msg, strlen(msg));
 MD5union u;
 printf("\n\n\nThe MD5 code for input string is : \n");
 printf("\t= 0x");
 for (j=0;j<4; j++){
 u.w = d[j];
 for (k=0;k<4;k++) printf("%02x",u.b[k]);
 }
 printf("\n");
 printf("\n\t MD5 Encyption Successfully Completed!!!\n\n");
 getchar();
 system("pause");
 return 0;
 }
 */
