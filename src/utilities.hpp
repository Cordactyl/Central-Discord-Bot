#pragma once
#include <dpp/dpp.h>

namespace cordactyl::utilities
{
    inline std::string get_current_timestamp(std::time_t t = 0) {
        if (! t) {
            t = std::time(nullptr); // get current time
        }
        const std::tm local_time = *std::localtime(&t); // to local time

        // format to string
        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S %Z");
        return oss.str();
    }
}