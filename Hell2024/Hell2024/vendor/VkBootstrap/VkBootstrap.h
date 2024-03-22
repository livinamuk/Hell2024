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

#pragma once

#include <cassert>
#include <cstdio>
#include <cstring>

#include <vector>
#include <string>
#include <system_error>

#include <vulkan/vulkan.h>

#include "VkBootstrapDispatch.h"

#ifdef VK_MAKE_API_VERSION
#define VKB_MAKE_VK_VERSION(variant, major, minor, patch) VK_MAKE_API_VERSION(variant, major, minor, patch)
#elif defined(VK_MAKE_VERSION)
#define VKB_MAKE_VK_VERSION(variant, major, minor, patch) VK_MAKE_VERSION(major, minor, patch)
#endif

#if defined(VK_API_VERSION_1_3) || defined(VK_VERSION_1_3)
#define VKB_VK_API_VERSION_1_3 VKB_MAKE_VK_VERSION(0, 1, 3, 0)
#endif

#if defined(VK_API_VERSION_1_2) || defined(VK_VERSION_1_2)
#define VKB_VK_API_VERSION_1_2 VKB_MAKE_VK_VERSION(0, 1, 2, 0)
#endif

#if defined(VK_API_VERSION_1_1) || defined(VK_VERSION_1_1)
#define VKB_VK_API_VERSION_1_1 VKB_MAKE_VK_VERSION(0, 1, 1, 0)
#endif

#if defined(VK_API_VERSION_1_0) || defined(VK_VERSION_1_0)
#define VKB_VK_API_VERSION_1_0 VKB_MAKE_VK_VERSION(0, 1, 0, 0)
#endif

namespace vkb {

struct Error {
	std::error_code type;
	VkResult vk_result = VK_SUCCESS; // optional error value if a vulkan call failed
};

template <typename T> class Result {
	public:
	Result(const T& value) noexcept : m_value{ value }, m_init{ true } {}
	Result(T&& value) noexcept : m_value{ std::move(value) }, m_init{ true } {}

	Result(Error error) noexcept : m_error{ error }, m_init{ false } {}

	Result(std::error_code error_code, VkResult result = VK_SUCCESS) noexcept
	: m_error{ error_code, result }, m_init{ false } {}

	~Result() noexcept { destroy(); }
	Result(Result const& expected) noexcept : m_init(expected.m_init) {
		if (m_init)
			new (&m_value) T{ expected.m_value };
		else
			m_error = expected.m_error;
	}
	Result& operator=(Result const& result) noexcept {
		m_init = result.m_init;
		if (m_init)
			new (&m_value) T{ result.m_value };
		else
			m_error = result.m_error;
	}
	Result(Result&& expected) noexcept : m_init(expected.m_init) {
		if (m_init)
			new (&m_value) T{ std::move(expected.m_value) };
		else
			m_error = std::move(expected.m_error);
		expected.destroy();
	}
	Result& operator=(Result&& result) noexcept {
		m_init = result.m_init;
		if (m_init)
			new (&m_value) T{ std::move(result.m_value) };
		else
			m_error = std::move(result.m_error);
	}
	Result& operator=(const T& expect) noexcept {
		destroy();
		m_init = true;
		new (&m_value) T{ expect };
		return *this;
	}
	Result& operator=(T&& expect) noexcept {
		destroy();
		m_init = true;
		new (&m_value) T{ std::move(expect) };
		return *this;
	}
	Result& operator=(const Error& error) noexcept {
		destroy();
		m_init = false;
		m_error = error;
		return *this;
	}
	Result& operator=(Error&& error) noexcept {
		destroy();
		m_init = false;
		m_error = error;
		return *this;
	}
	// clang-format off
	const T* operator-> () const noexcept { assert (m_init); return &m_value; }
	T*       operator-> ()       noexcept { assert (m_init); return &m_value; }
	const T& operator* () const& noexcept { assert (m_init);	return m_value; }
	T&       operator* () &      noexcept { assert (m_init); return m_value; }
	T&&      operator* () &&	 noexcept { assert (m_init); return std::move (m_value); }
	const T&  value () const&    noexcept { assert (m_init); return m_value; }
	T&        value () &         noexcept { assert (m_init); return m_value; }
	const T&& value () const&&   noexcept { assert (m_init); return std::move (m_value); }
	T&&       value () &&        noexcept { assert (m_init); return std::move (m_value); }

    // std::error_code associated with the error
    std::error_code error() const { assert (!m_init); return m_error.type; }
    // optional VkResult that could of been produced due to the error
    VkResult vk_result() const { assert (!m_init); return m_error.vk_result; }
    // Returns the struct that holds the std::error_code and VkResult
    Error full_error() const { assert (!m_init); return m_error; }
	// clang-format on

	// check if the result has an error that matches a specific error case
	template <typename E> bool matches_error(E error_enum_value) const {
		return !m_init && static_cast<E>(m_error.type.value()) == error_enum_value;
	}

	bool has_value() const { return m_init; }
	explicit operator bool() const { return m_init; }

	private:
	void destroy() {
		if (m_init) m_value.~T();
	}
	union {
		T m_value;
		Error m_error;
	};
	bool m_init;
};

namespace detail {
struct GenericFeaturesPNextNode {

