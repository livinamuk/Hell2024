#include "Animation.h"
#include "../../Util.hpp"

Animation::Animation(std::string filepath) {

    FileInfo info = Util::GetFileInfo(filepath);
    _filename = info.filename;
}

Animation::~Animation()
{
}

float Animation::GetTicksPerSecond()
{
    return m_ticksPerSecond != 0 ? m_ticksPerSecond : 25.0f;;
}
