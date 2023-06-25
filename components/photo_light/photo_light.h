#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
  namespace photo_light {
    class PhotoLightOutput : public light::LightOutput, public Component {
    public:
      light::LightTraits get_traits() override;
      void write_state(light::LightState *state) override;
      void dump_config() override;
      void set_transmitter(remote_transmitter::RemoteTransmitterComponent *emitter) { emitter_ = emitter; }

    private:

      void send_nec_(uint16_t address, uint16_t command);
      void send_repeat_(uint8_t times);

      esphome::remote_transmitter::RemoteTransmitterComponent *emitter_{nullptr};
      float last_color_temperature_ {0};
      float last_brightness_ {0};
    };
  }
}