	static const uint32_t field_capacity = 256;

	GenericFeaturesPNextNode();

	template <typename T> GenericFeaturesPNextNode(T const& features) noexcept {
		memset(fields, UINT8_MAX, sizeof(VkBool32) * field_capacity);
		memcpy(this, &features, sizeof(T));
	}

	static bool match(GenericFeaturesPNextNode const& requested, GenericFeaturesPNextNode const& supported) noexcept;

	VkStructureType sType = static_cast<VkStructureType>(0);
	void* pNext = nullptr;
	VkBool32 fields[field_capacity];
};

} // namespace detail

enum class InstanceError {
	vulkan_unavailable,
	vulkan_version_unavailable,
	vulkan_version_1_1_unavailable,
	vulkan_version_1_2_unavailable,
	failed_create_instance,
	failed_create_debug_messenger,
	requested_layers_not_present,
	requested_extensions_not_present,
	windowing_extensions_not_present,
};
enum class PhysicalDeviceError {
	no_surface_provided,
	failed_enumerate_physical_devices,
	no_physical_devices_found,
	no_suitable_device,
};
enum class QueueError {
	present_unavailable,
	graphics_unavailable,
	compute_unavailable,
	transfer_unavailable,
	queue_index_out_of_range,
	invalid_queue_family_index
};
enum class DeviceError {
	failed_create_device,
	VkPhysicalDeviceFeatures2_in_pNext_chain_while_using_add_required_extension_features,
};
enum class SwapchainError {
	surface_handle_not_provided,
	failed_query_surface_support_details,
	failed_create_swapchain,
	failed_get_swapchain_images,
	failed_create_swapchain_image_views,
	required_min_image_count_too_low,
	required_usage_not_supported
};

std::error_code make_error_code(InstanceError instance_error);
std::error_code make_error_code(PhysicalDeviceError physical_device_error);
std::error_code make_error_code(QueueError queue_error);
std::error_code make_error_code(DeviceError device_error);
std::error_code make_error_code(SwapchainError swapchain_error);

const char* to_string_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT s);
const char* to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s);

const char* to_string(InstanceError err);
const char* to_string(PhysicalDeviceError err);
const char* to_string(QueueError err);
const char* to_string(DeviceError err);
const char* to_string(SwapchainError err);

// Gathers useful information about the available vulkan capabilities, like layers and instance
// extensions. Use this for enabling features conditionally, ie if you would like an extension but
// can use a fallback if it isn't supported but need to know if support is available first.
struct SystemInfo {
	private:
	SystemInfo();

	public:
	// Use get_system_info to create a SystemInfo struct. This is because loading vulkan could fail.
	static Result<SystemInfo> get_system_info();
	static Result<SystemInfo> get_system_info(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);

	// Returns true if a layer is available
	bool is_layer_available(const char* layer_name) const;
	// Returns true if an extension is available
	bool is_extension_available(const char* extension_name) const;

	std::vector<VkLayerProperties> available_layers;
	std::vector<VkExtensionProperties> available_extensions;
	bool validation_layers_available = false;
	bool debug_utils_available = false;
};

// Forward declared - check VkBoostrap.cpp for implementations
const char* to_string_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT s);
const char* to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s);

// Default debug messenger
// Feel free to copy-paste it into your own code, change it as needed, then call `set_debug_callback()` to use that instead
inline VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*) {
	auto ms = to_string_message_severity(messageSeverity);
	auto mt = to_string_message_type(messageType);
	printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE; // Applications must return false here
}

class InstanceBuilder;
class PhysicalDeviceSelector;

struct Instance {
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;
	PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = nullptr;
	PFN_vkGetDeviceProcAddr fp_vkGetDeviceProcAddr = nullptr;

	// A conversion function which allows this Instance to be used
	// in places where VkInstance would have been used.
	operator VkInstance() const;

	private:
	bool headless = false;
	bool properties2_ext_enabled = false;
	uint32_t instance_version = VKB_VK_API_VERSION_1_0;
	uint32_t api_version = VKB_VK_API_VERSION_1_0;

	friend class InstanceBuilder;
	friend class PhysicalDeviceSelector;
};

void destroy_surface(Instance instance, VkSurfaceKHR surface); // release surface handle
void destroy_surface(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks* callbacks = nullptr); // release surface handle
void destroy_instance(Instance instance); // release instance resources

