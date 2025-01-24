#pragma once
#include "../Common/Types.h"
#include "../File/FileFormats.h"
#include <string>

namespace AssimpImporter {

    ModelData ImportFbx(const std::string filepath);

}