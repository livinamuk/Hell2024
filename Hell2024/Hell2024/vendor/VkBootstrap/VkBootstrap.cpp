/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2020 Charles Giessen (charles@lunarg.com)
 */

#include "VkBootstrap.h"

#include <cstring>
#include <iostream>
#if defined(_WIN32)
#include <fcntl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif // _WIN32

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

#include <mutex>
#include <algorithm>

namespace vkb {

namespace detail {

GenericFeaturesPNextNode::GenericFeaturesPNextNode() { memset(fields, UINT8_MAX, sizeof(VkBool32) * field_capacity); }

bool GenericFeaturesPNextNode::match(GenericFeaturesPNextNode const& requested, GenericFeaturesPNextNode const& supported) noexcept {
	assert(requested.sType == supported.sType && "Non-matching sTypes in features nodes!");
	for (uint32_t i = 0; i < field_capacity; i++) {
		if (requested.fields[i] && !supported.fields[i]) return false;
	}
	return true;
}

class VulkanFunctions {
	private:
	std::mutex init_mutex;
	struct VulkanLibrary {
#if defined(__linux__) || defined(__APPLE__)
		void* library;
#elif defined(_WIN32)
		HMODULE library;
#endif
		PFN_vkGetInstanceProcAddr ptr_vkGetInstanceProcAddr = VK_NULL_HANDLE;

		VulkanLibrary() {
#if defined(__linux__)
			library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
			if (!library) library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
#elif defined(__APPLE__)
			library = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
			if (!library) library = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
#elif defined(_WIN32)
			library = LoadLibrary(TEXT("vulkan-1.dll"));
#else
			assert(false && "Unsupported platform");
#endif
			if (!library) return;
			load_func(ptr_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
		}

		template <typename T> void load_func(T& func_dest, const char* func_name) {
#if defined(__linux__) || defined(__APPLE__)
			func_dest = reinterpret_cast<T>(dlsym(library, func_name));
#elif defined(_WIN32)
			func_dest = reinterpret_cast<T>(GetProcAddress(library, func_name));
#endif
		}
		void close() {
#if defined(__linux__) || defined(__APPLE__)
			dlclose(library);
#elif defined(_WIN32)
			FreeLibrary(library);
#endif
			library = 0;
		}
	};
	VulkanLibrary& get_vulkan_library() {
		static VulkanLibrary lib;
		return lib;
	}

	bool load_vulkan(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = nullptr) {
		if (fp_vkGetInstanceProcAddr != nullptr) {
			ptr_vkGetInstanceProcAddr = fp_vkGetInstanceProcAddr;
			return true;
		} else {
			auto& lib = get_vulkan_library();
			ptr_vkGetInstanceProcAddr = lib.ptr_vkGetInstanceProcAddr;
			return lib.library != nullptr && lib.ptr_vkGetInstanceProcAddr != VK_NULL_HANDLE;
		}
	}

	void init_pre_instance_funcs() {
		fp_vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
		    ptr_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
		fp_vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
		    ptr_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
		fp_vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
		    ptr_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
		fp_vkCreateInstance =
		    reinterpret_cast<PFN_vkCreateInstance>(ptr_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
	}

	public:
	template <typename T> void get_inst_proc_addr(T& out_ptr, const char* func_name) {
		out_ptr = reinterpret_cast<T>(ptr_vkGetInstanceProcAddr(instance, func_name));
	}

	template <typename T> void get_device_proc_addr(VkDevice device, T& out_ptr, const char* func_name) {
		out_ptr = reinterpret_cast<T>(fp_vkGetDeviceProcAddr(device, func_name));
	}

	PFN_vkGetInstanceProcAddr ptr_vkGetInstanceProcAddr = nullptr;
	VkInstance instance = nullptr;

	PFN_vkEnumerateInstanceExtensionProperties fp_vkEnumerateInstanceExtensionProperties = nullptr;
	PFN_vkEnumerateInstanceLayerProperties fp_vkEnumerateInstanceLayerProperties = nullptr;
	PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion = nullptr;
	PFN_vkCreateInstance fp_vkCreateInstance = nullptr;
	PFN_vkDestroyInstance fp_vkDestroyInstance = nullptr;

	PFN_vkEnumeratePhysicalDevices fp_vkEnumeratePhysicalDevices = nullptr;
	PFN_vkGetPhysicalDeviceFeatures fp_vkGetPhysicalDeviceFeatures = nullptr;
	PFN_vkGetPhysicalDeviceFeatures2 fp_vkGetPhysicalDeviceFeatures2 = nullptr;
	PFN_vkGetPhysicalDeviceFeatures2KHR fp_vkGetPhysicalDeviceFeatures2KHR = nullptr;
	PFN_vkGetPhysicalDeviceFormatProperties fp_vkGetPhysicalDeviceFormatProperties = nullptr;
	PFN_vkGetPhysicalDeviceImageFormatProperties fp_vkGetPhysicalDeviceImageFormatProperties = nullptr;
	PFN_vkGetPhysicalDeviceProperties fp_vkGetPhysicalDeviceProperties = nullptr;
	PFN_vkGetPhysicalDeviceProperties2 fp_vkGetPhysicalDeviceProperties2 = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties fp_vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2 fp_vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
	PFN_vkGetPhysicalDeviceMemoryProperties fp_vkGetPhysicalDeviceMemoryProperties = nullptr;
	PFN_vkGetPhysicalDeviceFormatProperties2 fp_vkGetPhysicalDeviceFormatProperties2 = nullptr;
	PFN_vkGetPhysicalDeviceMemoryProperties2 fp_vkGetPhysicalDeviceMemoryProperties2 = nullptr;

	PFN_vkGetDeviceProcAddr fp_vkGetDeviceProcAddr = nullptr;
	PFN_vkCreateDevice fp_vkCreateDevice = nullptr;
	PFN_vkEnumerateDeviceExtensionProperties fp_vkEnumerateDeviceExtensionProperties = nullptr;

	PFN_vkDestroySurfaceKHR fp_vkDestroySurfaceKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fp_vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fp_vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fp_vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;

	bool init_vulkan_funcs(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {
		std::lock_guard<std::mutex> lg(init_mutex);
		if (!load_vulkan(fp_vkGetInstanceProcAddr)) return false;
		init_pre_instance_funcs();
		return true;
	}

	void init_instance_funcs(VkInstance inst) {
		instance = inst;
		get_inst_proc_addr(fp_vkDestroyInstance, "vkDestroyInstance");
		get_inst_proc_addr(fp_vkEnumeratePhysicalDevices, "vkEnumeratePhysicalDevices");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceFeatures, "vkGetPhysicalDeviceFeatures");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceFeatures2, "vkGetPhysicalDeviceFeatures2");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceFeatures2KHR, "vkGetPhysicalDeviceFeatures2KHR");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceFormatProperties, "vkGetPhysicalDeviceFormatProperties");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceImageFormatProperties, "vkGetPhysicalDeviceImageFormatProperties");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceProperties, "vkGetPhysicalDeviceProperties");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceProperties2, "vkGetPhysicalDeviceProperties2");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceQueueFamilyProperties, "vkGetPhysicalDeviceQueueFamilyProperties");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceQueueFamilyProperties2, "vkGetPhysicalDeviceQueueFamilyProperties2");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceMemoryProperties, "vkGetPhysicalDeviceMemoryProperties");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceFormatProperties2, "vkGetPhysicalDeviceFormatProperties2");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceMemoryProperties2, "vkGetPhysicalDeviceMemoryProperties2");

		get_inst_proc_addr(fp_vkGetDeviceProcAddr, "vkGetDeviceProcAddr");
		get_inst_proc_addr(fp_vkCreateDevice, "vkCreateDevice");
		get_inst_proc_addr(fp_vkEnumerateDeviceExtensionProperties, "vkEnumerateDeviceExtensionProperties");

		get_inst_proc_addr(fp_vkDestroySurfaceKHR, "vkDestroySurfaceKHR");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceSurfaceSupportKHR, "vkGetPhysicalDeviceSurfaceSupportKHR");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceSurfaceFormatsKHR, "vkGetPhysicalDeviceSurfaceFormatsKHR");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceSurfacePresentModesKHR, "vkGetPhysicalDeviceSurfacePresentModesKHR");
		get_inst_proc_addr(fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	}
};

VulkanFunctions& vulkan_functions() {
	static VulkanFunctions v;
	return v;
}

// Helper for robustly executing the two-call pattern
template <typename T, typename F, typename... Ts> auto get_vector(std::vector<T>& out, F&& f, Ts&&... ts) -> VkResult {
	uint32_t count = 0;
	VkResult err;
	do {
		err = f(ts..., &count, nullptr);
		if (err != VK_SUCCESS) {
			return err;
		};
		out.resize(count);
		err = f(ts..., &count, out.data());
		out.resize(count);
	} while (err == VK_INCOMPLETE);
	return err;
}

template <typename T, typename F, typename... Ts> auto get_vector_noerror(F&& f, Ts&&... ts) -> std::vector<T> {
	uint32_t count = 0;
	std::vector<T> results;
	f(ts..., &count, nullptr);
	results.resize(count);
	f(ts..., &count, results.data());
	results.resize(count);
	return results;
}
} // namespace detail

const char* to_string_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
	switch (s) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return "VERBOSE";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return "ERROR";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return "WARNING";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return "INFO";
		default:
			return "UNKNOWN";
	}
}
const char* to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s) {
	if (s == 7) return "General | Validation | Performance";
	if (s == 6) return "Validation | Performance";
	if (s == 5) return "General | Performance";
	if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
	if (s == 3) return "General | Validation";
	if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
	if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
	return "Unknown";
}