/* If headless mode is false, by default vk-bootstrap use the following logic to enable the windowing extensions

#if defined(_WIN32)
    VK_KHR_win32_surface
#elif defined(__linux__)
    VK_KHR_xcb_surface
    VK_KHR_xlib_surface
    VK_KHR_wayland_surface
#elif defined(__APPLE__)
    VK_EXT_metal_surface
#elif defined(__ANDROID__)
    VK_KHR_android_surface
#elif defined(_DIRECT2DISPLAY)
    VK_KHR_display
#endif

Use `InstanceBuilder::enable_extension()` to add new extensions without altering the default behavior
Feel free to make a PR or raise an issue to include additional platforms.
*/

class InstanceBuilder {
	public:
	// Default constructor, will load vulkan.
	explicit InstanceBuilder();
	// Optional: Can use your own PFN_vkGetInstanceProcAddr
	explicit InstanceBuilder(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);

	// Create a VkInstance. Return an error if it failed.
	Result<Instance> build() const;

	// Sets the name of the application. Defaults to "" if none is provided.
	InstanceBuilder& set_app_name(const char* app_name);
	// Sets the name of the engine. Defaults to "" if none is provided.
	InstanceBuilder& set_engine_name(const char* engine_name);

	// Sets the version of the application.
	// Should be constructed with VK_MAKE_VERSION or VK_MAKE_API_VERSION.
	InstanceBuilder& set_app_version(uint32_t app_version);
	// Sets the (major, minor, patch) version of the application.
	InstanceBuilder& set_app_version(uint32_t major, uint32_t minor, uint32_t patch = 0);

	// Sets the version of the engine.
	// Should be constructed with VK_MAKE_VERSION or VK_MAKE_API_VERSION.
	InstanceBuilder& set_engine_version(uint32_t engine_version);
	// Sets the (major, minor, patch) version of the engine.
	InstanceBuilder& set_engine_version(uint32_t major, uint32_t minor, uint32_t patch = 0);

	// Require a vulkan API version. Will fail to create if this version isn't available.
	// Should be constructed with VK_MAKE_VERSION or VK_MAKE_API_VERSION.
	InstanceBuilder& require_api_version(uint32_t required_api_version);
	// Require a vulkan API version. Will fail to create if this version isn't available.
	InstanceBuilder& require_api_version(uint32_t major, uint32_t minor, uint32_t patch = 0);

	// Overrides required API version for instance creation. Will fail to create if this version isn't available.
	// Should be constructed with VK_MAKE_VERSION or VK_MAKE_API_VERSION.
	InstanceBuilder& set_minimum_instance_version(uint32_t minimum_instance_version);
	// Overrides required API version for instance creation. Will fail to create if this version isn't available.
	InstanceBuilder& set_minimum_instance_version(uint32_t major, uint32_t minor, uint32_t patch = 0);

	// Prefer a vulkan instance API version. If the desired version isn't available, it will use the
	// highest version available. Should be constructed with VK_MAKE_VERSION or VK_MAKE_API_VERSION.
	[[deprecated("Use require_api_version + set_minimum_instance_version instead.")]] InstanceBuilder&
	desire_api_version(uint32_t preferred_vulkan_version);

	// Prefer a vulkan instance API version. If the desired version isn't available, it will use the highest version available.
	[[deprecated("Use require_api_version + set_minimum_instance_version instead.")]] InstanceBuilder&
	desire_api_version(uint32_t major, uint32_t minor, uint32_t patch = 0);

	// Adds a layer to be enabled. Will fail to create an instance if the layer isn't available.
	InstanceBuilder& enable_layer(const char* layer_name);
	// Adds an extension to be enabled. Will fail to create an instance if the extension isn't available.
	InstanceBuilder& enable_extension(const char* extension_name);

	// Headless Mode does not load the required extensions for presentation. Defaults to true.
	InstanceBuilder& set_headless(bool headless = true);

	// Enables the validation layers. Will fail to create an instance if the validation layers aren't available.
	InstanceBuilder& enable_validation_layers(bool require_validation = true);
	// Checks if the validation layers are available and loads them if they are.
	InstanceBuilder& request_validation_layers(bool enable_validation = true);

	// Use a default debug callback that prints to standard out.
	InstanceBuilder& use_default_debug_messenger();
	// Provide a user defined debug callback.
	InstanceBuilder& set_debug_callback(PFN_vkDebugUtilsMessengerCallbackEXT callback);
	// Sets the void* to use in the debug messenger - only useful with a custom callback
	InstanceBuilder& set_debug_callback_user_data_pointer(void* user_data_pointer);
	// Set what message severity is needed to trigger the callback.
	InstanceBuilder& set_debug_messenger_severity(VkDebugUtilsMessageSeverityFlagsEXT severity);
	// Add a message severity to the list that triggers the callback.
	InstanceBuilder& add_debug_messenger_severity(VkDebugUtilsMessageSeverityFlagsEXT severity);
	// Set what message type triggers the callback.
	InstanceBuilder& set_debug_messenger_type(VkDebugUtilsMessageTypeFlagsEXT type);
	// Add a message type to the list of that triggers the callback.
	InstanceBuilder& add_debug_messenger_type(VkDebugUtilsMessageTypeFlagsEXT type);

