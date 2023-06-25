#include "esphome/core/log.h"
#include "sara_light.h"

#include "esphome/components/remote_base/nec_protocol.h"

namespace esphome {
  namespace sara_light {
      static const char *TAG = "sara_light";

      static const uint16_t ADDR = 0xC580;

      static const uint16_t CMD_OFF = 0xF708;

      constexpr static const uint16_t CMDS_COOL[] = {
          0xA758,
          0xA55A,
          0xA35C,
          0xA15E,
          0x9F60,
          0x9D62,
          0x9B64,
          0x9966,
          0x9768,
          0x956A,
      };

      constexpr static const uint16_t CMDS_WARM[] = {
          0x936C,
          0x916E,
          0x8F70,
          0x8D72,
          0x8B74,
          0x8976,
          0x8778,
          0x857A,
          0x837C,
          0x817E,
      };

      // static const uint16_t CMD_COOL_LVL_1  = 0xA758;
      // static const uint16_t CMD_COOL_LVL_2  = 0xA55A;
      // static const uint16_t CMD_COOL_LVL_3  = 0xA35C;
      // static const uint16_t CMD_COOL_LVL_4  = 0xA15E;
      // static const uint16_t CMD_COOL_LVL_5  = 0x9F60;
      // static const uint16_t CMD_COOL_LVL_6  = 0x9D62;
      // static const uint16_t CMD_COOL_LVL_7  = 0x9B64;
      // static const uint16_t CMD_COOL_LVL_8  = 0x9966;
      // static const uint16_t CMD_COOL_LVL_9  = 0x9768;
      // static const uint16_t CMD_COOL_LVL_10 = 0x956A;

      // static const uint16_t CMD_WARM_LVL_1  = 0x936C;
      // static const uint16_t CMD_WARM_LVL_2  = 0x916E;
      // static const uint16_t CMD_WARM_LVL_3  = 0x8F70;
      // static const uint16_t CMD_WARM_LVL_4  = 0x8D72;
      // static const uint16_t CMD_WARM_LVL_5  = 0x8B74;
      // static const uint16_t CMD_WARM_LVL_6  = 0x8976;
      // static const uint16_t CMD_WARM_LVL_7  = 0x8778;
      // static const uint16_t CMD_WARM_LVL_8  = 0x857A;
      // static const uint16_t CMD_WARM_LVL_9  = 0x837C;
      // static const uint16_t CMD_WARM_LVL_10 = 0x817E;

      static const int COMMAND_DELAY = 255;

      light::LightTraits SaraLightOutput::get_traits() {
          auto traits = light::LightTraits();
          traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});

          traits.set_min_mireds(154);
          traits.set_max_mireds(370);

          return traits;
      }

      void SaraLightOutput::write_state(light::LightState *state) {
          light::LightColorValues current_values = state->current_values;

          // Read the raw color temperature, because we want to convert from mireds
          // directly to our mapped ranges
          float ct_mireds = current_values.get_color_temperature();

          // Read brightness with as_brightness to perform gamma correction
          float brightness;
          state->current_values_as_brightness(&brightness);

          ESP_LOGD(TAG, "Light received state: brightness=%f, color_temperature=%f mireds",
                   brightness, ct_mireds);

          if (brightness == 0.0f) {
              ESP_LOGD(TAG, "Turning off");
              send_command_(CMD_OFF);
          }
          else {
              SaraLightOutput::color_level color_level = select_color_level_(ct_mireds);
              SaraLightOutput::brightness_levels brightness_levels = select_brightness_levels_(brightness, color_level);

              ESP_LOGD(TAG, "Selected levels: warm brightness=%d cool brightness=%d, color_temperature=%d",
                       brightness_levels.warm, brightness_levels.cool, color_level);

              uint16_t cool_cmd = CMDS_COOL[brightness_levels.cool];
              uint16_t warm_cmd = CMDS_COOL[brightness_levels.warm];


              send_command_(cool_cmd);
              delay(COMMAND_DELAY);
              send_command_(warm_cmd);
          }
      }

      void SaraLightOutput::dump_config() {
          ESP_LOGCONFIG(TAG, "Sara IR Ceiling Light");
      }

      // Interpolating points between the color levels in Kelvin
      // (since HA's CT selectors are linear in Kelvin):
      // (6500) 6250 (6000) 5750 (5500) 4800 (4100) 3400 (2700) K
      // (154)  160  (167)  174  (182)  208  (244)  294  (370) mired
      static const float CT_THRESHOLDS[] = { 160, 174, 208, 294 };

      SaraLightOutput::color_level SaraLightOutput::select_color_level_(float mired_val) {
          for(int i = 0; i < (sizeof(CT_THRESHOLDS) / sizeof(CT_THRESHOLDS[0])); ++i) {
              if (mired_val < CT_THRESHOLDS[i]) {
                  return (color_level) i;
              }
          }
          return CT_WARM;
      }

      SaraLightOutput::brightness_levels SaraLightOutput::select_brightness_levels_(float brightness_val, SaraLightOutput::color_level color) {
          float cool_brightness_val, warm_brightness_val = 1.0;
          brightness_levels levels;

          switch (color) {
          case CT_COOL:
              cool_brightness_val = brightness_val;
              warm_brightness_val = 0;
              break;
          case CT_COOLER:
              cool_brightness_val = brightness_val;
              warm_brightness_val = brightness_val / 2.0;
          case CT_WHITE:
              cool_brightness_val = brightness_val;
              warm_brightness_val = brightness_val / 2.0;
          case CT_WARMER:
              cool_brightness_val = brightness_val / 2.0;
              warm_brightness_val = brightness_val;
          case CT_WARM:
              cool_brightness_val = 0;
              warm_brightness_val = brightness_val;
          };

          levels.cool = round_brightness_level_(cool_brightness_val);
          levels.warm = round_brightness_level_(warm_brightness_val);

          return levels;
      }

      SaraLightOutput::brightness_level SaraLightOutput::round_brightness_level_(float brightness_val) {
          brightness_level brightness = (brightness_level) (brightness_val * 10.0);
          return std::min(brightness, BRT_MAX);
      }

      void SaraLightOutput::send_command_(uint16_t command) {
          send_nec_(ADDR, command);
      }

      void SaraLightOutput::send_nec_(uint16_t address, uint16_t command) {
          auto transmit = this->emitter_->transmit();
          remote_base::NECData data{address, command};
          remote_base::NECProtocol().encode(transmit.get_data(), data);
          transmit.perform();
      }
  }
}
