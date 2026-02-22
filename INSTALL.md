Installing the pyDynAA package
==============================

Here we describe how to install the pyDynAA package as a user.
If you are a developer wanting to build and install pyDynAA or the cDynAA C++ library from source then see the [INSTALL-SOURCE.md](INSTALL-SOURCE.md) file.

The prerequisites for the user installation of pyDynAA are:
- A modern Linux or MacOS 64-bit (x86_64) operating system. If you don't have linux you could run it via virtualization, e.g. using [VirtualBox](https://www.virtualbox.org/). If you have Windows 10 you can also use the [Bash on Ubuntu](https://www.howtogeek.com/249966/how-to-install-and-use-the-linux-bash-shell-on-windows-10/) subsystem.
- Python version 3.6 or higher for use and up to version 3.12 (for development). Some tests and use of setuptools need to be upgraded for higher versions than 3.12 yet.
- The python package install [pip](https://pip.pypa.io/en/stable/) version 19 or higher. To upgrade your current pip version run `pip3 install --upgrade pip`.
- The Python package _cysignals_.

### Installing using a PyPI server

Work in progress.  We are working to provide wheels from the pyPI server.

### Installing using a wheel file

If you have received a binary wheel distribution (`.whl` file) of pyDynAA it is important that your system's architecture, operating system, and major Python version  match that of the file.
For example, the file `pydynaa-0.1.0-cp36m-linux_x86_64.whl` is intended for a linux OS running on a 64 bit architecture with Python 3.6 (i.e. cp36m).

This file can be installed using the [Python _pip_ installer](https://pip.pypa.io/en/stable/user_guide/#user-installs). For example, to install locally as a user run:
```
pip3 install --user pydynaa-0.1.0-cp36m-linux_x86_64.whl
```
To install the package system-wide (for all users) remove the `--user` argument.
