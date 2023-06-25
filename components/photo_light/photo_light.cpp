#include "esphome/core/log.h"
#include "photo_light.h"

#include "esphome/components/remote_base/nec_protocol.h"
#include "esphome/components/remote_base/pronto_protocol.h"

namespace esphome {
  namespace photo_light {
    static const char *TAG = "photo_light";

    static const uint16_t ADDR = 0xfe01;

    static const uint16_t TOGGLE = 0xff00;

    static const uint16_t BRT_100   = 0xf40b;
    static const uint16_t BRT_50    = 0xf807;
    static const uint16_t BRT_20    = 0xfc03;
    static const uint16_t BRT_SLEEP = 0xf906;

    static const uint16_t CT_COLD   = 0xb748;
    static const uint16_t CT_WHITE  = 0xbb44;
    static const uint16_t CT_WARM   = 0xbf40;

    static const uint16_t CT_WARMER = 0xf50a;
    static const uint16_t CT_COOLER = 0xfd02;

    struct color_setting {
      // applies if the current color state is less than the threshold
      float threshold;
      uint16_t base;
      uint16_t adjustment;
      const char* name;
    };

    constexpr static const color_setting color_settings[] = {
      { 0.08, CT_COLD,  0,         "Cold"   },
      { 0.25, CT_COLD,  CT_WARMER, "Cold+"  },
      { 0.42, CT_WHITE, CT_COOLER, "White-" },
      { 0.59, CT_WHITE, 0,         "White"  },
      { 0.76, CT_WHITE, CT_WARMER, "White+" },
      { 0.93, CT_WARM,  CT_COOLER, "Warm-"  },
      { 1.00, CT_WARM,  0,         "Warm"   }
    };

    struct brightness_setting {
      float threshold;
      uint16_t setting;
      const char* name;
    };

    constexpr static const brightness_setting brightness_settings[] = {
      { 0.2, 0,         "Off" },
      { 0.5, BRT_SLEEP, "Sleep" },
      { 0.8, BRT_50,    "50%" },
      { 1.0, BRT_100,   "100%" }
    };

    light::LightTraits PhotoLightOutput::get_traits() {
      auto traits = light::LightTraits();
      traits.set_supported_color_modes({ light::ColorMode::COLOR_TEMPERATURE });

      // 70 mired is a blatant lie, but the goal here is to make the "white" color
      // line up with the documented-as-5500K (182 mired) white color of the NEC
      // lights, which results in wildly stretching out the cold side.
      traits.set_min_mireds(50);
      traits.set_max_mireds(370);
      return traits;
    }

    void PhotoLightOutput::write_state(light::LightState *state) {
      float color_temperature_val, brightness_val;
      brightness_setting brightness = brightness_settings[0];
      color_setting color = color_settings[0];

      state->current_values_as_ct(&color_temperature_val, &brightness_val);

      ESP_LOGD("photo_light", "Received state: brightness=%f, color_temperature=%f",
               brightness_val, color_temperature_val);

      for(int i = 0; i < (sizeof(color_settings) / sizeof(color_settings[0])); ++i) {
        if (color_temperature_val <= color_settings[i].threshold) {
          color = color_settings[i];
          break;
        }
      }

      for(int i = 0; i < (sizeof(brightness_settings) / sizeof(brightness_settings[0])); ++i) {
        if (brightness_val <= brightness_settings[i].threshold) {
          brightness = brightness_settings[i];
          break;
        }
      }

      ESP_LOGD("photo_light", "Selected settings: brightness=%s, color=%s",
               brightness.name, color.name);

      // Set color temperature first, because the base color commands force 100% brightness
      if (brightness.setting) {
        this->send_nec_(ADDR, color.base);
        // brief delay: don't know if this is necessary
        delay(25);
      }

      // Now set brightness
      if (brightness.setting) {
        this->send_nec_(ADDR, brightness.setting);
      }
      else {
        // Off is handled specially
        ESP_LOGD("photo_light", "brightness: off");
        this->send_nec_(ADDR, BRT_SLEEP);
        delay(25);
        this->send_nec_(ADDR, TOGGLE);
      }

      // Set refined color temperature where necessary
      if (brightness.setting && color.adjustment) {
        delay(25); // again, don't know if the delay is necessary
        this->send_nec_(ADDR, color.adjustment);
        this->send_repeat_(8);
      }

      this->last_brightness_  = brightness_val;
      this->last_color_temperature_ = color_temperature_val;
    }

    void PhotoLightOutput::dump_config() {
      ESP_LOGCONFIG(TAG, "Photographic Light Box Bulb");
    }

    void PhotoLightOutput::send_nec_(uint16_t address, uint16_t command) {
      auto transmit = this->emitter_->transmit();
      remote_base::NECData data{address, command};
      remote_base::NECProtocol().encode(transmit.get_data(), data);
      transmit.perform();
    }

    static const uint32_t repeat_send_wait = 50000; // 50ms
    constexpr static const char* repeat_pronto_code = "0000 006D 0002 0000 0159 0057 0015 06C3";

    void PhotoLightOutput::send_repeat_(uint8_t times) {
      auto transmit = this->emitter_->transmit();
      remote_base::ProntoData data {repeat_pronto_code};
      remote_base::ProntoProtocol().encode(transmit.get_data(), data);
      transmit.set_send_times(times);
      transmit.set_send_wait(repeat_send_wait);
      transmit.perform();
    }
  }
}
