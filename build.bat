REM SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
REM
REM SPDX-License-Identifier: BSD-3-Clause

@echo off
@setlocal enableextensions enabledelayedexpansion

set CC=gcc
set MESON=meson
set BUILD_DIR=builddir
set GIT=git

set INFO=
set CONFIG=
set BUILD=
set CLEAN=
set CLOBBER=
set INSTALL=

if "%CC%"=="clang" set "CXX=clang++"
if "%CC%"=="gcc" set "CXX=g++"

:: set msys2-shell
set SH=call msys2_shell -no-start -here -use-full-path -defterm

for %%i in (%*) do (
	if "%%i"=="info" set INFO=info
	if "%%i"=="config" set CONFIG=config
	if "%%i"=="config-debug" set CONFIG=config-debug
	if "%%i"=="build" set BUILD=build
	if "%%i"=="clean" set CLEAN=clean
	if "%%i"=="clobber" set CLOBBER=clobber
	if "%%i"=="install" set INSTALL=install
	if "%%i"=="uninstall" set INSTALL=uninstall
)

set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;%PATH%"
set "PATH=%SystemDrive%\tools\msys64;%PATH%"
set "PATH=%SystemDrive%\tools\msys64\mingw64\bin;%PATH%"


if "%INFO%"=="info" (
	@call :info
	goto :eof
)

if "%CONFIG%"=="config" (
	@call :config
	goto :eof
)

if "%CONFIG%"=="config-debug" (
	@call :config-debug
	goto :eof
)

if "%BUILD%"=="build" (
	@call :build
	goto :eof
)

if "%CLEAN%"=="clean" (
	@call :clean
	goto :eof
)

if "%INSTALL%"=="install" (
	@call :install
	@goto :eof
)

if "%INSTALL%"=="uninstall" (
	@call :uninstall
	@goto :eof
)

if "%CLOBBER%"=="clobber" (
	@call :clobber
	@goto :eof
)

if [%1]==[] (call :default) else (echo "Wrong arguments")

goto :eof

:default
	call :info
	@echo "## xNVMe: make default"
	@if not exist %BUILD_DIR% (
		call :config
	)
	call :build
	@goto :eof

:config
	@echo "## xNVMe: make config"
	@set CC=%CC%
	%SH% -c "%MESON% setup %BUILD_DIR% --prefix=/usr"
	@echo "## xNVMe: make config [DONE]"
	@goto :eof

:config-debug
	@echo "## xNVMe: make config-debug"
	@set CC=%CC%
	%SH% -c "%MESON% setup %BUILD_DIR% --buildtype=debug"
	@echo "## xNVMe: make config-debug [DONE]"
	@goto :eof

:build
	@call :info
	@call :_require_builddir
	@echo "## xNVMe: make build"
	%SH% -c "%MESON% compile -C %BUILD_DIR%"
	@echo "## xNVMe: make build [DONE]"
	@goto :eof

:install
	@call :_require_builddir
	@echo "## xNVMe: make install"
	%SH% -c "%MESON% install -C %BUILD_DIR%"
	@echo "## xNVMe: make install [DONE]"
	@goto :eof

:uninstall
	@echo "## xNVMe: make install"
	@if not exist %BUILD_DIR% (
		echo "Missing builddir('%BUILD_DIR%)"
	)
	@cd %BUILD_DIR%
	%SH% -c "%MESON% --internal uninstall"
	@echo "## xNVMe: make install [DONE]"
	@goto :eof

:clean
	@echo "## xNVMe: make clean"
	%SH% -c "rm -fr %BUILD_DIR% || true"
	@echo "## xNVMe: make clean [DONE]"
	@goto :eof

:info
	@echo "## xNVMe: make info"
	@echo "CC: %CC%"
	@echo "## xNVMe: make info [DONE]"
	@goto :eof

:_require_builddir
	@if not exist %BUILD_DIR% (
		echo ""
		echo "+========================================+"
		echo "                                          "
		echo " ERR: missing builddir: '%BUILD_DIR%'    "
		echo "                                          "
		echo " Configure it first by running:           "
		echo "                                          "
		echo " 'meson setup %BUILD_DIR%'               "
		echo "                                          "
		echo "+========================================+"
		echo ""
	)
	goto :eof

::
:: clobber: clean third-party builds, drop any third-party changes and clean any
:: untracked stuff lying around
::
:clobber
	@call :clean
	@echo "## xNVMe: clobber"
	%SH% -c "rm -fr %BUILD_DIR% || true"
	@git clean -dfX
	@git checkout .
	@echo "## xNVMe: clobber [DONE]"
	@goto :eof
