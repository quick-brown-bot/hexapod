#include <Arduino.h>
#include <Servo.h>

// XIAO RP2040 onboard RGB — active LOW
static constexpr int LED_R = 17;
static constexpr int LED_G = 16;
static constexpr int LED_B = 25;

// PWM pins (GPIO = D8/D9/D10 on XIAO RP2040)
static constexpr int COXA_PWM_PIN  = 3;   // D10/GPIO3
static constexpr int FEMUR_PWM_PIN = 4;   // D9/GPIO4
static constexpr int TIBIA_PWM_PIN = 2;   // D8/GPIO2

// ADC — new board layout: A0=total, A1=femur, A2=tibia, A3=coxa
static constexpr int   I_TOTAL_PIN = 26;
static constexpr int   I_FEMUR_PIN = 27;
static constexpr int   I_TIBIA_PIN = 28;
static constexpr int   I_COXA_PIN  = 29;

// INA4181A3IPWR gain=100, shunt=10mΩ → 1 A/V
static constexpr float ADC_TO_V   = 3.3f / 4095.0f;
static constexpr float V_TO_A     = 1.0f / (100.0f * 0.010f);

// Safety limits
static constexpr float BRANCH_TRIP_A = 3.0f;   // per servo — detach on trip
static constexpr float TOTAL_TRIP_A  = 9.0f;   // whole leg — cut all on trip
static constexpr int   COOLDOWN_MS   = 2000;

static constexpr int HOLD_MS    = 3000;
static constexpr int SAMPLE_MS  = 50;
static constexpr int BASELINE_N = 64;

static float b_total = 0.0f, b_femur = 0.0f, b_tibia = 0.0f, b_coxa = 0.0f;

Servo coxa, femur, tibia;

struct Currents { float total, femur, tibia, coxa; };

Currents readCurrents() {
    return {
        analogRead(I_TOTAL_PIN) * ADC_TO_V * V_TO_A - b_total,
        analogRead(I_FEMUR_PIN) * ADC_TO_V * V_TO_A - b_femur,
        analogRead(I_TIBIA_PIN) * ADC_TO_V * V_TO_A - b_tibia,
        analogRead(I_COXA_PIN)  * ADC_TO_V * V_TO_A - b_coxa,
    };
}

// Returns true if any trip fired. Detaches the offending servo(s).
bool checkSafety(const Currents& c) {
    bool tripped = false;

    if (c.total > TOTAL_TRIP_A) {
        Serial.printf("!TRIP total=%.2fA > %.1fA — cutting all servos\n",
                      c.total, TOTAL_TRIP_A);
        coxa.detach(); femur.detach(); tibia.detach();
        digitalWrite(LED_R, LOW); digitalWrite(LED_B, LOW);
        return true;
    }
    if (c.coxa > BRANCH_TRIP_A) {
        Serial.printf("!TRIP coxa=%.2fA > %.1fA — releasing coxa\n",
                      c.coxa, BRANCH_TRIP_A);
        coxa.detach();
        tripped = true;
    }
    if (c.femur > BRANCH_TRIP_A) {
        Serial.printf("!TRIP femur=%.2fA > %.1fA — releasing femur\n",
                      c.femur, BRANCH_TRIP_A);
        femur.detach();
        tripped = true;
    }
    if (c.tibia > BRANCH_TRIP_A) {
        Serial.printf("!TRIP tibia=%.2fA > %.1fA — releasing tibia\n",
                      c.tibia, BRANCH_TRIP_A);
        tibia.detach();
        tripped = true;
    }
    return tripped;
}

void reattach() {
    delay(COOLDOWN_MS);
    coxa.attach(COXA_PWM_PIN,  500, 2500);
    femur.attach(FEMUR_PWM_PIN, 500, 2500);
    tibia.attach(TIBIA_PWM_PIN, 500, 2500);
    coxa.write(90); femur.write(90); tibia.write(90);
    digitalWrite(LED_R, HIGH); digitalWrite(LED_B, HIGH);
    Serial.println("re-attached — resuming");
    delay(500);
}

void sampleBaseline() {
    float st = 0, sf = 0, sb = 0, sc = 0;
    for (int i = 0; i < BASELINE_N; i++) {
        st += analogRead(I_TOTAL_PIN) * ADC_TO_V * V_TO_A;
        sf += analogRead(I_FEMUR_PIN) * ADC_TO_V * V_TO_A;
        sb += analogRead(I_TIBIA_PIN) * ADC_TO_V * V_TO_A;
        sc += analogRead(I_COXA_PIN)  * ADC_TO_V * V_TO_A;
        delay(5);
    }
    b_total = st / BASELINE_N;
    b_femur = sf / BASELINE_N;
    b_tibia = sb / BASELINE_N;
    b_coxa  = sc / BASELINE_N;
}

void holdAndSample(int ms) {
    for (int elapsed = 0; elapsed < ms; elapsed += SAMPLE_MS) {
        Currents c = readCurrents();
        Serial.printf("total=%.3fA  coxa=%.3fA  femur=%.3fA  tibia=%.3fA\n",
                      c.total, c.coxa, c.femur, c.tibia);
        if (checkSafety(c)) {
            reattach();
            return;
        }
        delay(SAMPLE_MS);
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_R, OUTPUT); digitalWrite(LED_R, HIGH);
    pinMode(LED_G, OUTPUT); digitalWrite(LED_G, HIGH);
    pinMode(LED_B, OUTPUT); digitalWrite(LED_B, HIGH);

    analogReadResolution(12);

    coxa.attach(COXA_PWM_PIN,  500, 2500);
    femur.attach(FEMUR_PWM_PIN, 500, 2500);
    tibia.attach(TIBIA_PWM_PIN, 500, 2500);
    coxa.write(90); femur.write(90); tibia.write(90);
    delay(1000);

    Serial.println("=== zeroing baseline ===");
    sampleBaseline();
    Serial.printf("baseline  total=%.4fA  coxa=%.4fA  femur=%.4fA  tibia=%.4fA\n",
                  b_total, b_coxa, b_femur, b_tibia);
    Serial.printf("limits    branch=%.1fA  total=%.1fA\n",
                  BRANCH_TRIP_A, TOTAL_TRIP_A);

    Serial.println("=== leg stall test ===");
    Serial.println("idle:");
    holdAndSample(500);
}

void loop() {
    Serial.println("--- stall @ 0 deg ---");
    coxa.write(0); femur.write(0); tibia.write(0);
    digitalWrite(LED_R, LOW);
    holdAndSample(HOLD_MS);
    digitalWrite(LED_R, HIGH);

    coxa.write(90); femur.write(90); tibia.write(90);
    delay(500);

    Serial.println("--- stall @ 180 deg ---");
    coxa.write(180); femur.write(180); tibia.write(180);
    digitalWrite(LED_B, LOW);
    holdAndSample(HOLD_MS);
    digitalWrite(LED_B, HIGH);

    coxa.write(90); femur.write(90); tibia.write(90);
    Serial.println("--- centre ---");
    holdAndSample(1000);
}
