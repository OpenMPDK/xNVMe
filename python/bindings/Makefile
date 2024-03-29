#
# This Makefile serves as convenient command-line auto-completion when working on Python bindings
#
#
PY ?= python3
BUILD_ENV_ACTIVATE = . .build-py-env/bin/activate
BUILD_ENV = ${BUILD_ENV_ACTIVATE} && python
WORKSPACE_ROOT ?= ..
TOOLBOX_DIR?=../../toolbox
PROJECT_NAME=xnvme
XNVME_CAPI_HEADERS=$(shell ls ../../include/libxnvme* | grep -v util | grep -v libxnvmec)

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"

define .build-py-env-help
# Init python build virtual env
endef
.build-py-env:
	@echo "Creating Python virtual env"
	@${PY} -m venv .build-py-env
	@echo "Upgrading pip"
	@${BUILD_ENV} -m pip install --upgrade pip
	@echo "Install python dependencies"
	@${BUILD_ENV} -m pip install -r requirements.txt

define all-help
# Do the common task during development of: uninstall clean build install test
endef
.PHONY: all
all: uninstall clean build install test

define build-help
# Generate ctypes, patch them, then create a source package
endef
.PHONY: build
build: .build-py-env generate-ctypes patch-ctypes
	@echo "## py: make build-sdist"
	@${BUILD_ENV} setup.py sdist
	@echo "## py: make build-sdist [DONE]"

define deps-help
# dependencies via pipx 
endef
.PHONY: deps
deps:
	@pipx install cijoe --include-deps
	@pipx inject cijoe numpy

define install-help
# install for current user
endef
.PHONY: install
install: deps
	@echo "## py: make install"
	@pipx inject cijoe dist/xnvme-*.tar.gz
	@echo "## py: make install [DONE]"

define uninstall-help
# uninstall
#
# Prefix with 'sudo' when uninstalling a system-wide installation
endef
.PHONY: uninstall
uninstall:
	@echo "## py: make uninstall"
	@pipx uninstall cijoe || true
	@echo "## py: make uninstall [DONE]"

define test-help
# Run pytest on tests/
endef
.PHONY: test
test:
	@echo "## py: make test"
	@pytest --pyargs xnvme
	@echo "## py: make test [DONE]"

define generate-ctypes-help
# Generate definitions (xnvme_ctypes.header) from xNVMe public C API headers
endef
.PHONY: generate-ctypes
generate-ctypes: .build-py-env
	@echo "## py: gen"
	@echo "## py:headers: ${XNVME_CAPI_HEADERS}"
	${BUILD_ENV_ACTIVATE} && clang2py ${XNVME_CAPI_HEADERS} --clang-args="-I../../include -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/ -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/" > xnvme/ctypes_bindings/api.py
	@echo "## Remember to also call 'make patch-ctypes' to fix format"
	@echo "## py: gen [DONE]"

define patch-ctypes-help
# Patch definitions (xnvme_ctypes.header)
endef
.PHONY: patch-ctypes
patch-ctypes: .build-py-env
	@echo "## py: gen"
	@${BUILD_ENV} auxiliary/patch_ctypes_bindings.py --path xnvme/ctypes_bindings/api.py
	@${BUILD_ENV_ACTIVATE} && black xnvme/ctypes_bindings/api.py || echo "could not fix format"
	@echo "## py: gen [DONE]"

define clean-help
# clean the Python build dirs (build, dist)
endef
.PHONY: clean
clean:
	@echo "## py: clean"
	@rm -f -r .build-py-env
	@rm -f -r build
	@rm -f -r dist
	@rm -f -r *.egg-info
	@rm -f MANIFEST
	@rm -f xnvme/ctypes_bindings/api.py
	@echo "## py: clean [DONE]"

define help-help
# Print the description of every target
endef
.PHONY: help
help:
	@./$(TOOLBOX_DIR)/print_help.py --repos .

define help-verbose-help
# Print the verbose description of every target
endef
.PHONY: help-verbose
help-verbose:
	@./$(TOOLBOX_DIR)/print_help.py --verbose --repos .
