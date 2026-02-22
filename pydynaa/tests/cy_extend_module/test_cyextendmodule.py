import os
import unittest
from setuptools import sandbox


class TestCyExtendModule(unittest.TestCase):
    """
    Tests creating a cython module that extends pydynaa.
    """

    dir_path = os.path.dirname(os.path.realpath(__file__))

    def setUp(self):
        """Setup the cython extension module."""
        sandbox.run_setup(
            os.path.join(self.dir_path, "setup.py"), ["clean", "build_ext", "--inplace"]
        )

    def tearDown(self):
        """Setup the cython extension module."""
        sandbox.run_setup(os.path.join(self.dir_path, "setup.py"), ["clean", "--all"])

    def test_module_import(self):
        """Test importing of extending package with module."""
        # Should not raise import error e.g. due to undefined symbols
        from .cyextendpkg import cyextendmodule  # noqa

        cyextendmodule.run_test()
        # TODO also test some output result


if __name__ == "__main__":
    unittest.main(verbosity=2)
