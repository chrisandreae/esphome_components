import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import light, remote_transmitter, remote_base
from esphome.components.remote_base import CONF_TRANSMITTER_ID
from esphome.const import CONF_OUTPUT_ID, CONF_CHANNEL

DEPENDENCIES = ["remote_base", "light"]

photo_light_ns = cg.esphome_ns.namespace('photo_light')
PhotoLightOutput = photo_light_ns.class_('PhotoLightOutput', cg.Component, light.LightOutput)

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(PhotoLightOutput),
    cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
  }).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))

    await light.register_light(var, config)
