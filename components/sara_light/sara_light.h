#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
  namespace sara_light {
    class SaraLightOutput : public light::LightOutput, public Component {
    public:
      light::LightTraits get_traits() override;
      void write_state(light::LightState *state) override;
      void dump_config() override;
      void set_transmitter(remote_transmitter::RemoteTransmitterComponent *emitter) { emitter_ = emitter; }

    private:
      // Light is internally represented as two completely independent lights,
      // one warm and one cool. We map that into five levels, where the
      // intermediate states represent "half-brightness".
      enum color_level {
        CT_COOL = 0,
        CT_COOLER,
        CT_WHITE,
        CT_WARMER,
        CT_WARM,
      };

      // Light has 10 selectable brightness levels for each color
      enum brightness_level {
        BRT_MIN = 0,
        BRT_MAX = 9,
      };

      struct brightness_levels {
        brightness_level warm;
        brightness_level cool;
      };

      color_level select_color_level_(float mired_val);
      brightness_levels select_brightness_levels_(float brightness_val, color_level color);
      brightness_level round_brightness_level_(float brightness_val);
      void send_command_(uint16_t command);
      void send_nec_(uint16_t address, uint16_t command);

      esphome::remote_transmitter::RemoteTransmitterComponent *emitter_{nullptr};
    };
  }
}