	// Disable some validation checks.
	// Checks: All, and Shaders
	InstanceBuilder& add_validation_disable(VkValidationCheckEXT check);

	// Enables optional parts of the validation layers.
	// Parts: best practices, gpu assisted, and gpu assisted reserve binding slot.
	InstanceBuilder& add_validation_feature_enable(VkValidationFeatureEnableEXT enable);

	// Disables sections of the validation layers.
	// Options: All, shaders, thread safety, api parameters, object lifetimes, core checks, and unique handles.
	InstanceBuilder& add_validation_feature_disable(VkValidationFeatureDisableEXT disable);

	// Provide custom allocation callbacks.
	InstanceBuilder& set_allocation_callbacks(VkAllocationCallbacks* callbacks);

	private:
	struct InstanceInfo {
		// VkApplicationInfo
		const char* app_name = nullptr;
		const char* engine_name = nullptr;
		uint32_t application_version = 0;
		uint32_t engine_version = 0;
		uint32_t minimum_instance_version = 0;
		uint32_t required_api_version = VKB_VK_API_VERSION_1_0;
		uint32_t desired_api_version = VKB_VK_API_VERSION_1_0;

		// VkInstanceCreateInfo
		std::vector<const char*> layers;
		std::vector<const char*> extensions;
		VkInstanceCreateFlags flags = static_cast<VkInstanceCreateFlags>(0);
		std::vector<VkBaseOutStructure*> pNext_elements;

		// debug callback - use the default so it is not nullptr
		PFN_vkDebugUtilsMessengerCallbackEXT debug_callback = default_debug_callback;
		VkDebugUtilsMessageSeverityFlagsEXT debug_message_severity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		VkDebugUtilsMessageTypeFlagsEXT debug_message_type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		void* debug_user_data_pointer = nullptr;

		// validation features
		std::vector<VkValidationCheckEXT> disabled_validation_checks;
		std::vector<VkValidationFeatureEnableEXT> enabled_validation_features;
		std::vector<VkValidationFeatureDisableEXT> disabled_validation_features;

		// Custom allocator
		VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;

		bool request_validation_layers = false;
		bool enable_validation_layers = false;
		bool use_debug_messenger = false;
		bool headless_context = false;

		PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = nullptr;
	} info;
};

VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

void destroy_debug_utils_messenger(
    VkInstance const instance, VkDebugUtilsMessengerEXT const messenger, VkAllocationCallbacks* allocation_callbacks = nullptr);

// ---- Physical Device ---- //
class PhysicalDeviceSelector;
class DeviceBuilder;

struct PhysicalDevice {
	std::string name;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	// Note that this reflects selected features carried over from required features, not all features the physical device supports.
	VkPhysicalDeviceFeatures features{};
	VkPhysicalDeviceProperties properties{};
	VkPhysicalDeviceMemoryProperties memory_properties{};

	// Has a queue family that supports compute operations but not graphics nor transfer.
	bool has_dedicated_compute_queue() const;
	// Has a queue family that supports transfer operations but not graphics nor compute.
	bool has_dedicated_transfer_queue() const;

	// Has a queue family that supports transfer operations but not graphics.
	bool has_separate_compute_queue() const;
	// Has a queue family that supports transfer operations but not graphics.
	bool has_separate_transfer_queue() const;

	// Advanced: Get the VkQueueFamilyProperties of the device if special queue setup is needed
	std::vector<VkQueueFamilyProperties> get_queue_families() const;

	// Query the list of extensions which should be enabled
	std::vector<std::string> get_extensions() const;

	// A conversion function which allows this PhysicalDevice to be used
	// in places where VkPhysicalDevice would have been used.
	operator VkPhysicalDevice() const;

	private:
	uint32_t instance_version = VKB_VK_API_VERSION_1_0;
	std::vector<std::string> extensions;
	std::vector<VkQueueFamilyProperties> queue_families;
	std::vector<detail::GenericFeaturesPNextNode> extended_features_chain;
	VkPhysicalDeviceFeatures2 features2{};

	bool defer_surface_initialization = false;
	bool properties2_ext_enabled = false;
	enum class Suitable { yes, partial, no };
	Suitable suitable = Suitable::yes;
	friend class PhysicalDeviceSelector;
	friend class DeviceBuilder;
};

enum class PreferredDeviceType { other = 0, integrated = 1, discrete = 2, virtual_gpu = 3, cpu = 4 };

enum class DeviceSelectionMode {
	// return all suitable and partially suitable devices
	partially_and_fully_suitable,
	// return only physical devices which are fully suitable
	only_fully_suitable
};

// Enumerates the physical devices on the system, and based on the added criteria, returns a physical device or list of physical devies
// A device is considered suitable if it meets all the 'required' and 'desired' criteria.
// A device is considered partially suitable if it meets only the 'required' criteria.
class PhysicalDeviceSelector {
	public:
	// Requires a vkb::Instance to construct, needed to pass instance creation info.
	explicit PhysicalDeviceSelector(Instance const& instance);
	// Requires a vkb::Instance to construct, needed to pass instance creation info, optionally specify the surface here
	explicit PhysicalDeviceSelector(Instance const& instance, VkSurfaceKHR surface);

