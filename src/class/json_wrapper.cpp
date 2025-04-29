#include "json_wrapper.h"
namespace cordactyl
{
    void json_wrapper::deep_merge(nlohmann::json& target, const nlohmann::json& source) {
        for (auto& [key, value] : source.items()) {
            if (target.contains(key)) {
                // Both values are objects - recurse
                if (target[key].is_object() && value.is_object()) {
                    deep_merge(target[key], value);
                }
                // Both values are arrays - concatenate
                else if (target[key].is_array() && value.is_array()) {
                    target[key].insert(target[key].end(),
                                     value.begin(), value.end());
                }
                else {
                    // Overwrite with source value
                    target[key] = value;
                }
            } else {
                // Add new key
                target[key] = value;
            }
        }
    }

    json_wrapper::json_wrapper() {
        this->payload = {};
    }

    void json_wrapper::merge(const nlohmann::json& source) {
        std::unique_lock lock(mutex);
        deep_merge(payload, source);
    }

    nlohmann::json json_wrapper::get_payload() {
        std::shared_lock lock(mutex);
        return payload;
    }
}
