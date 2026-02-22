"""Setup module for PyDynAA."""

import os
import glob
import re
import subprocess
from setuptools import setup, Extension, Command
from setuptools.command.build_ext import build_ext as _build_ext
from setuptools.command.build_py import build_py as build_py_orig

### Utility functions for versioning

# This file uses NumPy style docstrings: https://github.com/numpy/numpy/blob/master/doc/HOWTO_DOCUMENT.rst.txt

"""
Functions for determining the current version and Git branch.
Git tags are used to determine the latest minor version.

"""

# Current major version; will override the latest git version tag if not equal
MAJOR_VERSION = 2


def get_git_branch():
    """Return name of current Git branch.
    Returns
    -------
    str
        Name of current Git branch, or an empty string if not applicable.
    """
    if "CI_COMMIT_REF_NAME" in os.environ:
        return os.environ["CI_COMMIT_REF_NAME"]
    try:
        git_output = subprocess.run(
            ("git", "symbolic-ref", "HEAD"), stdout=subprocess.PIPE
        ).stdout
        return re.sub("refs/heads/", "", git_output.decode("utf-8").rstrip())
    except FileNotFoundError:
        print("WARNING: failed to run Git command.")
        return ""


def get_git_commit():
    """Return hash of current Git commit (HEAD).
    Returns
    -------
    str
        Hash of current current Git commit (HEAD), or an empty string if not applicable.
    """
    try:
        git_output = subprocess.run(
            ("git", "rev-parse", "HEAD"), stdout=subprocess.PIPE
        ).stdout
        return git_output.decode("utf-8").rstrip()
    except FileNotFoundError:
        print("WARNING: failed to run Git command.")
        return ""


def get_git_version_tag(merged=True):
    """Get the latest version tag from Git history.

    Parameters
    ----------
    merged : bool, optional
        Whether to only look in the current branch's merged history. If False, will look
        for the latest tag in all branches. Default is True.

    Returns
    -------
    tuple of int
        Latest version tag from Git, or an empty tuple if not found or applicable.
    int
        Number of commits behind the current (HEAD) commit, or -1 if a version not found or applicable.

    """
    # Get list of tags sorted by most recent first:
    try:
        args = ["git", "tag", "--sort=-version:refname"]
        if merged:
            args.append("--merged")
        git_output = subprocess.run(tuple(args), stdout=subprocess.PIPE).stdout.decode(
            "utf-8"
        )
        tags = git_output.splitlines()
    except FileNotFoundError:
        print("WARNING: failed to run Git command.")
        return (), -1
    # Get first tag matching a version pattern:
    for tag in tags:
        if re.match(r"^[vV]\d+(\.\d+)+$", tag):
            break
    else:
        print("WARNING: failed to find a Git tag with version format vX.Y.Z")
        return (), -1
    # Count the number of commits we are ahead of this tag
    git_output = subprocess.run(
        ("git", "rev-list", "{}..HEAD".format(tag)), stdout=subprocess.PIPE
    ).stdout.decode("utf-8")
    N = len(git_output.splitlines())
    m = re.match(r"^[vV](?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)$", tag)
    version = (int(m.group("major")), int(m.group("minor")), int(m.group("patch")))
    return version, N


def get_version(short=False, merged=True):
    """Get the full version of the package.

    If the current commit (HEAD) is ahead of the latest Git version tag by ``N`` commits,
    the minor version of this tag is bumped up one, and a suffix is
    added to uniquely identify it as a development commit i.e.
    if the latest version tag is ``vX.Y.Z`` and ``N > 0`` the returned version is
    ``X.Y.Z.dev{N-1}+{commit hash}``.

    ``.profile``  is appended as to the local label -- i.e. following a ``+`` -- if
    cython profiling is enabled.
    ``.coverage`` is appended as to the local label -- i.e. following a ``+`` -- if
    coverage is enabled.

    Parameters
    ----------
    short : bool, optional
        Only return X.Y.Z without suffixes. Still increments the minor version.
    merged : bool, optional
        Whether to only look in the current branch's merged history. If False, will look
        for the latest tag in all branches. Default is True.

    Returns
    -------
    str
        The full version of the package.

    """
    vnums, N = get_git_version_tag(merged=merged)
    if vnums[0] != MAJOR_VERSION:
        # We're at a higher major version
        vnums = (MAJOR_VERSION, 0)

    version = ".".join([str(i) for i in vnums])
    # Check if pure version commit:
    if not short:
        if N > 0 and len(version) > 0:
            commit = get_git_commit()
            version = "{}.dev{}+{}".format(version, N - 1, commit[:8])
        if os.environ.get("PYDYNAA_PROFILING", 0):
            if "+" not in version:
                version += "+"
            version += ".profile"
        if os.environ.get("PYDYNAA_COVERAGE", 0):
            if "+" not in version:
                version += "+"
            version += ".coverage"
    return version


