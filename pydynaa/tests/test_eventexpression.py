import unittest
import math
from itertools import chain
import pydynaa as pd


class TestEntity(pd.Entity):
    TEST_TYPE = pd.EventType("test_type", "An event type for TestEntity")

    def __init__(self, callback=None):
        #super().__init__()  # no longer necessary!
        self.counter = 0
        if callback is None:
            callback = self._callback
        self.test_handler = pd.EventHandler(lambda event: callback(event))
        self.scheduled_events = []
        self.triggered_events = []

    def _callback(self, event):
        self.counter += 1
        self.triggered_events.append(event)
        self.scheduled_events.append(self._schedule_after(10., self.TEST_TYPE))

    def start(self):
        self.scheduled_events.append(self._schedule_after(10., self.TEST_TYPE))
        self._wait(self.test_handler, entity=self, event_type=self.TEST_TYPE)
        #return self.scheduled_events[-1]
        return self

    def stop(self):
        self._dismiss(self.test_handler, self, self.TEST_TYPE)

    def schedule_with_vanishing_type(self):
        event_type = pd.EventType('VANISH', 'This type vanishes from user control when object goes out of scope')
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

    def test_creation_of_atomics(self):
        """Test basic creation of Atomic EventExpressions"""
        ev_expression = pd.EventExpression()  # generic event expression
        self.assertEqual(ev_expression.value, False)
        self.assertEqual(ev_expression.type, pd.EventExpression.ATOMIC)
        self.assertEqual(len(ev_expression.triggered_events), 0)
        self.assertEqual(ev_expression.atomic_source, None)
        self.assertEqual(ev_expression.atomic_type, pd.Event.any_type)
        self.assertEqual(ev_expression.atomic_id, pd.Event.any_id)
        self.assertEqual(ev_expression.triggered_time, None)
        ev_expression2 = pd.EventExpression(self.entity1, self.eventType1, self.eventId1)
        self.assertEqual(ev_expression2.value, False)
        self.assertEqual(ev_expression2.type, pd.EventExpression.ATOMIC)
        self.assertEqual(ev_expression2.atomic_source, self.entity1)
        self.assertEqual(ev_expression2.atomic_type, self.eventType1)
        self.assertEqual(ev_expression2.atomic_id, self.eventId1)
        self.assertEqual(ev_expression2.triggered_time, None)
        self.assertNotEqual(ev_expression, ev_expression2)

    def test_creation_by_operators(self):
        """Test creation of complex EventExpressions with & and | operators """
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression()
        ev_expression_and_combined = ev_expression & ev_expression2
        ev_expression_or_combined = ev_expression | ev_expression2
        self.assertEqual(ev_expression_and_combined.value, False)
        self.assertEqual(ev_expression_and_combined.type, pd.EventExpression.AND)
        self.assertEqual(ev_expression_and_combined.first_term, ev_expression)
        self.assertEqual(ev_expression_and_combined.second_term, ev_expression2)
        self.assertEqual(ev_expression_or_combined.value, False)
        self.assertEqual(ev_expression_or_combined.type, pd.EventExpression.OR)
        self.assertEqual(ev_expression_or_combined.first_term, ev_expression)
        self.assertEqual(ev_expression_or_combined.second_term, ev_expression2)
        # Test AND/OR with None copies original expression
        for newexpr in [
                ev_expression & None,
                None & ev_expression,
                ev_expression | None,
                None | ev_expression]:
            self.assertIsInstance(newexpr, pd.EventExpression)
            # Different object:
            self.assertTrue(newexpr is not ev_expression)
            # But equal comparison:
            self.assertEqual(newexpr, ev_expression)

    def test_creation_by_mixed_operators(self):
        """Test creation of complex EventExpressions with & and | operators alternated """
        ev_expression = pd.EventExpression()
        ev_expression2 = pd.EventExpression(source=self.entity1)
        ev_expression3 = pd.EventExpression(source=self.entity2)
        ev_expression4 = pd.EventExpression()
        ev_expression_complex1 = ev_expression & ev_expression2 | ev_expression3
        ev_expression_complex2 = ev_expression | ev_expression2 & ev_expression3
        self.assertEqual(ev_expression_complex1.type, pd.EventExpression.OR)
        self.assertEqual(ev_expression_complex2.type, pd.EventExpression.OR)
        self.assertNotEqual(ev_expression_complex1, ev_expression_complex2)
        # Atomic properties should all be None
        self.assertEqual(ev_expression_complex1.atomic_source, None)
        self.assertEqual(ev_expression_complex1.atomic_type, None)
        self.assertEqual(ev_expression_complex1.atomic_id, None)
        self.assertEqual(ev_expression_complex1.triggered_time, None)
        ev_expression_complex3 = (ev_expression | ev_expression2) & ev_expression3
        self.assertEqual(ev_expression_complex3.type, pd.EventExpression.AND)
        ev_expression_complex4 = (ev_expression | ev_expression2) & (ev_expression3 | ev_expression4)
        self.assertEqual(ev_expression_complex4.type, pd.EventExpression.AND)
        self.assertEqual(ev_expression_complex4.first_term.first_term, ev_expression)
        self.assertEqual(ev_expression_complex4.first_term.second_term, ev_expression2)
        self.assertEqual(ev_expression_complex4.second_term.first_term, ev_expression3)
        self.assertEqual(ev_expression_complex4.second_term.second_term, ev_expression)

    def test_simple_activation(self):
        """Test basic activations of event expressions during simulation"""
        #ev_expression.wait()
        entity1 = TestEntity().start()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        entity1._wait_once(pd.ExpressionHandler(), expression=ev_expression)
        first_event = entity1.scheduled_events[0]
        self.assertEqual(ev_expression.value, False)
        self.assertTrue(not ev_expression)
        self.assertEqual(len(ev_expression.triggered_events), 0)
        self.assertEqual(ev_expression.triggered_time, None)
        self.simengine.run(11)
        self.assertEqual(ev_expression.value, True)
        self.assertTrue(ev_expression)
        self.assertEqual(len(ev_expression.triggered_events), 1)
        self.assertAlmostEqual(ev_expression.triggered_time, 10)
        self.assertTrue(ev_expression.triggered_events[0], first_event)
        # Should not activate, as expected source is entity2
        ev_expression1 = pd.EventExpression(source=self.entity2)
        entity1._wait_once(pd.ExpressionHandler(), expression=ev_expression1)
        self.assertEqual(ev_expression1.value, False)
        self.simengine.run(21)
        self.assertEqual(ev_expression1.value, False)

    def test_triggered_events(self):
        """Test composite event expression has correct triggered events."""
        N = 5
        evtypes = [pd.EventType("Event{}".format(i), "") for i in range(N)]
        evexprs = [pd.EventExpression(event_type=evtype) for evtype in evtypes]
        comp_evexpr = (evexprs[0] & (evexprs[1] & evexprs[2])) & (evexprs[3] & evexprs[4])
        triggered_events = None

        def callback(eventExpr):
            nonlocal triggered_events
            triggered_events = eventExpr.triggered_events

        entity = pd.Entity()
        entity._wait_once(pd.ExpressionHandler(callback), expression=comp_evexpr)
        events = [entity._schedule_at(i + 1, evtype) for i, evtype in enumerate(evtypes)]
        self.simengine.run()
        self.assertAlmostEqual(self.simengine.current_time, N)
        self.assertTrue(isinstance(triggered_events, list))
        self.assertEqual(len(triggered_events), N)
        self.assertEqual(triggered_events, events)

    def test_simple_callback(self):
        """Test callbacks with simple activation during simulation."""
        # Test single call of callback
        counter = []

        def callback1(eventExpr):
            nonlocal counter
            counter.append(self.simengine.current_time)
            self.assertEqual(len(eventExpr.triggered_events), 1)
            self.assertAlmostEqual(eventExpr.triggered_time, self.simengine.current_time)

        # Create event expression that will trigger on any event
        entity1 = TestEntity().start()
        entity2 = pd.Entity()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        entity2._wait_once(pd.ExpressionHandler(callback1), expression=ev_expression)
        self.assertEqual(ev_expression.value, False)
        self.assertTrue(not ev_expression)
        self.simengine.run(11)
        self.assertEqual(ev_expression.value, True)
        self.assertTrue(ev_expression)
        ev_expression.reprime()
        self.assertFalse(ev_expression)
        self.assertEqual(len(counter), 1)
        self.assertTrue(math.isclose(counter[0], 10))
        # Test self looping of callback:
        exprHandler = None
        counter = []

        def callback2(eventExpr):
            nonlocal counter
            counter.append(self.simengine.current_time)
            # reset event expression
            self.assertAlmostEqual(eventExpr.triggered_time, self.simengine.current_time)
            entity2._wait_once(exprHandler, expression=eventExpr)
            self.assertEqual(eventExpr.triggered_time, None)

        exprHandler = pd.ExpressionHandler(callback2)
        entity2._wait_once(exprHandler, expression=ev_expression)
        self.assertEqual(ev_expression.value, False)
        self.simengine.run(41)
        # Expect false because of continuous reset:
        self.assertEqual(ev_expression.value, False)
        self.assertEqual(len(counter), 3)
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(counter, [20, 30, 40])]))

    def test_continuous_wait(self):
        """Test continuous wait with callbacks."""
        # Test single call of callback
        counter = []

        def callback1(eventExpr):
            nonlocal counter
            counter.append(self.simengine.current_time)
            self.assertEqual(len(eventExpr.triggered_events), 1)
            self.assertAlmostEqual(eventExpr.triggered_time, self.simengine.current_time)

        # Create event expression that will trigger on any event
        entity1 = TestEntity().start()
        entity2 = pd.Entity()
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        entity2._wait(pd.ExpressionHandler(callback1), expression=ev_expression)
        self.assertEqual(ev_expression.value, False)
        self.assertTrue(not ev_expression)
        self.assertEqual(ev_expression.triggered_time, None)
        self.simengine.run(41)
        # Value should be False as just reprimed
        self.assertEqual(ev_expression.value, False)
        self.assertEqual(ev_expression.triggered_time, None)
        self.assertEqual(len(counter), 4)
        self.assertTrue(all([math.isclose(counter[i], 10 * (i + 1)) for i in range(len(counter))]))

    def test_simple_callback_with_priority(self):
        """Test callbacks with priority during simulation"""
        entity2 = pd.Entity()
        # Test priority out of range
        with self.assertRaises(RuntimeError):
            entity2._wait_once(pd.ExpressionHandler(priority=(pd.EventHandler.lowest_priority + 1)),
                               expression=pd.EventExpression())
        with self.assertRaises(RuntimeError):
            entity2._wait_once(pd.ExpressionHandler(priority=(pd.EventHandler.highest_priority - 1)),
                               expression=pd.EventExpression())

        def create_callback(index):
            def callback(evExpr, index=index):
                nonlocal results
                results.append(index)
            return callback

        results = []
        entity1 = TestEntity().start()
        # Test default priority; and waiting on the same event expression
        evexpression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        handlers = [pd.ExpressionHandler(create_callback(i), priority=(pd.EventHandler.default_priority - 1 + i))
                    for i in range(3)]
        for handler in handlers:
            entity2._wait(handler, expression=evexpression)
        self.simengine.run(11)
        self.assertEqual(results, list(range(3)))
        for handler in handlers:
            entity2._dismiss(handler, expression=evexpression)
        # Test priorities of a combination of EventExpressions and EventHandlers
        results.clear()
        for i in range(1, 11):
            callback = create_callback(i)
            if i % 2 == 0:
                # These expression handlers will leave scope
                entity2._wait_once(pd.ExpressionHandler(callback, priority=i), expression=evexpression.copy())
            else:
                entity1._wait(pd.EventHandler(callback, priority=i), entity=entity1, event_type=entity1.TEST_TYPE)
        self.simengine.run(21)
        self.assertEqual(results, list(range(1, 11)))
        # Test that EventExpression waits only once, while EventHandlers keep waiting.
        results.clear()
        self.simengine.run(31)
        self.assertEqual(results, [1, 3, 5, 7, 9])
        # Test priorities when mixing up the sequence of adding EventExpression.wait
        results.clear()
        for j, i in enumerate(range(10, 0, -2)):
            entity2._wait_once(pd.ExpressionHandler(create_callback(j), priority=i), expression=evexpression.copy())
        self.simengine.run(41)
        expected_result = list(range(1, 11))
        for value, idx in enumerate(range(10, 0, -2)):
            expected_result[idx - 1] = value
        self.assertEqual(results, expected_result)

    def test_operator_activation(self):
        """Test operator activation of event expressions during simulation."""
        entity1 = TestEntity()
        entity2 = TestEntity()
        entity3 = pd.Entity()
        ev_expression1 = pd.EventExpression(source=entity1)
        ev_expression2 = pd.EventExpression(source=entity2)
        ev_expression_and_combined = ev_expression1 & ev_expression2
        ev_expression_or_combined = ev_expression1 | ev_expression2
        # Test OR
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression1)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression2)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression_and_combined)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression_or_combined)
        entity1.start()  # Start only entity 1
        self.simengine.run(11)
        self.assertTrue(bool(ev_expression1) is True)
        self.assertTrue(bool(ev_expression2) is False)
        self.assertTrue(bool(ev_expression_and_combined) is False)
        self.assertTrue(bool(ev_expression_and_combined.first_term) is True)
        self.assertTrue(bool(ev_expression_and_combined.second_term) is False)
        self.assertTrue(bool(ev_expression_or_combined) is True)
        self.assertTrue(bool(ev_expression_or_combined.first_term) is True)
        self.assertTrue(bool(ev_expression_or_combined.second_term) is False)
        # Test AND
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression1)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression2)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression_and_combined)
        entity3._wait_once(pd.ExpressionHandler(), expression=ev_expression_or_combined)
        self.assertTrue(bool(ev_expression1) is False)
        self.assertTrue(bool(ev_expression2) is False)
        self.assertTrue(bool(ev_expression_and_combined) is False)
        self.assertTrue(bool(ev_expression_or_combined) is False)
        entity2.start()  # Also start entity 2
        self.simengine.run(31)  # By this time Entity2 has also clicked
        self.assertTrue(bool(ev_expression1) is True)
        self.assertTrue(bool(ev_expression2) is True)
        self.assertTrue(bool(ev_expression_and_combined) is True)
        self.assertTrue(bool(ev_expression_and_combined.first_term) is True)
        self.assertTrue(bool(ev_expression_and_combined.second_term) is True)
        self.assertTrue(bool(ev_expression_or_combined) is True)
        #self.assertTrue(bool(ev_expression_or_combined.first_term) is True)
        #self.assertTrue(bool(ev_expression_or_combined.second_term) is True)
        # NOTE the instance the expr triggered only one of atoms is true
        self.assertTrue(bool(ev_expression_or_combined.first_term) != bool(ev_expression_or_combined.second_term))

    def test_operator_callback(self):
        """Test callbacks with operator activation during simulation."""
        evtype_click0 = pd.EventType("Click0", "Click on 0")
        evtype_click1 = pd.EventType("Click1", "Click on 1")
        evexpr_click0 = pd.EventExpression(event_type=evtype_click0)
        evexpr_click1 = pd.EventExpression(event_type=evtype_click1)
        evexpr_anyclick = evexpr_click0 | evexpr_click1
        results = []
        expr_handler = None

        def callback(evExpr):
            nonlocal results
            results.append((pd.SimulationEngine().current_time,
                            evExpr.value, evExpr.first_term.value,
                            evExpr.second_term.value))
            if len(results) < 2:
                # Wait command should reset all the expressions below the complex expression
                self.entity1._wait_once(expr_handler, expression=evExpr)
                # schedule a click of type 1
                self.entity1._schedule_after(1, evtype_click1)

        expr_handler = pd.ExpressionHandler(callback)
        self.entity1._wait_once(expr_handler, expression=evexpr_anyclick)
        self.entity1._schedule_now(evtype_click0)  # schedule a click of type 0
        self.simengine.run()
        self.assertEqual(len(results), 2)
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(results[0], [0., True, True, False])]))
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(results[1], [1., True, False, True])]))

    def test_expression_handler_dismiss(self):
        """Test dismissal of expression handlers (using triggered events)."""
        def create_callback(results):
            def callback(evExpr):
                assert(evExpr.type == evExpr.ATOMIC)
                results.append(evExpr.triggered_events[-1])
            return callback

        # Step 1: Create a set of 3 unique expressions and 2 handlers for each
        entities = [TestEntity().start() for i in range(3)]
        expressions = [pd.EventExpression(source=entity, event_type=entity.TEST_TYPE) for entity in entities]
        waitentity = pd.Entity()
        resultsA = []
        resultsB = []
        handlerA = pd.ExpressionHandler(create_callback(resultsA))
        handlerB = pd.ExpressionHandler(create_callback(resultsB))
        for expression in expressions:
            waitentity._wait(handlerA, expression=expression)
            waitentity._wait(handlerB, expression=expression)
        self.simengine.run(21)
        self.assertEqual(len(set(resultsA)), 6)
        self.assertEqual(len(set(resultsB)), 6)
        triggered_events_step1 = set(chain.from_iterable([entity.triggered_events[:] for entity in entities]))
        self.assertEqual(set(resultsA), triggered_events_step1)
        self.assertEqual(set(resultsB), triggered_events_step1)
        # Step 2: Dismiss handler for one expression
        waitentity._dismiss(handlerB, expression=expressions[0])
        resultsA.clear()
        resultsB.clear()
        self.simengine.run(41)
        self.assertEqual(len(set(resultsA)), 6)
        self.assertEqual(len(set(resultsB)), 4)
        triggered_events_step2A = (
            set(chain.from_iterable([entity.triggered_events[:] for entity in entities[:]])) - triggered_events_step1)
        triggered_events_step2B = (
            set(chain.from_iterable([entity.triggered_events[:] for entity in entities[1:]])) - triggered_events_step1)
        self.assertEqual(set(resultsA), triggered_events_step2A)
        self.assertEqual(set(resultsB), triggered_events_step2B)
        # Step 3: Dismiss handlerA for all expression, and B for another expression
        waitentity._dismiss(handlerA)
        waitentity._dismiss(handlerB, expression=expressions[1])
        resultsA.clear()
        resultsB.clear()
        self.simengine.run(61)
        self.assertEqual(len(set(resultsA)), 0)
        self.assertEqual(len(set(resultsB)), 2)
        triggered_events_step3B = (
            set(chain.from_iterable([entity.triggered_events[:] for entity in entities[2:]])) - triggered_events_step2B - triggered_events_step1)
        self.assertEqual(set(resultsB), triggered_events_step3B)


if __name__ == "__main__":
    unittest.main(verbosity=2)
