// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include "misc.h"


//78
#ifndef BIGNUM_INTERNAL
typedef unsigned __int32 *Bignum; // this is only true if defined _MSC_VER && defined _M_IX86. If retargeting, see sshbn.h
#endif

struct RSAKey {
int bits;
int bytes;
#ifdef MSCRYPTOAPI
unsigned long exponent;
unsigned char *modulus;
#else
Bignum modulus;
Bignum exponent;
Bignum private_exponent;
Bignum p;
Bignum q;
Bignum iqmp;
#endif
char *comment;
};

struct dss_key {
	Bignum p, q, g, y, x;
};


//118
#ifndef PUTTY_UINT32_DEFINED
/* This makes assumptions about the int type. */
typedef unsigned int uint32;
#define PUTTY_UINT32_DEFINED
#endif
typedef uint32 word32;

// from int64.h
typedef struct {
	unsigned long hi, lo;
} uint64;

/* 134
* SSH2 RSA key exchange functions
*/
struct ssh_hash;
void *ssh_rsakex_newkey(char *data, int len);
void ssh_rsakex_freekey(void *key);
int ssh_rsakex_klen(void *key);
//void ssh_rsakex_encrypt(const struct ssh_hash *h, unsigned char *in, int inlen,
//	unsigned char *out, int outlen,
//	void *key);

typedef struct {
	uint32 h[4];
} MD5_Core_State;

