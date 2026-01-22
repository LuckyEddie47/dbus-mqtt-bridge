#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mqtt/async_client.h>

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("mqtt_simulator");
const std::string SIGNAL_TOPIC("simulator/signals/notify");
const std::string COMMAND_TOPIC("simulator/commands/echo");

class callback : public virtual mqtt::callback {
public:
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "[MQTT Sim] Message arrived on topic '" << msg->get_topic() << "': " << msg->to_string() << std::endl;
    }

    void connected(const std::string& cause) override {
        std::cout << "[MQTT Sim] Connected: " << cause << std::endl;
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "[MQTT Sim] Connection lost: " << cause << std::endl;
    }
};

int main() {
    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);
    callback cb;
    client.set_callback(cb);

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    try {
        std::cout << "[MQTT Sim] Connecting to broker at " << SERVER_ADDRESS << "..." << std::endl;
        client.connect(connOpts)->wait();
        std::cout << "[MQTT Sim] Connection successful." << std::endl;

        std::cout << "[MQTT Sim] Subscribing to '" << SIGNAL_TOPIC << "'..." << std::endl;
        client.subscribe(SIGNAL_TOPIC, 1)->wait();

        int count = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::string payload = "[\"MQTT Command " + std::to_string(++count) + "\"]";
            std::cout << "[MQTT Sim] Publishing command to '" << COMMAND_TOPIC << "': " << payload << std::endl;
            client.publish(COMMAND_TOPIC, payload, 1, false);
        }

    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT Sim] Error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
