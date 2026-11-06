from distutils.core import setup
from distutils.extension import Extension

include_dirs = ["../../pyDynAA/pydynaa/include/", "../../core/include/"]

ext_channel = Extension("delay_channel",
                        ["delay_channel.cpp"],
                        language="c++",
                        include_dirs=include_dirs,
                        extra_compile_args=["-std=c++11", "-static", "-fPIC"]
                        )

setup(
    name='delay_channel',
    ext_modules=[ext_channel],
    )
