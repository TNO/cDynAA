from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize


ext_callback = Extension(
    "std_func_callback",
    ["call_obj.pyx", "std_func_callback.pyx"],
    language="c++",
    extra_compile_args=["-std=c++17", "-static", "-fPIC"],
)

setup(name="callback", ext_modules=cythonize([ext_callback]))
