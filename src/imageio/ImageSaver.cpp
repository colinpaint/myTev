// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.

#include <imageio/ImageSaver.h>

#include <imageio/ExrImageSaver.h>
#include <imageio/QoiImageSaver.h>
#include <imageio/StbiHdrImageSaver.h>
#include <imageio/StbiLdrImageSaver.h>

#include <vector>

using namespace std;

TEV_NAMESPACE_BEGIN

const vector<unique_ptr<ImageSaver>>& ImageSaver::getSavers() {
    auto makeSavers = [] {
        vector<unique_ptr<ImageSaver>> imageSavers;
        imageSavers.emplace_back(new ExrImageSaver());
        imageSavers.emplace_back(new QoiImageSaver());
        imageSavers.emplace_back(new StbiHdrImageSaver());
        imageSavers.emplace_back(new StbiLdrImageSaver());
        return imageSavers;
    };

    static const vector imageSavers = makeSavers();
    return imageSavers;
}

TEV_NAMESPACE_END
