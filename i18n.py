import json
import glob
import os
from functools import reduce


_translations = {}


def init_translations(translations_dir):
    global _translations
    # get list of files with specific extensions
    files = glob.glob(os.path.join(translations_dir, f'*.json'))
    for file in files:
        # get the name of the file without extension, will be used as locale name
        loc = os.path.splitext(os.path.basename(file))[0]
        with open(file, 'r', encoding='utf8') as f:
            _translations[loc] = json.load(f)


def deep_get(dictionary, keys, default=None):
    return reduce(lambda d, key: d.get(key, default) if isinstance(d, dict) else default, keys.split("."), dictionary)


def _(key, locale) -> str:
    """
    :param key: The translation key. Use dot-notation to access nested dict keys. e.g. 'frontend.footer.description'

    :param locale: The IETF language tag.
    Defaults to 'en' if the translation is not found in the provided language.
    https://en.wikipedia.org/wiki/IETF_language_tag

    :return: The translation
    """
    default_locale = 'en'
    if locale not in _translations and '-' in locale:
        locale = locale.split('-')[0]
    return deep_get(_translations.get(locale), key) or deep_get(_translations.get(default_locale), key) or key


init_translations('lang')