VkResult create_debug_utils_messenger(VkInstance instance,
    PFN_vkDebugUtilsMessengerCallbackEXT debug_callback,
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    void* user_data_pointer,
    VkDebugUtilsMessengerEXT* pDebugMessenger,
    VkAllocationCallbacks* allocation_callbacks) {

	if (debug_callback == nullptr) debug_callback = default_debug_callback;
	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.pNext = nullptr;
	messengerCreateInfo.messageSeverity = severity;
	messengerCreateInfo.messageType = type;
	messengerCreateInfo.pfnUserCallback = debug_callback;
	messengerCreateInfo.pUserData = user_data_pointer;

	PFN_vkCreateDebugUtilsMessengerEXT createMessengerFunc;
	detail::vulkan_functions().get_inst_proc_addr(createMessengerFunc, "vkCreateDebugUtilsMessengerEXT");

	if (createMessengerFunc != nullptr) {
		return createMessengerFunc(instance, &messengerCreateInfo, allocation_callbacks, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils_messenger(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks* allocation_callbacks) {

	PFN_vkDestroyDebugUtilsMessengerEXT deleteMessengerFunc;
	detail::vulkan_functions().get_inst_proc_addr(deleteMessengerFunc, "vkDestroyDebugUtilsMessengerEXT");

	if (deleteMessengerFunc != nullptr) {
		deleteMessengerFunc(instance, debugMessenger, allocation_callbacks);
	}
}

namespace detail {
bool check_layer_supported(std::vector<VkLayerProperties> const& available_layers, const char* layer_name) {
	if (!layer_name) return false;
	for (const auto& layer_properties : available_layers) {
		if (strcmp(layer_name, layer_properties.layerName) == 0) {
			return true;
		}
	}
	return false;
}

bool check_layers_supported(std::vector<VkLayerProperties> const& available_layers, std::vector<const char*> const& layer_names) {
	bool all_found = true;
	for (const auto& layer_name : layer_names) {
		bool found = check_layer_supported(available_layers, layer_name);
		if (!found) all_found = false;
	}
	return all_found;
}

bool check_extension_supported(std::vector<VkExtensionProperties> const& available_extensions, const char* extension_name) {
	if (!extension_name) return false;
	for (const auto& extension_properties : available_extensions) {
		if (strcmp(extension_name, extension_properties.extensionName) == 0) {
			return true;
		}
	}
	return false;
}

bool check_extensions_supported(
    std::vector<VkExtensionProperties> const& available_extensions, std::vector<const char*> const& extension_names) {
	bool all_found = true;
	for (const auto& extension_name : extension_names) {
		bool found = check_extension_supported(available_extensions, extension_name);
		if (!found) all_found = false;
	}
	return all_found;
}

template <typename T> void setup_pNext_chain(T& structure, std::vector<VkBaseOutStructure*> const& structs) {
	structure.pNext = nullptr;
	if (structs.size() <= 0) return;
	for (size_t i = 0; i < structs.size() - 1; i++) {
		structs.at(i)->pNext = structs.at(i + 1);
	}
	structure.pNext = structs.at(0);
}
const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";

struct InstanceErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vkb_instance"; }
	std::string message(int err) const override { return to_string(static_cast<InstanceError>(err)); }
};
const InstanceErrorCategory instance_error_category;

struct PhysicalDeviceErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vkb_physical_device"; }
	std::string message(int err) const override { return to_string(static_cast<PhysicalDeviceError>(err)); }
};
const PhysicalDeviceErrorCategory physical_device_error_category;

struct QueueErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vkb_queue"; }
	std::string message(int err) const override { return to_string(static_cast<QueueError>(err)); }
};
const QueueErrorCategory queue_error_category;

struct DeviceErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vkb_device"; }
	std::string message(int err) const override { return to_string(static_cast<DeviceError>(err)); }
};
const DeviceErrorCategory device_error_category;

struct SwapchainErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vbk_swapchain"; }
	std::string message(int err) const override { return to_string(static_cast<SwapchainError>(err)); }
};
const SwapchainErrorCategory swapchain_error_category;

} // namespace detail

std::error_code make_error_code(InstanceError instance_error) {
	return { static_cast<int>(instance_error), detail::instance_error_category };
}
std::error_code make_error_code(PhysicalDeviceError physical_device_error) {
	return { static_cast<int>(physical_device_error), detail::physical_device_error_category };
}
std::error_code make_error_code(QueueError queue_error) {
	return { static_cast<int>(queue_error), detail::queue_error_category };
}
std::error_code make_error_code(DeviceError device_error) {
	return { static_cast<int>(device_error), detail::device_error_category };
}
std::error_code make_error_code(SwapchainError swapchain_error) {
	return { static_cast<int>(swapchain_error), detail::swapchain_error_category };
}
#define CASE_TO_STRING(CATEGORY, TYPE)                                                                                 \
	case CATEGORY::TYPE:                                                                                               \
		return #TYPE;

const char* to_string(InstanceError err) {
	switch (err) {
		CASE_TO_STRING(InstanceError, vulkan_unavailable)
		CASE_TO_STRING(InstanceError, vulkan_version_unavailable)
		CASE_TO_STRING(InstanceError, vulkan_version_1_1_unavailable)
		CASE_TO_STRING(InstanceError, vulkan_version_1_2_unavailable)
		CASE_TO_STRING(InstanceError, failed_create_debug_messenger)
		CASE_TO_STRING(InstanceError, failed_create_instance)
		CASE_TO_STRING(InstanceError, requested_layers_not_present)
		CASE_TO_STRING(InstanceError, requested_extensions_not_present)
		CASE_TO_STRING(InstanceError, windowing_extensions_not_present)
		default:
			return "";
	}
}
const char* to_string(PhysicalDeviceError err) {
	switch (err) {
		CASE_TO_STRING(PhysicalDeviceError, no_surface_provided)
		CASE_TO_STRING(PhysicalDeviceError, failed_enumerate_physical_devices)
		CASE_TO_STRING(PhysicalDeviceError, no_physical_devices_found)
		CASE_TO_STRING(PhysicalDeviceError, no_suitable_device)
		default:
			return "";
	}
}
const char* to_string(QueueError err) {
	switch (err) {
		CASE_TO_STRING(QueueError, present_unavailable)
		CASE_TO_STRING(QueueError, graphics_unavailable)
		CASE_TO_STRING(QueueError, compute_unavailable)
		CASE_TO_STRING(QueueError, transfer_unavailable)
		CASE_TO_STRING(QueueError, queue_index_out_of_range)
		CASE_TO_STRING(QueueError, invalid_queue_family_index)
		default:
			return "";
	}
}
const char* to_string(DeviceError err) {
	switch (err) {
		CASE_TO_STRING(DeviceError, failed_create_device)
		default:
			return "";
	}
}
const char* to_string(SwapchainError err) {
	switch (err) {
		CASE_TO_STRING(SwapchainError, surface_handle_not_provided)
		CASE_TO_STRING(SwapchainError, failed_query_surface_support_details)
		CASE_TO_STRING(SwapchainError, failed_create_swapchain)
		CASE_TO_STRING(SwapchainError, failed_get_swapchain_images)
		CASE_TO_STRING(SwapchainError, failed_create_swapchain_image_views)
		CASE_TO_STRING(SwapchainError, required_min_image_count_too_low)
		CASE_TO_STRING(SwapchainError, required_usage_not_supported)
		default:
			return "";
	}
}

Result<SystemInfo> SystemInfo::get_system_info() {
	if (!detail::vulkan_functions().init_vulkan_funcs(nullptr)) {
		return make_error_code(InstanceError::vulkan_unavailable);
	}
	return SystemInfo();
}

Result<SystemInfo> SystemInfo::get_system_info(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {
	// Using externally provided function pointers, assume the loader is available
	if (!detail::vulkan_functions().init_vulkan_funcs(fp_vkGetInstanceProcAddr)) {
		return make_error_code(InstanceError::vulkan_unavailable);
	}
	return SystemInfo();
}

SystemInfo::SystemInfo() {
	auto available_layers_ret = detail::get_vector<VkLayerProperties>(
	    this->available_layers, detail::vulkan_functions().fp_vkEnumerateInstanceLayerProperties);
	if (available_layers_ret != VK_SUCCESS) {
		this->available_layers.clear();
	}

	for (auto& layer : this->available_layers)
		if (strcmp(layer.layerName, detail::validation_layer_name) == 0) validation_layers_available = true;

	auto available_extensions_ret = detail::get_vector<VkExtensionProperties>(
	    this->available_extensions, detail::vulkan_functions().fp_vkEnumerateInstanceExtensionProperties, nullptr);
	if (available_extensions_ret != VK_SUCCESS) {
		this->available_extensions.clear();
	}

	for (auto& ext : this->available_extensions) {
		if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
			debug_utils_available = true;
		}
	}

	for (auto& layer : this->available_layers) {
		std::vector<VkExtensionProperties> layer_extensions;
		auto layer_extensions_ret = detail::get_vector<VkExtensionProperties>(
		    layer_extensions, detail::vulkan_functions().fp_vkEnumerateInstanceExtensionProperties, layer.layerName);
		if (layer_extensions_ret == VK_SUCCESS) {
			this->available_extensions.insert(
			    this->available_extensions.end(), layer_extensions.begin(), layer_extensions.end());
			for (auto& ext : layer_extensions) {
				if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
					debug_utils_available = true;
				}
			}
		}
	}
}
bool SystemInfo::is_extension_available(const char* extension_name) const {
	if (!extension_name) return false;
	return detail::check_extension_supported(available_extensions, extension_name);
}
bool SystemInfo::is_layer_available(const char* layer_name) const {
	if (!layer_name) return false;
	return detail::check_layer_supported(available_layers, layer_name);
}
void destroy_surface(Instance instance, VkSurfaceKHR surface) {
	if (instance.instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
		detail::vulkan_functions().fp_vkDestroySurfaceKHR(instance.instance, surface, instance.allocation_callbacks);
	}
}
void destroy_surface(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks* callbacks) {
	if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
		detail::vulkan_functions().fp_vkDestroySurfaceKHR(instance, surface, callbacks);
	}
}
void destroy_instance(Instance instance) {
	if (instance.instance != VK_NULL_HANDLE) {
		if (instance.debug_messenger != VK_NULL_HANDLE)
			destroy_debug_utils_messenger(instance.instance, instance.debug_messenger, instance.allocation_callbacks);
		detail::vulkan_functions().fp_vkDestroyInstance(instance.instance, instance.allocation_callbacks);
	}
}

Instance::operator VkInstance() const { return this->instance; }

