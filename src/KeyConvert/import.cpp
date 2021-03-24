// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "malloc.h"
#include "ssh.h"

#include "misc.h"

/* 112
* Strip trailing CRs and LFs at the end of a line of text.
*/
void strip_crlf(char *str)
{
	char *p = str + strlen(str);

	while (p > str && (p[-1] == '\r' || p[-1] == '\n'))
		*--p = '\0';
}

/* 123-------------------------------------------------------------------
* Helper routines. (The base64 ones are defined in sshpubk.c.)
*/

#define isbase64(c) (    ((c) >= 'A' && (c) <= 'Z') || \
                         ((c) >= 'a' && (c) <= 'z') || \
                         ((c) >= '0' && (c) <= '9') || \
                         (c) == '+' || (c) == '/' || (c) == '=' \
                         )

//151
static int ber_read_id_len(void *source, int sourcelen,
	int *id, int *length, int *flags)
{
	unsigned char *p = (unsigned char *)source;

	if (sourcelen == 0)
		return -1;

	*flags = (*p & 0xE0);
	if ((*p & 0x1F) == 0x1F) {
		*id = 0;
		while (*p & 0x80) {
			p++, sourcelen--;
			if (sourcelen == 0)
				return -1;
			*id = (*id << 7) | (*p & 0x7F);
		}
		p++, sourcelen--;
	}
	else {
		*id = *p & 0x1F;
		p++, sourcelen--;
	}

	if (sourcelen == 0)
		return -1;

	if (*p & 0x80) {
		unsigned len;
		int n = *p & 0x7F;
		p++, sourcelen--;
		if (sourcelen < n)
			return -1;
		len = 0;
		while (n--)
			len = (len << 8) | (*p++);
		sourcelen -= n;
		*length = toint(len);
	}
	else {
		*length = *p;
		p++, sourcelen--;
	}

	return p - (unsigned char *)source;
}

/* 308----------------------------------------------------------------------
* Code to read and write OpenSSH private keys.
*/

enum { OSSH_DSA, OSSH_RSA };
enum { OSSH_ENC_3DES, OSSH_ENC_AES };
struct openssh_key {
	int type;
	int encrypted, encryption;
	char iv[32];
	unsigned char *keyblob;
	int keyblob_len, keyblob_size;
};

static struct openssh_key *load_openssh_key(const Filename *filename,
	const char **errmsg_p)
{
	struct openssh_key *ret;
	FILE *fp = NULL;
	char *line = NULL;
	char *errmsg, *p;
	int headers_done;
	char base64_bit[4];
	int base64_chars = 0;

	ret = snew(struct openssh_key);
	ret->keyblob = NULL;
	ret->keyblob_len = ret->keyblob_size = 0;
	ret->encrypted = 0;
	memset(ret->iv, 0, sizeof(ret->iv));

	fp = f_open(filename, "r", FALSE);
	if (!fp) {
		errmsg = "unable to open key file";
		goto error;
	}

	if (!(line = fgetline(fp))) {
		errmsg = "unexpected end of file";
		goto error;
	}
	strip_crlf(line);
	if (!strstartswith(line, "-----BEGIN ") ||
		!strendswith(line, "PRIVATE KEY-----")) {
		errmsg = "file does not begin with OpenSSH key header";
		goto error;
	}
	if (!strcmp(line, "-----BEGIN RSA PRIVATE KEY-----"))
		ret->type = OSSH_RSA;
	else if (!strcmp(line, "-----BEGIN DSA PRIVATE KEY-----"))
		ret->type = OSSH_DSA;
	else {
		errmsg = "unrecognised key type";
		goto error;
	}
	smemclr(line, strlen(line));
	sfree(line);
	line = NULL;

	headers_done = 0;
	while (1) {
		if (!(line = fgetline(fp))) {
			errmsg = "unexpected end of file";
			goto error;
		}
		strip_crlf(line);
		if (strstartswith(line, "-----END ") &&
			strendswith(line, "PRIVATE KEY-----")) {
			sfree(line);
			line = NULL;
			break;		       /* done */
		}
		if ((p = strchr(line, ':')) != NULL) {
			if (headers_done) {
				errmsg = "header found in body of key data";
				goto error;
			}
			*p++ = '\0';
			while (*p && isspace((unsigned char)*p)) p++;
			if (!strcmp(line, "Proc-Type")) {
				if (p[0] != '4' || p[1] != ',') {
					errmsg = "Proc-Type is not 4 (only 4 is supported)";
					goto error;
				}
				p += 2;
				if (!strcmp(p, "ENCRYPTED"))
					ret->encrypted = 1;
			}
			else if (!strcmp(line, "DEK-Info")) {
				int i, j, ivlen;

				if (!strncmp(p, "DES-EDE3-CBC,", 13)) {
					ret->encryption = OSSH_ENC_3DES;
					ivlen = 8;
				}
				else if (!strncmp(p, "AES-128-CBC,", 12)) {
					ret->encryption = OSSH_ENC_AES;
					ivlen = 16;
				}
				else {
					errmsg = "unsupported cipher";
					goto error;
				}
				p = strchr(p, ',') + 1;/* always non-NULL, by above checks */
				for (i = 0; i < ivlen; i++) {
					if (1 != sscanf(p, "%2x", &j)) {
						errmsg = "expected more iv data in DEK-Info";
						goto error;
					}
					ret->iv[i] = j;
					p += 2;
				}
				if (*p) {
					errmsg = "more iv data than expected in DEK-Info";
					goto error;
				}
			}
		}
		else {
			headers_done = 1;

			p = line;
			while (isbase64(*p)) {
				base64_bit[base64_chars++] = *p;
				if (base64_chars == 4) {
					unsigned char out[3];
					int len;

					base64_chars = 0;

					len = base64_decode_atom(base64_bit, out);

					if (len <= 0) {
						errmsg = "invalid base64 encoding";
						goto error;
					}

					if (ret->keyblob_len + len > ret->keyblob_size) {
						ret->keyblob_size = ret->keyblob_len + len + 256;
						ret->keyblob = sresize(ret->keyblob, ret->keyblob_size,
							unsigned char);
					}

					memcpy(ret->keyblob + ret->keyblob_len, out, len);
					ret->keyblob_len += len;

					smemclr(out, sizeof(out));
				}

				p++;
			}
		}
		smemclr(line, strlen(line));
		sfree(line);
		line = NULL;
	}

	fclose(fp);
	fp = NULL;

	if (ret->keyblob_len == 0 || !ret->keyblob) {
		errmsg = "key body not present";
		goto error;
	}

	if (ret->encrypted && ret->keyblob_len % 8 != 0) {
		errmsg = "encrypted key blob is not a multiple of cipher block size";
		goto error;
	}

	smemclr(base64_bit, sizeof(base64_bit));
	if (errmsg_p) *errmsg_p = NULL;
	return ret;

error:
	if (line) {
		smemclr(line, strlen(line));
		sfree(line);
		line = NULL;
	}
	smemclr(base64_bit, sizeof(base64_bit));
	if (ret) {
		if (ret->keyblob) {
			smemclr(ret->keyblob, ret->keyblob_size);
			sfree(ret->keyblob);
		}
		smemclr(ret, sizeof(*ret));
		sfree(ret);
	}
	if (errmsg_p) *errmsg_p = errmsg;
	if (fp) fclose(fp);
	return NULL;
}


