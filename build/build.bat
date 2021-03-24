:: Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
:: SPDX-License-Identifier: MIT-0

@ECHO OFF
ECHO BUILD STARTED %DATE% %TIME%

REM ------- FIND MY ABSOLUTE ROOT -------
SET REL_ROOT=..\
SET ABS_ROOT=
PUSHD %REL_ROOT%
SET ABS_ROOT=%CD%
POPD

REM ------- 32 OR 64 BIT -------
Set OS_BITS=64
IF %PROCESSOR_ARCHITECTURE% == x86 (
  IF NOT DEFINED PROCESSOR_ARCHITEW6432 SET OS_BITS=32
  )

REM ------- CREATE ENVIRONMENT DIRECTORY -------
IF NOT EXIST %ABS_ROOT%\env\NUL mkdir %ABS_ROOT%\env
PUSHD %ABS_ROOT%\env

REM ------- GET NUGET.EXE -------
IF NOT EXIST NUGET.EXE (
  ECHO INSTALLING NUGET
  POWERSHELL -ex unrestricted -Command "[Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12;(New-Object System.Net.WebClient).DownloadFile(""""https://dist.nuget.org/win-x86-commandline/latest/nuget.exe"""", """".\NUGET.EXE"""")"
  ECHO DONE
  )

REM ------- INSTALL AWS .NET SDK FILES IF NEEDED -------
IF NOT EXIST "AWSSDK.GameLift.3.5.4.1\lib\net45\AWSSDK.GameLift.dll" (
  ECHO INSTALLING AWS SDK
  CALL "nuget.exe" install AWSSDK.Gamelift -Version 3.5.4.1
  ECHO DONE
  )

REM ------- BACK TO BUILD DIR -------
CD %~dp0

REM ------- COPY AWS SDK .NET35 DLLS FROM AWS .NET SDK -------
IF NOT EXIST "%ABS_ROOT%\src\AWSSDK.Core.dll" COPY "%ABS_ROOT%\env\AWSSDK.Core.3.5.3.8\lib\net45\AWSSDK.Core.dll" "%ABS_ROOT%\src\AWSSDK.Core.dll"
IF NOT EXIST "%ABS_ROOT%\src\AWSSDK.GameLift.dll" COPY "%ABS_ROOT%\env\AWSSDK.GameLift.3.5.4.1\lib\net45\AWSSDK.GameLift.dll" "%ABS_ROOT%\src\AWSSDK.GameLift.dll"

REM SET VISUAL STUDIO ENVIRONMENT - USE OLDEST TO AVOID UNNECESSARILY UPGRADING PROJECT FILES
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" GOTO VS2017C
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars32.bat" GOTO VS2017P
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars32.bat" GOTO VS2017E
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" GOTO VS2019C
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" GOTO VS2019P
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" GOTO VS2019E
GOTO VSMISSING

:VS2017C
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.19041.0"""", """"10.0.17763.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
SET PLATFORM_TOOLSET=v141
GOTO STARTBUILD

:VS2017P
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars32.bat"
SET PLATFORM_TOOLSET=v141
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.19041.0"""", """"10.0.17763.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
GOTO STARTBUILD

:VS2017E
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
SET PLATFORM_TOOLSET=v141
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.19041.0"""", """"10.0.17763.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
GOTO STARTBUILD

:VS2019C
SET VSCMD_DEBUG=0
SET PLATFORM_TOOLSET=v142
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.17763.0"""", """"10.0.19041.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
GOTO STARTBUILD

:VS2019P
SET VSCMD_DEBUG=0
SET PLATFORM_TOOLSET=v142
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.17763.0"""", """"10.0.19041.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"
GOTO STARTBUILD

:VS2019E
SET VSCMD_DEBUG=0
SET PLATFORM_TOOLSET=v142
POWERSHELL -ex unrestricted -Command "(Get-Content """"..\src\KeyConvert\CLR.vcxproj"""") | Foreach-Object {$_ -replace """"10.0.17763.0"""", """"10.0.19041.0""""} | Set-Content """"..\src\KeyConvert\CLR.vcxproj"""""
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
GOTO STARTBUILD

:STARTBUILD
MSBUILD "%ABS_ROOT%\src\GameLiftRemotePlus.sln" /p:Configuration=Release /p:PlatformToolset=%PLATFORM_TOOLSET%
