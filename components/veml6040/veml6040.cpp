#include "veml6040.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace veml6040 {

namespace cct {

constexpr uint16_t MIN_RGB_COUNT = 10;

constexpr std::array<float, 9> OPEN_AIR_MATRIX = {
     0.048403,  0.183633, -0.253589,    // M1, M2, M3
     0.022916,  0.176388, -0.183205,    // M4, M5, M6
    -0.077436,  0.124541,  0.032081,    // M7, M8, M9
};

constexpr std::array<float, 9> ROOM_4K_MATRIX = {
    -0.023249,  0.291014, -0.364880,    // M1, M2, M3
    -0.042799,  0.272148, -0.279591,    // M4, M5, M6
    -0.155901,  0.251534, -0.076240,    // M7, M8, M9
};

}

namespace reg {

constexpr uint8_t CONF  = 0x00;
constexpr uint8_t RED   = 0x08;
constexpr uint8_t GREEN = 0x09;
constexpr uint8_t BLUE  = 0x0A;
constexpr uint8_t WHITE = 0x0B;

}  // namespace reg

namespace config {

constexpr uint16_t TRIGGER_DISABLED = 0x0000;
constexpr uint16_t SENSOR_ENABLED = 0x0000;
constexpr uint16_t SENSOR_DISABLED = 0x0001;
constexpr uint16_t DEFAULT =
    static_cast<uint16_t>(IntegrationTime::IT_40MS) |
    TRIGGER_DISABLED |
    SENSOR_ENABLED;

}  // namespace config

namespace {

const char *cct_profile_to_string(CCTProfile profile) {
  switch (profile) {
    case CCTProfile::CALIBRATED:
      return "calibrated";
    case CCTProfile::OPEN_AIR:
      return "open_air";
    case CCTProfile::ROOM_4K:
      return "room_4k";
  }

  return "unknown";
}

}

static const char *const TAG = "veml6040";

float VEML6040Component::get_lux_per_count_() const {
  switch (this->integration_time_) {
    case IntegrationTime::IT_40MS:
      return 0.25168f;
    case IntegrationTime::IT_80MS:
      return 0.12584f;
    case IntegrationTime::IT_160MS:
      return 0.06292f;
    case IntegrationTime::IT_320MS:
      return 0.03146f;
    case IntegrationTime::IT_640MS:
      return 0.01573f;
    case IntegrationTime::IT_1280MS:
      return 0.007865f;
    default:
      return 0.25168f;
  }
}

float VEML6040Component::calculate_color_temperature_(uint16_t red, uint16_t green, uint16_t blue) const {

  if (red < cct::MIN_RGB_COUNT && green < cct::MIN_RGB_COUNT && blue < cct::MIN_RGB_COUNT) {
    ESP_LOGD(TAG, "Skipping CCT Calculation: RGB signal too low");
    return NAN;
  }

  const float r = static_cast<float>(red);
  const float g = static_cast<float>(green);
  const float b = static_cast<float>(blue);
  
  const std::array<float, 9> *matrix = &cct::OPEN_AIR_MATRIX;
  
  switch (this->cct_profile_) {
    case CCTProfile::OPEN_AIR:
      matrix = &cct::OPEN_AIR_MATRIX;
      break;
    case CCTProfile::ROOM_4K:
      matrix = &cct::ROOM_4K_MATRIX;
      break;
    case CCTProfile::CALIBRATED:
      matrix = &this->custom_cct_matrix_;
      break;
  }
  
  const float X = (*matrix)[0] * r + (*matrix)[1] * g + (*matrix)[2] * b;
  const float Y = (*matrix)[3] * r + (*matrix)[4] * g + (*matrix)[5] * b;
  const float Z = (*matrix)[6] * r + (*matrix)[7] * g + (*matrix)[8] * b;

  const float sum = X + Y + Z;
  if (sum <= 0.0f)
    return NAN;

  const float x = X / sum;
  const float y = Y / sum;

  if (y == 0.1858f)
    return NAN;

  const float n = (x - 0.3320f) / (0.1858f - y);

  return 449.0f * n * n * n +
         3525.0f * n * n +
         6823.3f * n +
         5520.33f;
}

