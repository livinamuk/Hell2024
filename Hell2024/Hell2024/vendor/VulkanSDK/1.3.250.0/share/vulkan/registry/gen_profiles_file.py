#!/usr/bin/python3
#
# Copyright (c) 2022 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Ziga Markus <ziga@lunarg.com>

from datetime import datetime
import argparse
import json
import gen_profiles_solution
import re
import os
import collections

class ProfileMerger():
    def __init__(self, registry):
        self.registry = registry

    def merge(self, jsons, profiles, profile_names, merged_path, merged_profile, profile_label, profile_description, profile_api_version, profile_stage, profile_date, mode):
        self.mode = mode

        # Find the api version to use
        if args.profile_api_version is not None:
            self.api_version = self.get_api_version_list(profile_api_version)
        else:
            self.api_version = self.get_api_version(profiles)

        print('Building a Vulkan ' + '.'.join(self.api_version) + ' profile')

        # Begin constructing merged profile
        merged = dict()
        merged['$schema'] = 'https://schema.khronos.org/vulkan/profiles-0.8.0-' + self.api_version[2] + '.json#'
        merged['capabilities'] = self.merge_capabilities(jsons, profile_names, self.api_version)
        merged['profiles'] = self.get_profiles(merged_profile, self.api_version, profile_label, profile_description, profile_stage, profile_date)

        # Wite new merged profile
        with open(merged_path, 'w') as file:
            json.dump(merged, file, indent = 4)

    def merge_capabilities(self, jsons, profile_names, api_version):
        merged_extensions = dict()
        merged_features = dict()
        merged_properties = dict()
        merged_formats = dict()
        merged_qfp = list()

        for i in range(len(jsons)):
            self.first = i == 0
            for capability_name in jsons[i]['profiles'][profile_names[i]]['capabilities']:
                capability = jsons[i]['capabilities'][capability_name]

                # Removed feature and properties not in the current json from already merged dicts
                if self.mode == 'intersection' and self.first is False:
                    if 'features' in capability:
                        for feature in dict(merged_features):
                            if feature not in capability['features']:
                                del merged_features[feature]
                    else:
                        merged_features.clear()

                    if 'properties' in capability:
                        for property in dict(merged_properties):
                            if property not in capability['properties'] or property == 'VkPhysicalDeviceSparseProperties' or property == 'sparseProperties':
                                del merged_properties[property]
                    else:
                        merged_properties.clear()

                if 'extensions' in capability:
                    if self.mode == 'union' or self.first:
                        for extension in capability['extensions']:
                            # vk_version = self.get_promoted_version(self.registry.extensions[extension].promotedTo)
                            # Check if the extension was not promoted in the version used
                            # if vk_version is None or (vk_version[0] > api_version[0]) or (vk_version[0] == api_version[0] and vk_version[1] > api_version[1]):
                            merged_extensions[extension] = capability['extensions'][extension]
                    else:
                        for extension in list(merged_extensions):
                            if not extension in capability['extensions']:
                                del merged_extensions[extension]
                if 'features' in capability:
                    for feature in capability['features']:
                        # Feature already exists, add or overwrite members
                        if feature in merged_features:
                            self.add_struct(feature, capability['features'][feature], merged_features)
                        else:
                            written = False
                            # Check if the promoted struct of current feature was already added
                            promoted_struct = self.get_promoted_struct_name(feature, True)
                            if promoted_struct and promoted_struct in merged_features:
                                self.add_members(merged_features[promoted_struct], capability['features'][feature])
                                written = True
                            # If this is a promoted struct, check if any structs already exists which are extension struct that are promoted to this struct
                            elif promoted_struct == feature:
                                # Add this structure
                                self.add_struct(feature, capability['features'][feature], merged_features)
                                # Combine all other extension structures (which are promoted to this version) into this structure
                                self.promote_structs(feature, merged_features, True)
                                written = True
                            if not written:
                                aliases = self.registry.structs[feature].aliases
                                for alias in aliases:
                                    if alias in merged_features:
                                        # Alias is already set, find which one to use
                                        struct = self.find_higher_struct(feature, alias)
                                        if struct == feature:
                                            merged_features[feature] = merged_features.pop(alias)
                                            self.add_members(merged_features[feature], capability['features'][feature])
                                        if struct == alias:
                                            self.add_members(merged_features[alias], capability['features'][feature])
                                        written = True
                                        break
                            if not written:
                                self.add_struct(feature, capability['features'][feature], merged_features)

                if 'properties' in capability:
                    for property in capability['properties']:
                        # Property already exists, add or overwrite members
                        if property == 'sparseProperties':
                            continue
                        elif property in merged_properties:
                            self.add_members(merged_properties[property], capability['properties'][property], property)
                        else:
                            # Check if the promoted struct of current property was already added
                            promoted_struct = self.get_promoted_struct_name(property, True)
                            if promoted_struct and promoted_struct in merged_properties:
                                self.add_members(merged_properties[promoted_struct], capability['properties'][property])
                            # If this is a promoted struct, check if any structs already exists which are extension struct that are promoted to this struct
                            elif promoted_struct == property:
                                # Add this structure
                                self.add_struct(property, capability['properties'][property], merged_properties)
                                # Combine all other extension structures (which are promoted to this version) into this structure
                                self.promote_structs(feature, merged_properties, True)
                            else:
                                aliases = self.registry.structs[property].aliases
                                for alias in aliases:
                                    if alias in merged_properties:
                                        # Alias is already set, find which one to use
                                        struct = self.find_higher_struct(property, alias)
                                        if struct == property:
                                            merged_properties[property] = merged_properties.pop(alias)
                                            self.add_members(merged_properties[property], capability['properties'][property])
                                        if struct == alias:
                                            self.add_members(merged_properties[alias], capability['properties'][property])
                                        break
                                self.add_struct(property, capability['properties'][property], merged_properties)
                if 'formats' in capability:
                    for format in capability['formats']:
                        if (format not in merged_formats) and (self.mode == 'union' or self.first):
                            merged_formats[format] = dict()
                            merged_formats[format]['VkFormatProperties'] = dict()
                            merged_formats[format]['VkFormatProperties3'] = dict()
                            merged_formats[format]['VkFormatProperties3KHR'] = dict()

                        if (format in merged_formats):
                            for prop_name in ['VkFormatProperties', 'VkFormatProperties3', 'VkFormatProperties3KHR']:
                                for features in ['linearTilingFeatures', 'optimalTilingFeatures', 'bufferFeatures']:
                                    self.merge_format_features(merged_formats, format, capability, prop_name, features)

                            # Remove empty entries (can occur when using intersect)
                            #if not dict(merged_formats[format]['VkFormatProperties']) and not dict(merged_formats[format]['VkFormatProperties3']) and not dict(merged_formats[format]['VkFormatProperties3KHR']):
                            #    del merged_formats[format]

                if 'queueFamiliesProperties' in capability:
                    if self.mode == 'intersection':
                        # If this is the first json just append all queue family properties
                        if self.first:
                            for qfp in capability['queueFamiliesProperties']:
                                merged_qfp.append(qfp)
                        # Otherwise do an intersect
                        else:
                            for mqfp in list(merged_qfp):
                                found = False
                                #if self.compareList(mqfp['VkQueueFamilyProperties']['queueFlags'], qfp['VkQueueFamilyProperties']['queueFlags']):
                                #    found = True
                                #    if (qfp['VkQueueFamilyProperties']['queueCount'] < mqfp['VkQueueFamilyProperties']['queueCount']):
                                #        mqfp['VkQueueFamilyProperties']['queueCount'] = qfp['VkQueueFamilyProperties']['queueCount']
                                #    if (qfp['VkQueueFamilyProperties']['timestampValidBits'] < mqfp['VkQueueFamilyProperties']['timestampValidBits']):
                                #        mqfp['VkQueueFamilyProperties']['timestampValidBits'] = qfp['VkQueueFamilyProperties']['timestampValidBits']
                                #    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width'] > mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width']):
                                #        mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width'] = qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width']
                                #    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height'] > mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height']):
                                #        mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height'] = qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height']
                                #    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth'] > mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth']):
                                #        mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth'] = qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth']
                                for qfp in capability['queueFamiliesProperties']:
                                    if mqfp['VkQueueFamilyProperties']['queueFlags'] != qfp['VkQueueFamilyProperties']['queueFlags']:
                                        continue
                                    if (qfp['VkQueueFamilyProperties']['queueCount'] != mqfp['VkQueueFamilyProperties']['queueCount']):
                                        continue
                                    if (qfp['VkQueueFamilyProperties']['timestampValidBits'] != mqfp['VkQueueFamilyProperties']['timestampValidBits']):
                                        continue
                                    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width']):
                                        continue
                                    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height']):
                                        continue
                                    if (qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth']):
                                        continue
                                    found = True
                                if not found:
                                    merged_qfp.remove(mqfp)
                                    
                    elif self.mode == 'union':
                        for qfp in capability['queueFamiliesProperties']:
                            if not merged_qfp:
                                merged_qfp.append(qfp)
                            else:
                                for mqfp in merged_qfp:
                                    if not self.compareList(mqfp['VkQueueFamilyProperties']['queueFlags'], qfp['VkQueueFamilyProperties']['queueFlags']):
                                        merged_qfp.append(qfp)
                                    elif qfp['VkQueueFamilyProperties']['queueCount'] != mqfp['VkQueueFamilyProperties']['queueCount']:
                                        merged_qfp.append(qfp)
                                    elif qfp['VkQueueFamilyProperties']['timestampValidBits'] != mqfp['VkQueueFamilyProperties']['timestampValidBits']:
                                        merged_qfp.append(qfp)
                                    elif qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['width']:
                                        merged_qfp.append(qfp)
                                    elif qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['height']:
                                        merged_qfp.append(qfp)
                                    elif qfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth'] != mqfp['VkQueueFamilyProperties']['minImageTransferGranularity']['depth']:
                                        merged_qfp.append(qfp)

                    else:
                        print("ERROR: Unknown combination mode: " + self.mode)

        capabilities = dict()
        capabilities['baseline'] = dict()
        if merged_extensions:
            sorted_extensions = collections.OrderedDict(sorted(merged_extensions.items()))
            capabilities['baseline']['extensions'] = dict(sorted_extensions)
        if merged_features:
            for feature in dict(merged_features):
                if not merged_features[feature]:
                    del merged_features[feature]

            sorted_features = collections.OrderedDict(sorted(merged_features.items()))
            capabilities['baseline']['features'] = dict(sorted_features)
        if merged_properties:
            for property in dict(merged_properties):
                if not merged_properties[property]:
                    del merged_properties[property]

            sorted_properties = collections.OrderedDict(sorted(merged_properties.items()))
            capabilities['baseline']['properties'] = dict(sorted_properties)
        if merged_formats:
            sorted_formats = collections.OrderedDict(sorted(merged_formats.items()))
            capabilities['baseline']['formats'] = dict(sorted_formats)

            # remove all empty elements
            formatsToRemove = list()

            for format in capabilities['baseline']['formats']:
                for prop_name in ['VkFormatProperties', 'VkFormatProperties3', 'VkFormatProperties3KHR']:
                    for features in ['linearTilingFeatures', 'optimalTilingFeatures', 'bufferFeatures']:
                        if features in capabilities['baseline']['formats'][format][prop_name]:
                            if not capabilities['baseline']['formats'][format][prop_name][features]:
                                del capabilities['baseline']['formats'][format][prop_name][features]
                    if prop_name in capabilities['baseline']['formats'][format]:
                        if not capabilities['baseline']['formats'][format][prop_name]:
                            del capabilities['baseline']['formats'][format][prop_name]
                if not capabilities['baseline']['formats'][format]:
                    formatsToRemove.append(format)

            for format in formatsToRemove:
                del capabilities['baseline']['formats'][format]

        if merged_qfp:
            capabilities['baseline']['queueFamiliesProperties'] = merged_qfp

        return capabilities

    def compareList(self, l1, l2):
        return collections.Counter(l1) == collections.Counter(l2)

    def merge_format_features(self, merged_formats, format, capability, prop_name, features):
        # Remove all format features not in current json if intersect is used
        if self.mode == 'intersection' and self.first is False:
            for mformat in dict(merged_formats):
                if mformat not in capability['formats']:
                    del merged_formats[mformat]

            # Remove format features not in intersect
            for feature in list(merged_formats[format][prop_name]):
                if feature not in capability['formats'][format][prop_name]:
                    merged_formats[format][prop_name].remove(feature)

        # Iterate all format features in current json
        if prop_name in capability['formats'][format]:
            if features in capability['formats'][format][prop_name]:
                # If mode is union or this is the first json when using intersect add the features if not already in merged features
                if features not in merged_formats[format][prop_name]:
                    if self.mode == 'union' or self.first == True:
                        merged_formats[format][prop_name][features] = capability['formats'][format][prop_name][features]
                else:
                    # In union add all aditional features
                    if self.mode == 'union':
                        for feature in capability['formats'][format][prop_name][features]:
                            if feature not in merged_formats[format][prop_name][features]:
                                merged_formats[format][prop_name][features].append(feature)
                    # In intersect removed features which are not set in the current json
                    else:
                        for feature in list(merged_formats[format][prop_name][features]):
                            if feature not in capability['formats'][format][prop_name][features]:
                                merged_formats[format][prop_name][features].remove(feature)

    def promote_structs(self, promoted, merged, feature):
        for struct in dict(merged):
            if self.get_promoted_struct_name(struct, feature) == promoted and struct is not promoted:
                # Union
                if self.mode == 'union':
                    for member in merged[struct]:
                        merged[promoted][member] = merged[struct][member]
                # Intersect
                #elif self.mode == 'intersection':
                #    if promoted in merged:
                #        for member in list(merged[promoted]):
                #            if member not in merged[struct]:
                #                del merged[promoted][member]
                #else:
                #    print("ERROR: Unknown combination mode: " + self.mode)
                #del merged[struct]


    def get_promoted_struct_name(self, struct, feature):
        # Workaround, because Vulkan11 structs were added in vulkan 1.2
        if struct == 'VkPhysicalDeviceVulkan11Features':
            return 'VkPhysicalDeviceVulkan11Features'
        if struct == 'VkPhysicalDeviceVulkan11Properties':
            return 'VkPhysicalDeviceVulkan11Properties'

        version = None
        if self.registry.structs[struct].definedByVersion:
            version = self.registry.structs[struct].definedByVersion
        else:
            aliases = self.registry.structs[struct].aliases
            for alias in aliases:
                if registry.structs[alias].definedByVersion:
                    version = registry.structs[alias].definedByVersion
                    break
        if version is None:
            return False
        return 'VkPhysicalDeviceVulkan' + str(version.major) + str(version.minor) + 'Features' if feature else 'Properties'

    def add_struct(self, struct_name, struct, merged):
        if struct_name == 'VkPhysicalDeviceSparseProperties':
            return
        elif struct_name in merged:
            # Union
            if self.mode == 'union':
                for member in struct:
                    if member in merged[struct_name]:
                        merged[struct_name][member] = merged[struct_name][member] or struct[member]
                    else:
                        merged[struct_name][member] = struct[member]
            # Intersect
            elif self.mode == 'intersection':
                if self.first is True:
                    for member in struct:
                        merged[struct_name][member] = struct[member]
                for member in list(merged[struct_name]):
                    if member not in struct:
                        del merged[struct_name][member]
                    elif struct[member] != merged[struct_name][member]:
                        del merged[struct_name][member]
            else:
                print("ERROR: Unknown combination mode: " + self.mode)
        else:
            if self.mode == 'union' or self.first is True:
                merged[struct_name] = struct

    def add_members(self, merged, entry, property = None):
        # First, remove all noauto member, they can't be merged
        if property != None:
            for member in list(merged):
                if (member not in self.registry.structs[property].members):
                    print('member: ' + member)
                    continue

                xmlmember = self.registry.structs[property].members[member]
                if xmlmember.limittype == 'noauto':
                    del merged[member]
                #elif 'mul'  in xmlmember.limittype and xmlmember.type == 'float':
                #    del merged[member]
        for member in entry:
            if property is None:
                if self.mode == 'union' or self.first is True:
                    merged[member] = entry[member]
            elif not member in merged:
                xmlmember = self.registry.structs[property].members[member]
                if xmlmember.limittype == 'noauto':
                    continue
                elif self.mode == 'union' or self.first is True:
                    if xmlmember.type == 'uint64_t' or xmlmember.type == 'VkDeviceSize':
                        merged[member] = int(entry[member])
                    else:
                        merged[member] = entry[member]
            else:
                # Merge properties
                xmlmember = self.registry.structs[property].members[member]
                if xmlmember.limittype == 'struct':
                    s = self.registry.structs[xmlmember.type].members
                    for smember in s:
                        if smember in merged[member]:
                            if smember in entry[member]:
                                self.merge_members(merged[member], smember, entry[member], s[smember])
                        #elif self.mode == 'union' and smember in entry[member]:
                        elif smember in entry[member]:
                            if xmlmember.type == 'uint64_t' or xmlmember.type == 'VkDeviceSize':
                                merged[member][smember] = int(entry[member][smember])
                            else:
                                merged[member][smember] = entry[member][smember]
                else:
                    self.merge_members(merged, member, entry, xmlmember)

    def merge_members(self, merged, member, entry, xmlmember):
        if xmlmember.limittype == 'exact':
            #if merged[member] != entry[member]:
            del merged[member]
        elif xmlmember.limittype == 'noauto':
            del merged[member]
        elif self.mode == 'union':
            #if xmlmember.limittype == 'exact':
                #if merged[member] != entry[member]:
                    # merged.remove(member)
                    # del merged[member]
                    # del entry[member]
                    #print("ERROR: '" + member + " 'values with 'exact' limittype have different values.")
            #if 'mul'  in xmlmember.limittype and xmlmember.type == 'float':
            #    del merged[member]
            if 'max' in xmlmember.limittype or xmlmember.limittype == 'bits':
                if xmlmember.type == 'VkExtent2D':
                    if entry[member]['width'] > merged[member]['width']:
                        merged[member]['width'] = entry[member]['width']
                    if entry[member]['height'] > merged[member]['height']:
                        merged[member]['height'] = entry[member]['height']
                elif xmlmember.arraySize == 3:
                    if entry[member][0] > merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] > merged[member][1]:
                        merged[member][1] = entry[member][1]
                    if entry[member][2] > merged[member][2]:
                        merged[member][2] = entry[member][2]
                elif xmlmember.arraySize == 2:
                    if entry[member][0] > merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] > merged[member][1]:
                        merged[member][1] = entry[member][1]
                else:
                    if entry[member] > merged[member]:
                        merged[member] = entry[member]
            elif 'min' in xmlmember.limittype:
                if xmlmember.type == 'VkExtent2D':
                    if entry[member]['width'] < merged[member]['width']:
                        merged[member]['width'] = entry[member]['width']
                    if entry[member]['height'] < merged[member]['height']:
                        merged[member]['height'] = entry[member]['height']
                elif xmlmember.arraySize == 3:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                    if entry[member][2] < merged[member][2]:
                        merged[member][2] = entry[member][2]
                elif xmlmember.arraySize == 2:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                else:
                    if entry[member] < merged[member]:
                        merged[member] = entry[member]
            elif xmlmember.limittype == 'bitmask':
                for smember in entry[member]:
                    if smember in merged[member]:
                        merged[member] = merged[member] or smember
                    else:
                        merged[member].append(smember)
            elif xmlmember.limittype == 'range':
                if entry[member][0] < merged[member][0]:
                    merged[member][0] = entry[member][0]
                if entry[member][1] > merged[member][1]:
                    merged[member][1] = entry[member][1]
            else:
                print("ERROR: Unknown limitype: " + xmlmember.limittype + " for " + member)
        elif self.mode == 'intersection':
            #if xmlmember.limittype == 'exact':
                #if merged[member] != entry[member]:
                    #merged.remove(member)
                    #del merged[member]
                    #del entry[member]
                    #print("ERROR: '" + member + " 'values with 'exact' limittype have different values.")
            #if 'mul'  in xmlmember.limittype and xmlmember.type == 'float':
            #    del merged[member]
            if 'max' in xmlmember.limittype or xmlmember.limittype == 'bits':
                if xmlmember.type == 'VkExtent2D':
                    if entry[member]['width'] < merged[member]['width']:
                        merged[member]['width'] = entry[member]['width']
                    if entry[member]['height'] < merged[member]['height']:
                        merged[member]['height'] = entry[member]['height']
                elif xmlmember.arraySize == 3:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                    if entry[member][2] < merged[member][2]:
                        merged[member][2] = entry[member][2]
                elif xmlmember.arraySize == 2:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                elif xmlmember.type == 'uint64_t' or xmlmember.type == 'VkDeviceSize':
                    if int(entry[member]) < int(merged[member]):
                        merged[member] = int(entry[member])
                    else:
                        merged[member] = int(merged[member])
                elif xmlmember.type == 'uint32_t' or xmlmember.type == 'int32_t'  or xmlmember.type == 'size_t' or xmlmember.type == 'float' or 'VkSampleCountFlagBits':
                    if entry[member] < merged[member]:
                        merged[member] = entry[member]
                else:
                    print("ERROR: '" + member + " 'values with 'max' limittype unknown case.")
            elif 'min' in xmlmember.limittype:
                if xmlmember.type == 'VkExtent2D':
                    if entry[member]['width'] < merged[member]['width']:
                        merged[member]['width'] = entry[member]['width']
                    if entry[member]['height'] < merged[member]['height']:
                        merged[member]['height'] = entry[member]['height']
                elif xmlmember.arraySize == 3:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                    if entry[member][2] < merged[member][2]:
                        merged[member][2] = entry[member][2]
                elif xmlmember.arraySize == 2:
                    if entry[member][0] < merged[member][0]:
                        merged[member][0] = entry[member][0]
                    if entry[member][1] < merged[member][1]:
                        merged[member][1] = entry[member][1]
                elif xmlmember.type == 'uint64_t' or xmlmember.type == 'VkDeviceSize':
                    if int(entry[member]) > int(merged[member]):
                        merged[member] = int(entry[member])
                    else:
                        merged[member] = int(merged[member])
                elif xmlmember.type == 'uint32_t' or xmlmember.type == 'int32_t' or xmlmember.type == 'size_t' or xmlmember.type == 'float' or 'VkSampleCountFlagBits':
                    if entry[member] > merged[member]:
                        merged[member] = entry[member]
                else:
                    print("ERROR: '" + member + " 'values with 'min' limittype unknown case.")
            elif xmlmember.limittype == 'bitmask':
                if xmlmember.type == 'VkBool32':
                    if member in entry:
                        merged[member] = merged[member] and entry[member]
                        if (not merged[member]):
                            del merged[member]
                    else:
                        merged.remove(member)
                else:
                    for value in merged[member]:
                        if value not in entry[member]:
                            merged[member].remove(value)
            elif xmlmember.limittype == 'range':
                if entry[member][0] > merged[member][0]:
                    merged[member][0] = entry[member][0]
                if entry[member][1] < merged[member][1]:
                    merged[member][1] = entry[member][1]
                #if member[1] < member[0]:
                #    merged.pop(member, None)
            else:
                print("ERROR: Unknown limitype: " + xmlmember.limittype + " for " + member)
        else:
            print("ERROR: Unknown combination mode: " + self.mode)

    def find_higher_struct(self, struct1, struct2):
        if registry.structs[struct1].definedByVersion:
            return struct1
        if registry.structs[struct2].definedByVersion:
            return struct2
        ext1_ext = False
        ext1_other = False
        ext2_ext = False
        ext2_other = False
        for ext in registry.structs[struct1].definedByExtensions:
            if registry.extensions[ext].name[3:6] == 'EXT':
                ext1_ext = True
            else:
                ext1_other = True
        for ext in registry.structs[struct2].definedByExtensions:
            if registry.extensions[ext].name[3:6] == 'EXT':
                ext2_ext = True
            else:
                ext2_other = True
        
        if not ext1_ext and not ext1_other:
            return struct1
        if not ext2_ext and not ext2_other:
            return struct2
        if not ext1_other:
            return struct1
        if not ext2_other:
            return struct2

        return struct1

    def get_promoted_version(self, vk_version):
        if vk_version is None:
            return None
        version = vk_version[11:]
        underscore = version.find('_')
        major = version[:underscore]
        minor = version[underscore+1:]
        return [major, minor]

    def get_profiles(self, profile_name, api_version, label, description, stage, date):
        profiles = dict()
        profiles[profile_name] = dict()
        profiles[profile_name]['version'] = 1
        if stage != 'STABLE':
            profiles[profile_name]['status'] = stage
        profiles[profile_name]['api-version'] = '.'.join(api_version)
        profiles[profile_name]['label'] = label
        profiles[profile_name]['description'] = description
        profiles[profile_name]['contributors'] = dict()
        profiles[profile_name]['history'] = list()
        revision = dict()
        revision['revision'] = 1
        revision['date'] = date
        revision['author'] = 'LunarG Profiles Generation'
        revision['comment'] = 'Generated profile'
        profiles[profile_name]['history'].append(revision)
        profiles[profile_name]['capabilities'] = list()
        profiles[profile_name]['capabilities'].append('baseline')
        return profiles

    def get_api_version(self, profiles):
        api_version = self.get_api_version_list(profiles[0]['api-version'])
        for profile in profiles:
            current_api_version = self.get_api_version_list(profile['api-version'])
            if self.mode == 'union':
                for i in range(len(api_version)):
                    if (api_version[i] > current_api_version[i]):
                        break
                    elif (api_version[i] < current_api_version[i]):
                        api_version = current_api_version
                        break
            elif self.mode == 'intersection':
                for i in range(len(api_version)):
                    if (api_version[i] < current_api_version[i]):
                        break
                    elif (api_version[i] > current_api_version[i]):
                        api_version = current_api_version
                        break
            else:
                print('ERROR: Unknown mode when computing api-version')
        return api_version

    def get_api_version_list(self, ver):
        version = ver.split('.')
        return version

    def get_profile_description(self, profile_names, mode):
        desc = 'Generated profile doing an ' + mode + ' between profiles: '
        count = len(profile_names)
        for i in range(count):
            desc += profile_names[i]
            if i == count - 2:
                desc += ' and '
            elif i < count - 2:
                desc += ', '
        return desc

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate Vulkan profile JSON files')

    parser.add_argument('--registry', '-r', action='store', required=True,
                        help='Use specified registry file instead of vk.xml.')
    parser.add_argument('--input', '-i', action='store', required=True,
                        help='Path to directory with profiles.')
    parser.add_argument('--input-profiles', action='store',
                        help='Comma separated list of profiles.')
    parser.add_argument('--output-path', '-o', action='store', required=True,
                        help='Path to output profile.')
    parser.add_argument('--output-profile', action='store',
                        help='Profile name of the output profile. If the argument is not set, the value is generated.')
    parser.add_argument('--profile-label', action='store',
                        help='Override the Label of the generated profile. If the argument is not set, the value is generated.')
    parser.add_argument('--profile-desc', action='store',
                        help='Override the Description of the generated profile. If the argument is not set, the value is generated.')
    parser.add_argument('--profile-date', action='store',
                        help='Override the release date of the generated profile. If the argument is not set, the value is generated.')
    parser.add_argument('--profile-api-version', action='store',
                        help='Override the Vulkan API version of the generated profile. If the argument is not set, the value is generated.')
    parser.add_argument('--profile-stage', action='store', choices=['ALPHA', 'BETA', 'STABLE'], default='STABLE',
                        help='Override the development stage of the generated profile. If the argument is not set, the value is set to "stable".')
    parser.add_argument('--mode', '-m', action='store', choices=['union', 'intersection'], default='intersection',
                        help='Mode of profile combination. If the argument is not set, the value is set to "intersection".')
          
    parser.set_defaults(mode='intersection')

    args = parser.parse_args()

    if args.registry is None:
        gen_profiles_solution.Log.e('Merging the profiles requires specifying --registry')
        parser.print_help()
        exit()

    if (args.mode.lower() != 'union' and args.mode.lower() != 'intersection'):
        gen_profiles_solution.Log.e('Mode must be either union or intersection')
        parser.print_help()
        exit()

    if args.output_profile is None:
        args.output_profile = 'VP_LUNARG_merged_' + datetime.now().strftime('%Y_%m_%d_%H_%M')
    elif not re.match('^VP_[A-Z0-9]+[A-Za-z0-9]+', args.output_profile):
        gen_profiles_solution.Log.e('Invalid output_profile, must follow regex pattern ^VP_[A-Z0-9]+[A-Za-z0-9]+')
        exit()

    if args.input_profiles is not None:
        profile_names = args.input_profiles.split(',')
    else:
        profile_names = list()

    if args.profile_label is not None:
        profile_label = args.profile_label
    else:
        profile_label = 'Generated profile'

    # Open file and load json
    jsons = list()
    profiles = list()
    if args.input is not None:
        profiles_not_found = profile_names.copy()
        # Find all jsons in the folder
        paths = [args.input + '/' + pos_json for pos_json in os.listdir(args.input) if pos_json.endswith('.json')]
        json_files = list()
        for i in range(len(paths)):
            print('Opening: ' + paths[i])
            file = open(paths[i], "r")
            json_files.append(json.load(file))
        # We need to iterate through profile names first, so the indices of jsons and profiles lists will match
        if (len(profile_names) > 0):
            for profile_name in profile_names:
                for json_file in json_files:
                    if 'profiles' in json_file and profile_name in json_file['profiles']:
                        jsons.append(json_file)
                        # Select profiles and capabilities
                        profiles.append(json_file['profiles'][profile_name])
                        profiles_not_found.remove(profile_name)
                        break
            if profiles_not_found:
                print('Profiles: ' + ' '.join(profiles_not_found) + ' not found in directory ' + args.input)
                exit()
        else:
            for json_file in json_files:
                if 'profiles' in json_file:
                    for profile in json_file['profiles']:
                        jsons.append(json_file)
                        profiles.append(json_file['profiles'][profile])
                        profile_names.append(profile)
    else:
        print('ERROR: Not input directory set, use --input')
        exit()

    registry = gen_profiles_solution.VulkanRegistry(args.registry)
    profile_merger = ProfileMerger(registry)

    if args.profile_desc is not None:
        profile_description = args.profile_desc
    else:
        profile_description = profile_merger.get_profile_description(profile_names, args.mode)

    if args.profile_stage is not None:
        profile_stage = args.profile_stage
    else:
        profile_stage = 'STABLE'

    if args.profile_date is not None:
        profile_date = args.profile_date
    else:
        # Get current time
        now = datetime.now()
        profile_date = str(now.year) + '-' + str(now.month).zfill(2) + '-' + str(now.day).zfill(2)

    profile_merger.merge(jsons, profiles, profile_names, args.output_path, args.output_profile, args.profile_label, args.profile_desc, args.profile_api_version, profile_stage, profile_date, args.mode)
    