#pragma once
#include "Compressonator.h"

CMP_FORMAT GetFormat(CMP_DWORD dwFourCC);
CMP_DWORD GetFourCC(CMP_FORMAT nFormat);
bool IsDXT5SwizzledFormat(CMP_FORMAT nFormat);
CMP_FORMAT ParseFormat(const char* pszFormat);
const char* GetFormatDesc(CMP_FORMAT nFormat);

bool LoadDDSFile(const char* pszFile, CMP_Texture& texture);
void SaveDDSFile(const char* pszFile, CMP_Texture& texture);