	// Return the first device which is suitable
	// use the `selection` parameter to configure if partially
	Result<PhysicalDevice> select(DeviceSelectionMode selection = DeviceSelectionMode::partially_and_fully_suitable) const;

	// Return all devices which are considered suitable - intended for applications which want to let the user pick the physical device
	Result<std::vector<PhysicalDevice>> select_devices(
	    DeviceSelectionMode selection = DeviceSelectionMode::partially_and_fully_suitable) const;

	// Return the names of all devices which are considered suitable - intended for applications which want to let the user pick the physical device
	Result<std::vector<std::string>> select_device_names(
	    DeviceSelectionMode selection = DeviceSelectionMode::partially_and_fully_suitable) const;

	// Set the surface in which the physical device should render to.
	// Be sure to set it if swapchain functionality is to be used.
	PhysicalDeviceSelector& set_surface(VkSurfaceKHR surface);

	// Set the name of the device to select.
	PhysicalDeviceSelector& set_name(std::string const& name);
	// Set the desired physical device type to select. Defaults to PreferredDeviceType::discrete.
	PhysicalDeviceSelector& prefer_gpu_device_type(PreferredDeviceType type = PreferredDeviceType::discrete);
	// Allow selection of a gpu device type that isn't the preferred physical device type. Defaults to true.
	PhysicalDeviceSelector& allow_any_gpu_device_type(bool allow_any_type = true);

	// Require that a physical device supports presentation. Defaults to true.
	PhysicalDeviceSelector& require_present(bool require = true);

	// Require a queue family that supports compute operations but not graphics nor transfer.
	PhysicalDeviceSelector& require_dedicated_compute_queue();
	// Require a queue family that supports transfer operations but not graphics nor compute.
	PhysicalDeviceSelector& require_dedicated_transfer_queue();

	// Require a queue family that supports compute operations but not graphics.
	PhysicalDeviceSelector& require_separate_compute_queue();
	// Require a queue family that supports transfer operations but not graphics.
	PhysicalDeviceSelector& require_separate_transfer_queue();

	// Require a memory heap from VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with `size` memory available.
	PhysicalDeviceSelector& required_device_memory_size(VkDeviceSize size);
	// Prefer a memory heap from VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with `size` memory available.
	PhysicalDeviceSelector& desired_device_memory_size(VkDeviceSize size);

	// Require a physical device which supports a specific extension.
	PhysicalDeviceSelector& add_required_extension(const char* extension);
	// Require a physical device which supports a set of extensions.
	PhysicalDeviceSelector& add_required_extensions(std::vector<const char*> extensions);

	// Prefer a physical device which supports a specific extension.
	PhysicalDeviceSelector& add_desired_extension(const char* extension);
	// Prefer a physical device which supports a set of extensions.
	PhysicalDeviceSelector& add_desired_extensions(std::vector<const char*> extensions);

	// Prefer a physical device that supports a (major, minor) version of vulkan.
	[[deprecated("Use set_minimum_version + InstanceBuilder::require_api_version.")]] PhysicalDeviceSelector&
	set_desired_version(uint32_t major, uint32_t minor);

	// Require a physical device that supports a (major, minor) version of vulkan.
	PhysicalDeviceSelector& set_minimum_version(uint32_t major, uint32_t minor);

	// By default PhysicalDeviceSelector enables the portability subset if available
	// This function disables that behavior
	PhysicalDeviceSelector& disable_portability_subset();

	// Require a physical device which supports a specific set of general/extension features.
	// If this function is used, the user should not put their own VkPhysicalDeviceFeatures2 in
	// the pNext chain of VkDeviceCreateInfo.
	template <typename T> PhysicalDeviceSelector& add_required_extension_features(T const& features) {
		criteria.extended_features_chain.push_back(features);
		return *this;
	}

	// Require a physical device which supports the features in VkPhysicalDeviceFeatures.
	PhysicalDeviceSelector& set_required_features(VkPhysicalDeviceFeatures const& features);
#if defined(VKB_VK_API_VERSION_1_2)
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan11Features.
	// Must have vulkan version 1.2 - This is due to the VkPhysicalDeviceVulkan11Features struct being added in 1.2, not 1.1
	PhysicalDeviceSelector& set_required_features_11(VkPhysicalDeviceVulkan11Features features_11);
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan12Features.
	// Must have vulkan version 1.2
	PhysicalDeviceSelector& set_required_features_12(VkPhysicalDeviceVulkan12Features features_12);
#endif
#if defined(VKB_VK_API_VERSION_1_3)
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan13Features.
	// Must have vulkan version 1.3
	PhysicalDeviceSelector& set_required_features_13(VkPhysicalDeviceVulkan13Features features_13);
#endif

