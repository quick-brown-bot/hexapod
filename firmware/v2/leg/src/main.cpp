#include <Arduino.h>
#include <Servo.h>

// XIAO RP2040 onboard RGB — active LOW
static constexpr int LED_R = 17;
static constexpr int LED_G = 16;
static constexpr int LED_B = 25;

// PWM pins from schematic
static constexpr int TIBIA_PWM_PIN  = 2;   // P2/D8

// ADC pins from schematic: A0–A3 = GPIO26–29
static constexpr int I_TOTAL_PIN = 26;  // A0 — total leg current
static constexpr int I_FEMUR_PIN = 27;  // A1 — femur branch
static constexpr int I_TIBIA_PIN = 28;  // A2 — tibia branch
static constexpr int I_COXA_PIN  = 29;  // A3 — coxa branch

// INA4181A3IPWR gain = 100, shunt = 10mΩ → 1A = 1000mV output
// ADC ref = 3.3V, 12-bit (4096 counts)
static constexpr float ADC_TO_V   = 3.3f / 4095.0f;
static constexpr float V_TO_A     = 1.0f / (100.0f * 0.010f);  // 1/(gain * shunt)

Servo tibia;

void printCurrents();

void setup() {
    Serial.begin(115200);

    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);

    analogReadResolution(12);

    tibia.attach(TIBIA_PWM_PIN, 500, 2500);
    tibia.write(90);
}

void loop() {
    // sweep 0 → 180
    digitalWrite(LED_R, LOW);
    for (int pos = 0; pos <= 180; pos++) {
        tibia.write(pos);
        delay(15);
        printCurrents();
    }
    digitalWrite(LED_R, HIGH);
    delay(500);

    // sweep 180 → 0
    digitalWrite(LED_B, LOW);
    for (int pos = 180; pos >= 0; pos--) {
        tibia.write(pos);
        delay(15);
        printCurrents();
    }
    digitalWrite(LED_B, HIGH);
    delay(500);
}

void printCurrents() {
    float total = analogRead(I_TOTAL_PIN) * ADC_TO_V * V_TO_A;
    float coxa  = analogRead(I_COXA_PIN)  * ADC_TO_V * V_TO_A;
    float femur = analogRead(I_FEMUR_PIN) * ADC_TO_V * V_TO_A;
    float tibia = analogRead(I_TIBIA_PIN) * ADC_TO_V * V_TO_A;

    Serial.printf("total=%.3fA  coxa=%.3fA  femur=%.3fA  tibia=%.3fA\n",
                  total, coxa, femur, tibia);
}
