#if defined(_WIN32)

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif // !defined(WIN32_LEAN_AND_MEAN)

#include <windows.h>
#include <winerror.h>
#include <stringapiset.h>
#include <shlobj.h>

#elif defined(__linux__)

#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#else
#error [Filesystem Manager] Unknown platform
#endif

#include <stack>
#include <tuple>
#include <ranges>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "FilesystemManager.h"

namespace core {

namespace {

std::string get_cwd();
std::string get_user_dir();

bool file_exists(const std::string_view filename);
bool remove_file(const std::string_view filename);

bool dir_exists(const std::string_view filename);
bool make_dir(const std::string_view path);
bool remove_dir(const std::string_view path);
bool remove_recursive(const std::string_view path);

} // anonymous namespace

bool FilesystemManager::Initialize(const std::string_view app_name, const std::string_view res_dir) {
	m_asset_directory.reserve(defaults::MaxPathLength);
	m_user_directory.reserve(defaults::MaxPathLength);

	m_asset_directory = get_cwd() + "/" + std::string{ res_dir };
	if (defaults::Separators.find(m_asset_directory.back()) == std::string_view::npos) {
		m_asset_directory += "/";
	}
	m_user_directory = get_user_dir() + "/" + std::string{ app_name } + "/";

	bool success{ true };
	if (!dir_exists(m_asset_directory)) {
		std::cerr << "[Filesystem Manager] Aw shit, there's no assets directory called '"
			<< res_dir << "'!\n";
		assert(false && "Assets directory has fucked up");
		make_dir(m_asset_directory);
	}

	if (!dir_exists(m_user_directory)) {
		success = make_dir(m_user_directory);
	}

	std::cout << "[Filesystem Manager] Assets directory   : " << m_asset_directory << '\n';
	std::cout << "[Filesystem Manager] User data directory: " << m_user_directory << '\n';

	return success;
}

std::string FilesystemManager::ReadText(const std::string_view filename) {
	const auto fullpath{ Translate(filename) };
	if (FILE *file{ nullptr }; fopen_s(&file, fullpath.data(), "rb") == 0) {
		fseek(file, 0, SEEK_END);
		std::string result(std::ftell(file), '\0');
		std::rewind(file);
		std::fread(result.data(), sizeof(std::string::value_type), result.size(), file);
		std::fclose(file);
		return std::move(result);
	}

	return "";
}

std::vector<std::byte> FilesystemManager::ReadBinary(const std::string_view filename) {
	const auto fullpath{ Translate(filename) };
	if (FILE *file{ nullptr }; fopen_s(&file, fullpath.data(), "rb") == 0) {
		fseek(file, 0, SEEK_END);
		std::vector<std::byte> result(std::ftell(file));
		std::rewind(file);
		std::fread(result.data(), sizeof(std::byte), result.size(), file);
		std::fclose(file);
		return std::move(result);
	}

	return {};
}

bool FilesystemManager::WriteText(const std::string_view filename,
		const std::string_view text, const WriteModeOverwrite) {
	return WriteData(filename, reinterpret_cast<const std::byte *>(text.data()), text.size(), "w+");
}

bool FilesystemManager::WriteText(const std::string_view filename,
		const std::string_view text, const WriteModeAppend) {
	return WriteData(filename, reinterpret_cast<const std::byte *>(text.data()), text.size(), "a+");
}

bool FilesystemManager::WriteBinary(const std::string_view filename,
		const std::span<std::byte> bytes, const WriteModeOverwrite) {
	return WriteData(filename, bytes.data(), bytes.size(), "w+b");
}

bool FilesystemManager::WriteBinary(const std::string_view filename,
		const std::span<std::byte> bytes, const WriteModeAppend) {
	return WriteData(filename, bytes.data(), bytes.size(), "a+b");
}

bool FilesystemManager::IsFile(const std::string_view filename) {
	return defaults::Separators.find(filename.back()) == std::string_view::npos;
}

bool FilesystemManager::IsDir(const std::string_view filename) {
	return filename.back() == '/' || filename.back() == '\\';
}

bool FilesystemManager::FileExists(const std::string_view filename) {
	return file_exists(Translate(filename));
}

bool FilesystemManager::DirExists(const std::string_view filename) {
	return dir_exists(Translate(filename));
}

bool FilesystemManager::MakeDir(const std::string_view path, const DirModeNormal) {
	if (const auto fullpath{ Translate(path) }; !dir_exists(fullpath)) {
		return make_dir(fullpath);
	}
	return true; // Nothing to do
}

bool FilesystemManager::MakeDir(const std::string_view path, const DirModeRecursive) {
	const auto fullpath{ Translate(path) };
	const std::string_view full_view{ fullpath }; // Needed to not allocate string during substr

	if (dir_exists(full_view)) return true; // Already exists

	std::string current_path;
	current_path.reserve(full_view.size());

	if (path.starts_with(Prefix::Asset)) {
		current_path += m_asset_directory;
	} else if (path.starts_with(Prefix::User)) {
		current_path += m_user_directory;
	}

	for (size_t first{ current_path.size() }, last{}; last != std::string_view::npos;) {
		last = full_view.find_first_of(defaults::Separators, first);
		const auto subpath{ last != std::string_view::npos
			? full_view.substr(first, last - first + 1)
			: full_view.substr(first)
		};
		first = last + 1;

		if (subpath.empty() || subpath == "./") continue;

		current_path += subpath;
		if (DirExists(current_path)) continue;

		if (!make_dir(current_path)) return false;
	}

	return true;
}

bool FilesystemManager::RemoveDir(const std::string_view path, const DirModeNormal) {
	if (const auto fullpath{ Translate(path) }; dir_exists(fullpath)) {
		return remove_dir(fullpath);
	}
	return true; // Nothing to do
}


bool FilesystemManager::RemoveDir(const std::string_view path, const DirModeRecursive) {
	if (const auto fullpath{ Translate(path) }; dir_exists(fullpath)) {
		return remove_recursive(fullpath);
	}
	return true; // Nothing to do
}

bool FilesystemManager::WriteData(const std::string_view filename, const std::byte *data,
		const size_t length, const std::string_view mode) {
	if (!EnsureDirExists(filename)) return false;

	const auto fullpath{ Translate(filename) };
	if (FILE *file{ nullptr }; fopen_s(&file, fullpath.data(), mode.data()) == 0) {
		std::fwrite(data, sizeof(std::byte), length, file);
		std::fclose(file);
		return true;
	}

	return false;
}

bool FilesystemManager::EnsureDirExists(const std::string_view path) {
	std::string_view current_path{ path };
	if (IsFile(current_path)) {
		current_path.remove_suffix(
			current_path.size() - current_path.find_last_of(defaults::Separators)
		);
	}

	return MakeDir(current_path, Recursive);
}

std::string FilesystemManager::Translate(const std::string_view path) noexcept {
	if (path.starts_with(Prefix::Asset)) {
		return m_asset_directory + std::string{ path.substr(Prefix::Asset.size()) };
	} else if (path.starts_with(Prefix::User)) {
		return m_user_directory + std::string{ path.substr(Prefix::User.size()) };
	}
	std::cout << "[Filesystem Manager] Warning: " << path << " does not start with a prefix\n";
	return std::string{ path };
}


// =================================== HELPER FUNCTIONS =================================== //



namespace {

std::string get_cwd() {
	std::string path(defaults::MaxPathLength, L'\0');

#if defined(_WIN32)
	if (const size_t length{ GetCurrentDirectoryA(path.size(), path.data()) }; length != 0) {
		path.resize(length);
		return std::move(path);
	}
#elif defined(__linux__)
	if (getcwd(path.data(), path.size()) != nullptr) {
		path.resize(std::strlen(path.data()));
		return std::move(path);
	}
#endif // defined(_WIN32)

	return "./";
}

std::string get_user_dir() {
#if defined(_WIN32)
	struct clean_path final {
		~clean_path() { CoTaskMemFree(ptr); }
		LPWSTR ptr{ nullptr };
	};

	clean_path path;
	const auto result{
		SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &path.ptr)
	};
	if (SUCCEEDED(result)) {
		static std::mbstate_t state{};

		const size_t length{ std::wcslen(path.ptr) };
		std::string result(length * sizeof(std::wstring::value_type), '\0');

		const wchar_t *wstr{ path.ptr };
		size_t actual_length{ 0 };
		std::ignore = wcsrtombs_s(&actual_length, result.data(), result.size(), &wstr, result.capacity(), &state);
		result.resize(actual_length != std::string::npos ? actual_length - 1 : 0);
		return std::move(result);
	}

#elif defined(__linux__)