InstanceBuilder::InstanceBuilder(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {
	info.fp_vkGetInstanceProcAddr = fp_vkGetInstanceProcAddr;
}
InstanceBuilder::InstanceBuilder() {}

Result<Instance> InstanceBuilder::build() const {

	auto sys_info_ret = SystemInfo::get_system_info(info.fp_vkGetInstanceProcAddr);
	if (!sys_info_ret) return sys_info_ret.error();
	auto system = sys_info_ret.value();

	uint32_t instance_version = VKB_VK_API_VERSION_1_0;

	if (info.minimum_instance_version > VKB_VK_API_VERSION_1_0 || info.required_api_version > VKB_VK_API_VERSION_1_0 ||
	    info.desired_api_version > VKB_VK_API_VERSION_1_0) {
		PFN_vkEnumerateInstanceVersion pfn_vkEnumerateInstanceVersion = detail::vulkan_functions().fp_vkEnumerateInstanceVersion;

		if (pfn_vkEnumerateInstanceVersion != nullptr) {
			VkResult res = pfn_vkEnumerateInstanceVersion(&instance_version);
			// Should always return VK_SUCCESS
			if (res != VK_SUCCESS && info.required_api_version > 0)
				return make_error_code(InstanceError::vulkan_version_unavailable);
		}
		if (pfn_vkEnumerateInstanceVersion == nullptr || instance_version < info.minimum_instance_version ||
		    (info.minimum_instance_version == 0 && instance_version < info.required_api_version)) {
			if (VK_VERSION_MINOR(info.required_api_version) == 2)
				return make_error_code(InstanceError::vulkan_version_1_2_unavailable);
			else if (VK_VERSION_MINOR(info.required_api_version))
				return make_error_code(InstanceError::vulkan_version_1_1_unavailable);
			else
				return make_error_code(InstanceError::vulkan_version_unavailable);
		}
	}

	uint32_t api_version = instance_version < VKB_VK_API_VERSION_1_1 ? instance_version : info.required_api_version;

	if (info.desired_api_version > VKB_VK_API_VERSION_1_0 && instance_version >= info.desired_api_version) {
		instance_version = info.desired_api_version;
		api_version = info.desired_api_version;
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = nullptr;
	app_info.pApplicationName = info.app_name != nullptr ? info.app_name : "";
	app_info.applicationVersion = info.application_version;
	app_info.pEngineName = info.engine_name != nullptr ? info.engine_name : "";
	app_info.engineVersion = info.engine_version;
	app_info.apiVersion = api_version;

	std::vector<const char*> extensions;
	std::vector<const char*> layers;

	for (auto& ext : info.extensions)
		extensions.push_back(ext);
	if (info.debug_callback != nullptr && system.debug_utils_available) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	bool properties2_ext_enabled =
	    api_version < VKB_VK_API_VERSION_1_1 && detail::check_extension_supported(system.available_extensions,
	                                                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	if (properties2_ext_enabled) {
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

#if defined(VK_KHR_portability_enumeration)
	bool portability_enumeration_support =
	    detail::check_extension_supported(system.available_extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if (portability_enumeration_support) {
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	}
#else
	bool portability_enumeration_support = false;
#endif
	if (!info.headless_context) {
		auto check_add_window_ext = [&](const char* name) -> bool {
			if (!detail::check_extension_supported(system.available_extensions, name)) return false;
			extensions.push_back(name);
			return true;
		};
		bool khr_surface_added = check_add_window_ext("VK_KHR_surface");
#if defined(_WIN32)
		bool added_window_exts = check_add_window_ext("VK_KHR_win32_surface");
#elif defined(__ANDROID__)
		bool added_window_exts = check_add_window_ext("VK_KHR_android_surface");
#elif defined(_DIRECT2DISPLAY)
		bool added_window_exts = check_add_window_ext("VK_KHR_display");
#elif defined(__linux__)
		// make sure all three calls to check_add_window_ext, don't allow short circuiting
		bool added_window_exts = check_add_window_ext("VK_KHR_xcb_surface");
		added_window_exts = check_add_window_ext("VK_KHR_xlib_surface") || added_window_exts;
		added_window_exts = check_add_window_ext("VK_KHR_wayland_surface") || added_window_exts;
#elif defined(__APPLE__)
		bool added_window_exts = check_add_window_ext("VK_EXT_metal_surface");
#endif
		if (!khr_surface_added || !added_window_exts)
			return make_error_code(InstanceError::windowing_extensions_not_present);
	}
	bool all_extensions_supported = detail::check_extensions_supported(system.available_extensions, extensions);
	if (!all_extensions_supported) {
		return make_error_code(InstanceError::requested_extensions_not_present);
	}

	for (auto& layer : info.layers)
		layers.push_back(layer);

	if (info.enable_validation_layers || (info.request_validation_layers && system.validation_layers_available)) {
		layers.push_back(detail::validation_layer_name);
	}
	bool all_layers_supported = detail::check_layers_supported(system.available_layers, layers);
	if (!all_layers_supported) {
		return make_error_code(InstanceError::requested_layers_not_present);
	}

	std::vector<VkBaseOutStructure*> pNext_chain;

	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	if (info.use_debug_messenger) {
		messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		messengerCreateInfo.pNext = nullptr;
		messengerCreateInfo.messageSeverity = info.debug_message_severity;
		messengerCreateInfo.messageType = info.debug_message_type;
		messengerCreateInfo.pfnUserCallback = info.debug_callback;
		messengerCreateInfo.pUserData = info.debug_user_data_pointer;
		pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&messengerCreateInfo));
	}

	VkValidationFeaturesEXT features{};
	if (info.enabled_validation_features.size() != 0 || info.disabled_validation_features.size()) {
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.pNext = nullptr;
		features.enabledValidationFeatureCount = static_cast<uint32_t>(info.enabled_validation_features.size());
		features.pEnabledValidationFeatures = info.enabled_validation_features.data();
		features.disabledValidationFeatureCount = static_cast<uint32_t>(info.disabled_validation_features.size());
		features.pDisabledValidationFeatures = info.disabled_validation_features.data();
		pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features));
	}

	VkValidationFlagsEXT checks{};
	if (info.disabled_validation_checks.size() != 0) {
		checks.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
		checks.pNext = nullptr;
		checks.disabledValidationCheckCount = static_cast<uint32_t>(info.disabled_validation_checks.size());
		checks.pDisabledValidationChecks = info.disabled_validation_checks.data();
		pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&checks));
	}

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	detail::setup_pNext_chain(instance_create_info, pNext_chain);
#if !defined(NDEBUG)
	for (auto& node : pNext_chain) {
		assert(node->sType != VK_STRUCTURE_TYPE_APPLICATION_INFO);
	}
#endif
	instance_create_info.flags = info.flags;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_create_info.ppEnabledExtensionNames = extensions.data();
	instance_create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instance_create_info.ppEnabledLayerNames = layers.data();
#if defined(VK_KHR_portability_enumeration)
	if (portability_enumeration_support) {
		instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

	Instance instance;
	VkResult res =
	    detail::vulkan_functions().fp_vkCreateInstance(&instance_create_info, info.allocation_callbacks, &instance.instance);
	if (res != VK_SUCCESS) return Result<Instance>(InstanceError::failed_create_instance, res);

	detail::vulkan_functions().init_instance_funcs(instance.instance);

	if (info.use_debug_messenger) {
		res = create_debug_utils_messenger(instance.instance,
		    info.debug_callback,
		    info.debug_message_severity,
		    info.debug_message_type,
		    info.debug_user_data_pointer,
		    &instance.debug_messenger,
		    info.allocation_callbacks);
		if (res != VK_SUCCESS) {
			return Result<Instance>(InstanceError::failed_create_debug_messenger, res);
		}
	}

	instance.headless = info.headless_context;
	instance.properties2_ext_enabled = properties2_ext_enabled;
	instance.allocation_callbacks = info.allocation_callbacks;
	instance.instance_version = instance_version;
	instance.api_version = api_version;
	instance.fp_vkGetInstanceProcAddr = detail::vulkan_functions().ptr_vkGetInstanceProcAddr;
	instance.fp_vkGetDeviceProcAddr = detail::vulkan_functions().fp_vkGetDeviceProcAddr;
	return instance;
}

