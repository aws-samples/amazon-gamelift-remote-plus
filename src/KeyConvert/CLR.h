// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#pragma once

namespace CLR {
	using namespace System;

	public ref class KeyConvert
	{
	public:
		static int Convert(String ^importPath, String ^exportPath);
	};
}
