import unittest
import pydynaa as pd


class TestEventType(unittest.TestCase):
    """
    Unit tests for event types.
    """

    def setUp(self):
        """Initializion for a test fixture"""
        self.event_type1 = pd.EventType("Storm", "Storm event type")
        self.event_type2 = pd.EventType("Rain", "Rain event type")
        self.event_type_raw = pd.EventType()

    def test_creation_of_eventtypes(self):
        """Test basic creation of event types"""
        event_type = pd.EventType("aName", "aDescription")
        self.assertIsInstance(event_type, pd.EventType)
        self.assertEqual(event_type.name, "aName")
        self.assertEqual(event_type.description, "aDescription")
        self.assertNotEqual(event_type, self.event_type1)

    def test_event_type_basic_api(self):
        self.assertEqual(self.event_type1.name, "Storm")
        self.assertEqual(self.event_type1.description, "Storm event type")
        self.assertNotEqual(self.event_type1.id, self.event_type2.id)
        self.assertTrue(self.event_type1 == self.event_type1)
        self.assertFalse(self.event_type1 == self.event_type2)
        self.assertFalse(self.event_type1 is None)
        self.assertTrue(self.event_type1 is not None)
        with self.assertRaises(TypeError):
            self.event_type1 == "False"


if __name__ == "__main__":
    unittest.main(verbosity=2)