InstanceBuilder& InstanceBuilder::set_app_name(const char* app_name) {
	if (!app_name) return *this;
	info.app_name = app_name;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_engine_name(const char* engine_name) {
	if (!engine_name) return *this;
	info.engine_name = engine_name;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_app_version(uint32_t app_version) {
	info.application_version = app_version;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_app_version(uint32_t major, uint32_t minor, uint32_t patch) {
	info.application_version = VKB_MAKE_VK_VERSION(0, major, minor, patch);
	return *this;
}
InstanceBuilder& InstanceBuilder::set_engine_version(uint32_t engine_version) {
	info.engine_version = engine_version;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_engine_version(uint32_t major, uint32_t minor, uint32_t patch) {
	info.engine_version = VKB_MAKE_VK_VERSION(0, major, minor, patch);
	return *this;
}
InstanceBuilder& InstanceBuilder::require_api_version(uint32_t required_api_version) {
	info.required_api_version = required_api_version;
	return *this;
}
InstanceBuilder& InstanceBuilder::require_api_version(uint32_t major, uint32_t minor, uint32_t patch) {
	info.required_api_version = VKB_MAKE_VK_VERSION(0, major, minor, patch);
	return *this;
}
InstanceBuilder& InstanceBuilder::set_minimum_instance_version(uint32_t minimum_instance_version) {
	info.minimum_instance_version = minimum_instance_version;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_minimum_instance_version(uint32_t major, uint32_t minor, uint32_t patch) {
	info.minimum_instance_version = VKB_MAKE_VK_VERSION(0, major, minor, patch);
	return *this;
}
InstanceBuilder& InstanceBuilder::desire_api_version(uint32_t preferred_vulkan_version) {
	info.desired_api_version = preferred_vulkan_version;
	return *this;
}
InstanceBuilder& InstanceBuilder::desire_api_version(uint32_t major, uint32_t minor, uint32_t patch) {
	info.desired_api_version = VKB_MAKE_VK_VERSION(0, major, minor, patch);
	return *this;
}
InstanceBuilder& InstanceBuilder::enable_layer(const char* layer_name) {
	if (!layer_name) return *this;
	info.layers.push_back(layer_name);
	return *this;
}
InstanceBuilder& InstanceBuilder::enable_extension(const char* extension_name) {
	if (!extension_name) return *this;
	info.extensions.push_back(extension_name);
	return *this;
}
InstanceBuilder& InstanceBuilder::enable_validation_layers(bool enable_validation) {
	info.enable_validation_layers = enable_validation;
	return *this;
}
InstanceBuilder& InstanceBuilder::request_validation_layers(bool enable_validation) {
	info.request_validation_layers = enable_validation;
	return *this;
}
InstanceBuilder& InstanceBuilder::use_default_debug_messenger() {
	info.use_debug_messenger = true;
	info.debug_callback = default_debug_callback;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_debug_callback(PFN_vkDebugUtilsMessengerCallbackEXT callback) {
	info.use_debug_messenger = true;
	info.debug_callback = callback;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_debug_callback_user_data_pointer(void* user_data_pointer) {
	info.debug_user_data_pointer = user_data_pointer;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_headless(bool headless) {
	info.headless_context = headless;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_debug_messenger_severity(VkDebugUtilsMessageSeverityFlagsEXT severity) {
	info.debug_message_severity = severity;
	return *this;
}
InstanceBuilder& InstanceBuilder::add_debug_messenger_severity(VkDebugUtilsMessageSeverityFlagsEXT severity) {
	info.debug_message_severity = info.debug_message_severity | severity;
	return *this;
}
InstanceBuilder& InstanceBuilder::set_debug_messenger_type(VkDebugUtilsMessageTypeFlagsEXT type) {
	info.debug_message_type = type;
	return *this;
}
InstanceBuilder& InstanceBuilder::add_debug_messenger_type(VkDebugUtilsMessageTypeFlagsEXT type) {
	info.debug_message_type = info.debug_message_type | type;
	return *this;
}
InstanceBuilder& InstanceBuilder::add_validation_disable(VkValidationCheckEXT check) {
	info.disabled_validation_checks.push_back(check);
	return *this;
}
InstanceBuilder& InstanceBuilder::add_validation_feature_enable(VkValidationFeatureEnableEXT enable) {
	info.enabled_validation_features.push_back(enable);
	return *this;
}
InstanceBuilder& InstanceBuilder::add_validation_feature_disable(VkValidationFeatureDisableEXT disable) {
	info.disabled_validation_features.push_back(disable);
	return *this;
}
InstanceBuilder& InstanceBuilder::set_allocation_callbacks(VkAllocationCallbacks* callbacks) {
	info.allocation_callbacks = callbacks;
	return *this;
}

void destroy_debug_messenger(VkInstance const instance, VkDebugUtilsMessengerEXT const messenger);


// ---- Physical Device ---- //

namespace detail {

std::vector<std::string> check_device_extension_support(
    std::vector<std::string> const& available_extensions, std::vector<std::string> const& desired_extensions) {
	std::vector<std::string> extensions_to_enable;
	for (const auto& avail_ext : available_extensions) {
		for (auto& req_ext : desired_extensions) {
			if (avail_ext == req_ext) {
				extensions_to_enable.push_back(req_ext);
				break;
			}
		}
	}
	return extensions_to_enable;
}

// clang-format off
bool supports_features(VkPhysicalDeviceFeatures supported,
					   VkPhysicalDeviceFeatures requested,
					   std::vector<GenericFeaturesPNextNode> const& extension_supported,
					   std::vector<GenericFeaturesPNextNode> const& extension_requested) {
	if (requested.robustBufferAccess && !supported.robustBufferAccess) return false;
	if (requested.fullDrawIndexUint32 && !supported.fullDrawIndexUint32) return false;
	if (requested.imageCubeArray && !supported.imageCubeArray) return false;
	if (requested.independentBlend && !supported.independentBlend) return false;
	if (requested.geometryShader && !supported.geometryShader) return false;
	if (requested.tessellationShader && !supported.tessellationShader) return false;
	if (requested.sampleRateShading && !supported.sampleRateShading) return false;
	if (requested.dualSrcBlend && !supported.dualSrcBlend) return false;
	if (requested.logicOp && !supported.logicOp) return false;
	if (requested.multiDrawIndirect && !supported.multiDrawIndirect) return false;
	if (requested.drawIndirectFirstInstance && !supported.drawIndirectFirstInstance) return false;
	if (requested.depthClamp && !supported.depthClamp) return false;
	if (requested.depthBiasClamp && !supported.depthBiasClamp) return false;
	if (requested.fillModeNonSolid && !supported.fillModeNonSolid) return false;
	if (requested.depthBounds && !supported.depthBounds) return false;
	if (requested.wideLines && !supported.wideLines) return false;
	if (requested.largePoints && !supported.largePoints) return false;
	if (requested.alphaToOne && !supported.alphaToOne) return false;
	if (requested.multiViewport && !supported.multiViewport) return false;
	if (requested.samplerAnisotropy && !supported.samplerAnisotropy) return false;
	if (requested.textureCompressionETC2 && !supported.textureCompressionETC2) return false;
	if (requested.textureCompressionASTC_LDR && !supported.textureCompressionASTC_LDR) return false;
	if (requested.textureCompressionBC && !supported.textureCompressionBC) return false;
	if (requested.occlusionQueryPrecise && !supported.occlusionQueryPrecise) return false;
	if (requested.pipelineStatisticsQuery && !supported.pipelineStatisticsQuery) return false;
	if (requested.vertexPipelineStoresAndAtomics && !supported.vertexPipelineStoresAndAtomics) return false;
	if (requested.fragmentStoresAndAtomics && !supported.fragmentStoresAndAtomics) return false;
	if (requested.shaderTessellationAndGeometryPointSize && !supported.shaderTessellationAndGeometryPointSize) return false;
	if (requested.shaderImageGatherExtended && !supported.shaderImageGatherExtended) return false;
	if (requested.shaderStorageImageExtendedFormats && !supported.shaderStorageImageExtendedFormats) return false;
	if (requested.shaderStorageImageMultisample && !supported.shaderStorageImageMultisample) return false;
	if (requested.shaderStorageImageReadWithoutFormat && !supported.shaderStorageImageReadWithoutFormat) return false;
	if (requested.shaderStorageImageWriteWithoutFormat && !supported.shaderStorageImageWriteWithoutFormat) return false;
	if (requested.shaderUniformBufferArrayDynamicIndexing && !supported.shaderUniformBufferArrayDynamicIndexing) return false;
	if (requested.shaderSampledImageArrayDynamicIndexing && !supported.shaderSampledImageArrayDynamicIndexing) return false;
	if (requested.shaderStorageBufferArrayDynamicIndexing && !supported.shaderStorageBufferArrayDynamicIndexing) return false;
	if (requested.shaderStorageImageArrayDynamicIndexing && !supported.shaderStorageImageArrayDynamicIndexing) return false;
	if (requested.shaderClipDistance && !supported.shaderClipDistance) return false;
	if (requested.shaderCullDistance && !supported.shaderCullDistance) return false;
	if (requested.shaderFloat64 && !supported.shaderFloat64) return false;
	if (requested.shaderInt64 && !supported.shaderInt64) return false;
	if (requested.shaderInt16 && !supported.shaderInt16) return false;
	if (requested.shaderResourceResidency && !supported.shaderResourceResidency) return false;
	if (requested.shaderResourceMinLod && !supported.shaderResourceMinLod) return false;
	if (requested.sparseBinding && !supported.sparseBinding) return false;
	if (requested.sparseResidencyBuffer && !supported.sparseResidencyBuffer) return false;
	if (requested.sparseResidencyImage2D && !supported.sparseResidencyImage2D) return false;
	if (requested.sparseResidencyImage3D && !supported.sparseResidencyImage3D) return false;
	if (requested.sparseResidency2Samples && !supported.sparseResidency2Samples) return false;
	if (requested.sparseResidency4Samples && !supported.sparseResidency4Samples) return false;
	if (requested.sparseResidency8Samples && !supported.sparseResidency8Samples) return false;
	if (requested.sparseResidency16Samples && !supported.sparseResidency16Samples) return false;
	if (requested.sparseResidencyAliased && !supported.sparseResidencyAliased) return false;
	if (requested.variableMultisampleRate && !supported.variableMultisampleRate) return false;
	if (requested.inheritedQueries && !supported.inheritedQueries) return false;

	for(size_t i = 0; i < extension_requested.size(); ++i) {
		auto res = GenericFeaturesPNextNode::match(extension_requested[i], extension_supported[i]);
		if(!res) return false;
	}

	return true;
}
// clang-format on
// Finds the first queue which supports the desired operations. Returns QUEUE_INDEX_MAX_VALUE if none is found
uint32_t get_first_queue_index(std::vector<VkQueueFamilyProperties> const& families, VkQueueFlags desired_flags) {
	for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); i++) {
		if ((families[i].queueFlags & desired_flags) == desired_flags) return i;
	}
	return QUEUE_INDEX_MAX_VALUE;
}
// Finds the queue which is separate from the graphics queue and has the desired flag and not the
// undesired flag, but will select it if no better options are available compute support. Returns
// QUEUE_INDEX_MAX_VALUE if none is found.
uint32_t get_separate_queue_index(
    std::vector<VkQueueFamilyProperties> const& families, VkQueueFlags desired_flags, VkQueueFlags undesired_flags) {
	uint32_t index = QUEUE_INDEX_MAX_VALUE;
	for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); i++) {
		if ((families[i].queueFlags & desired_flags) == desired_flags && ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
			if ((families[i].queueFlags & undesired_flags) == 0) {
				return i;
			} else {
				index = i;
			}
		}
	}
	return index;
}

// finds the first queue which supports only the desired flag (not graphics or transfer). Returns QUEUE_INDEX_MAX_VALUE if none is found.
uint32_t get_dedicated_queue_index(
    std::vector<VkQueueFamilyProperties> const& families, VkQueueFlags desired_flags, VkQueueFlags undesired_flags) {
	for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); i++) {
		if ((families[i].queueFlags & desired_flags) == desired_flags &&
		    (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 && (families[i].queueFlags & undesired_flags) == 0)
			return i;
	}
	return QUEUE_INDEX_MAX_VALUE;
}

// finds the first queue which supports presenting. returns QUEUE_INDEX_MAX_VALUE if none is found
uint32_t get_present_queue_index(
    VkPhysicalDevice const phys_device, VkSurfaceKHR const surface, std::vector<VkQueueFamilyProperties> const& families) {
	for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); i++) {
		VkBool32 presentSupport = false;
		if (surface != VK_NULL_HANDLE) {
			VkResult res = detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, i, surface, &presentSupport);
			if (res != VK_SUCCESS) return QUEUE_INDEX_MAX_VALUE; // TODO: determine if this should fail another way
		}
		if (presentSupport == VK_TRUE) return i;
	}
	return QUEUE_INDEX_MAX_VALUE;
}
} // namespace detail

PhysicalDevice PhysicalDeviceSelector::populate_device_details(VkPhysicalDevice vk_phys_device,
    std::vector<detail::GenericFeaturesPNextNode> const& src_extended_features_chain) const {
	PhysicalDevice physical_device{};
	physical_device.physical_device = vk_phys_device;
	physical_device.surface = instance_info.surface;
	physical_device.defer_surface_initialization = criteria.defer_surface_initialization;
	physical_device.instance_version = instance_info.version;
	auto queue_families = detail::get_vector_noerror<VkQueueFamilyProperties>(
	    detail::vulkan_functions().fp_vkGetPhysicalDeviceQueueFamilyProperties, vk_phys_device);
	physical_device.queue_families = queue_families;

	detail::vulkan_functions().fp_vkGetPhysicalDeviceProperties(vk_phys_device, &physical_device.properties);
	detail::vulkan_functions().fp_vkGetPhysicalDeviceFeatures(vk_phys_device, &physical_device.features);
	detail::vulkan_functions().fp_vkGetPhysicalDeviceMemoryProperties(vk_phys_device, &physical_device.memory_properties);

	physical_device.name = physical_device.properties.deviceName;

	std::vector<VkExtensionProperties> available_extensions;
	auto available_extensions_ret = detail::get_vector<VkExtensionProperties>(
	    available_extensions, detail::vulkan_functions().fp_vkEnumerateDeviceExtensionProperties, vk_phys_device, nullptr);
	if (available_extensions_ret != VK_SUCCESS) return physical_device;
	for (const auto& ext : available_extensions) {
		physical_device.extensions.push_back(&ext.extensionName[0]);
	}

	physical_device.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2; // same value as the non-KHR version
	physical_device.properties2_ext_enabled = instance_info.properties2_ext_enabled;

	auto fill_chain = src_extended_features_chain;

	bool instance_is_1_1 = instance_info.version >= VKB_VK_API_VERSION_1_1;
	if (!fill_chain.empty() && (instance_is_1_1 || instance_info.properties2_ext_enabled)) {

		detail::GenericFeaturesPNextNode* prev = nullptr;
		for (auto& extension : fill_chain) {
			if (prev != nullptr) {
				prev->pNext = &extension;
			}
			prev = &extension;
		}

		VkPhysicalDeviceFeatures2 local_features{};
		local_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2; // KHR is same as core here
		local_features.pNext = &fill_chain.front();
		// Use KHR function if not able to use the core function
		if (instance_is_1_1) {
			detail::vulkan_functions().fp_vkGetPhysicalDeviceFeatures2(vk_phys_device, &local_features);
		} else {
			detail::vulkan_functions().fp_vkGetPhysicalDeviceFeatures2KHR(vk_phys_device, &local_features);
		}
		physical_device.extended_features_chain = fill_chain;
	}

	return physical_device;
}

PhysicalDevice::Suitable PhysicalDeviceSelector::is_device_suitable(PhysicalDevice const& pd) const {
	PhysicalDevice::Suitable suitable = PhysicalDevice::Suitable::yes;

	if (criteria.name.size() > 0 && criteria.name != pd.properties.deviceName) return PhysicalDevice::Suitable::no;

	if (criteria.required_version > pd.properties.apiVersion) return PhysicalDevice::Suitable::no;
	if (criteria.desired_version > pd.properties.apiVersion) suitable = PhysicalDevice::Suitable::partial;

	bool dedicated_compute = detail::get_dedicated_queue_index(pd.queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT) !=
	                         detail::QUEUE_INDEX_MAX_VALUE;
	bool dedicated_transfer = detail::get_dedicated_queue_index(pd.queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT) !=
	                          detail::QUEUE_INDEX_MAX_VALUE;
	bool separate_compute = detail::get_separate_queue_index(pd.queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT) !=
	                        detail::QUEUE_INDEX_MAX_VALUE;
	bool separate_transfer = detail::get_separate_queue_index(pd.queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT) !=
	                         detail::QUEUE_INDEX_MAX_VALUE;

	bool present_queue = detail::get_present_queue_index(pd.physical_device, instance_info.surface, pd.queue_families) !=
	                     detail::QUEUE_INDEX_MAX_VALUE;

	if (criteria.require_dedicated_compute_queue && !dedicated_compute) return PhysicalDevice::Suitable::no;
	if (criteria.require_dedicated_transfer_queue && !dedicated_transfer) return PhysicalDevice::Suitable::no;
	if (criteria.require_separate_compute_queue && !separate_compute) return PhysicalDevice::Suitable::no;
	if (criteria.require_separate_transfer_queue && !separate_transfer) return PhysicalDevice::Suitable::no;
	if (criteria.require_present && !present_queue && !criteria.defer_surface_initialization)
		return PhysicalDevice::Suitable::no;

	auto required_extensions_supported = detail::check_device_extension_support(pd.extensions, criteria.required_extensions);
	if (required_extensions_supported.size() != criteria.required_extensions.size())
		return PhysicalDevice::Suitable::no;

	auto desired_extensions_supported = detail::check_device_extension_support(pd.extensions, criteria.desired_extensions);
	if (desired_extensions_supported.size() != criteria.desired_extensions.size())
		suitable = PhysicalDevice::Suitable::partial;

	if (!criteria.defer_surface_initialization && criteria.require_present) {
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;

		auto formats_ret = detail::get_vector<VkSurfaceFormatKHR>(formats,
		    detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfaceFormatsKHR,
		    pd.physical_device,
		    instance_info.surface);
		auto present_modes_ret = detail::get_vector<VkPresentModeKHR>(present_modes,
		    detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfacePresentModesKHR,
		    pd.physical_device,
		    instance_info.surface);

		if (formats_ret != VK_SUCCESS || present_modes_ret != VK_SUCCESS || formats.empty() || present_modes.empty()) {
			return PhysicalDevice::Suitable::no;
		}
	}

	if (!criteria.allow_any_type && pd.properties.deviceType != static_cast<VkPhysicalDeviceType>(criteria.preferred_type)) {
		suitable = PhysicalDevice::Suitable::partial;
	}

	bool required_features_supported = detail::supports_features(
	    pd.features, criteria.required_features, pd.extended_features_chain, criteria.extended_features_chain);
	if (!required_features_supported) return PhysicalDevice::Suitable::no;

	for (uint32_t i = 0; i < pd.memory_properties.memoryHeapCount; i++) {
		if (pd.memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			if (pd.memory_properties.memoryHeaps[i].size < criteria.required_mem_size) {
				return PhysicalDevice::Suitable::no;
			} else if (pd.memory_properties.memoryHeaps[i].size < criteria.desired_mem_size) {
				suitable = PhysicalDevice::Suitable::partial;
			}
		}
	}

	return suitable;
}
// delegate construction to the one with an explicit surface parameter
PhysicalDeviceSelector::PhysicalDeviceSelector(Instance const& instance)
: PhysicalDeviceSelector(instance, VK_NULL_HANDLE) {}

PhysicalDeviceSelector::PhysicalDeviceSelector(Instance const& instance, VkSurfaceKHR surface) {
	instance_info.instance = instance.instance;
	instance_info.version = instance.instance_version;
	instance_info.properties2_ext_enabled = instance.properties2_ext_enabled;
	instance_info.surface = surface;
	criteria.require_present = !instance.headless;
	criteria.required_version = instance.api_version;
	criteria.desired_version = instance.api_version;
}

Result<std::vector<PhysicalDevice>> PhysicalDeviceSelector::select_impl(DeviceSelectionMode selection) const {
#if !defined(NDEBUG)
	// Validation
	for (const auto& node : criteria.extended_features_chain) {
		assert(node.sType != static_cast<VkStructureType>(0) &&
		       "Features struct sType must be filled with the struct's "
		       "corresponding VkStructureType enum");
		assert(node.sType != VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 &&
		       "Do not pass VkPhysicalDeviceFeatures2 as a required extension feature structure. An "
		       "instance of this is managed internally for selection criteria and device creation.");
	}
#endif

	if (criteria.require_present && !criteria.defer_surface_initialization) {
		if (instance_info.surface == VK_NULL_HANDLE)
			return Result<std::vector<PhysicalDevice>>{ PhysicalDeviceError::no_surface_provided };
	}

	// Get the VkPhysicalDevice handles on the system
	std::vector<VkPhysicalDevice> vk_physical_devices;

	auto vk_physical_devices_ret = detail::get_vector<VkPhysicalDevice>(
	    vk_physical_devices, detail::vulkan_functions().fp_vkEnumeratePhysicalDevices, instance_info.instance);
	if (vk_physical_devices_ret != VK_SUCCESS) {
		return Result<std::vector<PhysicalDevice>>{ PhysicalDeviceError::failed_enumerate_physical_devices, vk_physical_devices_ret };
	}
	if (vk_physical_devices.size() == 0) {
		return Result<std::vector<PhysicalDevice>>{ PhysicalDeviceError::no_physical_devices_found };
	}

	auto fill_out_phys_dev_with_criteria = [&](PhysicalDevice& phys_dev) {
		phys_dev.features = criteria.required_features;
		phys_dev.extended_features_chain = criteria.extended_features_chain;
		bool portability_ext_available = false;
		for (const auto& ext : phys_dev.extensions)
			if (criteria.enable_portability_subset && ext == "VK_KHR_portability_subset")
				portability_ext_available = true;

		auto desired_extensions_supported = detail::check_device_extension_support(phys_dev.extensions, criteria.desired_extensions);

		phys_dev.extensions.clear();
		phys_dev.extensions.insert(
		    phys_dev.extensions.end(), criteria.required_extensions.begin(), criteria.required_extensions.end());
		phys_dev.extensions.insert(
		    phys_dev.extensions.end(), desired_extensions_supported.begin(), desired_extensions_supported.end());
		if (portability_ext_available) {
			phys_dev.extensions.push_back("VK_KHR_portability_subset");
		}
	};

	// if this option is set, always return only the first physical device found
	if (criteria.use_first_gpu_unconditionally && vk_physical_devices.size() > 0) {
		PhysicalDevice physical_device = populate_device_details(vk_physical_devices[0], criteria.extended_features_chain);
		fill_out_phys_dev_with_criteria(physical_device);
		return std::vector<PhysicalDevice>{ physical_device };
	}

	// Populate their details and check their suitability
	std::vector<PhysicalDevice> physical_devices;
	for (auto& vk_physical_device : vk_physical_devices) {
		PhysicalDevice phys_dev = populate_device_details(vk_physical_device, criteria.extended_features_chain);
		phys_dev.suitable = is_device_suitable(phys_dev);
		if (phys_dev.suitable != PhysicalDevice::Suitable::no) {
			physical_devices.push_back(phys_dev);
		}
	}

	// sort the list into fully and partially suitable devices. use stable_partition to maintain relative order
	const auto partition_index = std::stable_partition(physical_devices.begin(), physical_devices.end(), [](auto const& pd) {
		return pd.suitable == PhysicalDevice::Suitable::yes;
	});

	// Remove the partially suitable elements if they aren't desired
	if (selection == DeviceSelectionMode::only_fully_suitable) {
		physical_devices.erase(partition_index, physical_devices.end());
	}

	// Make the physical device ready to be used to create a Device from it
	for (auto& physical_device : physical_devices) {
		fill_out_phys_dev_with_criteria(physical_device);
	}

	return physical_devices;
}

Result<PhysicalDevice> PhysicalDeviceSelector::select(DeviceSelectionMode selection) const {
	auto const selected_devices = select_impl(selection);

	if (!selected_devices) return Result<PhysicalDevice>{ selected_devices.error() };
	if (selected_devices.value().size() == 0) {
		return Result<PhysicalDevice>{ PhysicalDeviceError::no_suitable_device };
	}

	return selected_devices.value().at(0);
}

// Return all devices which are considered suitable - intended for applications which want to let the user pick the physical device
Result<std::vector<PhysicalDevice>> PhysicalDeviceSelector::select_devices(DeviceSelectionMode selection) const {
	auto const selected_devices = select_impl(selection);
	if (!selected_devices) return Result<std::vector<PhysicalDevice>>{ selected_devices.error() };
	if (selected_devices.value().size() == 0) {
		return Result<std::vector<PhysicalDevice>>{ PhysicalDeviceError::no_suitable_device };
	}
	return selected_devices.value();
}

Result<std::vector<std::string>> PhysicalDeviceSelector::select_device_names(DeviceSelectionMode selection) const {
	auto const selected_devices = select_impl(selection);
	if (!selected_devices) return Result<std::vector<std::string>>{ selected_devices.error() };
	if (selected_devices.value().size() == 0) {
		return Result<std::vector<std::string>>{ PhysicalDeviceError::no_suitable_device };
	}
	std::vector<std::string> names;
	for (const auto& pd : selected_devices.value()) {
		names.push_back(pd.name);
	}
	return names;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::set_surface(VkSurfaceKHR surface) {
	instance_info.surface = surface;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::set_name(std::string const& name) {
	criteria.name = name;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::prefer_gpu_device_type(PreferredDeviceType type) {
	criteria.preferred_type = type;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::allow_any_gpu_device_type(bool allow_any_type) {
	criteria.allow_any_type = allow_any_type;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::require_present(bool require) {
	criteria.require_present = require;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::require_dedicated_transfer_queue() {
	criteria.require_dedicated_transfer_queue = true;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::require_dedicated_compute_queue() {
	criteria.require_dedicated_compute_queue = true;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::require_separate_transfer_queue() {
	criteria.require_separate_transfer_queue = true;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::require_separate_compute_queue() {
	criteria.require_separate_compute_queue = true;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::required_device_memory_size(VkDeviceSize size) {
	criteria.required_mem_size = size;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::desired_device_memory_size(VkDeviceSize size) {
	criteria.desired_mem_size = size;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::add_required_extension(const char* extension) {
	criteria.required_extensions.push_back(extension);
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::add_required_extensions(std::vector<const char*> extensions) {
	for (const auto& ext : extensions) {
		criteria.required_extensions.push_back(ext);
	}
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::add_desired_extension(const char* extension) {
	criteria.desired_extensions.push_back(extension);
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::add_desired_extensions(std::vector<const char*> extensions) {
	for (const auto& ext : extensions) {
		criteria.desired_extensions.push_back(ext);
	}
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::set_minimum_version(uint32_t major, uint32_t minor) {
	criteria.required_version = VKB_MAKE_VK_VERSION(0, major, minor, 0);
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::set_desired_version(uint32_t major, uint32_t minor) {
	criteria.desired_version = VKB_MAKE_VK_VERSION(0, major, minor, 0);
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::disable_portability_subset() {
	criteria.enable_portability_subset = false;
	return *this;
}

PhysicalDeviceSelector& PhysicalDeviceSelector::set_required_features(VkPhysicalDeviceFeatures const& features) {
	criteria.required_features = features;
	return *this;
}
#if defined(VKB_VK_API_VERSION_1_2)
// Just calls add_required_features
PhysicalDeviceSelector& PhysicalDeviceSelector::set_required_features_11(VkPhysicalDeviceVulkan11Features features_11) {
	features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	add_required_extension_features(features_11);
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::set_required_features_12(VkPhysicalDeviceVulkan12Features features_12) {
	features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	add_required_extension_features(features_12);
	return *this;
}
#endif
#if defined(VKB_VK_API_VERSION_1_3)
PhysicalDeviceSelector& PhysicalDeviceSelector::set_required_features_13(VkPhysicalDeviceVulkan13Features features_13) {
	features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	add_required_extension_features(features_13);
	return *this;
}
#endif
PhysicalDeviceSelector& PhysicalDeviceSelector::defer_surface_initialization() {
	criteria.defer_surface_initialization = true;
	return *this;
}
PhysicalDeviceSelector& PhysicalDeviceSelector::select_first_device_unconditionally(bool unconditionally) {
	criteria.use_first_gpu_unconditionally = unconditionally;
	return *this;
}

// PhysicalDevice
bool PhysicalDevice::has_dedicated_compute_queue() const {
	return detail::get_dedicated_queue_index(queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT) != detail::QUEUE_INDEX_MAX_VALUE;
}
bool PhysicalDevice::has_separate_compute_queue() const {
	return detail::get_separate_queue_index(queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT) != detail::QUEUE_INDEX_MAX_VALUE;
}
bool PhysicalDevice::has_dedicated_transfer_queue() const {
	return detail::get_dedicated_queue_index(queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT) != detail::QUEUE_INDEX_MAX_VALUE;
}
bool PhysicalDevice::has_separate_transfer_queue() const {
	return detail::get_separate_queue_index(queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT) != detail::QUEUE_INDEX_MAX_VALUE;
}
std::vector<VkQueueFamilyProperties> PhysicalDevice::get_queue_families() const { return queue_families; }
std::vector<std::string> PhysicalDevice::get_extensions() const { return extensions; }
PhysicalDevice::operator VkPhysicalDevice() const { return this->physical_device; }

// ---- Queues ---- //

Result<uint32_t> Device::get_queue_index(QueueType type) const {
	uint32_t index = detail::QUEUE_INDEX_MAX_VALUE;
	switch (type) {
		case QueueType::present:
			index = detail::get_present_queue_index(physical_device.physical_device, surface, queue_families);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::present_unavailable };
			break;
		case QueueType::graphics:
			index = detail::get_first_queue_index(queue_families, VK_QUEUE_GRAPHICS_BIT);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::graphics_unavailable };
			break;
		case QueueType::compute:
			index = detail::get_separate_queue_index(queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::compute_unavailable };
			break;
		case QueueType::transfer:
			index = detail::get_separate_queue_index(queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::transfer_unavailable };
			break;
		default:
			return Result<uint32_t>{ QueueError::invalid_queue_family_index };
	}
	return index;
}
Result<uint32_t> Device::get_dedicated_queue_index(QueueType type) const {
	uint32_t index = detail::QUEUE_INDEX_MAX_VALUE;
	switch (type) {
		case QueueType::compute:
			index = detail::get_dedicated_queue_index(queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::compute_unavailable };
			break;
		case QueueType::transfer:
			index = detail::get_dedicated_queue_index(queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT);
			if (index == detail::QUEUE_INDEX_MAX_VALUE) return Result<uint32_t>{ QueueError::transfer_unavailable };
			break;
		default:
			return Result<uint32_t>{ QueueError::invalid_queue_family_index };
	}
	return index;
}

Result<VkQueue> Device::get_queue(QueueType type) const {
	auto index = get_queue_index(type);
	if (!index.has_value()) return { index.error() };
	VkQueue out_queue;
	internal_table.fp_vkGetDeviceQueue(device, index.value(), 0, &out_queue);
	return out_queue;
}
Result<VkQueue> Device::get_dedicated_queue(QueueType type) const {
	auto index = get_dedicated_queue_index(type);
	if (!index.has_value()) return { index.error() };
	VkQueue out_queue;
	internal_table.fp_vkGetDeviceQueue(device, index.value(), 0, &out_queue);
	return out_queue;
}

// ---- Dispatch ---- //

DispatchTable Device::make_table() const { return { device, fp_vkGetDeviceProcAddr }; }

// ---- Device ---- //

Device::operator VkDevice() const { return this->device; }

CustomQueueDescription::CustomQueueDescription(uint32_t index, uint32_t count, std::vector<float> priorities)
: index(index), count(count), priorities(priorities) {
	assert(count == priorities.size());
}

void destroy_device(Device device) {
	device.internal_table.fp_vkDestroyDevice(device.device, device.allocation_callbacks);
}

DeviceBuilder::DeviceBuilder(PhysicalDevice phys_device) { physical_device = phys_device; }

Result<Device> DeviceBuilder::build() const {

	std::vector<CustomQueueDescription> queue_descriptions;
	queue_descriptions.insert(queue_descriptions.end(), info.queue_descriptions.begin(), info.queue_descriptions.end());

	if (queue_descriptions.size() == 0) {
		for (uint32_t i = 0; i < physical_device.queue_families.size(); i++) {
			queue_descriptions.push_back(CustomQueueDescription{ i, 1, std::vector<float>{ 1.0f } });
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (auto& desc : queue_descriptions) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = desc.index;
		queue_create_info.queueCount = desc.count;
		queue_create_info.pQueuePriorities = desc.priorities.data();
		queueCreateInfos.push_back(queue_create_info);
	}

	std::vector<const char*> extensions;
	for (const auto& ext : physical_device.extensions) {
		extensions.push_back(ext.c_str());
	}
	if (physical_device.surface != VK_NULL_HANDLE || physical_device.defer_surface_initialization) {
		extensions.push_back({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });
		//extensions.push_back({ VK_KHR_MAINTENANCE_4_EXTENSION_NAME });
	}

	std::vector<VkBaseOutStructure*> final_pnext_chain;
	VkDeviceCreateInfo device_create_info = {};



	bool user_defined_phys_dev_features_2 = false;
	for (auto& pnext : info.pNext_chain) {
		if (pnext->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2) {
			user_defined_phys_dev_features_2 = true;
			break;
		}
	}

	if (user_defined_phys_dev_features_2 && physical_device.extended_features_chain.size() > 0) {
		return { DeviceError::VkPhysicalDeviceFeatures2_in_pNext_chain_while_using_add_required_extension_features };
	}

	// These objects must be alive during the call to vkCreateDevice
	auto physical_device_extension_features_copy = physical_device.extended_features_chain;
	VkPhysicalDeviceFeatures2 local_features2{};
	local_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	if (!user_defined_phys_dev_features_2) {
		if (physical_device.instance_version >= VKB_VK_API_VERSION_1_1 || physical_device.properties2_ext_enabled) {
			local_features2.features = physical_device.features;
			final_pnext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&local_features2));
			for (auto& features_node : physical_device_extension_features_copy) {
				final_pnext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features_node));
			}
		} else {
			// Only set device_create_info.pEnabledFeatures when the pNext chain does not contain a VkPhysicalDeviceFeatures2 structure
			device_create_info.pEnabledFeatures = &physical_device.features;
		}
	}

	for (auto& pnext : info.pNext_chain) {
		final_pnext_chain.push_back(pnext);
	}

	detail::setup_pNext_chain(device_create_info, final_pnext_chain);
#if !defined(NDEBUG)
	for (auto& node : final_pnext_chain) {
		assert(node->sType != VK_STRUCTURE_TYPE_APPLICATION_INFO);
	}
#endif
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.flags = info.flags;
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	device_create_info.pQueueCreateInfos = queueCreateInfos.data();
	device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	device_create_info.ppEnabledExtensionNames = extensions.data();



	//std::cout << "extensions.size(): " << extensions.size() << "\n";



	Device device;

	VkResult res = detail::vulkan_functions().fp_vkCreateDevice(
	    physical_device.physical_device, &device_create_info, info.allocation_callbacks, &device.device);
	if (res != VK_SUCCESS) {
		return { DeviceError::failed_create_device, res };
	}

	device.physical_device = physical_device;
	device.surface = physical_device.surface;
	device.queue_families = physical_device.queue_families;
	device.allocation_callbacks = info.allocation_callbacks;
	device.fp_vkGetDeviceProcAddr = detail::vulkan_functions().fp_vkGetDeviceProcAddr;
	detail::vulkan_functions().get_device_proc_addr(device.device, device.internal_table.fp_vkGetDeviceQueue, "vkGetDeviceQueue");
	detail::vulkan_functions().get_device_proc_addr(device.device, device.internal_table.fp_vkDestroyDevice, "vkDestroyDevice");
	device.instance_version = physical_device.instance_version;
	return device;
}
DeviceBuilder& DeviceBuilder::custom_queue_setup(std::vector<CustomQueueDescription> queue_descriptions) {
	info.queue_descriptions = queue_descriptions;
	return *this;
}
DeviceBuilder& DeviceBuilder::set_allocation_callbacks(VkAllocationCallbacks* callbacks) {
	info.allocation_callbacks = callbacks;
	return *this;
}

// ---- Swapchain ---- //

namespace detail {
struct SurfaceSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

enum class SurfaceSupportError {
	surface_handle_null,
	failed_get_surface_capabilities,
	failed_enumerate_surface_formats,
	failed_enumerate_present_modes,
	no_suitable_desired_format
};

struct SurfaceSupportErrorCategory : std::error_category {
	const char* name() const noexcept override { return "vbk_surface_support"; }
	std::string message(int err) const override {
		switch (static_cast<SurfaceSupportError>(err)) {
			CASE_TO_STRING(SurfaceSupportError, surface_handle_null)
			CASE_TO_STRING(SurfaceSupportError, failed_get_surface_capabilities)
			CASE_TO_STRING(SurfaceSupportError, failed_enumerate_surface_formats)
			CASE_TO_STRING(SurfaceSupportError, failed_enumerate_present_modes)
			CASE_TO_STRING(SurfaceSupportError, no_suitable_desired_format)
			default:
				return "";
		}
	}
};
const SurfaceSupportErrorCategory surface_support_error_category;

std::error_code make_error_code(SurfaceSupportError surface_support_error) {
	return { static_cast<int>(surface_support_error), detail::surface_support_error_category };
}

Result<SurfaceSupportDetails> query_surface_support_details(VkPhysicalDevice phys_device, VkSurfaceKHR surface) {
	if (surface == VK_NULL_HANDLE) return make_error_code(SurfaceSupportError::surface_handle_null);

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult res = detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &capabilities);
	if (res != VK_SUCCESS) {
		return { make_error_code(SurfaceSupportError::failed_get_surface_capabilities), res };
	}

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	auto formats_ret = detail::get_vector<VkSurfaceFormatKHR>(
	    formats, detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfaceFormatsKHR, phys_device, surface);
	if (formats_ret != VK_SUCCESS)
		return { make_error_code(SurfaceSupportError::failed_enumerate_surface_formats), formats_ret };
	auto present_modes_ret = detail::get_vector<VkPresentModeKHR>(
	    present_modes, detail::vulkan_functions().fp_vkGetPhysicalDeviceSurfacePresentModesKHR, phys_device, surface);
	if (present_modes_ret != VK_SUCCESS)
		return { make_error_code(SurfaceSupportError::failed_enumerate_present_modes), present_modes_ret };

	return SurfaceSupportDetails{ capabilities, formats, present_modes };
}

Result<VkSurfaceFormatKHR> find_desired_surface_format(
    std::vector<VkSurfaceFormatKHR> const& available_formats, std::vector<VkSurfaceFormatKHR> const& desired_formats) {
	for (auto const& desired_format : desired_formats) {
		for (auto const& available_format : available_formats) {
			// finds the first format that is desired and available
			if (desired_format.format == available_format.format && desired_format.colorSpace == available_format.colorSpace) {
				return desired_format;
			}
		}
	}

	// if no desired format is available, we report that no format is suitable to the user request
	return { make_error_code(SurfaceSupportError::no_suitable_desired_format) };
}

VkSurfaceFormatKHR find_best_surface_format(
    std::vector<VkSurfaceFormatKHR> const& available_formats, std::vector<VkSurfaceFormatKHR> const& desired_formats) {
	auto surface_format_ret = detail::find_desired_surface_format(available_formats, desired_formats);
	if (surface_format_ret.has_value()) return surface_format_ret.value();

	// use the first available format as a fallback if any desired formats aren't found
	return available_formats[0];
}

VkPresentModeKHR find_present_mode(std::vector<VkPresentModeKHR> const& available_resent_modes,
    std::vector<VkPresentModeKHR> const& desired_present_modes) {
	for (auto const& desired_pm : desired_present_modes) {
		for (auto const& available_pm : available_resent_modes) {
			// finds the first present mode that is desired and available
			if (desired_pm == available_pm) return desired_pm;
		}
	}
	// only present mode required, use as a fallback
	return VK_PRESENT_MODE_FIFO_KHR;
}

template <typename T> T minimum(T a, T b) { return a < b ? a : b; }
template <typename T> T maximum(T a, T b) { return a > b ? a : b; }

VkExtent2D find_extent(VkSurfaceCapabilitiesKHR const& capabilities, uint32_t desired_width, uint32_t desired_height) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = { desired_width, desired_height };

		actualExtent.width =
		    maximum(capabilities.minImageExtent.width, minimum(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height =
		    maximum(capabilities.minImageExtent.height, minimum(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
} // namespace detail

void destroy_swapchain(Swapchain const& swapchain) {
	if (swapchain.device != VK_NULL_HANDLE && swapchain.swapchain != VK_NULL_HANDLE) {
		swapchain.internal_table.fp_vkDestroySwapchainKHR(swapchain.device, swapchain.swapchain, swapchain.allocation_callbacks);
	}
}

SwapchainBuilder::SwapchainBuilder(Device const& device) {
	info.physical_device = device.physical_device.physical_device;
	info.device = device.device;
	info.surface = device.surface;
	info.instance_version = device.instance_version;
	auto present = device.get_queue_index(QueueType::present);
	auto graphics = device.get_queue_index(QueueType::graphics);
	assert(graphics.has_value() && present.has_value() && "Graphics and Present queue indexes must be valid");
	info.graphics_queue_index = present.value();
	info.present_queue_index = graphics.value();
	info.allocation_callbacks = device.allocation_callbacks;
}
SwapchainBuilder::SwapchainBuilder(Device const& device, VkSurfaceKHR const surface) {
	info.physical_device = device.physical_device.physical_device;
	info.device = device.device;
	info.surface = surface;
	info.instance_version = device.instance_version;
	Device temp_device = device;
	temp_device.surface = surface;
	auto present = temp_device.get_queue_index(QueueType::present);
	auto graphics = temp_device.get_queue_index(QueueType::graphics);
	assert(graphics.has_value() && present.has_value() && "Graphics and Present queue indexes must be valid");
	info.graphics_queue_index = graphics.value();
	info.present_queue_index = present.value();
	info.allocation_callbacks = device.allocation_callbacks;
}
SwapchainBuilder::SwapchainBuilder(VkPhysicalDevice const physical_device,
    VkDevice const device,
    VkSurfaceKHR const surface,
    uint32_t graphics_queue_index,
    uint32_t present_queue_index) {
	info.physical_device = physical_device;
	info.device = device;
	info.surface = surface;
	info.graphics_queue_index = graphics_queue_index;
	info.present_queue_index = present_queue_index;
	if (graphics_queue_index == detail::QUEUE_INDEX_MAX_VALUE || present_queue_index == detail::QUEUE_INDEX_MAX_VALUE) {
		auto queue_families = detail::get_vector_noerror<VkQueueFamilyProperties>(
		    detail::vulkan_functions().fp_vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
		if (graphics_queue_index == detail::QUEUE_INDEX_MAX_VALUE)
			info.graphics_queue_index = detail::get_first_queue_index(queue_families, VK_QUEUE_GRAPHICS_BIT);
		if (present_queue_index == detail::QUEUE_INDEX_MAX_VALUE)
			info.present_queue_index = detail::get_present_queue_index(physical_device, surface, queue_families);
	}
}
Result<Swapchain> SwapchainBuilder::build() const {
	if (info.surface == VK_NULL_HANDLE) {
		return Error{ SwapchainError::surface_handle_not_provided };
	}

	

	auto desired_formats = info.desired_formats;
	if (desired_formats.size() == 0) add_desired_formats(desired_formats);
	auto desired_present_modes = info.desired_present_modes;
	if (desired_present_modes.size() == 0) add_desired_present_modes(desired_present_modes);

	auto surface_support_ret = detail::query_surface_support_details(info.physical_device, info.surface);
	if (!surface_support_ret.has_value())
		return Error{ SwapchainError::failed_query_surface_support_details, surface_support_ret.vk_result() };
	auto surface_support = surface_support_ret.value();

	uint32_t image_count = info.min_image_count;
	if (info.required_min_image_count >= 1) {
		if (info.required_min_image_count < surface_support.capabilities.minImageCount)
			return make_error_code(SwapchainError::required_min_image_count_too_low);

		image_count = info.required_min_image_count;
	} else if (info.min_image_count == 0) {
		// We intentionally use minImageCount + 1 to maintain existing behavior, even if it typically results in triple buffering on most systems.
		image_count = surface_support.capabilities.minImageCount + 1;
	} else {
		image_count = info.min_image_count;
		if (image_count < surface_support.capabilities.minImageCount)
			image_count = surface_support.capabilities.minImageCount;
	}
	if (surface_support.capabilities.maxImageCount > 0 && image_count > surface_support.capabilities.maxImageCount) {
		image_count = surface_support.capabilities.maxImageCount;
	}

	VkSurfaceFormatKHR surface_format = detail::find_best_surface_format(surface_support.formats, desired_formats);

	VkExtent2D extent = detail::find_extent(surface_support.capabilities, info.desired_width, info.desired_height);

	uint32_t image_array_layers = info.array_layer_count;
	if (surface_support.capabilities.maxImageArrayLayers < info.array_layer_count)
		image_array_layers = surface_support.capabilities.maxImageArrayLayers;
	if (info.array_layer_count == 0) image_array_layers = 1;

	uint32_t queue_family_indices[] = { info.graphics_queue_index, info.present_queue_index };


	VkPresentModeKHR present_mode = detail::find_present_mode(surface_support.present_modes, desired_present_modes);

	// VkSurfaceCapabilitiesKHR::supportedUsageFlags is only only valid for some present modes. For shared present modes, we should also check VkSharedPresentSurfaceCapabilitiesKHR::sharedPresentSupportedUsageFlags.
	auto is_unextended_present_mode = [](VkPresentModeKHR present_mode) {
		return (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) || (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) ||
		       (present_mode == VK_PRESENT_MODE_FIFO_KHR) || (present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR);
	};

	if (is_unextended_present_mode(present_mode) &&
	    (info.image_usage_flags & surface_support.capabilities.supportedUsageFlags) != info.image_usage_flags) {
		return Error{ SwapchainError::required_usage_not_supported };
	}

	VkSurfaceTransformFlagBitsKHR pre_transform = info.pre_transform;
	if (info.pre_transform == static_cast<VkSurfaceTransformFlagBitsKHR>(0))
		pre_transform = surface_support.capabilities.currentTransform;

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	detail::setup_pNext_chain(swapchain_create_info, info.pNext_chain);
#if !defined(NDEBUG)
	for (auto& node : info.pNext_chain) {
		assert(node->sType != VK_STRUCTURE_TYPE_APPLICATION_INFO);
	}
#endif
	swapchain_create_info.flags = info.create_flags;
	swapchain_create_info.surface = info.surface;
	swapchain_create_info.minImageCount = image_count;
	swapchain_create_info.imageFormat = surface_format.format;
	swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	swapchain_create_info.imageExtent = extent;
	swapchain_create_info.imageArrayLayers = image_array_layers;
	swapchain_create_info.imageUsage = info.image_usage_flags;

	if (info.graphics_queue_index != info.present_queue_index) {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapchain_create_info.preTransform = pre_transform;
	swapchain_create_info.compositeAlpha = info.composite_alpha;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = info.clipped;
	swapchain_create_info.oldSwapchain = info.old_swapchain;
	Swapchain swapchain{};
	PFN_vkCreateSwapchainKHR swapchain_create_proc;
	detail::vulkan_functions().get_device_proc_addr(info.device, swapchain_create_proc, "vkCreateSwapchainKHR");
	auto res = swapchain_create_proc(info.device, &swapchain_create_info, info.allocation_callbacks, &swapchain.swapchain);

	if (res != VK_SUCCESS) {
		return Error{ SwapchainError::failed_create_swapchain, res };
	}
	swapchain.device = info.device;
	swapchain.image_format = surface_format.format;
	swapchain.color_space = surface_format.colorSpace;
	swapchain.image_usage_flags = info.image_usage_flags;
	swapchain.extent = extent;
	detail::vulkan_functions().get_device_proc_addr(
	    info.device, swapchain.internal_table.fp_vkGetSwapchainImagesKHR, "vkGetSwapchainImagesKHR");
	detail::vulkan_functions().get_device_proc_addr(info.device, swapchain.internal_table.fp_vkCreateImageView, "vkCreateImageView");
	detail::vulkan_functions().get_device_proc_addr(info.device, swapchain.internal_table.fp_vkDestroyImageView, "vkDestroyImageView");
	detail::vulkan_functions().get_device_proc_addr(
	    info.device, swapchain.internal_table.fp_vkDestroySwapchainKHR, "vkDestroySwapchainKHR");
	auto images = swapchain.get_images();
	if (!images) {
		return Error{ SwapchainError::failed_get_swapchain_images };
	}
	swapchain.requested_min_image_count = image_count;
	swapchain.present_mode = present_mode;
	swapchain.image_count = static_cast<uint32_t>(images.value().size());
	swapchain.instance_version = info.instance_version;
	swapchain.allocation_callbacks = info.allocation_callbacks;
	return swapchain;
}
Result<std::vector<VkImage>> Swapchain::get_images() {
	std::vector<VkImage> swapchain_images;

	auto swapchain_images_ret =
	    detail::get_vector<VkImage>(swapchain_images, internal_table.fp_vkGetSwapchainImagesKHR, device, swapchain);
	if (swapchain_images_ret != VK_SUCCESS) {
		return Error{ SwapchainError::failed_get_swapchain_images, swapchain_images_ret };
	}
	return swapchain_images;
}
Result<std::vector<VkImageView>> Swapchain::get_image_views() { return get_image_views(nullptr); }
Result<std::vector<VkImageView>> Swapchain::get_image_views(const void* pNext) {
	const auto swapchain_images_ret = get_images();
	if (!swapchain_images_ret) return swapchain_images_ret.error();
	const auto swapchain_images = swapchain_images_ret.value();

	bool already_contains_image_view_usage = false;
	while (pNext) {
		if (reinterpret_cast<const VkBaseInStructure*>(pNext)->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO) {
			already_contains_image_view_usage = true;
			break;
		}
		pNext = reinterpret_cast<const VkBaseInStructure*>(pNext)->pNext;
	}
	VkImageViewUsageCreateInfo desired_flags{};
	desired_flags.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	desired_flags.pNext = pNext;
	desired_flags.usage = image_usage_flags;

	std::vector<VkImageView> views(swapchain_images.size());
	for (size_t i = 0; i < swapchain_images.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		if (instance_version >= VKB_VK_API_VERSION_1_1 && !already_contains_image_view_usage) {
			createInfo.pNext = &desired_flags;
		} else {
			createInfo.pNext = pNext;
		}

		createInfo.image = swapchain_images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = image_format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VkResult res = internal_table.fp_vkCreateImageView(device, &createInfo, allocation_callbacks, &views[i]);
		if (res != VK_SUCCESS) return Error{ SwapchainError::failed_create_swapchain_image_views, res };
	}
	return views;
}
void Swapchain::destroy_image_views(std::vector<VkImageView> const& image_views) {
	for (auto& image_view : image_views) {
		internal_table.fp_vkDestroyImageView(device, image_view, allocation_callbacks);
	}
}
Swapchain::operator VkSwapchainKHR() const { return this->swapchain; }
SwapchainBuilder& SwapchainBuilder::set_old_swapchain(VkSwapchainKHR old_swapchain) {
	info.old_swapchain = old_swapchain;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_old_swapchain(Swapchain const& swapchain) {
	info.old_swapchain = swapchain.swapchain;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_desired_extent(uint32_t width, uint32_t height) {
	info.desired_width = width;
	info.desired_height = height;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_desired_format(VkSurfaceFormatKHR format) {
	info.desired_formats.insert(info.desired_formats.begin(), format);
	return *this;
}
SwapchainBuilder& SwapchainBuilder::add_fallback_format(VkSurfaceFormatKHR format) {
	info.desired_formats.push_back(format);
	return *this;
}
SwapchainBuilder& SwapchainBuilder::use_default_format_selection() {
	info.desired_formats.clear();
	add_desired_formats(info.desired_formats);
	return *this;
}

SwapchainBuilder& SwapchainBuilder::set_desired_present_mode(VkPresentModeKHR present_mode) {
	info.desired_present_modes.insert(info.desired_present_modes.begin(), present_mode);
	return *this;
}
SwapchainBuilder& SwapchainBuilder::add_fallback_present_mode(VkPresentModeKHR present_mode) {
	info.desired_present_modes.push_back(present_mode);
	return *this;
}
SwapchainBuilder& SwapchainBuilder::use_default_present_mode_selection() {
	info.desired_present_modes.clear();
	add_desired_present_modes(info.desired_present_modes);
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_allocation_callbacks(VkAllocationCallbacks* callbacks) {
	info.allocation_callbacks = callbacks;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_image_usage_flags(VkImageUsageFlags usage_flags) {
	info.image_usage_flags = usage_flags;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::add_image_usage_flags(VkImageUsageFlags usage_flags) {
	info.image_usage_flags = info.image_usage_flags | usage_flags;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::use_default_image_usage_flags() {
	info.image_usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_image_array_layer_count(uint32_t array_layer_count) {
	info.array_layer_count = array_layer_count;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_desired_min_image_count(uint32_t min_image_count) {
	info.min_image_count = min_image_count;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_required_min_image_count(uint32_t required_min_image_count) {
	info.required_min_image_count = required_min_image_count;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_clipped(bool clipped) {
	info.clipped = clipped;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_create_flags(VkSwapchainCreateFlagBitsKHR create_flags) {
	info.create_flags = create_flags;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_pre_transform_flags(VkSurfaceTransformFlagBitsKHR pre_transform_flags) {
	info.pre_transform = pre_transform_flags;
	return *this;
}
SwapchainBuilder& SwapchainBuilder::set_composite_alpha_flags(VkCompositeAlphaFlagBitsKHR composite_alpha_flags) {
	info.composite_alpha = composite_alpha_flags;
	return *this;
}

void SwapchainBuilder::add_desired_formats(std::vector<VkSurfaceFormatKHR>& formats) const {
	formats.push_back({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	formats.push_back({ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
}
void SwapchainBuilder::add_desired_present_modes(std::vector<VkPresentModeKHR>& modes) const {
	modes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
	modes.push_back(VK_PRESENT_MODE_FIFO_KHR);
}
} // namespace vkb
