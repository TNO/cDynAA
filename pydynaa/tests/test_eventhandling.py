import unittest
import pydynaa as pd
import random
from itertools import permutations


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


class CallbackException(Exception):
    """A specific exception to be raised in callbacks."""

    pass


class TestPyDynAA(unittest.TestCase):
    """
    Unit tests for PyDynAA
    """

    def setUp(self):
        """Initializion for a test fixture"""
        self.eventType1 = pd.EventType("Storm", "Storm event type")
        self.eventType2 = pd.EventType("Rain", "Rain event type")
        self.entity1 = pd.Entity()
        self.entity2 = pd.Entity()
        self.simengine = pd.DynAASim()
        self.simengine.reset()

    def test_EventType_init(self):
        """Test initialization of EventType"""
        self.assertEqual(self.eventType1.name, "Storm")
        self.assertEqual(self.eventType1.description, "Storm event type")
        self.assertEqual(self.eventType2.name, "Rain")
        self.assertEqual(self.eventType2.description, "Rain event type")
        self.assertTrue(self.eventType1.id != self.eventType2.id)
        self.assertTrue(self.eventType2.id > self.eventType1.id)
        self.assertTrue(self.eventType1 == self.eventType1)
        self.assertTrue(self.eventType1 != self.eventType2)
        # Test pretty print:
        self.assertEqual(
            "{}".format(self.eventType1), "{}".format(self.eventType1.name)
        )
        # Test hashing
        self.assertEqual(hash(self.eventType1), hash(self.eventType1))
        self.assertNotEqual(hash(self.eventType1), hash(self.eventType2))

    def test_Entity_init(self):
        """Test initialization of Entity"""
        self.assertTrue(self.entity2.uid > self.entity1.uid > 0)
        # Test pretty print:
        self.assertEqual(
            "{}".format(self.entity1), "entity_{}".format(self.entity1.uid)
        )
        # Test hashing
        self.assertEqual(hash(self.entity1), hash(self.entity1))
        self.assertNotEqual(hash(self.entity1), hash(self.entity2))

    def test_Event_description_constants(self):
        """Tests on the constants used to describe event wildcards.

        There are three:
        * ANY_EVENT_ID is provided by the pydynaa library and is an event id
        used as wildcard to describe all event ids.
        * ANY_EVENT_SOURCE is provided by the pydynaa library as wildcard to
        describe all event sources. Type must be (:py:class:`~pydynaa.core.Entity`) or None.
        * ANY_EVENT_TYPE is provided by the pydynaa library as a wildcard to
        describe all event types. Type must be :py:class:`~pydynaa.core.EventType`

        Notes
        ------
        Specifics on the internal values of these constants are NOT relevant.
        And as such there should be no tests, for example, checking that the
        value of ANY_EVENT_ID is -1.  Though this is the case in the
        implementation, it can be any value, as long as the constant is used
        overall to make operations (e.g. comparisons, etc.)

        """
        self.assertIsInstance(pd.ANY_EVENT_TYPE, pd.EventType)
        self.assertIsInstance(pd.ANY_EVENT_SOURCE, pd.Entity)

    def test_Event_init(self):
        """Test initialization of Event"""
        event1 = self.entity1._schedule_now(self.eventType1)
        event2 = self.entity2._schedule_at(10, self.eventType2)
        event3 = self.entity2._schedule_after(20, self.eventType1)
        self.assertTrue(event1.type == pd.EventExpression.ATOMIC)
        self.assertTrue(event2.type == pd.EventExpression.ATOMIC)
        self.assertTrue(event3.type == pd.EventExpression.ATOMIC)
        self.assertTrue(event3.atomic_id > event2.atomic_id > event1.atomic_id > 0)
        self.assertTrue(event1 != event2)
        event4 = event3
        self.assertTrue(event3 == event4)
        self.assertEqual(event1.atomic_type.name, self.eventType1.name)
        self.assertEqual(event2.atomic_type.name, self.eventType2.name)
        self.assertEqual(event3.atomic_type.name, self.eventType1.name)
        self.assertEqual(event1.atomic_source.uid, self.entity1.uid)
        self.assertEqual(event2.atomic_source.uid, self.entity2.uid)
        self.assertEqual(event3.atomic_source.uid, self.entity2.uid)
        self.assertEqual(event1.atomic_source, self.entity1)
        self.assertEqual(event2.atomic_source, self.entity2)
        self.assertEqual(event3.atomic_source, self.entity2)

        # Test hashing
        self.assertEqual(hash(event1), hash(event1))
        self.assertNotEqual(hash(event1), hash(event2))
        foo = {}
        events = [event1, event2, event3]
        test_items = ["A", "B", "C"]
        # Add items by hash
        for event, test_item in zip(events, test_items):
            foo[event] = test_item
        for event, test_item in zip(events, test_items):
            self.assertEqual(foo[event], test_item)
        for key, _ in foo.items():
            self.assertTrue(isinstance(key, pd.EventExpression))

    def test_Event_source(self):
        """Test that the source of an Event gives the Python Entity that created
        it or raises exception if the Entity was removed."""
        entity = TestEntity()
        test_count = random.randint(0, 10)
        entity.counter = test_count
        event = entity._schedule_now(self.eventType1)
        event2 = entity.schedule_with_vanishing_source()
        self.assertEqual(type(event.atomic_source), TestEntity)
        self.assertEqual(event.atomic_source.counter, test_count)
        self.assertEqual(type(event2.atomic_source), TestEntity)
        self.assertNotEqual(event2.atomic_source, event.atomic_source)
        self.assertEqual(
            event2.atomic_source.counter, 3
        )  # 3 is defined as a constant inside schedule_with_vanishing_source
        entity.remove()
        # del entity
        # with self.assertRaises(LookupError):
        #     event.atomic_source

    def test_Event_type(self):
        """Test the event type embedded in an Event."""
        entity = TestEntity()
        event = entity._schedule_now(self.eventType1)
        # Creates an event whose type is internal to the called method, and vanishes after method completion.
        event2 = entity.schedule_with_vanishing_type()
        self.assertEqual(event.atomic_type, self.eventType1)
        self.assertEqual(type(event2.atomic_source), TestEntity)
        self.assertEqual(event2.atomic_type.name, "VANISH")

    def test_EventHandler_init(self):
        """Test initialization of EventHandler"""

        def dummy_callback(handler_report):
            pass

        eh1 = pd.EventHandler(dummy_callback)
        self.assertEqual(eh1.identifier, "_")
        self.assertEqual(eh1.priority, pd.EventHandler.default_priority)
        eh2 = pd.EventHandler(dummy_callback, "foo")
        self.assertEqual(eh2.identifier, "foo")
        # Test non-default priority
        priority = 2
        eh3 = pd.EventHandler(dummy_callback, priority=priority)
        self.assertEqual(eh3.priority, priority)
        with self.assertRaises(RuntimeError):
            pd.EventHandler(
                dummy_callback, priority=(pd.EventHandler.lowest_priority + 1)
            )
        with self.assertRaises(RuntimeError):
            pd.EventHandler(
                dummy_callback, priority=(pd.EventHandler.highest_priority - 1)
            )
        # Test toggling of is_once
        # NOTE is this implemented as intended in cDynAA?
        event = self.entity1._schedule_now(self.eventType1)
        self.entity1._wait_once(eh2, expression=event)
        self.entity1._wait(eh2, event_type=self.eventType1)
        # Test pretty print:
        self.assertEqual("{}".format(eh1), "{}".format(eh1.identifier))

    def test_EventHandler_init_with_safe_guards(self):
        """Test initialization of EventHandler with safe guards
        Checks only the acceptance of entities as safe guards.
        Checks also raise on passing non-entities
        """

        def dummy_callback(event):
            pass

        eh1 = pd.EventHandler(dummy_callback, safe_guards=[self.entity1, self.entity2])
        self.assertEqual(eh1.identifier, "_")
        # self.assertTrue(not eh1.is_once)
        eh2 = pd.EventHandler(
            dummy_callback, identifier="foo", safe_guards=[self.entity1, self.entity2]
        )
        self.assertEqual(eh2.identifier, "foo")
        # self.assertTrue(not eh2.is_once)
        with self.assertRaises(TypeError):
            pd.EventHandler(dummy_callback, identifier="failing", safe_guards=[1, "as"])

    def test_EventHandler_callback(self):
        """Test callback functionality of EventHandler"""
        pass
        # TODO: rebuild this test with the correct structure and event reports
        # outer_id = -1
        # outer_source_uid = -1
        # outer_type_name = None

        # def dummy_callback(event):
        #     nonlocal outer_id, outer_source_uid, outer_type_name
        #     outer_id = event.id
        #     outer_type_name = event.type.name

        # eh = pd.EventHandler(dummy_callback)
        # event = self.entity1._schedule_now(self.eventType1)
        # eh.handle(event)
        # self.assertEqual(outer_id, event.id)
        # self.assertEqual(outer_type_name, self.eventType1.name)

    def test_prioritized_wait(self):
        """Test prioritized wait: multiple EventHandlers racing for one event"""

        def create_callback(index):
            def callback(handler_report):
                nonlocal results
                results.append(index)

            return callback

        ehs = [pd.EventHandler(create_callback(i), priority=i) for i in range(1, 5)]
        for eh_sequence in permutations(ehs):
            results = []
            self.simengine.reset()
            self.entity1._schedule_now(self.eventType1)
            for eh in eh_sequence:
                self.entity1._wait(eh, event_type=self.eventType1)
            self.simengine.run()
            self.assertEqual(results, [1, 2, 3, 4])

    def test_SimulationEngine_init(self):
        """Test initialization of SimulationEngine"""
        self.assertEqual(self.simengine.state, 0.0)
        self.simengine.run(100.0)
        self.assertEqual(self.simengine.state, 0.0)
        simengine2 = pd.DynAASim()
        self.assertTrue(simengine2.current_time == self.simengine.current_time)
        self.simengine.stop()
        self.assertEqual(self.simengine.state, 2.0)
        self.simengine.reset()
        runtime_largeint = 10 ** 10  # > 32bit int
        self.simengine.run(runtime_largeint)
        with self.assertRaises(TypeError):
            self.simengine.run(1j)

    def test_SimulationEngine_stop(self):
        """Tests the behavior of the simulation engine when stopping"""
        self.simengine.stop()
        with self.assertRaises(RuntimeError):
            self.simengine.run()

    def test_SimulationEngine_callback(self):
        """Test callback functionality of SimulationEngine"""
        test_ent = TestEntity()
        test_ent.start()
        self.simengine.run(101)
        self.assertEqual(test_ent.counter, 10)
        # run up to 199 and check dismiss works
        test_ent.stop()
        # Handler should be dismissed
        self.simengine.run(201.0)
        self.assertEqual(test_ent.counter, 10)

    def test_SimulationEngine_unscheduleEvents(self):
        """Test callback functionality of SimulationEngine"""
        test_ent = TestEntity()
        test_ent.start()
        self.simengine.run(11)
        self.assertEqual(test_ent.counter, 1)
        self.assertEqual(
            test_ent._unschedule(
                test_ent.scheduled_event.atomic_type, test_ent.scheduled_event.atomic_id
            ),
            1,
        )
        self.simengine.run(10)
        self.assertEqual(test_ent.counter, 1)  # next event is not issued !
        self.assertEqual(
            test_ent._unschedule(
                test_ent.scheduled_event.atomic_type,  # does not raise exception !
                test_ent.scheduled_event.atomic_id,
            ),
            0,
        )

    def test_SimulationEngine_ext_callback(self):
        """Test ability of external callback to access Event's source and type"""
        pass
        # TODO: Rebuild this test for new handler
        # def callback(event):
        #     event.source.counter += 1
        #     event.source._schedule_after(10., event.type)

        # test_ent = TestEntity(callback=callback)
        # test_ent.start()
        # self.simengine.run(101)
        # self.assertEqual(test_ent.counter, 10)
        # # run up to 199 and check dismiss works
        # test_ent.stop()
        # self.simengine.run(201.0)
        # self.assertEqual(test_ent.counter, 10)

    def test_SimulationEngine_callback_raise_exception(self):
        """Test raising of exceptions in callbacks"""

        def callback(handler_report):
            # Raise a specific exception that should be caught
            raise CallbackException

        test_ent = TestEntity(callback=callback)
        test_ent.start()
        # Check correct exception is caught
        with self.assertRaises(CallbackException):
            self.simengine.run(100)
        # the simulation should have stopped and remains in stop state
        self.assertEqual(self.simengine.state, 2.0)
        # it should have stopped in the time 10
        self.assertEqual(self.simengine.current_time, 10.0)

    def test_SimulationEngine_stops_during_callback(self):
        """Test behaviour after engine stops during callback"""

        def callback(handler_report):
            self.simengine.stop()

        test_ent = TestEntity(callback=callback)
        test_ent.start()
        self.simengine.run(100)
        # the simulation should have stopped
        self.assertEqual(self.simengine.state, 2.0)
        # it should have stopped in the time 10
        self.assertEqual(self.simengine.current_time, 10.0)
        # and it should throw an exception if we try to run it again
        with self.assertRaises(RuntimeError):
            self.simengine.run()

    def test_SchedulerAfter_with_large_sim_times(self):
        """Test use of scheduler after with large simulation times"""

        def callback(event):
            pass

        test_ent = TestEntity(callback=callback)
        test_ent.start()
        self.simengine.run(100)
        # it should have stopped in the time 10
        self.assertEqual(self.simengine.current_time, 100.0)
        # should generate a warning.  Cannot be captured
        test_ent._schedule_after(1e25, TestEntity.TEST_TYPE)
        with self.assertWarns(UserWarning):
            test_ent._schedule_after(1e25, TestEntity.TEST_TYPE)


if __name__ == "__main__":
    unittest.main(verbosity=2)
