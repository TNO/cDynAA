Building and installing from source
===================================

This file instructs how to build, install, and deploy pyDynAA from source,
as well as how to only build the C++ cDynAA library.
It is intended for developers and requires that you have access to the source repository.
If you only wish to install and run pyDynAA,
then please refer to the installation section of the [README](README.md) file instead.

## Building and installing the pyDynAA package

Prerequisites to build pyDynAA from source are:
- Download access (via HTTPS or SSH) to the [cDynAA](https://ci.tno.nl/gitlab/DynAA/cDynAA/) Git repository.
- Python 3 header files (e.g. _python3-dev_ package).
- Python 3.5 and up to Python 3.12 and the Python packages listed in [requirements.txt](requirements.txt).
- The python package install [pip](https://pip.pypa.io/en/stable/) version 19 or higher. To upgrade your current pip version run `pip3 install --upgrade pip`.
- Cython (>= 0.28.5)
- GCC or clang C++ compiler with support for C++17 (e.g. _build_essential_ package).

Clone this [Git](http://rogerdudler.github.io/git-guide/) repository e.g. to clone via HTTPS do
```
git clone https://ci.tno.nl/gitlab/DynAA/cDynAA.git
```

To install all of the Python package dependencies run:
```
make requirements
```

If you are installing on MacOS first read the [installing on MacOS section](#installing-from-source-on-macos).

The recommended way to install pyDynAA from the source repository is as an _editable (in-place) package_.
To do so run the following command:
```
make develop
```
in the repository root directory (this is equivalent to running `make`).
This will locally install pyDynAA by linking to this repository.

If you instead want to install pyDynAA as a _independent package_ (not linked to this repository), run the command:
```
make install
```
which installs the pyDynAA package locally. 
This package can later be uninstalled with:
```
make uninstall
```

### Installing pyDynAA from source on MacOS

PyDynAA can be installed on MacOS in two ways. 

Using the standard XCode compiler:
1. Set the `CFLAGS` environment variable to use the correct C++17 libraries:
```
export CFLAGS='-mmacosx-version-min=10.7 -std=c++17 -stdlib=libc++'
```
2. Follow the Linux installation instructions above.

Using [Homebrew](https://brew.sh/):
1. Use brew to install gcc: `brew install gcc`.
2. Update the system `PATH` to include the gcc binary directory.
3. Set the `CC` and `CXX` environment variables to point to the gcc binary e.g. `export CXX=g++-7`.
4. Follow the Linux installation instructions above.

## Building the cDynAA C++ library separately

To build only the cDynAA static C++ library file at `cdynaa/libdynaacore.a` run:
```
make cdynaa
```
To test this library was built successfully run:
```
make cdynaa_tests
```

To instead build a shared C++ library file at `cdynaa/libdynaacore.so` run:
```
cd cdynaa && make sharedlib
```
Note that the unit tests are currently only linked to the static library.
