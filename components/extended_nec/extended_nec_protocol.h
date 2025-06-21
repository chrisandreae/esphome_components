#pragma once

#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace remote_base {

struct ExtendedNECData {
  uint16_t address;
  uint16_t command;
  uint16_t command2;
  uint16_t command_repeats;

  bool operator==(const ExtendedNECData &rhs) const { return address == rhs.address && command == rhs.command && command2 == rhs.command2; }
};

class ExtendedNECProtocol : public RemoteProtocol<ExtendedNECData> {
 public:
  void encode(RemoteTransmitData *dst, const ExtendedNECData &data) override;
  optional<ExtendedNECData> decode(RemoteReceiveData src) override;
  void dump(const ExtendedNECData &data) override;
};

DECLARE_REMOTE_PROTOCOL(ExtendedNEC)

template<typename... Ts> class ExtendedNECAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(uint16_t, command)
  TEMPLATABLE_VALUE(uint16_t, command2)
  TEMPLATABLE_VALUE(uint16_t, command_repeats)

  void encode(RemoteTransmitData *dst, Ts... x) override {
	ExtendedNECData data{};
	data.address = this->address_.value(x...);
	data.command = this->command_.value(x...);
	data.command2 = this->command2_.value(x...);
	data.command_repeats = this->command_repeats_.value(x...);
	ExtendedNECProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
