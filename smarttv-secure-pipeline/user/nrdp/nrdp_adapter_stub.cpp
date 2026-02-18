#include "nrdp_adapter.h"
#include "../common/log.h"

class StubNrdpAdapter final : public INrdpAdapter {
public:
  bool initialize() override {
    LOGI("NRDP adapter stub initialized (bind to Netflix NRDP here)");
    return true;
  }
  OcaEndpoint select_best_oca() override {
    // Dummy endpoint; real selection logic lives in NRDP.
    return {"oca.netflix.example", 443};
  }
};

std::unique_ptr<INrdpAdapter> CreateNrdpAdapter() {
  return std::make_unique<StubNrdpAdapter>();
}