void VEML6040Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VEML6040...");
  
  const uint16_t expected_config = static_cast<uint16_t>(this->integration_time_) | config::SENSOR_ENABLED;

  if (!this->write_u16_(reg::CONF, config::SENSOR_DISABLED)) {
    ESP_LOGE(TAG, "Failed to put VEML6040 into shutdown");
    this->mark_failed();
    return;
  }

  if (!this->write_u16_(reg::CONF, expected_config)) {
    ESP_LOGE(TAG, "Failed to configure VEML6040");
    this->mark_failed();
    return;
  }
  
  uint16_t test_value;
  if (!this->read_u16_(reg::CONF, &test_value)) {
    ESP_LOGE(TAG, "Failed to read VEML6040 configuration");
    this->mark_failed();
    return;
  }

  ESP_LOGCONFIG(TAG, "VEML6040 detected, config register value: 0x%04X", test_value);
  
  if (test_value != expected_config) {
    ESP_LOGW(TAG, "Unexpected VEML6040 config: expected 0x%04X, got 0x%04X", expected_config, test_value);
  }
}

void VEML6040Component::dump_config() {
  ESP_LOGCONFIG(TAG, "VEML6040:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Lux compensation: %.3f", this->lux_compensation_);
  ESP_LOGCONFIG(TAG, "  CCT profile: %s", cct_profile_to_string(this->cct_profile_));
  
  LOG_SENSOR("  ", "Red", this->red_sensor_);
  LOG_SENSOR("  ", "Green", this->green_sensor_);
  LOG_SENSOR("  ", "Blue", this->blue_sensor_);
  LOG_SENSOR("  ", "White", this->white_sensor_);
  LOG_SENSOR("  ", "Illuminance", this->illuminance_sensor_);
  LOG_SENSOR("  ", "Color temperature", this->color_temperature_sensor_);
}

bool VEML6040Component::write_u16_(uint8_t reg, uint16_t value) {
  uint8_t data[2] = {
      static_cast<uint8_t>(value & 0xFF),
      static_cast<uint8_t>((value >> 8) & 0xFF),
  };

  return this->write_register(reg, data, 2) == i2c::ERROR_OK;
}

bool VEML6040Component::read_u16_(uint8_t reg, uint16_t *value) {
  uint8_t data[2];

  if (this->read_register(reg, data, 2) != i2c::ERROR_OK) {
    return false;
  }

  *value = static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
  return true;
}

void VEML6040Component::update() {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  uint16_t white;

  if (!this->read_u16_(reg::RED, &red) ||
      !this->read_u16_(reg::GREEN, &green) ||
      !this->read_u16_(reg::BLUE, &blue) ||
      !this->read_u16_(reg::WHITE, &white)) {
    ESP_LOGW(TAG, "Failed to read VEML6040 data");
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();

  ESP_LOGD(TAG, "RGBW: R=%u G=%u B=%u W=%u", red, green, blue, white);

  if (this->red_sensor_ != nullptr)
    this->red_sensor_->publish_state(red);
  if (this->green_sensor_ != nullptr)
    this->green_sensor_->publish_state(green);
  if (this->blue_sensor_ != nullptr)
    this->blue_sensor_->publish_state(blue);
  if (this->white_sensor_ != nullptr)
    this->white_sensor_->publish_state(white);
  if (this->illuminance_sensor_ != nullptr) {
    const float lux = green * this->get_lux_per_count_() * this->lux_compensation_;
    this->illuminance_sensor_->publish_state(lux);
  }
  if (this->color_temperature_sensor_ != nullptr) {
    const float cct = this->calculate_color_temperature_(red, green, blue);
    this->color_temperature_sensor_->publish_state(cct);
  }

}

}  // namespace veml6040
}  // namespace esphome
