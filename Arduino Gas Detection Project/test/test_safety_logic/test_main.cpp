/**
 * @file test_main.cpp
 * @brief Unit tests for safety logic, filtering, hysteresis, and state machines.
 */

#include <Arduino.h>
#include <unity.h>
#include "Config.h"
#include "SystemTypes.h"
#include "interfaces/ISensor.h"
#include "interfaces/IActuator.h"
#include "interfaces/IDisplay.h"
#include "interfaces/INotificationService.h"
#include "controllers/SafetyController.h"

// -----------------------------------------------------------------------------
// Mock Sensor for Testing
// -----------------------------------------------------------------------------
class MockSensor : public ISensor {
public:
    uint16_t raw;
    uint16_t filtered;
    uint16_t threshold;
    uint16_t hysteresis;

    MockSensor() : raw(100), filtered(100), threshold(350), hysteresis(50) {}

    void init() override {}
    void update() override {}

    uint16_t getRawValue() const override { return raw; }
    uint16_t getFilteredValue() const override { return filtered; }

    bool isThresholdExceeded() const override {
        return filtered >= threshold;
    }

    bool isRecoveryLevel() const override {
        return filtered <= (threshold - hysteresis);
    }
};

// -----------------------------------------------------------------------------
// Mock Actuators
// -----------------------------------------------------------------------------
class MockActuator : public IActuator {
public:
    bool state;
    MockActuator() : state(false) {}

    void init() override { state = false; }
    void activate() override { state = true; }
    void deactivate() override { state = false; }
    void update() override {}
    bool isActive() const override { return state; }
};

// -----------------------------------------------------------------------------
// Mock Display
// -----------------------------------------------------------------------------
class MockDisplay : public IDisplay {
public:
    SystemTelemetry lastTelemetry;
    void init() override {}
    void update(const SystemTelemetry& telemetry) override { lastTelemetry = telemetry; }
    void showMessage(const char*, const char*) override {}
    void clear() override {}
};

// -----------------------------------------------------------------------------
// Mock Notification Service
// -----------------------------------------------------------------------------
class MockNotificationService : public INotificationService {
public:
    bool alertSent;
    bool recoverySent;
    GsmNetworkStatus network;

    MockNotificationService() : alertSent(false), recoverySent(false), network(GsmNetworkStatus::REGISTERED_HOME) {}

    void init() override {}
    void update() override {}
    bool sendAlert() override { alertSent = true; return true; }
    bool sendRecovery() override { recoverySent = true; return true; }
    bool isBusy() const override { return false; }
    GsmNetworkStatus getNetworkStatus() const override { return network; }
};

// -----------------------------------------------------------------------------
// Unit Tests
// -----------------------------------------------------------------------------

void test_sensor_threshold_and_hysteresis() {
    MockSensor sensor;
    sensor.threshold = 350;
    sensor.hysteresis = 50;

    // Normal baseline (200)
    sensor.filtered = 200;
    TEST_ASSERT_FALSE(sensor.isThresholdExceeded());
    TEST_ASSERT_TRUE(sensor.isRecoveryLevel());

    // Boundary right below threshold (349)
    sensor.filtered = 349;
    TEST_ASSERT_FALSE(sensor.isThresholdExceeded());
    TEST_ASSERT_FALSE(sensor.isRecoveryLevel()); // In hysteresis deadband

    // At threshold (350) -> triggers alarm
    sensor.filtered = 350;
    TEST_ASSERT_TRUE(sensor.isThresholdExceeded());
    TEST_ASSERT_FALSE(sensor.isRecoveryLevel());

    // Above threshold (500)
    sensor.filtered = 500;
    TEST_ASSERT_TRUE(sensor.isThresholdExceeded());
    TEST_ASSERT_FALSE(sensor.isRecoveryLevel());

    // Dropping into hysteresis band (320) -> not yet recovery level (threshold - 50 = 300)
    sensor.filtered = 320;
    TEST_ASSERT_FALSE(sensor.isThresholdExceeded());
    TEST_ASSERT_FALSE(sensor.isRecoveryLevel());

    // Dropping to recovery level (300) -> recovery triggered
    sensor.filtered = 300;
    TEST_ASSERT_FALSE(sensor.isThresholdExceeded());
    TEST_ASSERT_TRUE(sensor.isRecoveryLevel());
}

void test_relay_and_buzzer_mocks() {
    MockActuator buzzer;
    MockActuator relay;

    buzzer.init();
    relay.init();
    TEST_ASSERT_FALSE(buzzer.isActive());
    TEST_ASSERT_FALSE(relay.isActive());

    relay.activate();
    TEST_ASSERT_TRUE(relay.isActive());

    relay.deactivate();
    TEST_ASSERT_FALSE(relay.isActive());

    buzzer.activate();
    TEST_ASSERT_TRUE(buzzer.isActive());

    buzzer.deactivate();
    TEST_ASSERT_FALSE(buzzer.isActive());
}

void test_safety_controller_initial_state() {
    MockSensor sensor;
    MockActuator buzzer;
    MockActuator relay;
    MockDisplay display;
    MockNotificationService notifier;

    SafetyController controller(sensor, buzzer, relay, display, notifier);
    controller.begin();

    SystemTelemetry telemetry = controller.getTelemetry();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::WARMUP), static_cast<uint8_t>(telemetry.systemState));
    TEST_ASSERT_FALSE(buzzer.isActive());
    TEST_ASSERT_FALSE(relay.isActive());
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_sensor_threshold_and_hysteresis);
    RUN_TEST(test_relay_and_buzzer_mocks);
    RUN_TEST(test_safety_controller_initial_state);
    UNITY_END();
}

void loop() {}
