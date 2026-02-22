# cython: language_level=3str

# Delay channel example with PyDynAA and model implemented in Cython
#
# NOTE: you need to add 'path/to/pyDynAA' to PYTHONPATH or install pydynaa

from pydynaa.core import Event
from pydynaa.core cimport Event

from pydynaa.core import Entity
from pydynaa.core cimport Entity

from pydynaa.core import EventType
from pydynaa.core cimport EventType

from pydynaa.core import EventHandler
from pydynaa.core cimport EventHandler

from pydynaa.core import SimulationEngine as DynAASim
from pydynaa.core cimport SimulationEngine as DynAASim

import numpy as np
cimport numpy as np


cdef class FilterProtocol(Entity):

    cdef EventType FILTER
    cdef double mean_delay

    def __init__(self, double mean_delay):
        self.mean_delay = mean_delay
        self.FILTER = EventType("FILTER_EVENT", "Activates the filter protocol")

    def put(self, int qubits):

        filter_handler = EventHandler(lambda Event event: self.filter_func(qubits, event))
        cdef double delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FILTER)
        self._wait_once(filter_handler, entity=self, event_type=self.FILTER)

    def filter_func(self, int qubits, Event event):
        cdef double time = DynAASim().current_time
        print("Filtering qubits = {} at {} for event = {}".format(qubits, time, event.id))


cdef class Sender(Entity):

    cdef EventType FIRE_EVENT
    cdef double fidelity
    cdef double mean_delay
    cdef FilterProtocol protocol
    cdef EventHandler fire_handler

    def __init__(self, double fidelity, double mean_delay, FilterProtocol protocol):
        self.fidelity = fidelity
        self.mean_delay = mean_delay 
        self.protocol = protocol
        self.FIRE_EVENT = EventType("Sender_FIRES", "Sender fired")
        self.fire_handler = EventHandler(lambda Event event: self.fire(event))
        self.handler_token = None

    def start(self):
        cdef double delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FIRE_EVENT)
        self.handler_token = self._wait(self.fire_handler, entity=self, event_type=self.FIRE_EVENT)

    def stop(self):
        if self.handler_token is not None:
            self._dismiss(self.handler_token)

    def fire(self, Event event):
        cdef int psi_in = 0
        if np.random.random() <= self.fidelity:
            psi_in = 1
        cdef int qubits = psi_in  # create qubits based on the state
        self.protocol.put(qubits)
        cdef double time = DynAASim().current_time
        print("Fire {} at {}".format(psi_in, time))
        cdef double delay = np.random.exponential(self.mean_delay)
        self._schedule_after(delay, self.FIRE_EVENT)


def run_test():
    np.random.seed(0)
    # Setup
    protocol = FilterProtocol(3)
    alice = Sender(0.5, 10., protocol)
    alice.start()

    # Execute
    print("===== Starting simulation")
    DynAASim().run(1000000)
    alice.stop()
    DynAASim().run(1500000)
    print("Stopped at {}".format(DynAASim().current_time))
