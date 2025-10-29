from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_COMMAND,
    CONF_COMMAND_REPEATS,
)

from esphome.components.remote_base import register_dumper, register_trigger, register_binary_sensor, register_action, declare_protocol

CODEOWNERS = ["@chrisandreae"]
CONF_COMMAND2 = "command2"

AUTO_LOAD = ["binary_sensor"]


# Extended NEC
ExtendedNECData, ExtendedNECBinarySensor, ExtendedNECTrigger, ExtendedNECAction, ExtendedNECDumper = declare_protocol("ExtendedNEC")
ExtendedNEC_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
        cv.Required(CONF_COMMAND): cv.hex_uint16_t,
        cv.Required(CONF_COMMAND2): cv.hex_uint16_t,
        cv.Optional(CONF_COMMAND_REPEATS, default=1): cv.uint16_t,
    }
)


@register_binary_sensor("extended_nec", ExtendedNECBinarySensor, ExtendedNEC_SCHEMA)
def extended_nec_binary_sensor(var, config):
    cg.add(
        var.set_data(
            cg.StructInitializer(
                ExtendedNECData,
                ("address", config[CONF_ADDRESS]),
                ("command", config[CONF_COMMAND]),
                ("command2", config[CONF_COMMAND2]),
                ("command_repeats", config[CONF_COMMAND_REPEATS]),
            )
        )
    )


@register_trigger("extended_nec", ExtendedNECTrigger, ExtendedNECData)
def extended_nec_trigger(var, config):
    pass


@register_dumper("extended_nec", ExtendedNECDumper)
def extended_nec_dumper(var, config):
    pass


@register_action("extended_nec", ExtendedNECAction, ExtendedNEC_SCHEMA)
async def extended_nec_action(var, config, args):
    template_ = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    cg.add(var.set_address(template_))
    template_ = await cg.templatable(config[CONF_COMMAND], args, cg.uint16)
    cg.add(var.set_command(template_))
    template_ = await cg.templatable(config[CONF_COMMAND2], args, cg.uint16)
    cg.add(var.set_command2(template_))
    template_ = await cg.templatable(config[CONF_COMMAND_REPEATS], args, cg.uint16)
    cg.add(var.set_command_repeats(template_))

# This is a horrible hack to allow `extended_nec:` at the top level of the yaml.
# We wouldn't want to do this, but without it this initializer simply doesn't
# seem to be picked up by the codegen and so remote_transmitter.transmit_extended_nec
# cannot be found.
CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    print("Test ======")
    pass