struct MD5Context {
#ifdef MSCRYPTOAPI
	unsigned long hHash;
#else
	MD5_Core_State core;
	unsigned char block[64];
	int blkused;
	uint32 lenhi, lenlo;
#endif
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Simple(void const *p, unsigned len, unsigned char output[16]);

void *hmacmd5_make_context(void);
void hmacmd5_free_context(void *handle);
void hmacmd5_key(void *handle, void const *key, int len);
void hmacmd5_do_hmac(void *handle, unsigned char const *blk, int len,
	unsigned char *hmac);




//172
typedef struct {
	uint32 h[5];
	unsigned char block[64];
	int blkused;
	uint32 lenhi, lenlo;
} SHA_State;
void SHA_Init(SHA_State * s);
void SHA_Bytes(SHA_State * s, const void *p, int len);
void SHA_Final(SHA_State * s, unsigned char *output);
void SHA_Simple(const void *p, int len, unsigned char *output);

void hmac_sha1_simple(void *key, int keylen, void *data, int datalen,
	unsigned char *output);



// 196
typedef struct {
	uint64 h[8];
	unsigned char block[128];
	int blkused;
	uint32 len[4];
} SHA512_State;
void SHA512_Init(SHA512_State * s);
void SHA512_Bytes(SHA512_State * s, const void *p, int len);
void SHA512_Final(SHA512_State * s, unsigned char *output);
void SHA512_Simple(const void *p, int len, unsigned char *output);

struct ssh_cipher {
	void *(*make_context)(void);
	void(*free_context)(void *);
	void(*sesskey) (void *, unsigned char *key);	/* for SSH-1 */
	void(*encrypt) (void *, unsigned char *blk, int len);
	void(*decrypt) (void *, unsigned char *blk, int len);
	int blksize;
	char *text_name;
};

struct ssh2_cipher {
	void *(*make_context)(void);
	void(*free_context)(void *);
	void(*setiv) (void *, unsigned char *key);	/* for SSH-2 */
	void(*setkey) (void *, unsigned char *key);/* for SSH-2 */
	void(*encrypt) (void *, unsigned char *blk, int len);
	void(*decrypt) (void *, unsigned char *blk, int len);
	char *name;
	int blksize;
	int keylen;
	unsigned int flags;
#define SSH_CIPHER_IS_CBC	1
	char *text_name;
};

struct ssh2_ciphers {
	int nciphers;
	const struct ssh2_cipher *const *list;
};

struct ssh_mac {
	void *(*make_context)(void);
	void(*free_context)(void *);
	void(*setkey) (void *, unsigned char *key);
	/* whole-packet operations */
	void(*generate) (void *, unsigned char *blk, int len, unsigned long seq);
	int(*verify) (void *, unsigned char *blk, int len, unsigned long seq);
	/* partial-packet operations */
	void(*start) (void *);
	void(*bytes) (void *, unsigned char const *, int);
	void(*genresult) (void *, unsigned char *);
	int(*verresult) (void *, unsigned char const *);
	char *name;
	int len;
	char *text_name;
};

struct ssh_hash {
	void *(*init)(void); /* also allocates context */
	void(*bytes)(void *, void *, int);
	void(*final)(void *, unsigned char *); /* also frees context */
	int hlen; /* output length in bytes */
	char *text_name;
};

typedef enum { KEXTYPE_DH, KEXTYPE_RSA } kextype;

struct ssh_kex {
	char *name, *groupname;
	kextype main_type;
	/* For DH */
	const unsigned char *pdata, *gdata; /* NULL means group exchange */
	int plen, glen;
	const struct ssh_hash *hash;
};

struct ssh_kexes {
	int nkexes;
	const struct ssh_kex *const *list;
};

struct ssh_signkey {
	void *(*newkey) (char *data, int len);
	void(*freekey) (void *key);
	char *(*fmtkey) (void *key);
	unsigned char *(*public_blob) (void *key, int *len);
	unsigned char *(*private_blob) (void *key, int *len);
	void *(*createkey) (unsigned char *pub_blob, int pub_len,
		unsigned char *priv_blob, int priv_len);
	void *(*openssh_createkey) (unsigned char **blob, int *len);
	int(*openssh_fmtkey) (void *key, unsigned char *blob, int len);
	int(*pubkey_bits) (void *blob, int len);
	char *(*fingerprint) (void *key);
	int(*verifysig) (void *key, char *sig, int siglen,
		char *data, int datalen);
	unsigned char *(*sign) (void *key, char *data, int datalen,
		int *siglen);
	char *name;
	char *keytype;		       /* for host key cache */
};



//313
struct ssh2_userkey {
	const struct ssh_signkey *alg;     /* the key algorithm */
	void *data;			       /* the key data */
	char *comment;		       /* the key comment */
};

/* The maximum length of any hash algorithm used in kex. (bytes) */
#define SSH2_KEX_MAX_HASH_LEN (32) /* SHA-256 */

//extern const struct ssh_cipher ssh_3des;
//extern const struct ssh_cipher ssh_des;
//extern const struct ssh_cipher ssh_blowfish_ssh1;
//extern const struct ssh2_ciphers ssh2_3des;
//extern const struct ssh2_ciphers ssh2_des;
//extern const struct ssh2_ciphers ssh2_aes;
//extern const struct ssh2_ciphers ssh2_blowfish;
//extern const struct ssh2_ciphers ssh2_arcfour;
extern const struct ssh_hash ssh_sha1;
extern const struct ssh_hash ssh_sha256;
//extern const struct ssh_kexes ssh_diffiehellman_group1;
//extern const struct ssh_kexes ssh_diffiehellman_group14;
//extern const struct ssh_kexes ssh_diffiehellman_gex;
//extern const struct ssh_kexes ssh_rsa_kex;
extern const struct ssh_signkey ssh_dss;
extern const struct ssh_signkey ssh_rsa;
//extern const struct ssh_mac ssh_hmac_md5;
//extern const struct ssh_mac ssh_hmac_sha1;
//extern const struct ssh_mac ssh_hmac_sha1_buggy;
//extern const struct ssh_mac ssh_hmac_sha1_96;
//extern const struct ssh_mac ssh_hmac_sha1_96_buggy;
//extern const struct ssh_mac ssh_hmac_sha256;

void *aes_make_context(void);
void aes_free_context(void *handle);
void aes128_key(void *handle, unsigned char *key);
void aes192_key(void *handle, unsigned char *key);
void aes256_key(void *handle, unsigned char *key);
void aes_iv(void *handle, unsigned char *iv);
void aes_ssh2_encrypt_blk(void *handle, unsigned char *blk, int len);
void aes_ssh2_decrypt_blk(void *handle, unsigned char *blk, int len);

//370
int random_byte(void);


//499
Bignum copybn(Bignum b);
//Bignum bn_power_2(int n);
void bn_restore_invariant(Bignum b);
//Bignum bignum_from_long(unsigned long n);
void freebn(Bignum b);
Bignum modpow(Bignum base, Bignum exp, Bignum mod);
Bignum modmul(Bignum a, Bignum b, Bignum mod);
void decbn(Bignum n);
extern Bignum Zero, One;
Bignum bignum_from_bytes(const unsigned char *data, int nbytes);
int ssh1_read_bignum(const unsigned char *data, int len, Bignum * result);
int bignum_bitcount(Bignum bn);
int ssh1_bignum_length(Bignum bn);
int ssh2_bignum_length(Bignum bn);
int bignum_byte(Bignum bn, int i);
//int bignum_bit(Bignum bn, int i);
void bignum_set_bit(Bignum bn, int i, int value);
int ssh1_write_bignum(void *data, Bignum bn);
//Bignum biggcd(Bignum a, Bignum b);
//unsigned short bignum_mod_short(Bignum number, unsigned short modulus);
//Bignum bignum_add_long(Bignum number, unsigned long addend);
Bignum bigadd(Bignum a, Bignum b);
Bignum bigsub(Bignum a, Bignum b);
Bignum bigmul(Bignum a, Bignum b);
Bignum bigmuladd(Bignum a, Bignum b, Bignum addend);
//Bignum bigdiv(Bignum a, Bignum b);
Bignum bigmod(Bignum a, Bignum b);
Bignum modinv(Bignum number, Bignum modulus);
//Bignum bignum_bitmask(Bignum number);
//Bignum bignum_rshift(Bignum number, int shift);
int bignum_cmp(Bignum a, Bignum b);
//char *bignum_decimal(Bignum x);


//552
int base64_lines(int datalen);


//554
void base64_encode(FILE *fp, unsigned char *data, int datalen, int cpl);


//556
/* ssh2_load_userkey can return this as an error */
extern struct ssh2_userkey ssh2_wrong_passphrase;
#define SSH2_WRONG_PASSPHRASE (&ssh2_wrong_passphrase)

//566
int ssh2_save_userkey(const Filename *filename, struct ssh2_userkey *key,
	char *passphrase);


//570
enum {
	SSH_KEYTYPE_UNOPENABLE,
	SSH_KEYTYPE_UNKNOWN,
	SSH_KEYTYPE_SSH1, SSH_KEYTYPE_SSH2,
	SSH_KEYTYPE_OPENSSH, SSH_KEYTYPE_SSHCOM
};


//584
struct ssh2_userkey *import_ssh2(const Filename *filename, int type,
	char *passphrase, const char **errmsg_p);

//593
void des3_decrypt_pubkey_ossh(unsigned char *key, unsigned char *iv,
	unsigned char *blk, int len);

//597
void aes256_encrypt_pubkey(unsigned char *key, unsigned char *blk,
	int len);