	const uid_t uid{ getuid() };

	if (const auto home{ std::getenv("HOME") }; uid != 0 && home != nullptr) {
		return std::string{ home };
	}

	const auto sysconf_out{ sysconf(_SC_GETPW_R_SIZE_MAX) };
	const size_t length{ sysconf_out > 0 ? static_cast<size_t>(sysconf_out) : 16384lu };

	std::string buffer(length, '\0');

	struct passwd* pw = nullptr;
	struct passwd pwd;
	if (auto err{ getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw) }; err != 0) {
		return L"./";
	}

	if (const auto dir{ pw->pw_dir }; dir != nullptr) {
		return dir;
	}
#endif

	return "./";
}

bool file_exists(const std::string_view filename) {
#if defined(_WIN32)

	const auto attr{ GetFileAttributesA(filename.data()) };
	return attr != INVALID_FILE_ATTRIBUTES
		&& (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;

#elif defined(__linux__)

	if (struct stat buffer; stat(filename.data(), &buffer) == 0) {
		return !S_ISDIR(buffer.st_mode);
	}

#endif // defined(_WIN32)

	return false;
}

bool remove_file(const std::string_view filename) {
#if defined(_WIN32)

	return DeleteFileA(filename.data()) != FALSE;

#elif defined(__linux__)

	return ::unlink(filename.data()) == 0;

#endif // defined(_WIN32)

	return false;
}

bool dir_exists(const std::string_view filename) {
#if defined(_WIN32)

	const auto attr{ GetFileAttributesA(filename.data()) };
	return attr != INVALID_FILE_ATTRIBUTES
		&& (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

#elif defined(__linux__)

	if (struct stat buffer; stat(filename.data(), &buffer) == 0) {
		return S_ISDIR(buffer.st_mode);
	}

#endif // defined(_WIN32)

	return false;
}

bool make_dir(const std::string_view path) {
#if defined(_WIN32)

	return CreateDirectoryA(path.data(), nullptr) != FALSE;

#elif defined(__linux__)

	return ::mkdir(filesystem::to_narrow(path).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;

#endif // defined(_WIN32)

	return false;
}

bool remove_dir(const std::string_view path) {
#if defined(_WIN32)

	return RemoveDirectoryA(path.data()) != FALSE;

#elif defined(__linux__)

	return ::rmdir(filesystem::to_narrow(path).c_str()) == 0;

#endif // defined(_WIN32)

	return false;
}

bool remove_recursive(const std::string_view path) {
#if defined(_WIN32)

	const auto pattern{ std::string{ path } + "\\*" };

	WIN32_FIND_DATAA data;
	HANDLE handle{ FindFirstFileA(pattern.data(), &data) };
	if (handle == INVALID_HANDLE_VALUE) {
		return false;
	}

	do {
		if (strcmp(data.cFileName, ".") == 0) continue;
		if (strcmp(data.cFileName, "..") == 0) continue;

		const auto filename{ std::string{ path } + "\\" + data.cFileName };

		bool success{ false };
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			success = remove_recursive(filename);
		} else {
			success = remove_file(filename);
		}
		if (!success) {
			FindClose(handle);
			return false;
		}
	} while (FindNextFileA(handle, &data) != 0);

	FindClose(handle);

#elif defined(__linux__)

	DIR *d{ opendir(path.c_str()) };
	if (d == nullptr) return false;

	struct dirent *p;
	while ((p = readdir(d))) {

		/* Skip the names "." and ".." as we don't want to recurse on them. */
		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			continue;

		const size_t len{ path.size() + strlen(p->d_name) + 2 };

		bool success{ false };
		if (std::unique_ptr<char[]> buf{ new char[len] }; buf) {
			snprintf(buf.get(), len, "%s/%s", path.data(), p->d_name);
			if (struct stat statbuf; !stat(buf, &statbuf)) {
				if (S_ISDIR(statbuf.st_mode)) {
					success = remove_recursive(buf);
				} else {
					success = unlink(buf);
				}
			}
		}
		if (!success) {
			closedir(d);
			return false;
		}
	}
	closedir(d);

#endif // defined(_WIN32)

	// directory should be empty now, remove it
	return remove_dir(path);
}

} // anonymous namespace

} // namespace core

