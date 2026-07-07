import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import (
    CONF_ID,
    CONF_RED,
    CONF_GREEN,
    CONF_BLUE,
    CONF_WHITE,
    CONF_ILLUMINANCE,
    STATE_CLASS_MEASUREMENT,
    DEVICE_CLASS_ILLUMINANCE,
    UNIT_LUX,
)

DEPENDENCIES = ["i2c"]

veml6040_ns = cg.esphome_ns.namespace("veml6040")
VEML6040Component = veml6040_ns.class_("VEML6040Component", cg.PollingComponent)

IntegrationTime = veml6040_ns.enum("IntegrationTime", is_class=True)
CCTProfile = veml6040_ns.enum("CCTProfile", is_class=True)

CONF_INTEGRATION_TIME = "integration_time"
CONF_LUX_COMPENSATION = "lux_compensation"
CONF_COLOR_TEMPERATURE = "color_temperature"
CONF_CCT_PROFILE = "cct_profile"

CONF_MATRIX = "matrix"

CCT_PROFILES = {
    "calibrated": CCTProfile.CALIBRATED,
    "open_air": CCTProfile.OPEN_AIR,
    "room_4k": CCTProfile.ROOM_4K,
}

INTEGRATION_TIMES = {
    "40ms": IntegrationTime.IT_40MS,
    "80ms": IntegrationTime.IT_80MS,
    "160ms": IntegrationTime.IT_160MS,
    "320ms": IntegrationTime.IT_320MS,
    "640ms": IntegrationTime.IT_640MS,
    "1280ms": IntegrationTime.IT_1280MS,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(VEML6040Component),
        cv.Optional(CONF_RED): sensor.sensor_schema(
            unit_of_measurement='counts',
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_GREEN): sensor.sensor_schema(
            unit_of_measurement='counts',
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BLUE): sensor.sensor_schema(
            unit_of_measurement='counts',
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_WHITE): sensor.sensor_schema(
            unit_of_measurement='counts',
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(
            CONF_INTEGRATION_TIME, 
            default="40ms",
        ): cv.enum(INTEGRATION_TIMES),
        cv.Optional(CONF_ILLUMINANCE): sensor.sensor_schema(
            unit_of_measurement=UNIT_LUX,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ILLUMINANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(
            CONF_LUX_COMPENSATION,
            default="1.0",
        ): cv.float_range(min=0.01, max=100.0),
        cv.Optional(CONF_COLOR_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="K",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ).extend(
            {
                cv.Optional(CONF_CCT_PROFILE, default="open_air"): cv.enum(CCT_PROFILES),
                cv.Optional(CONF_MATRIX): cv.All(
                    cv.ensure_list,
                    cv.Length(min=3, max=3),
                    [
                        cv.All(
                            cv.ensure_list,
                            cv.Length(min=3, max=3),
                            [cv.float_],
                        )
                    ],
                ),
            }
        ),
    }
).extend(cv.polling_component_schema("60s")).extend(i2c.i2c_device_schema(0x10))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_integration_time(config[CONF_INTEGRATION_TIME]))
    cg.add(var.set_lux_compensation(config[CONF_LUX_COMPENSATION]))
    if CONF_RED in config:
        sens = await sensor.new_sensor(config[CONF_RED])
        cg.add(var.set_red_sensor(sens))
    if CONF_GREEN in config:
        sens = await sensor.new_sensor(config[CONF_GREEN])
        cg.add(var.set_green_sensor(sens))
    if CONF_BLUE in config:
        sens = await sensor.new_sensor(config[CONF_BLUE])
        cg.add(var.set_blue_sensor(sens))
    if CONF_WHITE in config:
        sens = await sensor.new_sensor(config[CONF_WHITE])
        cg.add(var.set_white_sensor(sens))
    if CONF_ILLUMINANCE in config:
        sens = await sensor.new_sensor(config[CONF_ILLUMINANCE])
        cg.add(var.set_illuminance_sensor(sens))
    if CONF_COLOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_COLOR_TEMPERATURE])
        cg.add(var.set_color_temperature_sensor(sens))
        cg.add(var.set_cct_profile(config[CONF_COLOR_TEMPERATURE][CONF_CCT_PROFILE]))
    if CONF_MATRIX in config[CONF_COLOR_TEMPERATURE]:
        matrix = config[CONF_COLOR_TEMPERATURE][CONF_MATRIX]
        flat_matrix = [value for row in matrix for value in row]
        cg.add(var.set_custom_cct_matrix(flat_matrix))
