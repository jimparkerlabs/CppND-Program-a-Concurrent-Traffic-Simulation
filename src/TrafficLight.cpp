#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : <DONE> The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock);
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return std::move(msg);
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : <DONE> The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : <DONE> add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));  // you didn't say to, but you usually do...
        if (_messageQueue.receive() == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : <DONE> Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : <DONE> Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4000, 6000);
    int cycleDuration = distr(eng);  // why isn't this a member variable set in the constructor?

    auto lastUpdate = std::chrono::system_clock::now();

    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // get current time and compare to lastUpdate
        auto currentTime = std::chrono::system_clock::now();
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration) {
            std::unique_lock<std::mutex> lock(_mutex);

            // wait for the condition  notification
            _condition.wait(lock);

            // toggle the TrafficLightPhase
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ? TrafficLightPhase::green : TrafficLightPhase::red;

            // send an update to the message queue
            _messageQueue.send(std::move(_currentPhase));  // TODO:  MOVE a member variable ???  Think hard about this...

            lastUpdate = currentTime;  // TODO:  I might have been waiting, so maybe this should get the new current time?
            lock.unlock(); // unnecessary, but I like to be explicit
        }
    }
}

