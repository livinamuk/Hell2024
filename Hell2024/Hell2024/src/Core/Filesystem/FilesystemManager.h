#pragma once

#include <span>
#include <vector>
#include <string>
#include <string_view>

namespace core {

namespace defaults {

constexpr std::string_view ResourceDir{ "res/" };
constexpr std::string_view URLSeparator{ "://" };
constexpr std::string_view Separators{ "\\/" };
constexpr char ExtensionSeparator{ '.' };
constexpr size_t MaxPathLength{ 256 };

} // namespace defaults

class FilesystemManager {
public:
	struct Prefix {
		static constexpr std::string_view User { "usr://" };
		static constexpr std::string_view Asset{ "res://" };
	};
	static constexpr struct WriteModeAppend    {} Append   {};
	static constexpr struct WriteModeOverwrite {} Overwrite{};
	static constexpr struct DirModeNormal      {} Normal   {};
	static constexpr struct DirModeRecursive   {} Recursive{};

	FilesystemManager() = delete;

	static bool Initialize(
		const std::string_view app_name,
		const std::string_view res_dir = defaults::ResourceDir
	);

	static std::string ReadText(const std::string_view filename);
	static std::vector<std::byte> ReadBinary(const std::string_view filename);

	static bool WriteText(const std::string_view filename, const std::string_view text,
		const WriteModeOverwrite mode = Overwrite);
	static bool WriteText(const std::string_view filename, const std::string_view text,
		const WriteModeAppend);

	static bool WriteBinary(const std::string_view filename, const std::span<std::byte> bytes,
		const WriteModeOverwrite mode = Overwrite);
	static bool WriteBinary(const std::string_view filename, const std::span<std::byte> bytes,
		const WriteModeAppend);

	static bool IsFile(const std::string_view filename);
	static bool IsDir(const std::string_view filename);

	static bool FileExists(const std::string_view filename);
	static bool DirExists(const std::string_view filename);

	static bool MakeDir(const std::string_view path, const DirModeNormal mode = Normal);
	static bool MakeDir(const std::string_view path, const DirModeRecursive);
	static bool RemoveDir(const std::string_view path, const DirModeNormal mode = Normal);
	static bool RemoveDir(const std::string_view path, const DirModeRecursive);

private:
	inline static std::string m_asset_directory;
	inline static std::string m_user_directory;

	static bool WriteData(const std::string_view filename, const std::byte *data,
		const size_t length, const std::string_view mode);

	static bool EnsureDirExists(const std::string_view path);

	static std::string Translate(const std::string_view path) noexcept;
};

} // namespace core

using core::FilesystemManager;
using FS = FilesystemManager;
