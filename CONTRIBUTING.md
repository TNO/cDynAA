# Contributing
============

## Git and GitLab
--------------

We use the [Gitlab workflow](https://docs.gitlab.com/ee/workflow/gitlab_flow.html).
The `master` branch contains the latest _working_ development code, and new issue branches (see below) should branch off of _master_.
By _working_ we mean that the _master_ branch should be passing its tests (also see continuous integration below).
The _master_ branch is _protected_, meaning that developers must create a merge (pull) request to merge a feature branch into it.

The following branches that have a specific naming patterns (using wildcards) are also protected: `*-stable`, `v*-develop`, and `v*-deploy`.
- Branches named as `X-Y-stable` are used for stable releases (see below) of a version vX.Y.
- Branches named as `vX-develop` are used for the development of other major versions, vX, that differ from the major version currently being developed in the master branch.
- Branches named as `vX-deploy` are used to deploy development builds related to a specific major version, vX.

New code should be developed in a separate branch and then merged into one of the protected branches, usually the master branch.
Because any new code should address some kind of _issue_, e.g. a new feature or a bug, the name of the branch should refer to an _Issue_ number on the [Gitlab issue tracker](https://ci.tno.nl/gitlab/DynAA/cDynAA/issues) i.e. the _branch name_ has the format `<issue-number>-<issue-title>`.
For example, if Alice wants to work on a new feature to refactor qubits, she creates a new _Issue_ on the tracker, assigned the number 12 by Gitlab.
She creates the branch `12-refactor-qubits` with which she will address this issue -- by using this naming convention Gitlab automatically couples the branch to the Issue.
Once she is done with this issue she creates a merge request to merge _12-refactor-qubits_ into _master_.


### Git blame and bulk reformatting commits

Bulk reformatting commits ruin the git blame history. For example, a commit that renormalises line endings from CLRF (Windows) to LF (linux) potentially touches every line of every file (see commit 7fefe128d132e9f811f96dd92f61d7b6a5c7d0c1a).

To keep Git blame useful, such commits are tracked by us in the `.git-blame-ignore-revs` file.
To use this file to correct the behaviour of Git blame, you need to set the following setting locally for your repository from command line:

    $ git config blame.ignoreRevsFile .git-blame-ignore-revs

## Versioning and releases
-----------------------

PyDynAA uses [semantic versioning](https://semver.org/).
PyDynAA determines the major version using a `MAJOR_VERSION` variable in the _version.py_ module, while minor and patch versions are detmined using the last *version tag* in the git repository.
For example, if the current commit is tagged with `Version 0.3.6`, then the current commit is considered *stable* and the version of PyDynAA is `0.3.6`,
which can be inspected as follows:

```python
>>> import pydynaa
>>> pydynaa.__version__
0.3.6
```

or by running `python version.py` in the root directory.
If there are additional commits since the last version tag, then the current commit is a *development version*.
This is indicated by bumping the minor version by one and appending `.dev{N-1}+{commit hash}`, where N is the number of additional commits.
For example, if there are 5 commits beyond the tag `Version 0.3.2`, then the version would be `0.4.dev4+a19bbd50` (for some commit hash).
Special profiling or coverage builds append `.profile` or `.coverage` to the end of the version number.

Each major and minor release should be given its own stable branch named `X-Y-stable`, which is a naming pattern that is setup in Gitlab to be protected.
These stable branches can be updated by merging in patch commits to the given release.
Stables branches should always contain *stable versions* i.e. not *development versions*.
The [Gitlab workflow](https://docs.gitlab.com/ee/workflow/gitlab_flow.html) describes the workflow of updating a release branch e.g. `0.3-stable` for version 0.3:

> The stable branch uses master as a starting point and is created as late as possible. By branching as late as possible you minimize the time you have to apply bug fixes to multiple branches. After a release branch is announced, only serious bug fixes are included in the release branch. If possible these bug fixes are first merged into master and then cherry-picked into the release branch.

To release a new major or minor version, X.Y.0, follow these steps:

1. Identify the correct commit on the master branch, and ensure it passes all CI pipeline tests (eg `make verify`).
2. Ensure CHANGELOG.md is up to date. Note the target audience is *developers*.
3. Tag the commit in master with the correct version number e.g. `git tag -a "vX.Y.0" -m "Version X.Y.0"` and push it to origin with `git push origin vX.Y.0`.
4. Create a stable branch `X-Y-stable`, ensure it matches the commit above, and push it to origin. The CI pipeline automatically creates and uploads the binary distributions.

To release a patch version, i.e. X.Y.Z for Z > 0, follows steps 1, 2, and 4 above.
Then merge the tagged commit into the existing `X-Y-stable` branch, and push the result to origin.
The CI pipeline should do the rest.

Other branches that are deployed by the CI pipeline are `vX-deploy`, which requires that the commit is a development version
i.e. a stable version will not be deployed to avoid overriding deployed stable packages.
This branch can be used to share a development version on the python package index when necessary.

## GitLab continuous integration
-----------------------------

This repository makes use of the [GitLab Continuous Integration](https://docs.gitlab.com/ee/ci/) feature to check compilation and run unit tests after a commit to the remote `master` branch.
This behaviour is specified in the file `.gitlab-ci.yml`.

Note that only _[stable]_ unit tests are run on the C++ core library.

The CI tests are run on a _runner_ server (at ci.netsquid.org) within a [Docker container](https://hub.docker.com/r/rknegjens/netsquidci/). 
