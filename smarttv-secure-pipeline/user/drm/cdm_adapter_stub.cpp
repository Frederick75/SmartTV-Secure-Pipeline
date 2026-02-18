#include "cdm_adapter.h"
#include "../common/log.h"

class StubCdmAdapter final : public ICdmAdapter {
public:
  bool initialize() override {
    LOGI("CDM adapter stub initialized (bind to licensed PlayReady CDM here)");
    return true;
  }

  LicenseResponse process_license(const std::vector<uint8_t>& license_msg) override {
    (void)license_msg;
    LicenseResponse r;
    // This is a non-DRM opaque blob used only to test the TEE call path.
    r.key_blob.assign(256, 0x42);
    return r;
  }
};

std::unique_ptr<ICdmAdapter> CreateCdmAdapter() {
  return std::make_unique<StubCdmAdapter>();
}
