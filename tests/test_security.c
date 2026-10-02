#include "security.h"

#include <stdio.h>

static int tests_passed = 0;
static int tests_failed = 0;

static void check_test(const char *name, int condition)
{
    if (condition)
    {
        printf("[PASS] %s\n", name);
        tests_passed++;
    }
    else
    {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}

int main(void)
{
    check_test(
        "Root path accepted",
        security_validate_path("/") == 0);

    check_test(
        "Normal HTML path accepted",
        security_validate_path("/index.html") == 0);

    check_test(
        "CSS path accepted",
        security_validate_path("/style.css") == 0);

    check_test(
        "Parent directory traversal rejected",
        security_validate_path("/../etc/passwd") != 0);

    check_test(
        "Nested traversal rejected",
        security_validate_path("/images/../../etc/passwd") != 0);

    check_test(
        "Backslash path rejected",
        security_validate_path("/..\\etc\\passwd") != 0);

    check_test(
        "Relative path rejected",
        security_validate_path("index.html") != 0);

    check_test(
        "NULL path rejected",
        security_validate_path(NULL) != 0);

    printf("\n========================================\n");
    printf("Security Test Results\n");
    printf("========================================\n");
    printf("Passed : %d\n", tests_passed);
    printf("Failed : %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed == 0 ? 0 : 1;
}
