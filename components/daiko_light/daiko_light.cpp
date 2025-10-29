#include "esphome/core/log.h"
#include "daiko_light.h"

#include "esphome/components/remote_base/nec_protocol.h"
#include "../extended_nec/extended_nec_protocol.h"

namespace esphome {
  namespace daiko_light {
    static const char *TAG = "daiko_light";

    static const uint16_t ADDR = 0xfb85; // empirically

    static const uint16_t CMD_OFF = 0;
    static const uint16_t CMD_ON_BASE = 29;

    static const uint16_t CMD_BRT_BASE = 19;

    // Empirically chosen to make the same gap as in the original recording
    static const int COMMAND_DELAY = 32;

    static const int MIN_MIREDS = 154;
    static const int MAX_MIREDS = 370;

    light::LightTraits DaikoLightOutput::get_traits() {
      auto traits = light::LightTraits();
      traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});

      traits.set_min_mireds(MIN_MIREDS);
      traits.set_max_mireds(MAX_MIREDS);

      return traits;
    }

    void DaikoLightOutput::write_state(light::LightState *state) {
      light::LightColorValues current_values = state->current_values;

      // Read the raw color temperature, because we want to convert from mireds
      // directly to our mapped ranges
      float ct_mireds = current_values.get_color_temperature();

      // Read brightness with as_brightness to perform gamma correction
      float brightness;
      state->current_values_as_brightness(&brightness);

      ESP_LOGD(TAG, "Channel %u received state: brightness=%f, color_temperature=%f mireds",
               channel_, brightness, ct_mireds);

      if (brightness == 0.0f) {
        ESP_LOGD(TAG, "Turning off");
        send_command_(CMD_OFF);
        current_on_ = false;
      }
      else {
        DaikoLightOutput::color_level color_level = select_color_level_(ct_mireds);
        DaikoLightOutput::brightness_level brightness_level = select_brightness_level_(brightness, color_level);

        ESP_LOGD(TAG, "Selected levels: brightness=%d, color_temperature=%d",
                 brightness_level, color_level);

        send_command_(CMD_ON_BASE + color_level, CMD_BRT_BASE + brightness_level);
        current_on_ = true;
        current_brightness_level_ = brightness_level;
        current_color_level_ = color_level;
      }
    }

    void DaikoLightOutput::dump_config() {
      ESP_LOGCONFIG(TAG, "Daiko IR Ceiling Light");
    }

    DaikoLightOutput::color_level DaikoLightOutput::select_color_level_(float mired_val) {
      double proportion = ((double)mired_val - MIN_MIREDS) / (MAX_MIREDS - MIN_MIREDS);

      return (color_level) round(proportion * CT_WARM);
    }


    DaikoLightOutput::brightness_level DaikoLightOutput::select_brightness_level_(float brightness_val, DaikoLightOutput::color_level color_level) {
      // Each color temperature has a separate brightness cap.
      static const int brightness_caps[] = {
        6, 6, 7, 7, 8, 9, 8, 7, 7, 6, 6
      };

      if(color_level > CT_WARM) { color_level = CT_WARM; }
      int brightness_cap = brightness_caps[color_level];
      return (brightness_level) round(brightness_val * brightness_cap);
    }

    void DaikoLightOutput::send_command_(uint8_t command, uint8_t command2) {
      uint16_t encoded_command = 0, encoded_command2 = 0;

      // Channel 2 is represented by the high bit in the command
      if (channel_ == 2) { command |= 0x80; }

      // To match the recorded trace, it seems like the complement needs be the
      // high order byte
      encoded_command = command | ((~command << 8) & 0xff00);

      if (command2) {
        if (channel_ == 2) { command2 |= 0x80; }
        encoded_command2 = command2 | ((~command2 << 8) & 0xff00);
      }

      send_encoded_command_(encoded_command, encoded_command2);
      delay(COMMAND_DELAY);
      send_encoded_command_(encoded_command, encoded_command2);
    }

    void DaikoLightOutput::send_encoded_command_(uint16_t encoded_command, uint16_t encoded_command2) {
      auto transmit = this->emitter_->transmit();

      if (encoded_command2) {
        remote_base::ExtendedNECData data{ADDR, encoded_command, encoded_command2, 1};
        remote_base::ExtendedNECProtocol().encode(transmit.get_data(), data);
      } else {
        remote_base::NECData data{ADDR, encoded_command, 1};
        remote_base::NECProtocol().encode(transmit.get_data(), data);
      }

      transmit.perform();
    }
  }
}
