# cython: language_level=3str

# Delay channel example with PyDynAA and model implemented in Cython
#
# NOTE: you need to add 'path/to/pyDynAA' to PYTHONPATH or install pydynaa

import random
from pydynaa.core import Entity
from pydynaa.core cimport Entity
from pydynaa.core import EventType
from pydynaa.core cimport EventType
from pydynaa.core import EventExpression
from pydynaa.core cimport EventExpression
from pydynaa.core import EventHandler
from pydynaa.core cimport EventHandler
from pydynaa.core import HandlerToken
from pydynaa.core cimport HandlerToken
from pydynaa.core import HandlerReport
from pydynaa.core cimport HandlerReport
from pydynaa.core import SimulationEngine
from pydynaa.core cimport SimulationEngine


cdef class FilterProtocol(Entity):

    cdef EventType FILTER
    cdef double mean_delay

    def __init__(self, double mean_delay):
        self.mean_delay = mean_delay
        self.FILTER = EventType("FILTER_EVENT", "Activates the filter protocol")

    def put(self, int qubits):

        filter_handler = EventHandler(lambda HandlerReport evreport: self.filter_func(qubits, evreport))
        cdef double delay = self.mean_delay
        self._schedule_after(delay, self.FILTER)
        self._wait_once(filter_handler, entity=self, event_type=self.FILTER)

    def filter_func(self, int qubits, HandlerReport evreport):
        cdef double time = SimulationEngine().current_time
        #cdef long id = evreport.state_report[0].trigger_event.id
        cdef long id = evreport.state_report[0].atom_ee.atomic_id
        #print("Filtering qubits = {} at {} for event = {}".format(qubits, time, id))


cdef class Sender(Entity):

    cdef EventType FIRE_EVENT
    cdef double fidelity
    cdef double mean_delay
    cdef FilterProtocol protocol
    cdef EventHandler fire_handler
    cdef HandlerToken handler_token

    def __init__(self, double fidelity, double mean_delay, FilterProtocol protocol):
        self.fidelity = fidelity
        self.mean_delay = mean_delay 
        self.protocol = protocol
        self.FIRE_EVENT = EventType("Sender_FIRES", "Sender fired")
        self.fire_handler = EventHandler(lambda HandlerReport evreport: self.fire(evreport))
        self.handler_token = None

    def start(self):
        cdef double delay = self.mean_delay
        self._schedule_after(delay, self.FIRE_EVENT)
        self.handler_token = self._wait(self.fire_handler, entity=self, event_type=self.FIRE_EVENT)

    def stop(self):
        if self.handler_token is not None:
            self._dismiss(self.handler_token)

    def fire(self, HandlerReport evreport):
        cdef int psi_in = 0
        if random.random() <= self.fidelity:
            psi_in = 1
        cdef int qubits = psi_in  # create qubits based on the state
        self.protocol.put(qubits)
        cdef double time = SimulationEngine().current_time
        #print("Fire {} at {}".format(psi_in, time))
        cdef double delay = self.mean_delay
        self._schedule_after(delay, self.FIRE_EVENT)


def run_test():
    random.seed(0)
    # Setup
    protocol = FilterProtocol(3)
    alice = Sender(0.5, 10., protocol)
    alice.start()

    # Execute
    #print("===== Starting simulation")
    SimulationEngine().run(100)
    alice.stop()
    SimulationEngine().run(150)
    #print("Stopped at {}".format(SimulationEngine().current_time))
