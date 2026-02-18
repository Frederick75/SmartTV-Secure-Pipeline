#include "../common/log.h"
#include "../player/pipeline.h"
#include <string>

static std::string arg_value(int argc, char** argv, const char* key, const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == key) return argv[i + 1];
  }
  return def;
}

static bool has_flag(int argc, char** argv, const char* key) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == key) return true;
  }
  return false;
}

int main(int argc, char** argv) {
  std::string card = arg_value(argc, argv, "--card", "/dev/dri/card0");
  std::string heap = arg_value(argc, argv, "--heap", "secure");
  int width = std::stoi(arg_value(argc, argv, "--width", "1920"));
  int height = std::stoi(arg_value(argc, argv, "--height", "1080"));
  int frames = std::stoi(arg_value(argc, argv, "--frames", "120"));
  bool rdma = has_flag(argc, argv, "--rdma");

  LOGI("demo_player: card=%s heap_hint=%s %dx%d frames=%d rdma=%s",
       card.c_str(), heap.c_str(), width, height, frames, rdma ? "on" : "off");

  SecurePipeline p;
  int rc = p.run_demo(card, heap, width, height, rdma, frames);
  LOGI("demo_player exit rc=%d", rc);
  return rc;
}
