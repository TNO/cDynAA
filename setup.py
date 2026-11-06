"""Setup module for PyDynAA.

"""
import os
import glob
import sys
import re
import subprocess
from setuptools import setup, Extension, Command
from setuptools.command.build_ext import build_ext as _build_ext
from setuptools.command.build_py import build_py as build_py_orig
import version

# Config parameters
PYPI_SERVER = "pypi.netsquid.org"

# Get the name of the current git branch
GIT_BRANCH_NAME = version.get_git_branch()
VERSION = version.get_version()
IS_STABLE_VERSION = version.get_git_version_tag()[1] == 0
IS_STABLE_BRANCH = (GIT_BRANCH_NAME[-7:] == '-stable' and IS_STABLE_VERSION)
IS_MAIN_BRANCH = (GIT_BRANCH_NAME == 'master' or (re.match(r"^[vV].+-develop$", GIT_BRANCH_NAME)))
IS_DEPLOY_BRANCH = re.match(r"^[vV].+-deploy$", GIT_BRANCH_NAME)
# Write version into package
with open(os.path.join("pydynaa", "version.txt"), "w") as f:
    f.write(VERSION)

# Get the long description from the README, INSTALL, AUTHORS and CHANGELOG files
ROOT_PATH = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(ROOT_PATH, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()
with open(os.path.join(ROOT_PATH, 'INSTALL.md'), encoding='utf-8') as f:
    long_description += '\n' + f.read()
with open(os.path.join(ROOT_PATH, 'AUTHORS.md'), encoding='utf-8') as f:
    long_description += '\n' + f.read()
with open(os.path.join(ROOT_PATH, 'CHANGELOG.md'), encoding='utf-8') as f:
    long_description += '\n' + f.read()

# Get setup requirements from requirements.txt
with open(os.path.join(ROOT_PATH, 'requirements.txt'), encoding='utf-8') as f:
    setup_requires = [line.strip() for line in f.readlines()]


class BuildCDynAACommand(Command):
    description = "Build the cDynAA C++ library."
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        wd = os.getcwd()
        os.chdir("cdynaa/")
        try:
            subprocess.run(("make", "staticlib")).check_returncode()
        finally:
            os.chdir(wd)


class TestCDynAACommand(Command):
    description = "Test the cDynAA C++ library."
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        wd = os.getcwd()
        os.chdir("cdynaa/")
        try:
            subprocess.run(("make", "tests")).check_returncode()
            subprocess.run(("./run_tests", "[stable]")).check_returncode()
        finally:
            os.chdir(wd)


class build_py(build_py_orig):
    def run(self):
        super().run()
        # Add header guard to all files to be packaged:
        self.prepend_file_guard()

    @staticmethod
    def add_header_guard(file_path, comment='# ', start="# ", end=""):
        skip_line = "\n"
        border_line = "{}\n".format("".join('~' * (80 - len(comment))))
        guard_lines = [
            border_line, skip_line,
            "File: {}\n".format(os.path.basename(file_path)), skip_line,
            "This file is part of the PyDynAA package.\n",
            "It is subject to the NetSquid Software End User License Conditions.\n",
            "A copy of these conditions can be found in the LICENSE.md file of this package.\n",
            skip_line]
        #with open(os.path.join(ROOT_PATH, 'LICENSE.md'), encoding='utf-8') as f:
        #    for line in f:
        #        guard_lines.append(line)
        #guard_lines.append(skip_line)
        with open(os.path.join(ROOT_PATH, 'AUTHORS.md'), encoding='utf-8') as f:
            for line in f:
                guard_lines.append(line)
        guard_lines += [skip_line, border_line]
        header = start + comment.join(guard_lines)
        if end:
            header += "{}\n".format(end)
        with open(file_path, 'r', encoding='utf-8') as f:
            original_data = f.read()
        if original_data[:50] == header[:50]:
            return
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(header + original_data)

    def prepend_file_guard(self):
        # Locate and process python files
        build_dir = glob.glob(os.path.join(ROOT_PATH, "build/lib.*"))[0]
        py_file_paths = []
        cpp_file_paths = []
        for dirpath, dirnames, filenames in os.walk(build_dir):
            for filename in filenames:
                if filename.endswith(".py") or filename.endswith(".pxd"):
                    py_file_paths.append(os.path.join(dirpath, filename))
                elif filename.endswith(".h"):
                    cpp_file_paths.append(os.path.join(dirpath, filename))
        for file_path in py_file_paths:
            self.add_header_guard(file_path, comment='# ', start="# ", end="")
        for file_path in cpp_file_paths:
            self.add_header_guard(file_path, comment=" * ", start="/* ", end=" */")


class DeployBdistCommand(Command):
    """Run command for uploading binary wheel files to NetSquid PyPi index.

    """
    description = "Deploy binary wheel files to NetSquid PyPi index."
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        if not (IS_STABLE_BRANCH or IS_DEPLOY_BRANCH):
            sys.exit("ERROR: Attempting to upload from an invalid branch (not stable or deploy): {}, {}."
                     .format(GIT_BRANCH_NAME, VERSION))
        if 'NETSQUIDCI_USER' not in os.environ:
            sys.exit("ERROR: environment variable NETSQUIDCI_USER is not defined.")
        # Upload wheel files
        pkg_dir = "{}/pypi/pydynaa/".format(os.environ['NETSQUIDCI_BASEDIR'])
        sftp_args = ["/usr/bin/sftp"]
        if 'NETSQUIDCI_PROXY' in os.environ:
            #sftp_args += ["-J", f"{os.environ['NETSQUIDCI_PROXY']}"]
            # Compatible with older openssh clients:
            sftp_args += ["-o", "ProxyCommand=/usr/bin/ssh -W %h:%p {}"
                                .format(os.environ['NETSQUIDCI_PROXY'])]
        sftp_args.append("{}@{}".format(os.environ['NETSQUIDCI_USER'], PYPI_SERVER))
        if len([f for f in os.listdir("dist/") if f.endswith(".whl")]) > 0:
            subprocess.run(
                sftp_args, input="put -p dist/*.whl {}".format(pkg_dir).encode()).check_returncode()
        else:
            sys.exit("ERROR: no wheel files in dist/ to upload.")


class build_ext(_build_ext):
    # Build cDynAA before building the extension modules
    def run(self):
        self.run_command("build_cdynaa")
        return super().run()


def get_incl_dir_cysignals():
    """Get cysignal header files."""
    import cysignals
    return os.path.dirname(cysignals.__file__)


ext_core = Extension("pydynaa.core",
                     ["pydynaa/core.pyx"],
                     language="c++",
                     include_dirs=["cdynaa/include/",
                                   "pydynaa/include/",
                                   get_incl_dir_cysignals()],
                     library_dirs=["cdynaa/"],
                     libraries=["dynaacore"],
                     #extra_link_args=["-g"],
                     extra_compile_args=["-std=c++11", "-fpic"]  # , "-g"]
                     )

cython_profiling = bool(os.environ.get('PYDYNAA_PROFILING', 0))
coverage_build = bool(os.environ.get('PYDYNAA_COVERAGE', 0))
ext_core.cython_directives = {"embedsignature": True,
                              "profile": cython_profiling,
                              "linetrace": coverage_build,
                              "language_level": '3str'}
if coverage_build:
    ext_core.define_macros.append(("CYTHON_TRACE", "1"))
    ext_core.define_macros.append(("CYTHON_TRACE_NOGIL", "1"))


setup(
    name="pydynaa",
    version=VERSION,
    cmdclass={
        "build_py": build_py,
        "build_ext": build_ext,
        "build_cdynaa": BuildCDynAACommand,
        "test_cdynaa": TestCDynAACommand,
        "deploy_bdist": DeployBdistCommand},
    description="A discrete event simulation engine.",
    long_description=long_description,
    long_description_content_type='text/markdown',
    packages=["pydynaa"],
    ext_modules=[ext_core],
    python_requires='>=3.6',
    setup_requires=setup_requires,
    install_requires=["cysignals"],
    keywords='simulator discrete event simulation engine',
    #include_package_data=True,
    package_data={
        "pydynaa": ['*.pxd', 'include/*.h', 'LICENSE.md', 'version.txt']
        },
    classifiers=[
        'Development Status :: 3 - Alpha',
        #'Intended Audience :: Developers',
        #'Topic :: Software Development :: Build Tools',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
    ],
    )
