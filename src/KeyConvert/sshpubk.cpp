// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include "misc.h"
#include "ssh.h"

//981
int base64_lines(int datalen)
{
	/* When encoding, we use 64 chars/line, which equals 48 real chars. */
	return (datalen + 47) / 48;
}

void base64_encode(FILE * fp, unsigned char *data, int datalen, int cpl)
{
	int linelen = 0;
	char out[4];
	int n, i;

	while (datalen > 0) {
		n = (datalen < 3 ? datalen : 3);
		base64_encode_atom(data, n, out);
		data += n;
		datalen -= n;
		for (i = 0; i < 4; i++) {
			if (linelen >= cpl) {
				linelen = 0;
				fputc('\n', fp);
			}
			fputc(out[i], fp);
			linelen++;
		}
	}
	fputc('\n', fp);
}

//1010
int ssh2_save_userkey(const Filename *filename, struct ssh2_userkey *key,
	char *passphrase)
{
	FILE *fp;
	unsigned char *pub_blob, *priv_blob, *priv_blob_encrypted;
	int pub_blob_len, priv_blob_len, priv_encrypted_len;
	int passlen;
	int cipherblk;
	int i;
	char *cipherstr;
	unsigned char priv_mac[20];

	/*
	* Fetch the key component blobs.
	*/
	pub_blob = key->alg->public_blob(key->data, &pub_blob_len);
	priv_blob = key->alg->private_blob(key->data, &priv_blob_len);
	if (!pub_blob || !priv_blob) {
		sfree(pub_blob);
		sfree(priv_blob);
		return 0;
	}

	/*
	* Determine encryption details, and encrypt the private blob.
	*/
	if (passphrase) {
		cipherstr = "aes256-cbc";
		cipherblk = 16;
	}
	else {
		cipherstr = "none";
		cipherblk = 1;
	}
	priv_encrypted_len = priv_blob_len + cipherblk - 1;
	priv_encrypted_len -= priv_encrypted_len % cipherblk;
	priv_blob_encrypted = snewn(priv_encrypted_len, unsigned char);
	memset(priv_blob_encrypted, 0, priv_encrypted_len);
	memcpy(priv_blob_encrypted, priv_blob, priv_blob_len);
	/* Create padding based on the SHA hash of the unpadded blob. This prevents
	* too easy a known-plaintext attack on the last block. */
	SHA_Simple(priv_blob, priv_blob_len, priv_mac);
	assert(priv_encrypted_len - priv_blob_len < 20);
	memcpy(priv_blob_encrypted + priv_blob_len, priv_mac,
		priv_encrypted_len - priv_blob_len);

	/* Now create the MAC. */
	{
		unsigned char *macdata;
		int maclen;
		unsigned char *p;
		int namelen = strlen(key->alg->name);
		int enclen = strlen(cipherstr);
		int commlen = strlen(key->comment);
		SHA_State s;
		unsigned char mackey[20];
		char header[] = "putty-private-key-file-mac-key";

		maclen = (4 + namelen +
			4 + enclen +
			4 + commlen +
			4 + pub_blob_len +
			4 + priv_encrypted_len);
		macdata = snewn(maclen, unsigned char);
		p = macdata;
#define DO_STR(s,len) PUT_32BIT(p,(len));memcpy(p+4,(s),(len));p+=4+(len)
		DO_STR(key->alg->name, namelen);
		DO_STR(cipherstr, enclen);
		DO_STR(key->comment, commlen);
		DO_STR(pub_blob, pub_blob_len);
		DO_STR(priv_blob_encrypted, priv_encrypted_len);

		SHA_Init(&s);
		SHA_Bytes(&s, header, sizeof(header) - 1);
		if (passphrase)
			SHA_Bytes(&s, passphrase, strlen(passphrase));
		SHA_Final(&s, mackey);
		hmac_sha1_simple(mackey, 20, macdata, maclen, priv_mac);
		smemclr(macdata, maclen);
		sfree(macdata);
		smemclr(mackey, sizeof(mackey));
		smemclr(&s, sizeof(s));
	}

	if (passphrase) {
		unsigned char key[40];
		SHA_State s;

		passlen = strlen(passphrase);

		SHA_Init(&s);
		SHA_Bytes(&s, "\0\0\0\0", 4);
		SHA_Bytes(&s, passphrase, passlen);
		SHA_Final(&s, key + 0);
		SHA_Init(&s);
		SHA_Bytes(&s, "\0\0\0\1", 4);
		SHA_Bytes(&s, passphrase, passlen);
		SHA_Final(&s, key + 20);
		aes256_encrypt_pubkey(key, priv_blob_encrypted,
			priv_encrypted_len);

		smemclr(key, sizeof(key));
		smemclr(&s, sizeof(s));
	}

	fp = f_open(filename, "w", TRUE);
	if (!fp) {
		sfree(pub_blob);
		smemclr(priv_blob, priv_blob_len);
		sfree(priv_blob);
		smemclr(priv_blob_encrypted, priv_blob_len);
		sfree(priv_blob_encrypted);
		return 0;
	}
	fprintf(fp, "PuTTY-User-Key-File-2: %s\n", key->alg->name);
	fprintf(fp, "Encryption: %s\n", cipherstr);
	fprintf(fp, "Comment: %s\n", key->comment);
	fprintf(fp, "Public-Lines: %d\n", base64_lines(pub_blob_len));
	base64_encode(fp, pub_blob, pub_blob_len, 64);
	fprintf(fp, "Private-Lines: %d\n", base64_lines(priv_encrypted_len));
	base64_encode(fp, priv_blob_encrypted, priv_encrypted_len, 64);
	fprintf(fp, "Private-MAC: ");
	for (i = 0; i < 20; i++)
		fprintf(fp, "%02x", priv_mac[i]);
	fprintf(fp, "\n");
	fclose(fp);

	sfree(pub_blob);
	smemclr(priv_blob, priv_blob_len);
	sfree(priv_blob);
	smemclr(priv_blob_encrypted, priv_blob_len);
	sfree(priv_blob_encrypted);
	return 1;
}


//553
/*
* Magic error return value for when the passphrase is wrong.
*/
struct ssh2_userkey ssh2_wrong_passphrase = {
	NULL, NULL, NULL
};

