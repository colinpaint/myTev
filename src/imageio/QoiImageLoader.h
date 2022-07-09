// This file was developed by Tiago Chaves & Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.
#pragma once
#include <Image.h>
#include <imageio/ImageLoader.h>
#include <istream>

TEV_NAMESPACE_BEGIN

class QoiImageLoader : public ImageLoader {
public:
  bool canLoadFile(std::istream& iStream) const override;
  Task<std::vector<ImageData>> load(std::istream& iStream, const fs::path& path, const std::string& channelSelector, int priority) const override;

  std::string name() const override {
    return "QOI";
    }
  };

TEV_NAMESPACE_END
