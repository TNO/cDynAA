# This file uses NumPy style docstrings: https://github.com/numpy/numpy/blob/master/doc/HOWTO_DOCUMENT.rst.txt

"""
Functions for determining the current version and Git branch.
Git tags are used to determine the latest minor version.

"""
import os
import time
import subprocess
import re
__all__ = [
    "get_version",
    "get_git_branch",
    "get_git_commit",
    "get_git_version_tag",
    "bump_version",
    "get_stable_branches",
    "latest_tag_of_branch"
]

# Current major version; will override the latest git version tag if not equal
MAJOR_VERSION = 1


def get_git_branch():
    """Return name of current Git branch.

    Returns
    -------
    str
        Name of current Git branch, or an empty string if not applicable.

    """
    if 'CI_COMMIT_REF_NAME' in os.environ:
        return os.environ['CI_COMMIT_REF_NAME']
    try:
        git_output = subprocess.run(('git', 'symbolic-ref', 'HEAD'), stdout=subprocess.PIPE).stdout
        return re.sub('refs/heads/', '', git_output.decode('utf-8').rstrip())
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
        git_output = subprocess.run(('git', 'rev-parse', 'HEAD'), stdout=subprocess.PIPE).stdout
        return git_output.decode('utf-8').rstrip()
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
        args = ['git', 'tag', '--sort=-version:refname']
        if merged:
            args.append('--merged')
        git_output = subprocess.run(tuple(args), stdout=subprocess.PIPE).stdout.decode('utf-8')
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
    git_output = subprocess.run(('git', 'rev-list', '{}..HEAD'.format(tag)),
                                stdout=subprocess.PIPE).stdout.decode('utf-8')
    N = len(git_output.splitlines())
    m = re.match(r"^[vV](?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)$", tag)
    version = (int(m.group('major')), int(m.group('minor')), int(m.group('patch')))
    return version, N


def bump_version(version, level=1, drop_trailing=True):
    """Bump the major, minor or patch number of a version string.

    Parameters
    ----------
    version : tuple of int
        Version string to bump, with format ``(X, Y, Z)`` corresponding to X.Y.Z.
    level : int, optional
        Level of version to bump: major is 0, minor is 1, patch is 2.
    drop_trailing : bool, optional
        Whether to drop the trailing version numbers when bumping e.g.
        drop the patch number when bumping a minor version. If False,
        these numbers are set to 0.

    Returns
    -------
    tuple of int
        New bumped version string.

    """
    if not isinstance(version, tuple) or len(version) == 0:
        raise ValueError("{} does not have version format X.Y.Z".format(version))
    vnumbers = list(version)
    vnumbers[level] += 1
    if drop_trailing:
        vnumbers = vnumbers[:level + 1]
    elif level < 2:
        vnumbers[level + 1:3] = [0] * (2 - level)
    return tuple(vnumbers)


def get_version(short=False, merged=True):
    """Get the full version of the package.

    If the current commit (HEAD) is ahead of the latest Git version tag by ``N`` commits,
    the minor version of this tag is bumped up one, and a suffix is
    added to uniquely identify it as a development commit i.e.
    if the latest version tag is ``vX.Y.Z`` and ``N > 0`` the returned version is
    ``X.{Y+1}.dev{N-1}+{commit hash}``.

    ``.profile``  is appended as to the local label -- i.e. following a ``+`` -- if
    cython profiling is enabled.

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
    elif N > 0 and len(vnums) > 0:
        # bump the minor version number and designate as dev version
        vnums = bump_version(vnums, level=1, drop_trailing=True)
    version = ".".join([str(i) for i in vnums])
    # Check if pure version commit:
    if not short:
        if N > 0 and len(version) > 0:
            commit = get_git_commit()
            version = "{}.dev{}+{}".format(version, N - 1, commit[:8])
        if os.environ.get("PYDYNAA_PROFILING", 0):
            if '+' not in version:
                version += '+'
            version += '.profile'
        if os.environ.get("PYDYNAA_COVERAGE", 0):
            if '+' not in version:
                version += '+'
            version += '.coverage'
    return version


def check_dirty_version(print_warning=True, warning_delay=10):
    """Check whether build environment is potentially dirty due
    to mismatch in profiling or coverage builds.

    Parameters
    ----------
    print_warning : bool, optional
        Whether to print a warning to stdout.
    warning_delay : int, optional
        Time to pause for when warning printed, in seconds.

    Returns
    -------
    bool
        True if the build environment is potentially dirty, otherwise False.

    """
    try:
        with open(os.path.join("pydynaa", "version.txt"), "r") as f:
            version = f.readline()
    except FileNotFoundError:
        # If not version.txt assume build environment is clean
        return False
    profiling_envvar = bool(os.environ.get("PYDYNAA_PROFILING", 0))
    coverage_envvar = bool(os.environ.get("PYDYNAA_COVERAGE", 0))
    if (profiling_envvar == (".profile" in version)) and (coverage_envvar == (".coverage" in version)):
        return False
    if print_warning:
        print("\nWARNING: the build environment is potentially dirty.")
        print("         Last built version was: {}".format(version))
        print("         Version to be built is: {}".format(get_version()))
        print("         Consider running `make clean` before proceeding\n")
        if warning_delay > 0:
            print("... build will continue after a {}s delay. Quit build using Ctrl-C.\n".format(warning_delay))
            time.sleep(warning_delay)
    return True


def get_stable_branches():
    """Fetches the stable branches from the remote.

    Returns
    -------
    list
        All branches on the remote that are stable branches.

    """
    try:
        git_output = subprocess.run(('git', 'branch', '--remote'), stdout=subprocess.PIPE).stdout
        branches = git_output.decode('utf-8').rstrip().split('\n')
        branch_names = []
        for branch in branches:
            stable = re.match(r'.*?/(\d*-\d*-stable)$', branch)
            if stable:
                branch_names.append(stable.group(1))
        return branch_names
    except FileNotFoundError:
        print("WARNING: failed to run Git command.")
        return []


def latest_tag_of_branch(branch_names):
    """Fetches the latest tag from a branch on the remote.

    Parameters
    ----------
    branch_names : str or list of str
        The branch name or list of branch names.

    Returns
    -------
    list
        The tags without the 'v' prepended.

    """
    if not isinstance(branch_names, list):
        branch_names = [branch_names]
    tag_names = []
    for branch in branch_names:
        assert isinstance(branch, str)
        try:
            git_output = subprocess.run(('git', 'describe', '--abbrev=0', "origin/{}".format(branch)), stdout=subprocess.PIPE).stdout
            tag_names.append(re.sub(r"^[vV](\d+(\.\d+)+)$", r'\g<1>', git_output.decode('utf-8').rstrip()))
        except FileNotFoundError:
            print('WARNING: failed to run Git command.')
            return []
    return tag_names


if __name__ == "__main__":
    check_dirty_version(warning_delay=0)
    print(get_version())
