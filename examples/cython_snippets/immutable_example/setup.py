import os
import pkg_resources
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

ext_immutable = Extension("immutable_wrap",
                          ["immutable_wrap.pyx", "immutable.cpp"],
                          language="c++",
                          extra_compile_args=["-std=c++11", "-static", "-fPIC"]
                          )

setup(
    name='immutable',
    ext_modules=cythonize([ext_immutable]),
    )
