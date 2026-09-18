#include "UpdateChecker.h"

int main() {
    if (!IsNewerSemanticVersion(L"1.0.0", L"1.0.1")) return 1;
    if (!IsNewerSemanticVersion(L"1.0.9", L"v1.0.10")) return 2;
    if (!IsNewerSemanticVersion(L"1.9.9", L"2.0.0")) return 3;
    if (IsNewerSemanticVersion(L"1.0.1", L"1.0.1")) return 4;
    if (IsNewerSemanticVersion(L"1.0.2", L"1.0.1")) return 5;
    if (IsNewerSemanticVersion(L"1.0", L"1.0.1")) return 6;
    if (IsNewerSemanticVersion(L"1.0.0", L"release-1.0.1")) return 7;
    return 0;
}