//513
struct ssh2_userkey *openssh_read(const Filename *filename, char *passphrase,
	const char **errmsg_p)
{
	struct openssh_key *key = load_openssh_key(filename, errmsg_p);
	struct ssh2_userkey *retkey;
	unsigned char *p;
	int ret, id, len, flags;
	int i, num_integers;
	struct ssh2_userkey *retval = NULL;
	char *errmsg;
	unsigned char *blob;
	int blobsize = 0, blobptr, privptr;
	char *modptr = NULL;
	int modlen = 0;

	blob = NULL;

	if (!key)
		return NULL;

	if (key->encrypted) {
		/*
		* Derive encryption key from passphrase and iv/salt:
		*
		*  - let block A equal MD5(passphrase || iv)
		*  - let block B equal MD5(A || passphrase || iv)
		*  - block C would be MD5(B || passphrase || iv) and so on
		*  - encryption key is the first N bytes of A || B
		*
		* (Note that only 8 bytes of the iv are used for key
		* derivation, even when the key is encrypted with AES and
		* hence there are 16 bytes available.)
		*/
		struct MD5Context md5c;
		unsigned char keybuf[32];

		MD5Init(&md5c);
		MD5Update(&md5c, (unsigned char *)passphrase, strlen(passphrase));
		MD5Update(&md5c, (unsigned char *)key->iv, 8);
		MD5Final(keybuf, &md5c);

		MD5Init(&md5c);
		MD5Update(&md5c, keybuf, 16);
		MD5Update(&md5c, (unsigned char *)passphrase, strlen(passphrase));
		MD5Update(&md5c, (unsigned char *)key->iv, 8);
		MD5Final(keybuf + 16, &md5c);

		/*
		* Now decrypt the key blob.
		*/
		if (key->encryption == OSSH_ENC_3DES)
			des3_decrypt_pubkey_ossh(keybuf, (unsigned char *)key->iv,
			key->keyblob, key->keyblob_len);
		else {
			void *ctx;
			assert(key->encryption == OSSH_ENC_AES);
			ctx = aes_make_context();
			aes128_key(ctx, keybuf);
			aes_iv(ctx, (unsigned char *)key->iv);
			aes_ssh2_decrypt_blk(ctx, key->keyblob, key->keyblob_len);
			aes_free_context(ctx);
		}

		smemclr(&md5c, sizeof(md5c));
		smemclr(keybuf, sizeof(keybuf));
	}

	/*
	* Now we have a decrypted key blob, which contains an ASN.1
	* encoded private key. We must now untangle the ASN.1.
	*
	* We expect the whole key blob to be formatted as a SEQUENCE
	* (0x30 followed by a length code indicating that the rest of
	* the blob is part of the sequence). Within that SEQUENCE we
	* expect to see a bunch of INTEGERs. What those integers mean
	* depends on the key type:
	*
	*  - For RSA, we expect the integers to be 0, n, e, d, p, q,
	*    dmp1, dmq1, iqmp in that order. (The last three are d mod
	*    (p-1), d mod (q-1), inverse of q mod p respectively.)
	*
	*  - For DSA, we expect them to be 0, p, q, g, y, x in that
	*    order.
	*/

	p = key->keyblob;

	/* Expect the SEQUENCE header. Take its absence as a failure to
	* decrypt, if the key was encrypted. */
	ret = ber_read_id_len(p, key->keyblob_len, &id, &len, &flags);
	p += ret;
	if (ret < 0 || id != 16 || len < 0 ||
		key->keyblob + key->keyblob_len - p < len) {
		errmsg = "ASN.1 decoding failure";
		retval = key->encrypted ? SSH2_WRONG_PASSPHRASE : NULL;
		goto error;
	}

	/* Expect a load of INTEGERs. */
	if (key->type == OSSH_RSA)
		num_integers = 9;
	else if (key->type == OSSH_DSA)
		num_integers = 6;
	else
		num_integers = 0;	       /* placate compiler warnings */

	/*
	* Space to create key blob in.
	*/
	blobsize = 256 + key->keyblob_len;
	blob = snewn(blobsize, unsigned char);
	PUT_32BIT(blob, 7);
	if (key->type == OSSH_DSA)
		memcpy(blob + 4, "ssh-dss", 7);
	else if (key->type == OSSH_RSA)
		memcpy(blob + 4, "ssh-rsa", 7);
	blobptr = 4 + 7;
	privptr = -1;

	for (i = 0; i < num_integers; i++) {
		ret = ber_read_id_len(p, key->keyblob + key->keyblob_len - p,
			&id, &len, &flags);
		p += ret;
		if (ret < 0 || id != 2 || len < 0 ||
			key->keyblob + key->keyblob_len - p < len) {
			errmsg = "ASN.1 decoding failure";
			retval = key->encrypted ? SSH2_WRONG_PASSPHRASE : NULL;
			goto error;
		}

		if (i == 0) {
			/*
			* The first integer should be zero always (I think
			* this is some sort of version indication).
			*/
			if (len != 1 || p[0] != 0) {
				errmsg = "version number mismatch";
				goto error;
			}
		}
		else if (key->type == OSSH_RSA) {
			/*
			* Integers 1 and 2 go into the public blob but in the
			* opposite order; integers 3, 4, 5 and 8 go into the
			* private blob. The other two (6 and 7) are ignored.
			*/
			if (i == 1) {
				/* Save the details for after we deal with number 2. */
				modptr = (char *)p;
				modlen = len;
			}
			else if (i != 6 && i != 7) {
				PUT_32BIT(blob + blobptr, len);
				memcpy(blob + blobptr + 4, p, len);
				blobptr += 4 + len;
				if (i == 2) {
					PUT_32BIT(blob + blobptr, modlen);
					memcpy(blob + blobptr + 4, modptr, modlen);
					blobptr += 4 + modlen;
					privptr = blobptr;
				}
			}
		}
		else if (key->type == OSSH_DSA) {
			/*
			* Integers 1-4 go into the public blob; integer 5 goes
			* into the private blob.
			*/
			PUT_32BIT(blob + blobptr, len);
			memcpy(blob + blobptr + 4, p, len);
			blobptr += 4 + len;
			if (i == 4)
				privptr = blobptr;
		}

		/* Skip past the number. */
		p += len;
	}

	/*
	* Now put together the actual key. Simplest way to do this is
	* to assemble our own key blobs and feed them to the createkey
	* functions; this is a bit faffy but it does mean we get all
	* the sanity checks for free.
	*/
	assert(privptr > 0);	       /* should have bombed by now if not */
	retkey = snew(struct ssh2_userkey);
	retkey->alg = (key->type == OSSH_RSA ? &ssh_rsa : &ssh_dss);
	retkey->data = retkey->alg->createkey(blob, privptr,
		blob + privptr, blobptr - privptr);
	if (!retkey->data) {
		sfree(retkey);
		errmsg = "unable to create key data structure";
		goto error;
	}

	retkey->comment = dupstr("imported-openssh-key");
	errmsg = NULL;                     /* no error */
	retval = retkey;

error:
	if (blob) {
		smemclr(blob, blobsize);
		sfree(blob);
	}
	smemclr(key->keyblob, key->keyblob_size);
	sfree(key->keyblob);
	smemclr(key, sizeof(*key));
	sfree(key);
	if (errmsg_p) *errmsg_p = errmsg;
	return retval;
}

/*
* Import an SSH-2 key.
*/
struct ssh2_userkey *import_ssh2(const Filename *filename, int type,
	char *passphrase, const char **errmsg_p)
{
	if (type == SSH_KEYTYPE_OPENSSH)
		return openssh_read(filename, passphrase, errmsg_p);
	//if (type == SSH_KEYTYPE_SSHCOM)
	//	return sshcom_read(filename, passphrase, errmsg_p);
	return NULL;
}