	// Used when surface creation happens after physical device selection.
	// Warning: This disables checking if the physical device supports a given surface.
	PhysicalDeviceSelector& defer_surface_initialization();

	// Ignore all criteria and choose the first physical device that is available.
	// Only use when: The first gpu in the list may be set by global user preferences and an application may wish to respect it.
	PhysicalDeviceSelector& select_first_device_unconditionally(bool unconditionally = true);

	private:
	struct InstanceInfo {
		VkInstance instance = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		uint32_t version = VKB_VK_API_VERSION_1_0;
		bool headless = false;
		bool properties2_ext_enabled = false;
	} instance_info;

	// We copy the extension features stored in the selector criteria under the prose of a
	// "template" to ensure that after fetching everything is compared 1:1 during a match.

	struct SelectionCriteria {
		std::string name;
		PreferredDeviceType preferred_type = PreferredDeviceType::discrete;
		bool allow_any_type = true;
		bool require_present = true;
		bool require_dedicated_transfer_queue = false;
		bool require_dedicated_compute_queue = false;
		bool require_separate_transfer_queue = false;
		bool require_separate_compute_queue = false;
		VkDeviceSize required_mem_size = 0;
		VkDeviceSize desired_mem_size = 0;

		std::vector<std::string> required_extensions;
		std::vector<std::string> desired_extensions;

		uint32_t required_version = VKB_VK_API_VERSION_1_0;
		uint32_t desired_version = VKB_VK_API_VERSION_1_0;

		VkPhysicalDeviceFeatures required_features{};
		VkPhysicalDeviceFeatures2 required_features2{};

		std::vector<detail::GenericFeaturesPNextNode> extended_features_chain;
		bool defer_surface_initialization = false;
		bool use_first_gpu_unconditionally = false;
		bool enable_portability_subset = true;
	} criteria;

	PhysicalDevice populate_device_details(VkPhysicalDevice phys_device,
	    std::vector<detail::GenericFeaturesPNextNode> const& src_extended_features_chain) const;

	PhysicalDevice::Suitable is_device_suitable(PhysicalDevice const& phys_device) const;

	Result<std::vector<PhysicalDevice>> select_impl(DeviceSelectionMode selection) const;
};

// ---- Queue ---- //
enum class QueueType { present, graphics, compute, transfer };

namespace detail {
// Sentinel value, used in implementation only
const uint32_t QUEUE_INDEX_MAX_VALUE = 65536;
} // namespace detail

// ---- Device ---- //

struct Device {
	VkDevice device = VK_NULL_HANDLE;
	PhysicalDevice physical_device;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	std::vector<VkQueueFamilyProperties> queue_families;
	VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;
	PFN_vkGetDeviceProcAddr fp_vkGetDeviceProcAddr = nullptr;
	uint32_t instance_version = VKB_VK_API_VERSION_1_0;

	Result<uint32_t> get_queue_index(QueueType type) const;
	// Only a compute or transfer queue type is valid. All other queue types do not support a 'dedicated' queue index
	Result<uint32_t> get_dedicated_queue_index(QueueType type) const;

	Result<VkQueue> get_queue(QueueType type) const;
	// Only a compute or transfer queue type is valid. All other queue types do not support a 'dedicated' queue
	Result<VkQueue> get_dedicated_queue(QueueType type) const;

	// Return a loaded dispatch table
	DispatchTable make_table() const;

	// A conversion function which allows this Device to be used
	// in places where VkDevice would have been used.
	operator VkDevice() const;

	private:
	struct {
		PFN_vkGetDeviceQueue fp_vkGetDeviceQueue = nullptr;
		PFN_vkDestroyDevice fp_vkDestroyDevice = nullptr;
	} internal_table;
	friend class DeviceBuilder;
	friend void destroy_device(Device device);
};

// For advanced device queue setup
struct CustomQueueDescription {
	explicit CustomQueueDescription(uint32_t index, uint32_t count, std::vector<float> priorities);
	uint32_t index = 0;
	uint32_t count = 0;
	std::vector<float> priorities;
};

void destroy_device(Device device);

class DeviceBuilder {
	public:
	// Any features and extensions that are requested/required in PhysicalDeviceSelector are automatically enabled.
	explicit DeviceBuilder(PhysicalDevice physical_device);

	Result<Device> build() const;

	// For Advanced Users: specify the exact list of VkDeviceQueueCreateInfo's needed for the application.
	// If a custom queue setup is provided, getting the queues and queue indexes is up to the application.
	DeviceBuilder& custom_queue_setup(std::vector<CustomQueueDescription> queue_descriptions);

