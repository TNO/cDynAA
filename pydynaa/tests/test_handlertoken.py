import unittest
import pydynaa as pd


class TestEntity(pd.Entity):
    TEST_TYPE = pd.EventType("test_type", "An event type for TestEntity")

    def __init__(self, callback=None):
        # super().__init__()  # no longer necessary!
        self.counter = 0
        if callback is None:
            callback = self._callback
        self.test_handler = pd.EventHandler(
            lambda handler_report: callback(handler_report)
        )
        self.scheduled_event = None
        self.received_event_report = None

    def _callback(self, handler_report):
        self.counter += 1
        self.scheduled_event = self._schedule_after(10.0, self.TEST_TYPE)
        self.received_event_report = handler_report

    def start(self):
        self.scheduled_event = self._schedule_after(10.0, self.TEST_TYPE)
        self.handlerToken = self._wait(
            self.test_handler, entity=self, event_type=self.TEST_TYPE
        )

    def stop(self):
        self._dismiss(self.handlerToken)

    def schedule_with_vanishing_type(self):
        event_type = pd.EventType(
            "VANISH",
            "This type vanishes from user control when object goes out of scope",
        )
        return self._schedule_now(event_type)

    def schedule_with_vanishing_source(self):
        entity = TestEntity()
        entity.counter = 3
        return entity._schedule_now(self.TEST_TYPE)


class TestPyDynAAHandlerToken(unittest.TestCase):
    """
    Unit tests for PyDynAA`s handlertoken
    """

    def setUp(self):
        """Initializion for a test fixture"""
        self.simengine = pd.DynAASim()
        self.simengine.reset()

    def test_basic_HandlerToken_API(self):
        """Test callback functionality of SimulationEngine"""
        test_ent = TestEntity()
        test_ent.start()
        # Creates on handler and schedules 1 event
        handler_token = test_ent.handlerToken
        self.assertIsInstance(handler_token, pd.HandlerToken)

        # Comparisons with handler tokens
        self.assertEqual(handler_token, handler_token)
        self.assertFalse(handler_token is None)
        self.assertTrue(handler_token is not None)
        with self.assertRaises(TypeError):
            handler_token == False  # noqa: E712 -- do not use "is" as we test __eq__ here
        with self.assertRaises(TypeError):
            handler_token == test_ent

        test_ent.stop()
