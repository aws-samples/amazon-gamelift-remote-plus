// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include "CLR.h"

#include "tchar.h"

#include <direct.h>
#define GetCurrentDir _getcwd

#include "ssh.h"

int KeyConvertNative(char *importPath, char *exportPath)
{
	// idea based on https://stackoverflow.com/questions/29646720

	const char *errmsg_p;
	Filename importFilename;
	importFilename.path = importPath;
	int type = SSH_KEYTYPE_OPENSSH;
	char *importPassphrase = NULL;
	Filename exportFilename;
	exportFilename.path = exportPath;
	char *exportPassphrase = NULL;

	ssh2_userkey *key = import_ssh2(&importFilename, type, importPassphrase, &errmsg_p);

	if (errmsg_p != NULL)
	{
		printf("Error: %s\n", errmsg_p);
		return 22; // EINVAL
	}


	int retval = ssh2_save_userkey(&exportFilename, key, exportPassphrase);

	return 0;
}
