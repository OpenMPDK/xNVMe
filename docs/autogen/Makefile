# The builddir path is hardcoded in the config/doxy.conf and config/conf.py
BUILD_DIR?=builddir
SOURCE_DIR?=../../
CONFIG_DIR?=./
DOCS_DIR?=../

VENV_NAME = .venv
VENV = . ${VENV_NAME}/bin/activate && 
PYTHON = ${VENV} python3

default: docs docs-view-html

.PHONY: deps
deps:
#	python3 -m venv $(VENV_NAME)
	pipx install poetry
	poetry install --no-root
	echo "export PYTHONPATH=${PYTHONPATH}:${PWD}/$(SOURCE_DIR)/toolbox" >> ${VENV_NAME}/bin/activate

# The PYTHONPATH is needed such that we can call the 'xnvme_ver.py' script

.PHONY: clean
clean: clean-apis
	rm -fr $(BUILD_DIR) || true

.PHONY: configure
configure: clean
	mkdir -p $(BUILD_DIR)

# This needs the xNVMe source-code
.PHONY: apis
apis:
	cd $(SOURCE_DIR) && make tags-xnvme-public
	${PYTHON} apigen.py --tags $(SOURCE_DIR)/tags --output $(DOCS_DIR)/capis

.PHONY: clean-apis
clean-apis:
	rm -f $(DOCS_DIR)/capis/core/xnvme*
	rm -f $(DOCS_DIR)/capis/extended/xnvme*


.PHONY: commands
commands:
	@echo "# commands"
	${VENV} kmdo $(DOCS_DIR)/getting_started --exclude "win"
	${VENV} kmdo $(DOCS_DIR)/capis
	${VENV} kmdo $(DOCS_DIR)/examples
	${VENV} kmdo $(DOCS_DIR)/tools
	${VENV} kmdo $(DOCS_DIR)/tutorial

.PHONY: doxy
doxy:
	mkdir -p $(BUILD_DIR)/doxy
	doxygen doxy.cfg

.PHONY: doxy-view
doxy-view:
	xdg-open $(BUILD_DIR)/doxy/html/index.html || open $(BUILD_DIR)/doxy/html/index.html &

.PHONY: sphinx
sphinx:
	mkdir -p $(BUILD_DIR)/html
	${PYTHON} -m sphinx -E -b html -c $(CONFIG_DIR) $(DOCS_DIR) $(BUILD_DIR)/html

# Produce the sphinx stuff (without commands)
.PHONY: docs
docs: clean deps configure doxy apis sphinx
	@echo "# DONE"

.PHONY: docs-view-html
docs-view-html:
	@xdg-open $(BUILD_DIR)/html/index.html || open $(BUILD_DIR)/html/index.html &
