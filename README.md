DynAA discrete event simulation engine for C and Python
=======================================================

DynAA  is a high-performance simulation engine based on discrete event specifications. Developed in C++ for extreme speed and minimal resource consumption, DynAA provides a robust foundation for modelling complex computing systems. Its architecture supports domain-specific extensions, enabling developers to create modelling layers tailored to their application domains. These layers define components and interactions, while DynAA ensures accurate scheduling and event handling during simulations.

The DynAA code base includes:

 pyDynAA, a Python wrapper built with Cython . This integration allows developers to implement event handlers in Python while retaining the option to accelerate performance-critical components using Cython. This flexibility makes DynAA suitable for both rapid prototyping and high-performance simulation tasks.

- *cDynAA* is the C++ port of DynAA(a library originally written in Java) core layer. Like the java core of DynAA, it is the base simulation engine for several other domain specific simulators, each of which includes a specific *model layer*.

- *pyDynAA* is a Python wrapper around the C++ DynAA core.
The wrapping is done using Cython and supports event handler callback functions in pure Python, as well as in Cython.
Because the pyDynAA package includes Cython definitions of the core classes (Event, Entity, etc.), a model layer using pyDynAA can itself (optionally) implement speed critical components in Cython.

## Installation

See the [INSTALL.md](INSTALL.md) file for instructions of how to install the package as a user or [INSTALL-SOURCE.md](INSTALL-SOURCE.md) for how to build and install from the source repository as a developer.

The developers and creators are listed in the [AUTHORS](AUTHORS.md) file.
The user license can be found in the [LICENSE](LICENSE.md) file.


## License
DynAA will releases under the Apache 2.0 license, a permissive framework that facilitates widespread adoption. 
This license allows unrestricted use, modification, and redistribution, including the creation of proprietary derivative works. Importantly, there are no co-owned patents in the DynAA code base, eliminating potential legal complications.
The user license can be found in the [LICENSE](LICENSE.md) file.


## Versions
For historical reasons, and for use in the NetSquid Community the versions 1.x are kept in this repository.  There is no further development on these versions -- only bug fixes.

The current development follows v2.X onwards, which has a better design, is faster, and fixes many unsafe memory issues from versions 1.X.

We strongly advise using version 2.X onwards.

Restrictions on the requirements.txt file for dependencies are based on preserving support for older versions of python.  Example, most recent versions of cysignals require python 3.12 or higher.  We do not, so we limit the use of cysignals currently to v1.12.2.

## Known issues

### For developers

- The cDynAA tests do not work on Ubuntu Linux versions up from Ubuntu 24.00.  That is not related to the cDynAA library itself, but the test library used (catch.h) which uses OS signals that were deprecated in newer versions of Ubuntu.  A modernization of the cDynAA test is in the roadmap, but not yet done.  Tests work fine in older Linux versions.

- Some features used in the development code (e.g. generation of wheels) from the setuptools are deprecated in python versions >= 3.13.  The development of the code is temporarily limited to python versions >= 3.6 and <= 3.12 for this reason.  Modernization of the wheel generation is planned.


