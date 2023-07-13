#include "esphome/core/log.h"
#include "nec_light.h"

#include "esphome/components/remote_base/nec_protocol.h"

namespace esphome {
  namespace nec_light {
    static const char *TAG = "nec_light";

    static const uint16_t ADDR = 0x6d82;

    static const uint16_t CMD_ON  = 0x42bd;
    static const uint16_t CMD_OFF = 0x41be;

    static const uint16_t CMD_MAX_WARM  = 0x51ae;
    static const uint16_t CMD_MAX_WHITE = 0x52ad;
    static const uint16_t CMD_MID_WHITE = 0x5da2;
    static const uint16_t CMD_MAX_COOL  = 0x53ac;

    static const uint16_t CMD_BRIGHTER = 0x45ba;
    static const uint16_t CMD_DIMMER   = 0x44bb;

    static const uint16_t CMD_DIMMEST  = 0x1de2;

    static const uint16_t CMD_WARMER   = 0x57a8;
    static const uint16_t CMD_COOLER   = 0x58a7;

    static const int COMMAND_DELAY = 255;

    light::LightTraits NecLightOutput::get_traits() {
      auto traits = light::LightTraits();
      traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});

      traits.set_min_mireds(154);
      traits.set_max_mireds(370);

      return traits;
    }

    void NecLightOutput::write_state(light::LightState *state) {
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
        return;
      }

      NecLightOutput::brightness_level brightness_level = select_brightness_level_(brightness);
      NecLightOutput::color_level color_level = select_color_level_(ct_mireds);

      ESP_LOGD(TAG, "Selected levels: brightness=%d, color_temperature=%d",
               brightness_level, color_level);

      // If we match a state with an absolute code, send it
      if (brightness_level == BRT_MAX && color_level == CT_ACTIVE) {
        ESP_LOGD(TAG, "Absolute setting: max cool");
        send_command_(CMD_MAX_COOL);
      }
      else if (brightness_level == BRT_MAX && color_level == CT_NATURAL) {
        ESP_LOGD(TAG, "Absolute setting: max white");
        send_command_(CMD_MAX_WHITE);
      }
      else if (brightness_level == BRT_MID && color_level == CT_NATURAL) {
        ESP_LOGD(TAG, "Absolute setting: mid white");
        send_command_(CMD_MID_WHITE);
      }
      else if (brightness_level == BRT_MAX && color_level == CT_RELAX) {
        ESP_LOGD(TAG, "Absolute setting: max warm");
        send_command_(CMD_MAX_WARM);
      }
      // Otherwise, we need to send a relative change
      else {
        if (current_brightness_level_ == BRT_UNKNOWN || current_color_level_ == CT_UNKNOWN) {
          ESP_LOGD(TAG, "Current state unknown, resetting to mid white");
          send_command_(CMD_MID_WHITE);

          current_brightness_level_ = BRT_MID;
          current_color_level_ = CT_NATURAL;

          delay(COMMAND_DELAY);
        }
        else if (!current_on_) {
          ESP_LOGD(TAG, "Turning on");
          send_command_(CMD_ON);
          delay(COMMAND_DELAY);
        }

        int color_delta = color_level - current_color_level_;
        int brightness_delta = brightness_level - current_brightness_level_;

        ESP_LOGD(TAG, "Color %d -> %d: %d steps",
                 current_color_level_, color_level, color_delta);

        if (color_delta != 0) {
          uint16_t color_command = color_delta > 0 ? CMD_WARMER : CMD_COOLER;
          for (int i = 0; i < abs(color_delta); ++i) {
            send_command_(color_command);
            delay(COMMAND_DELAY);
          }
        }

        ESP_LOGD(TAG, "Brightness %d -> %d: %d steps",
                 current_brightness_level_, brightness_level, brightness_delta);

        if (brightness_delta != 0) {
          uint16_t brightness_command = brightness_delta > 0 ? CMD_BRIGHTER : CMD_DIMMER;
          for (int i = 0; i < abs(brightness_delta); ++i) {
            send_command_(brightness_command);
            delay(COMMAND_DELAY);
          }
        }
      }
      current_on_ = true;
      current_brightness_level_ = brightness_level;
      current_color_level_ = color_level;
    }

    void NecLightOutput::dump_config() {
      ESP_LOGCONFIG(TAG, "Nec IR Ceiling Light");
    }

    // Interpolating points between the color levels in Kelvin
    // (since HA's CT selectors are linear in Kelvin):
    // (6500) 6250 (6000) 5750 (5500) 4800 (4100) 3400 (2700) K
    // (154)  160  (167)  174  (182)  208  (244)  294  (370) mired
    static const float CT_THRESHOLDS[] = { 160, 174, 208, 294 };

    NecLightOutput::color_level NecLightOutput::select_color_level_(float mired_val) {
      for(int i = 0; i < (sizeof(CT_THRESHOLDS) / sizeof(CT_THRESHOLDS[0])); ++i) {
        if (mired_val < CT_THRESHOLDS[i]) {
          return (color_level) i;
        }
      }
      return CT_RELAX;
    }

    NecLightOutput::brightness_level NecLightOutput::select_brightness_level_(float brightness_val) {
      brightness_level brightness = (brightness_level) (brightness_val * 10.0);
      return std::min(brightness, BRT_MAX);
    }

    void NecLightOutput::send_command_(uint16_t command) {
      if (channel_ == 2) {
        command |= 0x8000;
        command &= ~0x0080;
      }
      send_nec_(ADDR, command);
    }

    void NecLightOutput::send_nec_(uint16_t address, uint16_t command) {
      auto transmit = this->emitter_->transmit();
      remote_base::NECData data{address, command};
      remote_base::NECProtocol().encode(transmit.get_data(), data);
      transmit.perform();
    }
  }
}
