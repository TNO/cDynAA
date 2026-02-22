from setuptools import setup, Extension
from Cython.Build import cythonize
import pydynaa


extmod = Extension(
    "cyextendpkg.cyextendmodule",
    ["cyextendpkg/cyextendmodule.pyx"],
    language="c++",
    include_dirs=[pydynaa.get_include()],
    extra_compile_args=["-std=c++17", "-fPIC"],
    # NOTE explicitly linking to dynaacore lib fixed earlier linking issue
    # Should however NOT be necessary!
    # libraries=["dynaacore"],
    # library_dirs=["/home/rob/tnowork/gitlab/cDynAA/cdynaa"],
)

setup(name="cyextendpkg", ext_modules=cythonize([extmod]))