	// Add a structure to the pNext chain of VkDeviceCreateInfo.
	// The structure must be valid when DeviceBuilder::build() is called.
	template <typename T> DeviceBuilder& add_pNext(T* structure) {
		info.pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(structure));
		return *this;
	}

	// Provide custom allocation callbacks.
	DeviceBuilder& set_allocation_callbacks(VkAllocationCallbacks* callbacks);

	private:
	PhysicalDevice physical_device;
	struct DeviceInfo {
		VkDeviceCreateFlags flags = static_cast<VkDeviceCreateFlags>(0);
		std::vector<VkBaseOutStructure*> pNext_chain;
		std::vector<CustomQueueDescription> queue_descriptions;
		VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;
	} info;
};

// ---- Swapchain ---- //
struct Swapchain {
	VkDevice device = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	uint32_t image_count = 0;
	VkFormat image_format = VK_FORMAT_UNDEFINED; // The image format actually used when creating the swapchain.
	VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // The color space actually used when creating the swapchain.
	VkImageUsageFlags image_usage_flags = 0;
	VkExtent2D extent = { 0, 0 };
	// The value of minImageCount actually used when creating the swapchain; note that the presentation engine is always free to create more images than that.
	uint32_t requested_min_image_count = 0;
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR; // The present mode actually used when creating the swapchain.
	uint32_t instance_version = VKB_VK_API_VERSION_1_0;
	VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;

	// Returns a vector of VkImage handles to the swapchain.
	Result<std::vector<VkImage>> get_images();

	// Returns a vector of VkImageView's to the VkImage's of the swapchain.
	// VkImageViews must be destroyed.  The pNext chain must be a nullptr or a valid
	// structure.
	Result<std::vector<VkImageView>> get_image_views();
	Result<std::vector<VkImageView>> get_image_views(const void* pNext);
	void destroy_image_views(std::vector<VkImageView> const& image_views);

	// A conversion function which allows this Swapchain to be used
	// in places where VkSwapchainKHR would have been used.
	operator VkSwapchainKHR() const;

	private:
	struct {
		PFN_vkGetSwapchainImagesKHR fp_vkGetSwapchainImagesKHR = nullptr;
		PFN_vkCreateImageView fp_vkCreateImageView = nullptr;
		PFN_vkDestroyImageView fp_vkDestroyImageView = nullptr;
		PFN_vkDestroySwapchainKHR fp_vkDestroySwapchainKHR = nullptr;
	} internal_table;
	friend class SwapchainBuilder;
	friend void destroy_swapchain(Swapchain const& swapchain);
};

void destroy_swapchain(Swapchain const& swapchain);

class SwapchainBuilder {
	public:
	// Construct a SwapchainBuilder with a `vkb::Device`
	explicit SwapchainBuilder(Device const& device);
	// Construct a SwapchainBuilder with a specific VkSurfaceKHR handle and `vkb::Device`
	explicit SwapchainBuilder(Device const& device, VkSurfaceKHR const surface);
	// Construct a SwapchainBuilder with Vulkan handles for the physical device, device, and surface
	// Optionally can provide the uint32_t indices for the graphics and present queue
	// Note: The constructor will query the graphics & present queue if the indices are not provided
	explicit SwapchainBuilder(VkPhysicalDevice const physical_device,
	    VkDevice const device,
	    VkSurfaceKHR const surface,
	    uint32_t graphics_queue_index = detail::QUEUE_INDEX_MAX_VALUE,
	    uint32_t present_queue_index = detail::QUEUE_INDEX_MAX_VALUE);

	Result<Swapchain> build() const;

	// Set the oldSwapchain member of VkSwapchainCreateInfoKHR.
	// For use in rebuilding a swapchain.
	SwapchainBuilder& set_old_swapchain(VkSwapchainKHR old_swapchain);
	SwapchainBuilder& set_old_swapchain(Swapchain const& swapchain);


	// Desired size of the swapchain. By default, the swapchain will use the size
	// of the window being drawn to.
	SwapchainBuilder& set_desired_extent(uint32_t width, uint32_t height);

	// When determining the surface format, make this the first to be used if supported.
	SwapchainBuilder& set_desired_format(VkSurfaceFormatKHR format);
	// Add this swapchain format to the end of the list of formats selected from.
	SwapchainBuilder& add_fallback_format(VkSurfaceFormatKHR format);
	// Use the default swapchain formats. This is done if no formats are provided.
	// Default surface format is {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
	SwapchainBuilder& use_default_format_selection();

	// When determining the present mode, make this the first to be used if supported.
	SwapchainBuilder& set_desired_present_mode(VkPresentModeKHR present_mode);
	// Add this present mode to the end of the list of present modes selected from.
	SwapchainBuilder& add_fallback_present_mode(VkPresentModeKHR present_mode);
	// Use the default presentation mode. This is done if no present modes are provided.
	// Default present modes: VK_PRESENT_MODE_MAILBOX_KHR with fallback VK_PRESENT_MODE_FIFO_KHR
	SwapchainBuilder& use_default_present_mode_selection();

