import unittest
import math
import pydynaa as pd


class TestEntity(pd.Entity):
    """Provides a simple mock entity for tests"""

    TEST_TYPE = pd.EventType("test_type", "An event type for TestEntity")
    TEST_TYPE2 = pd.EventType("test_type2", "Another event type for TestEntity")

    def __init__(self, callback=None):
        self.counter = 0
        if callback is None:
            callback = self._callback
        self.test_handler = pd.EventHandler(lambda event: callback(event))
        self.scheduled_events = []
        self.event_reports = []
        self.handler_token = None

    def _callback(self, handler_report):
        self.counter += 1
        self.event_reports.append(handler_report)
        self.scheduled_events.append(self._schedule_after(10.0, self.TEST_TYPE))

    def start(self):
        self.scheduled_events.append(self._schedule_after(10.0, self.TEST_TYPE))
        self.scheduled_events.append(self._schedule_after(10.0, self.TEST_TYPE2))
        self.handler_token = self._wait(
            self.test_handler, entity=self, event_type=self.TEST_TYPE
        )
        return self

    def stop(self):
        self._dismiss(self.test_handler, self, self.TEST_TYPE)

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


class TestEventExpression(unittest.TestCase):
    """
    Unit tests for event expressions.
    """

    def setUp(self):
        """Initializion for a test fixture"""
        self.eventType1 = pd.EventType("Storm", "Storm event type")
        self.eventType2 = pd.EventType("Rain", "Rain event type")
        self.entity1 = pd.Entity()
        self.entity2 = pd.Entity()
        self.eventId1 = 5
        self.simengine = pd.DynAASim()
        self.simengine.reset()
        self.testEntity = TestEntity().start()
        self.testEvent = self.testEntity.scheduled_events[0]
        self.testEvent2 = self.testEntity.scheduled_events[1]

    def test_creation_of_atomics(self):
        """Test basic creation of Atomic EventExpressions"""
        ev_expression = pd.EventExpression()  # generic event expression
        self.assertEqual(ev_expression.type, pd.EventExpression.ATOMIC)
        self.assertIs(ev_expression.atomic_source, pd.ANY_EVENT_SOURCE)
        self.assertEqual(ev_expression.atomic_type, pd.ANY_EVENT_TYPE)
        self.assertEqual(ev_expression.atomic_id, pd.ANY_EVENT_ID)
        self.assertEqual(ev_expression.first_term, ev_expression)
        self.assertEqual(ev_expression.second_term, ev_expression)

        ev_expression2 = pd.EventExpression(
            self.entity1, self.eventType1, self.eventId1
        )
        self.assertEqual(ev_expression2.type, pd.EventExpression.ATOMIC)
        self.assertEqual(ev_expression2.atomic_source, self.entity1)
        self.assertEqual(ev_expression2.atomic_type, self.eventType1)
        self.assertEqual(ev_expression2.atomic_id, self.eventId1)
        self.assertEqual(ev_expression2.first_term, ev_expression2)
        self.assertEqual(ev_expression2.second_term, ev_expression2)
        self.assertNotEqual(ev_expression, ev_expression2)

        ev_expression3 = ev_expression2.copy()
        self.assertEqual(ev_expression2, ev_expression3)
        self.assertIsNot(ev_expression2, ev_expression3)
        self.assertEqual(hash(ev_expression3), hash(ev_expression2))

        ev_expression4 = pd.EventExpression(
            self.entity1, self.eventType1, self.eventId1, "TestExpression"
        )
        self.assertEqual(ev_expression4.type, pd.EventExpression.ATOMIC)
        self.assertEqual(ev_expression4.atomic_source, self.entity1)
        self.assertEqual(ev_expression4.atomic_type, self.eventType1)
        self.assertEqual(ev_expression4.atomic_id, self.eventId1)
        self.assertEqual(ev_expression4.first_term, ev_expression4)
        self.assertEqual(ev_expression4.second_term, ev_expression4)
        self.assertNotEqual(ev_expression, ev_expression4)
        self.assertEqual(
            ev_expression2, ev_expression4
        )  # Descriptor is not important for the equallity
        self.assertEqual(ev_expression4.descriptor, "TestExpression")

    def test_creation_by_operators(self):
        """Test creation of complex EventExpressions with & and | operators"""
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression(descriptor="namedExpression")
        ev_expression_and_combined = ev_expression & ev_expression2
        ev_expression_or_combined = ev_expression | ev_expression2
        self.assertEqual(ev_expression_and_combined.type, pd.EventExpression.AND)
        self.assertEqual(ev_expression_and_combined.first_term, ev_expression)
        self.assertEqual(ev_expression_and_combined.second_term, ev_expression2)
        self.assertEqual(
            ev_expression_and_combined.second_term.descriptor, "namedExpression"
        )
        self.assertEqual(ev_expression_or_combined.type, pd.EventExpression.OR)
        self.assertEqual(ev_expression_or_combined.first_term, ev_expression)
        self.assertEqual(ev_expression_or_combined.second_term, ev_expression2)
        self.assertEqual(
            ev_expression_or_combined.second_term.descriptor, "namedExpression"
        )
        # Test AND/OR with None copies original expression
        # FIXME: This used to work, now not anymore (py3.9 plus) dont know why. check.
        # for newexpr in [
        #         ev_expression & None,
        #         None & ev_expression,
        #         ev_expression | None,
        #         None | ev_expression]:
        #     self.assertIsInstance(newexpr, pd.EventExpression)
        #     # Different object:
        #     self.assertTrue(newexpr is not ev_expression)
        #     # But equal comparison:
        #     self.assertEqual(newexpr, ev_expression)

        # We can create expressions using events
        and_expression_with_events = ev_expression & self.testEvent
        self.assertIsInstance(and_expression_with_events, pd.EventExpression)
        self.assertEqual(and_expression_with_events.type, pd.EventExpression.AND)
        self.assertEqual(and_expression_with_events.first_term, ev_expression)
        self.assertEqual(and_expression_with_events.second_term, self.testEvent)

        or_expression_with_all_events = self.testEvent & self.testEvent2
        self.assertIsInstance(or_expression_with_all_events, pd.EventExpression)
        self.assertEqual(or_expression_with_all_events.type, pd.EventExpression.AND)
        self.assertEqual(or_expression_with_all_events.first_term, self.testEvent)
        self.assertEqual(or_expression_with_all_events.second_term, self.testEvent2)

        or_expression_with_events = ev_expression | self.testEvent
        self.assertIsInstance(or_expression_with_events, pd.EventExpression)
        self.assertEqual(or_expression_with_events.type, pd.EventExpression.OR)
        self.assertEqual(or_expression_with_events.first_term, ev_expression)
        self.assertEqual(or_expression_with_events.second_term, self.testEvent)

        or_expression_with_all_events = self.testEvent | self.testEvent2
        self.assertIsInstance(or_expression_with_all_events, pd.EventExpression)
        self.assertEqual(or_expression_with_all_events.type, pd.EventExpression.OR)
        self.assertEqual(or_expression_with_all_events.first_term, self.testEvent)
        self.assertEqual(or_expression_with_all_events.second_term, self.testEvent2)

        # And other possible combinations
        self.assertIsInstance(self.testEvent & ev_expression, pd.EventExpression)
        self.assertIsInstance(self.testEvent | ev_expression, pd.EventExpression)
        self.assertIsInstance(
            self.testEvent | ev_expression & self.testEvent2, pd.EventExpression
        )
        self.assertIsInstance(
            self.testEvent & ev_expression | self.testEvent2, pd.EventExpression
        )
        self.assertIsInstance(
            self.testEvent & ev_expression & self.testEvent2, pd.EventExpression
        )
        self.assertIsInstance(
            self.testEvent | ev_expression | self.testEvent2, pd.EventExpression
        )
        self.assertIsInstance(
            self.testEvent & self.testEvent | self.testEvent2, pd.EventExpression
        )

    def test_creation_by_mixed_operators(self):
        """Test creation of complex EventExpressions with & and | operators alternated"""
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression(source=self.entity1)
        ev_expression3 = pd.EventExpression(source=self.entity2)
        ev_expression4 = pd.EventExpression(descriptor="namedExpression")
        ev_expression_complex1 = ev_expression & ev_expression2 | ev_expression3
        ev_expression_complex2 = ev_expression | ev_expression2 & ev_expression3
        self.assertEqual(ev_expression_complex1.type, pd.EventExpression.OR)
        self.assertEqual(ev_expression_complex2.type, pd.EventExpression.OR)
        self.assertNotEqual(ev_expression_complex1, ev_expression_complex2)
        # Atomic properties should all be None
        self.assertEqual(ev_expression_complex1.atomic_source, None)
        self.assertEqual(ev_expression_complex1.atomic_type, None)
        self.assertEqual(ev_expression_complex1.atomic_id, None)
        ev_expression_complex3 = (ev_expression | ev_expression2) & ev_expression3
        self.assertEqual(ev_expression_complex3.type, pd.EventExpression.AND)
        ev_expression_complex4 = (ev_expression | ev_expression2) & (
            ev_expression3 | ev_expression4
        )
        self.assertEqual(ev_expression_complex4.type, pd.EventExpression.AND)
        self.assertEqual(ev_expression_complex4.first_term.first_term, ev_expression)
        self.assertEqual(ev_expression_complex4.first_term.second_term, ev_expression2)
        self.assertEqual(ev_expression_complex4.second_term.first_term, ev_expression3)
        # Note descriptor does not matter for equality
        self.assertEqual(ev_expression_complex4.second_term.second_term, ev_expression)
        self.assertEqual(
            ev_expression_complex4.second_term.second_term.descriptor, "namedExpression"
        )

    def test_comparison_of_complex_event_expressions(self):
        """Test comparison between complex event expressions"""
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression(source=self.entity1)
        ev_expression3 = pd.EventExpression(source=self.entity2)
        ev_expression4 = pd.EventExpression(descriptor="namedExpression")
        ev_expression_complex1 = ev_expression & ev_expression2 | ev_expression3
        ev_expression_complex2 = ev_expression | ev_expression2 & ev_expression3
        ev_expression_complex3 = (ev_expression | ev_expression2) & ev_expression3
        ev_expression_complex4 = (ev_expression | ev_expression2) & (
            ev_expression3 | ev_expression4
        )
        ev_expression_complex5 = ev_expression_complex4.copy()
        # Comparing complex event expressions
        self.assertEqual(ev_expression_complex1, ev_expression_complex1)
        self.assertEqual(ev_expression_complex2, ev_expression_complex2)
        self.assertEqual(ev_expression_complex3, ev_expression_complex3)
        self.assertEqual(ev_expression_complex4, ev_expression_complex4)
        self.assertNotEqual(ev_expression_complex1, ev_expression_complex2)
        self.assertNotEqual(ev_expression_complex1, ev_expression_complex3)
        self.assertNotEqual(ev_expression_complex1, ev_expression_complex4)
        self.assertNotEqual(ev_expression_complex2, ev_expression_complex3)
        self.assertNotEqual(ev_expression_complex2, ev_expression_complex4)
        self.assertNotEqual(ev_expression_complex3, ev_expression_complex4)
        self.assertIsNot(ev_expression_complex4, ev_expression_complex5)
        self.assertEqual(ev_expression_complex4, ev_expression_complex5)
        self.assertEqual(hash(ev_expression_complex4), hash(ev_expression_complex5))

    def test_retrieval_of_atoms(self):
        """Test retrieval of atoms from complex event expressions"""
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression(source=self.entity1)
        ev_expression3 = pd.EventExpression(source=self.entity2)
        ev_expression4 = pd.EventExpression(descriptor="namedExpression")
        ev_expression_complex1 = ev_expression & ev_expression2 | ev_expression3
        ev_expression_complex2 = ev_expression | ev_expression2 & ev_expression3
        ev_expression_complex3 = (ev_expression | ev_expression2) & ev_expression3
        ev_expression_complex4 = (ev_expression | ev_expression2) & (
            ev_expression3 | ev_expression4
        )
        ev_expression_complex5 = ev_expression_complex4.copy()

        atoms_ev_expression = ev_expression.atoms
        atoms_ev_expression_complex1 = ev_expression_complex1.atoms
        atoms_ev_expression_complex2 = ev_expression_complex2.atoms
        atoms_ev_expression_complex3 = ev_expression_complex3.atoms
        atoms_ev_expression_complex4 = ev_expression_complex4.atoms
        atoms_ev_expression_complex5 = ev_expression_complex5.atoms

        self.assertIn(ev_expression, atoms_ev_expression)
        self.assertIn(ev_expression, atoms_ev_expression_complex1)
        self.assertIn(ev_expression2, atoms_ev_expression_complex1)
        self.assertIn(ev_expression3, atoms_ev_expression_complex1)
        self.assertIn(ev_expression, atoms_ev_expression_complex2)
        self.assertIn(ev_expression2, atoms_ev_expression_complex2)
        self.assertIn(ev_expression3, atoms_ev_expression_complex2)
        self.assertIn(ev_expression, atoms_ev_expression_complex3)
        self.assertIn(ev_expression2, atoms_ev_expression_complex3)
        self.assertIn(ev_expression3, atoms_ev_expression_complex3)
        self.assertIn(ev_expression, atoms_ev_expression_complex4)
        self.assertIn(ev_expression2, atoms_ev_expression_complex4)
        self.assertIn(ev_expression3, atoms_ev_expression_complex4)
        self.assertIn(ev_expression4, atoms_ev_expression_complex4)
        self.assertIn(ev_expression, atoms_ev_expression_complex5)
        self.assertIn(ev_expression2, atoms_ev_expression_complex5)
        self.assertIn(ev_expression3, atoms_ev_expression_complex5)
        self.assertIn(ev_expression4, atoms_ev_expression_complex5)

        self.assertEqual(len(atoms_ev_expression), 1)
        self.assertEqual(len(atoms_ev_expression_complex1), 3)
        self.assertEqual(len(atoms_ev_expression_complex2), 3)
        self.assertEqual(len(atoms_ev_expression_complex3), 3)
        self.assertEqual(len(atoms_ev_expression_complex4), 4)
        self.assertEqual(len(atoms_ev_expression_complex5), 4)

    def test_simple_activation(self):
        """Test basic activations of event expressions during simulation"""
        entity1 = TestEntity().start()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        first_event = entity1.scheduled_events[0]
        self.assertEqual(len(entity1.event_reports), 0)
        self.assertTrue(entity1.handler_token)
        self.simengine.run(11)
        self.assertEqual(len(entity1.event_reports), 1)
        handler_report = entity1.event_reports[0]
        self.assertTrue(isinstance(handler_report, pd.HandlerReport))
        self.assertEqual(handler_report.event_expression, ev_expression)
        self.assertEqual(handler_report.handler_token, entity1.handler_token)
        self.assertEqual(
            handler_report.handler_priority, pd.EventHandler.default_priority
        )
        self.assertFalse(handler_report.once)  # entity1 uses _wait, not _waitOnce
        self.assertTrue(handler_report.state_report)
        # Inspection of state report
        event_report = handler_report.state_report
        self.assertEqual(len(event_report), 1)
        self.assertTrue(isinstance(event_report[0], pd.EventReport))
        self.assertEqual(event_report[0].atom_ee, ev_expression)
        self.assertTrue(event_report[0].istriggered)
        self.assertEqual(event_report[0].trigger_ee, first_event)
        self.assertEqual(event_report[0].trigger_time, 10)

        # Should not activate, as expected source is entity2
        ev_expression1 = pd.EventExpression(source=self.entity2)
        DUMMY_TYPE = pd.EventType("dummy_type", "A dummy event type")
        entity1.event_reports.clear()
        entity1._wait_once(
            entity1.test_handler, expression=ev_expression1
        )  # wait on entity2
        entity1._schedule_after(4.0, DUMMY_TYPE)  # but entity1 is the source
        self.assertEqual(self.simengine.current_time, 11)
        self.assertEqual(len(entity1.event_reports), 0)
        self.simengine.run(end_time=16)
        self.assertEqual(self.simengine.current_time, 16)  # nothing activated
        self.assertEqual(len(entity1.event_reports), 0)

    def test_composite_event_expression(self):
        """Test composite event expression has correct istriggered events."""
        N = 5
        evtypes = [pd.EventType("Event{}".format(i), "") for i in range(N)]
        evexprs = [pd.EventExpression(event_type=evtype) for evtype in evtypes]
        comp_evexpr = (evexprs[0] & (evexprs[1] & evexprs[2])) & (
            evexprs[3] & evexprs[4]
        )
        ee_report = None

        def callback(handler_report):
            nonlocal ee_report
            ee_report = handler_report

        self.assertIsInstance(comp_evexpr, pd.EventExpression)
        entity = pd.Entity()
        entity._wait_once(pd.EventHandler(callback), expression=comp_evexpr)
        evexprs = [
            entity._schedule_at(i + 1, evtype) for i, evtype in enumerate(evtypes)
        ]
        self.simengine.run(9)
        trigger_times = [
            event_report.trigger_time for event_report in ee_report.state_report
        ]
        self.assertAlmostEqual(max(trigger_times), N)
        self.assertAlmostEqual(min(trigger_times), 1)
        self.assertListEqual(trigger_times, [1, 2, 3, 4, 5])
        self.assertTrue(isinstance(ee_report, pd.HandlerReport))
        self.assertEqual(len(ee_report.state_report), N)
        trigger_evexprs = [
            event_report.trigger_ee for event_report in ee_report.state_report
        ]
        self.assertEqual(trigger_evexprs, evexprs)

    def test_simple_callback(self):
        """Test callbacks with simple activation during simulation."""

        def callback1(handler_report):
            self.assertEqual(len(handler_report.state_report), 1)
            trigger_times = [
                event_report.trigger_time
                for event_report in handler_report.state_report
            ]
            self.assertAlmostEqual(max(trigger_times), self.simengine.current_time)

        # Create event expression that will trigger on any event
        entity1 = TestEntity().start()
        entity2 = pd.Entity()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        entity2._wait_once(pd.EventHandler(callback1), expression=ev_expression)
        self.simengine.run(11)

        # Test self looping of callback:
        counter = []

        def callback2(handler_report):
            nonlocal counter
            counter.append(self.simengine.current_time)
            self.assertEqual(len(handler_report.state_report), 1)
            trigger_times = [
                event_report.trigger_time
                for event_report in handler_report.state_report
            ]
            self.assertAlmostEqual(max(trigger_times), self.simengine.current_time)
            entity2._wait_once(handler, expression=handler_report.event_expression)
            self.assertEqual(handler_report.event_expression, ev_expression)
            self.assertEqual(
                handler_report.handler_priority, pd.EventHandler.default_priority
            )
            self.assertTrue(handler_report.once)

        handler = pd.EventHandler(callback2)
        entity2._wait_once(handler, expression=ev_expression)
        self.simengine.run(61)
        # Expect false because of continuous reset:
        self.assertEqual(len(counter), 5)
        self.assertTrue(
            all([math.isclose(a, b) for a, b in zip(counter, [20, 30, 40, 50, 60])])
        )

    def test_continuous_wait(self):
        """Test continuous wait with callbacks."""
        # Test single call of callback
        counter = []
        # Create event expression that will trigger on any event
        entity1 = TestEntity().start()
        entity2 = pd.Entity()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)

        def callback1(handler_report):
            nonlocal counter
            counter.append(self.simengine.current_time)
            self.assertEqual(len(handler_report.state_report), 1)
            trigger_times = [
                event_report.trigger_time
                for event_report in handler_report.state_report
            ]
            self.assertAlmostEqual(max(trigger_times), self.simengine.current_time)
            self.assertEqual(handler_report.event_expression, ev_expression)
            self.assertEqual(
                handler_report.handler_priority, pd.EventHandler.default_priority
            )
            self.assertFalse(handler_report.once)

        entity2._wait(pd.EventHandler(callback1), expression=ev_expression)
        self.simengine.run(41)
        self.assertEqual(len(counter), 4)
        self.assertTrue(
            all([math.isclose(counter[i], 10 * (i + 1)) for i in range(len(counter))])
        )

    def test_operator_activation(self):
        """Test operator activation of event expressions during simulation."""
        entity1 = TestEntity()
        entity2 = TestEntity()
        entity3 = pd.Entity()
        ev_expression1 = pd.EventExpression(source=entity1)
        ev_expression2 = pd.EventExpression(source=entity2)
        ev_expression_and_combined = ev_expression1 & ev_expression2
        ev_expression_or_combined = ev_expression1 | ev_expression2

        count = 0

        def callback1(handler_report):
            nonlocal count
            count = count + 1
            if handler_report.event_expression == ev_expression1:
                self.assertEqual(len(handler_report.state_report), 1)
                self.assertEqual(handler_report.state_report[0].atom_ee, ev_expression1)
                self.assertTrue(handler_report.state_report[0].istriggered)
                self.assertEqual(
                    handler_report.handler_priority, pd.EventHandler.default_priority
                )
                self.assertTrue(handler_report.once)
            elif handler_report.event_expression == ev_expression_or_combined:
                self.assertEqual(len(handler_report.state_report), 2)
                self.assertEqual(
                    handler_report.event_expression.type, pd.EventExpression.OR
                )
                self.assertEqual(handler_report.state_report[0].atom_ee, ev_expression1)
                self.assertEqual(handler_report.state_report[1].atom_ee, ev_expression2)
                self.assertTrue(handler_report.state_report[0].istriggered)
                self.assertFalse(handler_report.state_report[1].istriggered)
                self.assertFalse(handler_report.state_report[1].trigger_ee)
                self.assertTrue(handler_report.once)
                self.assertEqual(
                    handler_report.handler_priority, pd.EventHandler.default_priority
                )
            elif handler_report.event_expression == ev_expression_and_combined:
                self.assertEqual(len(handler_report.state_report), 2)
                self.assertEqual(
                    handler_report.event_expression.type, pd.EventExpression.AND
                )
                self.assertEqual(handler_report.state_report[0].atom_ee, ev_expression1)
                self.assertEqual(handler_report.state_report[1].atom_ee, ev_expression2)
                self.assertTrue(handler_report.state_report[0].istriggered)
                self.assertTrue(handler_report.state_report[1].istriggered)
                self.assertTrue(handler_report.state_report[1].trigger_ee)
                self.assertTrue(handler_report.once)
                self.assertEqual(
                    handler_report.handler_priority, pd.EventHandler.default_priority
                )

        # Test OR
        entity3._wait_once(pd.EventHandler(callback1), expression=ev_expression1)
        entity3._wait_once(pd.EventHandler(callback1), expression=ev_expression2)
        entity3._wait_once(
            pd.EventHandler(callback1), expression=ev_expression_and_combined
        )
        entity3._wait_once(
            pd.EventHandler(callback1), expression=ev_expression_or_combined
        )
        entity1.start()  # Start only entity 1
        self.simengine.run(11)
        self.assertEqual(
            count, 2
        )  # Invoked by ev_expression1 and by ev_expression_or_combined
        entity2.start()
        self.simengine.run(
            25
        )  # entity 2 will trigger 2 the ev_expression2 and the and_combined
        self.assertEqual(count, 4)


if __name__ == "__main__":
    unittest.main(verbosity=2)
