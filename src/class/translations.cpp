#include "translations.h"

namespace cordactyl
{
    void parse_localisations(const std::string& translations_dir) {
        for (const auto& entry : std::filesystem::directory_iterator(translations_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    std::string loc = entry.path().stem().string(); // Filename without extension
                    nlohmann::json json;
                    try {
                        file >> json;
                    } catch (const nlohmann::json::parse_error& e) {
                        throw std::runtime_error("Translation file \"" + entry.path().string() + "\" could not be parsed: " + e.what());
                    }

                    // extract translations
                    translation t;
                    for (const auto& item : json.items()) {
                        if (item.key().empty() || !item.value().is_string()) {
                            std::cerr << "Invalid element in translation file \"" << entry.path().string()
                                      << " Key: \"" << item.key() << "\""
                                      << " Value: \"" << item.value().dump() << "\"" << std::endl;
                            continue;
                        }
                        t[item.key()] = item.value();
                    }

                    translations[loc] = t;
                }
            }
        }
    }

    std::string _(const std::string& key, const std::string& language) {
        if (translations.contains(language) && translations[language].contains(key)) {
            return translations[language][key];
        }
        return key;
    }
}