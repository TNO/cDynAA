# cDynAA examples

Examples of how to use cDynAA library.

First build the core libraries before trying these examples.

`cpp_delay_channel` contains pure C++ implementation of a delay channel.
Run `make` to build the binary and run it with `./run_cpp_delay_channel`.

`py_delay_channel` uses the `pydynaa` python package, which is a cython wrapping of the C++ library.
The delay channel model is implemented in pure python.
You can run this example with `./run_py_delay_channel`.

`cy_delay_channel` uses the `pydynaa` python package and also implements the delay channel in Cython. Run `make` to build the cython library then run the example with `./run_cy_delay_channel` 
(NOTE: work in progress).
