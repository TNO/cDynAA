CDYNAA_DIR = cdynaa
PYDYNAA_DIR = pydynaa
DOCS_DIR = docs

PYTHON3 = python3
# Set pip to install as a user if python not running in a virtualenv
PIPFLAG_USER = $(shell $(PYTHON3) -c 'import sys; print ("--user" if not (hasattr(sys, "real_prefix") or sys.base_prefix != sys.prefix) else "")')
VERSION = $$($(PYTHON3) -m version)

DOCS_PDF_FILE = documentation-$(PYDYNAA_DIR)-$(VERSION).pdf

# Install requirements and build for development. Installs wheel locally.
default: requirements develop
# Runs cdynaa and pydynaa tests
tests: cdynaa_tests pydynaa_tests

cdynaa:
	$(PYTHON3) setup.py build_cdynaa

cdynaa_tests: $(OBJS_TEST)
	@echo -e "\n*** Building and running all [stable] unit tests for cDynAA library"
	$(PYTHON3) setup.py test_cdynaa

requirements:
	@echo -e "\n*** Installing Python package requirements."
	$(PYTHON3) -m pip install $(PIPFLAG_USER) "pip>=0.19"
	$(PYTHON3) -m pip install $(PIPFLAG_USER) -r requirements.txt

inplace: 
	@echo -e "\n*** Building pyDynAA package inplace"
	$(PYTHON3) setup.py build_ext --inplace

develop: 
	@echo -e "\n*** Installing inplace (editable) pyDynAA package locally"
	$(PYTHON3) -m pip install $(PIPFLAG_USER) --verbose --editable .

profile: export PYDYNAA_PROFILING = 1
profile: develop

install:
	@echo -e "\n*** Installing pyDynAA package locally"
	$(PYTHON3) -m pip install $(PIPFLAG_USER) --verbose .

uninstall:
	@echo -e "\n*** Uninstalling pyDynAA package"
	$(PYTHON3) -m pip uninstall $(PYDYNAA_DIR)

pydynaa_tests:
	@echo -e "\n*** Running unit tests for pyDynAA package"
	$(PYTHON3) -m unittest

pydynaa_lint:
	@echo -e "\n*** Checking that pyDynAA package conforms to PEP8 coding style"
	$(PYTHON3) -m flake8 $(PYDYNAA_DIR) setup.py

docs: inplace
	@echo -e "\n*** Installing dependencies for docs"
	$(PYTHON3) -m pip install -r ./docs/docs_requirements.txt
	@echo -e "\n*** Making pyDynAA html documentation"
	$(PYTHON3) -msphinx -M html $(DOCS_DIR)/ $(DOCS_DIR)/

docs-pdf: docs
	@echo -e "\n*** Making pyDynAA pdf documentation"
	@echo -e "\n*** Make sure you have apt installed packages: latexmk and texlive-latex-extra"
	cd $(DOCS_DIR) && make latexpdf LATEXMKOPTS="-silent"
	mkdir -p $(DOCS_DIR)/pdf
	mv $(DOCS_DIR)/_build/latex/PyDynAA.pdf $(DOCS_DIR)/pdf/$(DOCS_PDF_FILE)

wheel: 
	@echo -e "\n*** Create a binary pyDynAA wheel distribution file"
	$(PYTHON3) -m pip wheel . 

verify: pydynaa_lint tests

clean_dist:
	-/bin/rm -rf dist

clean_pydynaa_build:
	-/bin/rm -rf build *.egg-info
	-find $(PYDYNAA_DIR) -type f -name "*.so" -exec rm {} +
	-/bin/rm $(PYDYNAA_DIR)/version.txt
	/bin/rm -rf $(PYDYNAA_DIR)/core.cpp
	/bin/rm -rf $(PYDYNAA_DIR)/core.h

clean: clean_dist clean_pydynaa_build
	cd ./$(CDYNAA_DIR)/ && $(MAKE) clean
	find $(PYDYNAA_DIR) -type f -name "*.pyc" -exec rm {} +
	/bin/rm -rf $(DOCS_DIR)/html $(DOCS_DIR)/pdf $(DOCS_DIR)/doctrees $(DOCS_DIR)/_build

.PHONY: default cdynaa requirements inplace develop install uninstall tests cdynaa_tests\
	pydynaa_tests pydynaa_lint verify profile 
.PHONY: clean clean_dist clean_pydynaa_build 
