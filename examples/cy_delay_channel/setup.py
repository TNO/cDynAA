# from distutils.core import setup
# from distutils.extension import Extension
from setuptools import setup, Extension
from Cython.Build import cythonize
import numpy
import pydynaa


# include_dirs = ["../../pyDynAA/pydynaa/include/", "../../core/include/"]

ext_channel = Extension(
    "delay_channel",
    ["delay_channel.pyx"],
    language="c++",
    include_dirs=[pydynaa.get_include(), numpy.get_include()],
    extra_compile_args=["-std=c++17", "-static", "-fPIC"],
)

setup(name="delay_channel", ext_modules=cythonize([ext_channel]))
