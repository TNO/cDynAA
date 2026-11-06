# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
import os
import unittest
from .core import Event
from .core import Entity
from .core import EventType
from .core import EventHandler
from .core import ExpressionHandler
from .core import EventExpression
from .core import SimulationEngine
from .core import SimulationEngine as DynAASim
__all__ = [
    "__version__",
    "Event",
    "Entity",
    "EventType",
    "EventHandler",
    "ExpressionHandler",
    "EventExpression",
    "SimulationEngine",
    "DynAASim",
    "get_include",
    ]

try:
    _dirpath = os.path.abspath(os.path.dirname(__file__))
    with open(os.path.join(_dirpath, 'version.txt'), encoding='utf-8') as f:
        __version__ = f.read()
except FileNotFoundError:
    __version__ = ""


def get_include():
    """Return the directory containing the pyDynAA headers files (\\*.h).

    """
    return os.path.dirname(__file__) + "/include"


def test():
    """Run all discoverable tests.

    Returns
    -------
    bool
        True if there were no errors or failures, otherwise False.

    """
    test_loader = unittest.defaultTestLoader
    test_runner = unittest.TextTestRunner()
    test_suite = test_loader.discover('pydynaa', pattern="test*.py")
    result = test_runner.run(test_suite)
    return len(result.errors) == 0 and len(result.failures) == 0
