#pragma once

#include <dpp/dpp.h>

#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

namespace cordactyl
{
    class json_wrapper {
    protected:
        nlohmann::json payload;
        std::shared_mutex mutex;

        static void deep_merge(nlohmann::json& target, const nlohmann::json& source);
    public:
        json_wrapper();

        void merge(const nlohmann::json& source);

        nlohmann::json get_payload();
    };
}

#endif