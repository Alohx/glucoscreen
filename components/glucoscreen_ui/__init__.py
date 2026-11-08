"""Glucoscreen UI – translations and color palettes.

Header-only external component: ESPHome copies glucoscreen_ui.h into the
build and includes it automatically, so the lambdas in glucoscreen.yaml can
use ui::tr(...) and ui::pal(...). Activated by the top-level key
``glucoscreen_ui:`` in glucoscreen.yaml.
"""

import esphome.config_validation as cv

CODEOWNERS = ["@Alohx"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    # Nothing to generate – the header is included by ESPHome itself.
    pass
