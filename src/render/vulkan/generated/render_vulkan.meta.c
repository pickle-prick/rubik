// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#if 1
#define R_Vulkan_Cmd_Scope(v) DeferLoop(r_vulkan_push_cmd(v), r_vulkan_pop_cmd())
#endif
internal VkCommandBuffer r_vulkan_top_cmd(void) { R_Vulkan_StackTopImpl(r_vulkan_state, Cmd, cmd) }
internal VkCommandBuffer r_vulkan_bottom_cmd(void) { R_Vulkan_StackBottomImpl(r_vulkan_state, Cmd, cmd) }
internal VkCommandBuffer r_vulkan_push_cmd(VkCommandBuffer v) { R_Vulkan_StackPushImpl(r_vulkan_state, Cmd, cmd, VkCommandBuffer, v) }
internal VkCommandBuffer r_vulkan_pop_cmd(void) { R_Vulkan_StackPopImpl(r_vulkan_state, Cmd, cmd) }
internal VkCommandBuffer r_vulkan_set_next_cmd(VkCommandBuffer v) { R_Vulkan_StackSetNextImpl(r_vulkan_state, Cmd, cmd, VkCommandBuffer, v) }