	// Set the bitmask of the image usage for acquired swapchain images.
	// If the surface capabilities cannot allow it, building the swapchain will result in the `SwapchainError::required_usage_not_supported` error.
	SwapchainBuilder& set_image_usage_flags(VkImageUsageFlags usage_flags);
	// Add a image usage to the bitmask for acquired swapchain images.
	SwapchainBuilder& add_image_usage_flags(VkImageUsageFlags usage_flags);
	// Use the default image usage bitmask values. This is the default if no image usages
	// are provided. The default is VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	SwapchainBuilder& use_default_image_usage_flags();

	// Set the number of views in for multiview/stereo surface
	SwapchainBuilder& set_image_array_layer_count(uint32_t array_layer_count);

	// Convenient named constants for passing to set_desired_min_image_count().
	// Note that it is not an `enum class`, so its constants can be passed as an integer value without casting
	// In other words, these might as well be `static const int`, but they benefit from being grouped together this way.
	enum BufferMode {
		SINGLE_BUFFERING = 1,
		DOUBLE_BUFFERING = 2,
		TRIPLE_BUFFERING = 3,
	};

	// Sets the desired minimum image count for the swapchain.
	// Note that the presentation engine is always free to create more images than requested.
	// You may pass one of the values specified in the BufferMode enum, or any integer value.
	// For instance, if you pass DOUBLE_BUFFERING, the presentation engine is allowed to give you a double buffering setup, triple buffering, or more. This is up to the drivers.
	SwapchainBuilder& set_desired_min_image_count(uint32_t min_image_count);

	// Sets a required minimum image count for the swapchain.
	// If the surface capabilities cannot allow it, building the swapchain will result in the `SwapchainError::required_min_image_count_too_low` error.
	// Otherwise, the same observations from set_desired_min_image_count() apply.
	// A value of 0 is specially interpreted as meaning "no requirement", and is the behavior by default.
	SwapchainBuilder& set_required_min_image_count(uint32_t required_min_image_count);

	// Set whether the Vulkan implementation is allowed to discard rendering operations that
	// affect regions of the surface that are not visible. Default is true.
	// Note: Applications should use the default of true if they do not expect to read back the content
	// of presentable images before presenting them or after reacquiring them, and if their fragment
	// shaders do not have any side effects that require them to run for all pixels in the presentable image.
	SwapchainBuilder& set_clipped(bool clipped = true);

	// Set the VkSwapchainCreateFlagBitsKHR.
	SwapchainBuilder& set_create_flags(VkSwapchainCreateFlagBitsKHR create_flags);
	// Set the transform to be applied, like a 90 degree rotation. Default is no transform.
	SwapchainBuilder& set_pre_transform_flags(VkSurfaceTransformFlagBitsKHR pre_transform_flags);
	// Set the alpha channel to be used with other windows in on the system. Default is VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
	SwapchainBuilder& set_composite_alpha_flags(VkCompositeAlphaFlagBitsKHR composite_alpha_flags);

	// Add a structure to the pNext chain of VkSwapchainCreateInfoKHR.
	// The structure must be valid when SwapchainBuilder::build() is called.
	template <typename T> SwapchainBuilder& add_pNext(T* structure) {
		info.pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(structure));
		return *this;
	}

	// Provide custom allocation callbacks.
	SwapchainBuilder& set_allocation_callbacks(VkAllocationCallbacks* callbacks);

	private:
	void add_desired_formats(std::vector<VkSurfaceFormatKHR>& formats) const;
	void add_desired_present_modes(std::vector<VkPresentModeKHR>& modes) const;

	struct SwapchainInfo {
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		std::vector<VkBaseOutStructure*> pNext_chain;
		VkSwapchainCreateFlagBitsKHR create_flags = static_cast<VkSwapchainCreateFlagBitsKHR>(0);
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		std::vector<VkSurfaceFormatKHR> desired_formats;
		uint32_t instance_version = VKB_VK_API_VERSION_1_0;
		uint32_t desired_width = 256;
		uint32_t desired_height = 256;
		uint32_t array_layer_count = 1;
		uint32_t min_image_count = 0;
		uint32_t required_min_image_count = 0;
		VkImageUsageFlags image_usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		uint32_t graphics_queue_index = 0;
		uint32_t present_queue_index = 0;
		VkSurfaceTransformFlagBitsKHR pre_transform = static_cast<VkSurfaceTransformFlagBitsKHR>(0);
#if defined(__ANDROID__)
		VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
#else
		VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#endif
		std::vector<VkPresentModeKHR> desired_present_modes;
		bool clipped = true;
		VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
		VkAllocationCallbacks* allocation_callbacks = VK_NULL_HANDLE;
	} info;
};

} // namespace vkb


namespace std {
template <> struct is_error_code_enum<vkb::InstanceError> : true_type {};
template <> struct is_error_code_enum<vkb::PhysicalDeviceError> : true_type {};
template <> struct is_error_code_enum<vkb::QueueError> : true_type {};
template <> struct is_error_code_enum<vkb::DeviceError> : true_type {};
template <> struct is_error_code_enum<vkb::SwapchainError> : true_type {};
} // namespace std
