#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
  namespace daiko_light {
    class DaikoLightOutput : public light::LightOutput, public Component {
    public:
      light::LightTraits get_traits() override;
      void write_state(light::LightState *state) override;
      void dump_config() override;
      void set_transmitter(remote_transmitter::RemoteTransmitterComponent *emitter) { emitter_ = emitter; }
      void set_channel(uint8_t channel) { channel_ = channel; }

    private:
      // Light has 11 selectable warmth levels ("on" commands 29 through 39)
      enum color_level {
        CT_UNKNOWN = -1,
        CT_COOL = 0,
        CT_WARM = 10,
      };

      // The brightness extended command allows values from 19 to 127; 0 through
      // 18 are unknown other commands
      enum brightness_level {
        BRT_UNKNOWN = -1,
        BRT_MIN = 19,
        BRT_MAX = 127
      };

      color_level select_color_level_(float mired_val);
      brightness_level select_brightness_level_(float brightness_val);
      void send_command_(uint8_t command, uint8_t command2 = 0);

      esphome::remote_transmitter::RemoteTransmitterComponent *emitter_{nullptr};
      uint8_t channel_ { 1 };
      bool current_on_ { false };
      color_level current_color_level_{ CT_UNKNOWN };
      brightness_level current_brightness_level_{ BRT_UNKNOWN };
    };
  }
}
