// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include "CLR.h"
#include <msclr\marshal_cppstd.h>
#include "KeyConvert.h"
//#include <string>

using namespace CLR;
using namespace msclr::interop;
using namespace System::Text;
using namespace System::Diagnostics;

int KeyConvert::Convert(String ^importPath, String ^exportPath)
{
	char import_path[512] = { 0 };
	char export_path[512] = { 0 };

	std::string temp1 = marshal_as<std::string>(importPath);
	strcpy(import_path, temp1.c_str());

	std::string temp2 = marshal_as<std::string>(exportPath);
	strcpy(export_path, temp2.c_str());

	Console::WriteLine("Converting file: " + importPath + " to " + exportPath);

	int result = KeyConvertNative(import_path, export_path);

	return result;
}
