/**
 *  Delay channel example
 */

#include <iostream>
#include <functional>
#include <memory>
#include <random>
#include "../../cdynaa/include/entity.h"
#include "../../cdynaa/include/handlerreport.h"
#include "../../cdynaa/include/eventtype.h"

using namespace dynaa;

// Inspired by FilterProtocol in the Python files.
class FilterProtocol : public Entity {
public:
    static EventType FILTER;
private:
    std::default_random_engine generator;
    std::poisson_distribution<> poisson;

public:
    explicit FilterProtocol(double mean_delay):
        generator(2460093892354),
        poisson(mean_delay) {};

    void put(int qubits) {
        function<void(shared_ptr<FilterProtocol>, HandlerReport)> func = bind(&FilterProtocol::filter, std::placeholders::_1, qubits, std::placeholders::_2);
        shared_ptr<EventHandler> filterHandler = EventHandler::create(bind(&FilterProtocol::filter, *this, qubits, std::placeholders::_1), {shared_from_this()});
        scheduleAfter(poisson(generator), FilterProtocol::FILTER);
        waitOnce(filterHandler, shared_from_this(), FilterProtocol::FILTER);
    }

    void filter(int qubits, const HandlerReport& report) {
        auto event = report.getEventStateReport().begin()->last_trigger; 
        cout << "Filtering qubits = "<< qubits << " at " << DynAASim::getInstance().getCurrentTime() << " for event = " << event.atomicEventID() << endl;
    }
};

EventType FilterProtocol::FILTER("FILTER_EVENT", "Activates the filter protocol");


/** Sender
 *
 *  Inspired by this idea

 >>>
 def sender(env, protocol, psi, phi, f, delay=1):
    """
    Sender process that randomly generates input ket states

    'f' is the probability for the desired entangled state, otherwise
    the noisy state is returned
    """
    while True:
        # randomly select pure state from input density matrix:
        if random.random() <= f:
            psi_in = psi
        else:
            psi_in = phi
        # send state through protocol
        qubits = qc.kettools.create_qubits(psi_in, "A")
        protocol.put(qubits)
        # wait for next transmission
        yield env.timeout(delay)
 <<<
 */

class Sender: public Entity {
    /**
     * Sender process that randomly generates input ket states
     *
     * 'f' is the probability for the desired entangled state, otherwise
     *  the noisy state is returned
     */
public:
    static EventType FIRE_EVENT;

private:
    shared_ptr<EventHandler> fireHandler;
    std::default_random_engine generator;
    std::poisson_distribution<> poisson;
    std::uniform_real_distribution<double> uniform;
    double fidelity;
    shared_ptr<FilterProtocol> protocol;
    HandlerToken token;

public:
    Sender(double fidelity, double mean_delay, shared_ptr<FilterProtocol> protocol):
        fireHandler(EventHandler::create(std::bind(&Sender::fire,this,std::placeholders::_1))),
        generator(234532),
        poisson(mean_delay),
        uniform(0.0,1.0),
        fidelity(fidelity),
        protocol(protocol) {};

    void start() {
        scheduleAfter(poisson(generator), Sender::FIRE_EVENT);
        token = wait(fireHandler, shared_from_this(), Sender::FIRE_EVENT); // Waits for all the following FIRE_EVENT events from this obj.
    }

    void stop() {
        dismiss(token) ; // If you want to stop the sender.  Use start again to resume
    }

    void fire(const HandlerReport&) {
        int psi_in = 0;
        if (uniform(generator) <= fidelity) {
            psi_in = 1;
        }
        int qubits = psi_in;  // Create the qubits based on the state. create_qubits(psi_in)
        protocol->put(qubits);
        cout << "Fire " << psi_in << " at " << DynAASim::getInstance().getCurrentTime() <<  endl;
        scheduleAfter(poisson(generator), Sender::FIRE_EVENT);
    }
};

EventType Sender::FIRE_EVENT("Sender_FIRES", "Sender fired");


int main() {
    //Setup
    shared_ptr<FilterProtocol> protocol = make_shared<FilterProtocol>(3);
    shared_ptr<Sender> alice = make_shared<Sender>(0.5, 10, protocol);
    alice->start();

    // Executing the
    std::cout << "===== Starting simulation" << std::endl;
    DynAASim::getInstance().run(1000000);
    alice->stop();
    DynAASim::getInstance().run(1500000);
    cout << "Stopped at " << DynAASim::getInstance().getCurrentTime() << endl;
    return 0;
}
