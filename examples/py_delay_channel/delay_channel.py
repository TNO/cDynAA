# Delay channel example with PyDynAA
#
# NOTE: you need to add 'path/to/pyDynAA' to PYTHONPATH or install pydynaa

from pydynaa import Entity
from pydynaa import EventType
from pydynaa import EventHandler
from pydynaa import DynAASim
import numpy as np


class FilterProtocol(Entity):

    FILTER = EventType("FILTER_EVENT", "Activates the filter protocol")

    def __init__(self, mean_delay):
        super().__init__()
        self.mean_delay = mean_delay

    def put(self, qubits):
        filter_handler = EventHandler(lambda event: self.filter_func(qubits, event))
        delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FILTER)
        self._wait_once(filter_handler, entity=self, event_type=self.FILTER)

    def filter_func(self, qubits, event):
        time = DynAASim().current_time
        print("Filtering qubits = {} at {} for event = {}".format(qubits, time, event.id))


class Sender(Entity):

    FIRE_EVENT = EventType("Sender_FIRES", "Sender fired")

    def __init__(self, fidelity, mean_delay, protocol):
        super().__init__()
        self.fidelity = fidelity
        self.mean_delay = mean_delay
        self.protocol = protocol
        self.fire_handler = EventHandler(lambda event: self.fire(event))

    def start(self):
        delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FIRE_EVENT)
        self._wait(self.fire_handler, entity=self, event_type=self.FIRE_EVENT)

    def stop(self):
        self._dismiss(self.fire_handler, self, self.FIRE_EVENT)

    def fire(self, event):
        psi_in = 0
        if np.random.random() <= self.fidelity:
            psi_in = 1
        qubits = psi_in  # create qubits based on the state
        self.protocol.put(qubits)
        time = DynAASim().current_time
        print("Fire {} at {}".format(psi_in, time))
        delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FIRE_EVENT)


# Setup
np.random.seed(0)
protocol = FilterProtocol(3)
alice = Sender(0.5, 10., protocol)
alice.start()

# Execute
print("===== Starting simulation")
DynAASim().run(1000000)
alice.stop()
DynAASim().run(1500000)
print("Stopped at {}".format(DynAASim().current_time))
