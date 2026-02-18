#pragma once
#include <memory>
#include <string>

struct OcaEndpoint {
  std::string host;
  int port = 443;
};

class INrdpAdapter {
public:
  virtual ~INrdpAdapter() = default;
  virtual bool initialize() = 0;
  virtual OcaEndpoint select_best_oca() = 0;
};

std::unique_ptr<INrdpAdapter> CreateNrdpAdapter();
