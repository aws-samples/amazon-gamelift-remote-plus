:: Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
:: SPDX-License-Identifier: MIT-0

@ECHO OFF
ECHO CLEAN STARTED %DATE% %TIME%

REM ------- FIND MY ABSOLUTE ROOT -------
SET REL_ROOT=..\
SET ABS_ROOT=
PUSHD %REL_ROOT%
SET ABS_ROOT=%CD%
POPD

REM <TODO> CLOSE MSVS

REM ------- CLEAN UNWANTED FILES -------
IF EXIST "%ABS_ROOT%\bin" RMDIR /S /Q "%ABS_ROOT%\bin"
IF EXIST "%ABS_ROOT%\env" RMDIR /S /Q "%ABS_ROOT%\env"
IF EXIST "%ABS_ROOT%\obj" RMDIR /S /Q "%ABS_ROOT%\obj"
IF EXIST "%ABS_ROOT%\src\.vs" RMDIR /S /Q "%ABS_ROOT%\src\.vs"
IF EXIST "%ABS_ROOT%\src\Debug" RMDIR /S /Q "%ABS_ROOT%\src\Debug"
IF EXIST "%ABS_ROOT%\src\Release" RMDIR /S /Q "%ABS_ROOT%\src\Release"
IF EXIST "%ABS_ROOT%\src\KeyConvert\.vs" RMDIR /S /Q "%ABS_ROOT%\src\KeyConvert\.vs"
IF EXIST "%ABS_ROOT%\src\KeyConvert\Debug" RMDIR /S /Q "%ABS_ROOT%\src\KeyConvert\Debug"
IF EXIST "%ABS_ROOT%\src\KeyConvert\Release" RMDIR /S /Q "%ABS_ROOT%\src\KeyConvert\Release"
IF EXIST "%ABS_ROOT%\src\KeyConvert\CLRTest\obj" RMDIR /S /Q "%ABS_ROOT%\src\KeyConvert\CLRTest\obj"
IF EXIST "%ABS_ROOT%\src\KeyConvert\CLRTest\bin" RMDIR /S /Q "%ABS_ROOT%\src\KeyConvert\CLRTest\bin"
DEL /Q %ABS_ROOT%\src\*.*.suo 2> NUL
DEL /Q %ABS_ROOT%\runlatestversion.lnk 2> NUL

REM ------- DONE -------
ECHO CLEAN COMPLETED %DATE% %TIME%

