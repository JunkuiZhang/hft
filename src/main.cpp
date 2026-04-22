#include <iostream>
#include <cstddef>
#include "core/Types.h"

int main() {
    std::cout << "--- HFT Engine (C++20) ---" << std::endl;
    std::cout << "TickData Memory Layout:" << std::endl;
    std::cout << "  sizeof(TickData):  " << sizeof(hft::core::TickData) << " bytes" << std::endl;
    std::cout << "  alignof(TickData): " << alignof(hft::core::TickData) << " bytes" << std::endl;

    std::cout << "\\nOrderSignal Memory Layout:" << std::endl;
    std::cout << "  sizeof(OrderSignal):  " << sizeof(hft::core::OrderSignal) << " bytes" << std::endl;
    std::cout << "  alignof(OrderSignal): " << alignof(hft::core::OrderSignal) << " bytes" << std::endl;

    return 0;
}
