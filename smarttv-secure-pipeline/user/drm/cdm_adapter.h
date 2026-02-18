#pragma once
#include <cstdint>
#include <memory>
#include <vector>

struct LicenseResponse {
  std::vector<uint8_t> key_blob; // opaque blob intended for TEE import
};

class ICdmAdapter {
public:
  virtual ~ICdmAdapter() = default;
  virtual bool initialize() = 0;
  virtual LicenseResponse process_license(const std::vector<uint8_t>& license_msg) = 0;
};

std::unique_ptr<ICdmAdapter> CreateCdmAdapter();
