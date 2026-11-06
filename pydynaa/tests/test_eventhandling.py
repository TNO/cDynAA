import unittest
import pydynaa as pd
import random
from itertools import permutations


class TestEntity(pd.Entity):
    TEST_TYPE = pd.EventType("test_type", "An event type for TestEntity")

    def __init__(self, callback=None):
        #super().__init__()  # no longer necessary!
        self.counter = 0
        if callback is None:
            callback = self._callback
        self.test_handler = pd.EventHandler(lambda event: callback(event))
        self.scheduled_event = None

    def _callback(self, event):
        self.counter += 1
        self.scheduled_event = self._schedule_after(10., self.TEST_TYPE)

    def start(self):
        self.scheduled_event = self._schedule_after(10., self.TEST_TYPE)
        self._wait(self.test_handler, entity=self, event_type=self.TEST_TYPE)

    def stop(self):
        self._dismiss(self.test_handler, self, self.TEST_TYPE)

    def schedule_with_vanishing_type(self):
        event_type = pd.EventType('VANISH', 'This type vanishes from user control when object goes out of scope')
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
        eventType3 = self.eventType1
        self.assertTrue(eventType3 == self.eventType1)
        # Test pretty print:
        self.assertEqual("{}".format(self.eventType1), "{}".format(self.eventType1.name))
        # Test hashing
        self.assertEqual(hash(self.eventType1), hash(self.eventType1))
        self.assertNotEqual(hash(self.eventType1), hash(self.eventType2))

    def test_Entity_init(self):
        """Test initialization of Entity"""
        self.assertTrue(self.entity2.uid > self.entity1.uid > 0)
        # Test pretty print:
        self.assertEqual("{}".format(self.entity1), "entity_{}".format(self.entity1.uid))
        # Test hashing
        self.assertEqual(hash(self.entity1), hash(self.entity1))
        self.assertNotEqual(hash(self.entity1), hash(self.entity2))

    #def test_Entity_remove(self):
    #    """Test Event raises error if Python Entity removed"""
    #    entity = TestEntity()
    #    event = entity._schedule_now(self.eventType1)
    #    entity.remove()
    #    with self.assertRaises(LookupError):
    #        event.source

    def test_Event_init(self):
        """Test initialization of Event"""
        event1 = self.entity1._schedule_now(self.eventType1)
        event2 = self.entity2._schedule_at(10, self.eventType2)
        event3 = self.entity2._schedule_after(20, self.eventType1)
        self.assertTrue(event3.id > event2.id > event1.id > 0)
        self.assertTrue(event1 != event2)
        event4 = event3
        self.assertTrue(event3 == event4)
        self.assertEqual(event1.type.name, self.eventType1.name)
        self.assertEqual(event2.type.name, self.eventType2.name)
        self.assertEqual(event3.type.name, self.eventType1.name)
        self.assertEqual(event1.source.uid, self.entity1.uid)
        self.assertEqual(event2.source.uid, self.entity2.uid)
        self.assertEqual(event3.source.uid, self.entity2.uid)
        self.assertEqual(event1.source, self.entity1)
        self.assertEqual(event2.source, self.entity2)
        self.assertEqual(event3.source, self.entity2)
        # Test pretty print:
        self.assertEqual("{}".format(event1), "ev_{}".format(event1.id))
        # Tests ANY_* static methods
        self.assertEqual(pd.Event.any_id, -1)
        any_type = pd.Event.any_type
        self.assertEqual(any_type.name, "ANY_EVENT")
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
            self.assertTrue(isinstance(key, pd.Event))

    def test_Event_source(self):
        """Test that the source of an Event gives the Python Entity that created it or raises exception if the Entity was removed."""
        entity = TestEntity()
        test_count = random.randint(0, 10)
        entity.counter = test_count
        event = entity._schedule_now(self.eventType1)
        event2 = entity.schedule_with_vanishing_source()
        self.assertEqual(type(event.source), TestEntity)
        self.assertEqual(event.source.counter, test_count)
        self.assertEqual(type(event2.source), TestEntity)
        self.assertNotEqual(event2.source, event.source)
        self.assertEqual(event2.source.counter, 3)  # 3 is defined as a constant inside schedule_with_vanishing_source
        entity.remove()
        del entity
        with self.assertRaises(LookupError):
            event.source

    def test_Event_type(self):
        """Test the event type embedded in an Event."""
        entity = TestEntity()
        event = entity._schedule_now(self.eventType1)
        # Creates an event whose type is internal to the called method, and vanishes after method completion.
        event2 = entity.schedule_with_vanishing_type()
        self.assertEqual(event.type, self.eventType1)
        self.assertEqual(type(event2.source), TestEntity)
        self.assertEqual(event2.type.name, "VANISH")

    def test_EventHandler_init(self):
        """Test initialization of EventHandler"""

        def dummy_callback(event):
            pass

        eh1 = pd.EventHandler(dummy_callback)
        self.assertEqual(eh1.identifier, "_")
        #self.assertTrue(not eh1.is_once)
        self.assertEqual(eh1.priority, pd.EventHandler.default_priority)
        eh2 = pd.EventHandler(dummy_callback, "foo")
        self.assertEqual(eh2.identifier, "foo")
        #self.assertTrue(not eh2.is_once)
        # Test non-default priority
        priority = 2
        eh3 = pd.EventHandler(dummy_callback, priority=priority)
        self.assertEqual(eh3.priority, priority)
        with self.assertRaises(RuntimeError):
            eh3 = pd.EventHandler(dummy_callback, priority=(pd.EventHandler.lowest_priority + 1))
        with self.assertRaises(RuntimeError):
            eh3 = pd.EventHandler(dummy_callback, priority=(pd.EventHandler.highest_priority - 1))
        # Test toggling of is_once
        # NOTE is this implemented as intended in cDynAA?
        event = self.entity1._schedule_now(self.eventType1)
        self.entity1._wait_once(eh2, event=event)
        #self.assertTrue(eh2.is_once)
        self.entity1._wait(eh2, event_type=self.eventType1)
        #self.assertTrue(not eh2.is_once)
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
        #self.assertTrue(not eh1.is_once)
        eh2 = pd.EventHandler(dummy_callback, identifier="foo", safe_guards=[self.entity1, self.entity2])
        self.assertEqual(eh2.identifier, "foo")
        #self.assertTrue(not eh2.is_once)
        with self.assertRaises(TypeError):
            pd.EventHandler(dummy_callback, identifier="failing", safe_guards=[1, "as"])

    def test_EventHandler_callback(self):
        """Test callback functionality of EventHandler"""

        outer_id = -1
        outer_source_uid = -1
        outer_type_name = None

        def dummy_callback(event):
            nonlocal outer_id, outer_source_uid, outer_type_name
            outer_id = event.id
            outer_type_name = event.type.name

        eh = pd.EventHandler(dummy_callback)
        event = self.entity1._schedule_now(self.eventType1)
        eh.handle(event)
        self.assertEqual(outer_id, event.id)
        self.assertEqual(outer_type_name, self.eventType1.name)

    def test_EventHandler_safe_callback(self):
        """Test callback functionality of a safe EventHandler"""

        outer_id = -1
        outer_source_uid = -1
        outer_type_name = None

        def dummy_callback(event):
            nonlocal outer_id, outer_source_uid, outer_type_name
            outer_id = event.id
            outer_type_name = event.type.name

        def make_unsafe_callback():
            entity = TestEntity()  # This entity dies at the end of this scope !!!!
            eh = pd.EventHandler(dummy_callback, safe_guards=[entity])
            event = entity._schedule_now(self.eventType2)
            entity.remove()
            del entity
            return eh, event

        (eh2, event2) = make_unsafe_callback()
        # there exist no source for this event
        #self.assertTrue(event2.source is None)
        with self.assertRaises(LookupError):
            event2.source
        # should not run the callback this time
        eh2.handle(event2)
        self.assertEqual(outer_id, event2.id)  # should be assertNotEqual
        #self.assertEqual(outer_source_uid, uid)
        self.assertEqual(outer_type_name, self.eventType2.name)  # should be assertNotEqual
        # TODO: Think about better test on detecting removed entity... this one does not test anything.

    def test_prioritized_wait(self):
        """Test prioritized wait: multiple EventHandlers racing for one event"""
        def create_callback(index):
            def callback(event):
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
        runtime_largeint = 10**10  # > 32bit int
        self.simengine.run(runtime_largeint)
        with self.assertRaises(TypeError):
            self.simengine.run(1j)
        self.assertEqual(self.simengine.num_events, 0)
        self.assertEqual(self.simengine.num_instants, 0)
        self.assertEqual(self.simengine.num_handlers, 0)

    def test_SimulationEngine_stop(self):
        """ Tests the behavior of the simulation engine when stopping """
        self.simengine.stop()
        with self.assertRaises(RuntimeError):
            self.simengine.run()

    def test_SimulationEngine_callback(self):
        """Test callback functionality of SimulationEngine"""
        test_ent = TestEntity()
        test_ent.start()
        # Creates on handler and schedules 1 event
        self.assertEqual(self.simengine.num_events, 1)
        self.assertEqual(self.simengine.num_instants, 1)
        self.assertEqual(self.simengine.num_handlers, 1)
        self.simengine.run(101)
        self.assertEqual(test_ent.counter, 10)
        # Handler still exists and event scheduling cycles
        self.assertEqual(self.simengine.num_events, 1)
        self.assertEqual(self.simengine.num_instants, 1)
        self.assertEqual(self.simengine.num_handlers, 1)
        # run up to 199 and check dismiss works
        test_ent.stop()
        # Handler should be dismissed
        self.assertEqual(self.simengine.num_handlers, 0)
        self.simengine.run(201.0)
        self.assertEqual(test_ent.counter, 10)
        # No handler to keep cycling events
        self.assertEqual(self.simengine.num_events, 0)
        self.assertEqual(self.simengine.num_instants, 0)

    def test_SimulationEngine_unscheduleEvents(self):
        """Test callback functionality of SimulationEngine"""
        test_ent = TestEntity()
        test_ent.start()
        self.simengine.run(11)
        self.assertEqual(test_ent.counter, 1)
        test_ent.scheduled_event.unschedule()
        self.simengine.run(10)
        self.assertEqual(test_ent.counter, 1)  # next event is not issued !
        test_ent.scheduled_event.unschedule()  # does not raise exception !

    def test_SimulationEngine_ext_callback(self):
        """Test ability of external callback to access Event's source and type"""

        def callback(event):
            event.source.counter += 1
            event.source._schedule_after(10., event.type)

        test_ent = TestEntity(callback=callback)
        test_ent.start()
        self.simengine.run(101)
        self.assertEqual(test_ent.counter, 10)
        # run up to 199 and check dismiss works
        test_ent.stop()
        self.simengine.run(201.0)
        self.assertEqual(test_ent.counter, 10)

    def test_SimulationEngine_callback_raise_exception(self):
        """Test raising of exceptions in callbacks"""

        def callback(event):
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

        def callback(event):
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
