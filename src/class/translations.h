#pragma once
#include <dpp/dpp.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

namespace cordactyl
{
    typedef std::unordered_map<std::string, std::string> translation;

    typedef std::unordered_map<std::string, translation> translation_file;

    /**
     * Global localisation strings
     */
    inline translation_file translations;

    /**
     * @param translations_dir
     * @throws std::runtime_error On error
     */
    void parse_localisations(const std::string& translations_dir);

    /**
     * Translate a string.
     * @param key
     * @param language The translation file.
     * @return The translated string or key if not found.
     */
    std::string _(const std::string& key, const std::string& language = "en");
}

#endif
