// Unity unit test for the RS485 protocol module (host / native env).
//
//   pio test -e native
//
// Exercises CRC, the pull-frame parse path (the leg's entire input path), and
// response building, against the test vectors in
// docs/v2/interfaces/RS485_PROTOCOL.md.

#include <unity.h>
#include "protocol.h"
#include <string.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

// --- CRC test vectors ----------------------------------------------------
static void test_crc_pull_vector(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xF5, proto_crc8((const uint8_t *)">1,00,-45.5,-30.2,60.0", 22));
}

static void test_crc_response_vector(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x71, proto_crc8((const uint8_t *)"<1,00,1250,450,380,420", 22));
}

// --- Parse: basic pull ---------------------------------------------------
static void test_parse_basic_pull(void)
{
    char line[] = ">1,00,-45.5,-30.2,60.0*F5\n";
    proto_pull_t p;
    TEST_ASSERT_TRUE(proto_parse_pull(line, &p));
    TEST_ASSERT_EQUAL_UINT8(1, p.addr);
    TEST_ASSERT_EQUAL_UINT8(0x00, p.flags);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -45.5f, p.coxa_deg);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -30.2f, p.femur_deg);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 60.0f, p.tibia_deg);
    TEST_ASSERT_EQUAL_INT(0, p.n_params);
}

// --- Parse: flags + config params ----------------------------------------
static void test_parse_flags_and_params(void)
{
    char line[] = ">1,01,-45.5,-30.2,60.0,P01=10,P02=500*3A\n";
    proto_pull_t p;
    TEST_ASSERT_TRUE(proto_parse_pull(line, &p));
    TEST_ASSERT_EQUAL_UINT8(PROTO_FLAG_JOINT_POS, p.flags);
    TEST_ASSERT_EQUAL_INT(2, p.n_params);
    TEST_ASSERT_EQUAL_UINT8(0x01, p.params[0].id);
    TEST_ASSERT_EQUAL_INT32(10, p.params[0].value);
    TEST_ASSERT_EQUAL_UINT8(0x02, p.params[1].id);
    TEST_ASSERT_EQUAL_INT32(500, p.params[1].value);
}

// --- Parse: reject bad CRC -----------------------------------------------
static void test_parse_rejects_bad_crc(void)
{
    char line[] = ">1,00,-45.5,-30.2,60.0*00\n";
    proto_pull_t p;
    TEST_ASSERT_FALSE(proto_parse_pull(line, &p));
}

// --- Parse: reject non-'>' frame -----------------------------------------
static void test_parse_rejects_wrong_start(void)
{
    char line[] = "X1,00,-45.5,-30.2,60.0*F5\n";
    proto_pull_t p;
    TEST_ASSERT_FALSE(proto_parse_pull(line, &p));
}

// --- Build: basic response matches vector --------------------------------
static void test_build_basic_response(void)
{
    proto_response_t r;
    memset(&r, 0, sizeof(r));
    r.addr = 1; r.status = 0;
    r.current_total_ma = 1250; r.current_coxa_ma = 450;
    r.current_femur_ma = 380; r.current_tibia_ma = 420;
    char buf[128];
    int n = proto_build_response(&r, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL_STRING("<1,00,1250,450,380,420*71\n", buf);
}

// --- Build: response with joint positions matches vector -----------------
static void test_build_response_with_positions(void)
{
    proto_response_t r;
    memset(&r, 0, sizeof(r));
    r.addr = 1; r.status = 0;
    r.current_total_ma = 1250; r.current_coxa_ma = 450;
    r.current_femur_ma = 380; r.current_tibia_ma = 420;
    r.include_positions = true;
    r.pos_coxa_deg = -44.8f; r.pos_femur_deg = -29.5f; r.pos_tibia_deg = 59.2f;
    char buf[128];
    proto_build_response(&r, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("<1,00,1250,450,380,420,-44.8,-29.5,59.2*77\n", buf);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_pull_vector);
    RUN_TEST(test_crc_response_vector);
    RUN_TEST(test_parse_basic_pull);
    RUN_TEST(test_parse_flags_and_params);
    RUN_TEST(test_parse_rejects_bad_crc);
    RUN_TEST(test_parse_rejects_wrong_start);
    RUN_TEST(test_build_basic_response);
    RUN_TEST(test_build_response_with_positions);
    return UNITY_END();
}
