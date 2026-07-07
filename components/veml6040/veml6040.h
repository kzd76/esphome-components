#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include <array>

namespace esphome {
namespace veml6040 {

enum class CCTProfile : uint8_t {
  CALIBRATED = 0,
  OPEN_AIR = 1,
  ROOM_4K = 2,
};

enum class IntegrationTime : uint16_t {
  IT_40MS = 0x0000,
  IT_80MS = 0x0010,
  IT_160MS = 0x0020,
  IT_320MS = 0x0030,
  IT_640MS = 0x0040,
  IT_1280MS = 0x0050,
};

class VEML6040Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  
  void set_red_sensor(sensor::Sensor *red_sensor) { red_sensor_ = red_sensor; }
  void set_green_sensor(sensor::Sensor *green_sensor) { green_sensor_ = green_sensor; }
  void set_blue_sensor(sensor::Sensor *blue_sensor) { blue_sensor_ = blue_sensor; }
  void set_white_sensor(sensor::Sensor *white_sensor) { white_sensor_ = white_sensor; }
  
  void set_integration_time(IntegrationTime integration_time) { integration_time_ = integration_time; }
  
  void set_illuminance_sensor(sensor::Sensor *illuminance_sensor) { illuminance_sensor_ = illuminance_sensor; }
  
  void set_lux_compensation(float lux_compensation) { lux_compensation_ = lux_compensation; }
  
  void set_color_temperature_sensor(sensor::Sensor *color_temperature_sensor) { color_temperature_sensor_ = color_temperature_sensor; }
  void set_cct_profile(CCTProfile cct_profile) { cct_profile_ = cct_profile; }
  
  void set_custom_cct_matrix(const std::array<float, 9> &matrix) { custom_cct_matrix_ = matrix; }
 protected:
  bool write_u16_(uint8_t reg, uint16_t value);
  bool read_u16_(uint8_t reg, uint16_t *value);
  
  float get_lux_per_count_() const;
  
  float calculate_color_temperature_(uint16_t red, uint16_t green, uint16_t blue) const;
  
  sensor::Sensor *red_sensor_{nullptr};
  sensor::Sensor *green_sensor_{nullptr};
  sensor::Sensor *blue_sensor_{nullptr};
  sensor::Sensor *white_sensor_{nullptr};

  IntegrationTime integration_time_{IntegrationTime::IT_40MS};
  
  sensor::Sensor *illuminance_sensor_{nullptr};
  
  float lux_compensation_{1.0f};
  
  sensor::Sensor *color_temperature_sensor_{nullptr};
  CCTProfile cct_profile_{CCTProfile::OPEN_AIR};
  
  std::array<float, 9> custom_cct_matrix_{};
};

}  // namespace veml6040
}  // namespace esphome
