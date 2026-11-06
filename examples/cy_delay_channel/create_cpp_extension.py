# This script (re)generates the C++ source file delay_channel.cpp
#
# Note that because not every user may have Cython installed (nor the same version),
# a pregenerated version is included in this repository.
#
# To actually build this Cython module do:
#
#     python setup.py build_ext --inplace

from distutils.core import Extension
from Cython.Build import cythonize

ext_core = Extension("delay_channel", ["delay_channel.pyx"], language="c++", include_dirs=[])

cythonize([ext_core])