### End of utility functions for versioning

# Get the name of the current git branch
GIT_BRANCH_NAME = get_git_branch()
VERSION = get_version()
IS_STABLE_VERSION = get_git_version_tag()[1] == 0
IS_STABLE_BRANCH = GIT_BRANCH_NAME[-7:] == "-stable" and IS_STABLE_VERSION
IS_MAIN_BRANCH = GIT_BRANCH_NAME == "master" or (
    re.match(r"^[vV].+-develop$", GIT_BRANCH_NAME)
)
IS_DEPLOY_BRANCH = re.match(r"^[vV].+-deploy$", GIT_BRANCH_NAME)
# Write version into package
with open(os.path.join("pydynaa", "version.txt"), "w") as f:
    f.write(VERSION)

# Get the long description from the README, INSTALL, AUTHORS and CHANGELOG files
ROOT_PATH = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(ROOT_PATH, "README.md"), encoding="utf-8") as f:
    long_description = f.read()
with open(os.path.join(ROOT_PATH, "INSTALL.md"), encoding="utf-8") as f:
    long_description += "\n" + f.read()
with open(os.path.join(ROOT_PATH, "AUTHORS.md"), encoding="utf-8") as f:
    long_description += "\n" + f.read()
with open(os.path.join(ROOT_PATH, "CHANGELOG.md"), encoding="utf-8") as f:
    long_description += "\n" + f.read()

# Get setup requirements from requirements.txt
with open(os.path.join(ROOT_PATH, "requirements.txt"), encoding="utf-8") as f:
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
    def add_header_guard(file_path, comment="# ", start="# ", end=""):
        skip_line = "\n"
        border_line = "{}\n".format("".join("~" * (80 - len(comment))))
        guard_lines = [
            border_line,
            skip_line,
            "File: {}\n".format(os.path.basename(file_path)),
            skip_line,
            "This file is part of the PyDynAA package.\n",
            "It is subject to the NetSquid Software End User License Conditions.\n",
            "A copy of these conditions can be found in the LICENSE.md file of this package.\n",
            skip_line,
        ]
        with open(os.path.join(ROOT_PATH, "AUTHORS.md"), encoding="utf-8") as f:
            for line in f:
                guard_lines.append(line)
        guard_lines += [skip_line, border_line]
        header = start + comment.join(guard_lines)
        if end:
            header += "{}\n".format(end)
        with open(file_path, "r", encoding="utf-8") as f:
            original_data = f.read()
        if original_data[:50] == header[:50]:
            return
        with open(file_path, "w", encoding="utf-8") as f:
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
            self.add_header_guard(file_path, comment="# ", start="# ", end="")
        for file_path in cpp_file_paths:
            self.add_header_guard(file_path, comment=" * ", start="/* ", end=" */")


class build_ext(_build_ext):
    # Build cDynAA before building the extension modules
    def run(self):
        self.run_command("build_cdynaa")
        return super().run()


def get_incl_dir_cysignals():
    """Get cysignal header files."""
    import cysignals

    return os.path.dirname(cysignals.__file__)


ext_core = Extension(
    "pydynaa.core",
    ["pydynaa/core.pyx"],
    language="c++",
    include_dirs=["cdynaa/include/", "pydynaa/include/", get_incl_dir_cysignals()],
    library_dirs=["cdynaa/"],
    libraries=["dynaacore"],
    # extra_link_args=["-g"],
    extra_compile_args=["-std=c++17", "-fpic"],  # , "-g"]
)

cython_profiling = bool(os.environ.get("PYDYNAA_PROFILING", 0))
coverage_build = bool(os.environ.get("PYDYNAA_COVERAGE", 0))
ext_core.cython_directives = {
    "embedsignature": True,
    "profile": cython_profiling,
    "linetrace": coverage_build,
    "language_level": "3str",
}
if coverage_build:
    ext_core.define_macros.append(("CYTHON_TRACE", "1"))
    ext_core.define_macros.append(("CYTHON_TRACE_NOGIL", "1"))


dist = setup(
    name="pydynaa",
    # version=VERSION,
    cmdclass={
        "build_py": build_py,
        "build_ext": build_ext,
        "build_cdynaa": BuildCDynAACommand,
        "test_cdynaa": TestCDynAACommand,
    },
    description="A discrete event simulation engine.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    packages=["pydynaa"],
    ext_modules=[ext_core],
    python_requires=">=3.6,<=3.12",
    setup_requires=setup_requires,
    install_requires=["cysignals"],
    keywords="simulator discrete event simulation engine",
    # include_package_data=True,
    package_data={"pydynaa": ["*.pxd", "include/*.h", "LICENSE.md", "version.txt"]},
    classifiers=[
        "Development Status :: 3 - Alpha",
        #'Intended Audience :: Developers',
        #'Topic :: Software Development :: Build Tools',
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
    ],
)
