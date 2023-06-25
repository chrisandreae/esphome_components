#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
  namespace nec_light {
    class NecLightOutput : public light::LightOutput, public Component {
    public:
      light::LightTraits get_traits() override;
      void write_state(light::LightState *state) override;
      void dump_config() override;
      void set_transmitter(remote_transmitter::RemoteTransmitterComponent *emitter) { emitter_ = emitter; }
      void set_channel(uint8_t channel) { channel_ = channel; }

    private:
      // Light has 5 selectable color temperatures:
      // CT (k)  CT (m)    name
      // 2700K - 370 mired ("relax")
      // 4100K - 244 mired ("unwind")
      // 5500K - 182 mired ("natural")
      // 6000K - 167 mired ("refresh")
      // 6500K - 154 mired ("active")
      enum color_level {
        CT_UNKNOWN = -1,
        CT_ACTIVE = 0,
        CT_REFRESH,
        CT_NATURAL,
        CT_UNWIND,
        CT_RELAX
      };

      // Light has 10 selectable brightness levels
      enum brightness_level {
        BRT_UNKNOWN = -1,
        BRT_MIN = 0,
        BRT_MID = 5,
        BRT_MAX = 9
      };

      color_level select_color_level_(float mired_val);
      brightness_level select_brightness_level_(float brightness_val);
      void send_command_(uint16_t command);
      void send_nec_(uint16_t address, uint16_t command);

      esphome::remote_transmitter::RemoteTransmitterComponent *emitter_{nullptr};
      uint8_t channel_ { 1 };
      bool current_on_ { false };
      color_level current_color_level_{ CT_UNKNOWN };
      brightness_level current_brightness_level_{ BRT_UNKNOWN };
    };
  }
}
