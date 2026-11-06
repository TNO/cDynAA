DynAA discrete event simulation engine for C and Python
=======================================================

*cDynAA* is the C++ port of the [DynAA](https://ci.tno.nl/gitlab/DynAA/DynAA) core layer.
Like the java core of DynAA, it is the base simulation engine for several other domain specific simulators, each of which includes a specific *model layer*.

*pyDynAA* is a Python wrapper around the C++ DynAA core.
The wrapping is done using Cython and supports event handler callback functions in pure Python, as well as in Cython.
Because the pyDynAA package includes Cython definitions of the core classes (Event, Entity, etc.), a model layer using pyDynAA can itself (optionally) implement speed critical components in Cython.

See the [INSTALL.md](INSTALL.md) file for instructions of how to install the package as a user or [INSTALL-SOURCE.md](INSTALL-SOURCE.md) for how to build and install from the source repository as a developer.

The developers are listed in the [AUTHORS](AUTHORS.md) file.
The user license can be found in the [LICENSE](LICENSE.md) file.
