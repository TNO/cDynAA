import unittest
import math
import pydynaa as pd
from pydynaa.tests.test_eventexpression import TestEntity


class TestEventExpressionLegacy(unittest.TestCase):
    """
    Unit tests for legacy event expressions.
    """

    def setUp(self):
        """Initializion for a test fixture"""
        self.eventType1 = pd.EventType("Storm", "Storm event type")
        self.eventType2 = pd.EventType("Rain", "Rain event type")
        self.entity1 = pd.Entity()
        self.entity2 = pd.Entity()
        self.simengine = pd.DynAASim()
        self.simengine.reset()

    def test_EventExpression_simple_activation(self):
        """Test basic activations of event expressions during simulation"""
        ev_expression = pd.EventExpression()
        ev_expression.wait()
        entity1 = TestEntity()
        entity1.start()
        self.assertEqual(ev_expression.value, False)
        self.assertTrue(not ev_expression)
        self.simengine.run(11)
        self.assertEqual(ev_expression.value, True)
        self.assertTrue(ev_expression)
        # Should not activate, as expected source is entity2
        ev_expression1 = pd.EventExpression(source=self.entity2)
        ev_expression1.wait()
        self.assertEqual(ev_expression1.value, False)
        self.simengine.run(21)
        self.assertEqual(ev_expression1.value, False)

    def test_EventExpression_simple_callback(self):
        """Test callbacks with simple activation during simulation."""
        # Test single call of callback
        counter = []

        def callback1():
            nonlocal counter
            counter.append(self.simengine.current_time)

        # Create event expression that will trigger on any event
        ev_expression = pd.EventExpression()
        ev_expression.wait(callback1)
        entity1 = TestEntity()
        entity1.start()
        self.assertEqual(ev_expression.value, False)
        self.assertTrue(not ev_expression)
        self.simengine.run(11)
        self.assertEqual(ev_expression.value, True)
        self.assertTrue(ev_expression)
        self.assertEqual(len(counter), 1)
        self.assertTrue(math.isclose(counter[0], 10))
        # Test self looping of callback:
        counter = []

        def callback2():
            nonlocal counter
            counter.append(self.simengine.current_time)
            # reset event expression
            ev_expression.wait(callback2)

        # TODO calling wait no longer resets the expression
        ev_expression.wait(callback2)
        self.assertEqual(ev_expression.value, False)
        self.simengine.run(41)
        # Expect false because of continuous reset:
        self.assertEqual(ev_expression.value, False)
        self.assertEqual(len(counter), 3)
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(counter, [20, 30, 40])]))

    def test_EventExpression_simple_callback_with_priority(self):
        """Test callbacks with priority during simulation"""
        # Test priority out of range
        with self.assertRaises(RuntimeError):
            pd.EventExpression().wait(callback=None, priority=(pd.EventHandler.lowest_priority + 1))
        with self.assertRaises(RuntimeError):
            pd.EventExpression().wait(callback=None, priority=(pd.EventHandler.highest_priority - 1))

        def create_callback(index):
            def callback(event=None, index=index):
                nonlocal results
                results.append(index)
            return callback

        results = []
        entity1 = TestEntity()
        entity1.start()

        # Test default priority
        ev_expression = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        ev_expression.wait(create_callback(0), priority=pd.EventHandler.default_priority - 1)
        ev_expression1 = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        ev_expression1.wait(create_callback(1))
        ev_expression2 = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
        ev_expression2.wait(create_callback(2), priority=pd.EventHandler.default_priority + 1)
        self.simengine.run(11)
        self.assertEqual(results, list(range(3)))

        # Test priorities of a combination of EventExpressions and EventHandlers
        results.clear()
        evexprs = []  # NOTE needed to prevent garbage collection ..
        for i in range(1, 11):
            callback = create_callback(i)
            if i % 2 == 0:
                evexpr = pd.EventExpression(source=entity1, event_type=entity1.TEST_TYPE)
                # NOTE need to prevent garbage collection of EventExpression !!!
                evexprs.append(evexpr)
                evexpr.wait(callback, priority=i)
            else:
                eh = pd.EventHandler(callback, priority=i)
                entity1._wait(eh, entity=entity1, event_type=entity1.TEST_TYPE)
        self.simengine.run(21)
        self.assertEqual(results, list(range(1, 11)))
        # Test that EventExpression waits only once, while EventHandlers keep waiting.
        results.clear()
        self.simengine.run(31)
        self.assertEqual(results, [1, 3, 5, 7, 9])
        # Test priorities when mixing up the sequence of adding EventExpression.wait
        results.clear()
        for j, i in enumerate(range(10, 0, -2)):
            evexprs[j].wait(create_callback(j), priority=i)
        self.simengine.run(41)
        expected_result = list(range(1, 11))
        for value, idx in enumerate(range(10, 0, -2)):
            expected_result[idx - 1] = value
        self.assertEqual(results, expected_result)

    def test_Event_Expression_operator_activation(self):
        """Test operator activation of event expressions during simulation."""
        entity1 = TestEntity()
        entity2 = TestEntity()
        ev_expression1 = pd.EventExpression(source=entity1)
        ev_expression2 = pd.EventExpression(source=entity2)
        ev_expression_and_combined = ev_expression1 & ev_expression2
        ev_expression_or_combined = ev_expression1 | ev_expression2
        # Test OR
        ev_expression1.wait()
        ev_expression2.wait()
        ev_expression_and_combined.wait()
        ev_expression_or_combined.wait()
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
        ev_expression1.wait()
        ev_expression2.wait()
        ev_expression_and_combined.wait()
        ev_expression_or_combined.wait()
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

    def test_Event_Expression_operator_callback(self):
        """Test callbacks with operator activation during simulation."""
        evtype_click0 = pd.EventType("Click0", "Click on 0")
        evtype_click1 = pd.EventType("Click1", "Click on 1")
        evexpr_click0 = pd.EventExpression(event_type=evtype_click0)
        evexpr_click1 = pd.EventExpression(event_type=evtype_click1)
        evexpr_anyclick = evexpr_click0 | evexpr_click1
        results = []

        def callback():
            nonlocal results
            results.append((pd.SimulationEngine().current_time,
                            evexpr_anyclick.value, evexpr_anyclick.first_term.value,
                            evexpr_anyclick.second_term.value))
            if len(results) < 2:
                # Wait command should reset all the expressions below the complex expression
                evexpr_anyclick.wait(callback)
                # schedule a click of type 1
                self.entity1._schedule_after(1, evtype_click1)

        evexpr_anyclick.wait(callback)
        self.entity1._schedule_now(evtype_click0)  # schedule a click of type 0
        self.simengine.run()
        self.assertEqual(len(results), 2)
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(results[0], [0., True, True, False])]))
        self.assertTrue(all([math.isclose(a, b) for a, b in zip(results[1], [1., True, False, True])]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
