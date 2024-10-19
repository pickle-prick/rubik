// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#if 1
#define G_Parent_Scope(v) DeferLoop(g_push_parent(v), g_pop_parent())
#define G_Bucket_Scope(v) DeferLoop(g_push_bucket(v), g_pop_bucket())
#define G_Scene_Scope(v) DeferLoop(g_push_scene(v), g_pop_scene())
#define G_Flags_Scope(v) DeferLoop(g_push_flags(v), g_pop_flags())
#endif
internal G_Node * g_top_parent(void) { G_StackTopImpl(g_state, Parent, parent) }
internal G_Bucket * g_top_bucket(void) { G_StackTopImpl(g_state, Bucket, bucket) }
internal G_Scene * g_top_scene(void) { G_StackTopImpl(g_state, Scene, scene) }
internal G_NodeFlags g_top_flags(void) { G_StackTopImpl(g_state, Flags, flags) }
internal G_Node * g_bottom_parent(void) { G_StackBottomImpl(g_state, Parent, parent) }
internal G_Bucket * g_bottom_bucket(void) { G_StackBottomImpl(g_state, Bucket, bucket) }
internal G_Scene * g_bottom_scene(void) { G_StackBottomImpl(g_state, Scene, scene) }
internal G_NodeFlags g_bottom_flags(void) { G_StackBottomImpl(g_state, Flags, flags) }
internal G_Node * g_push_parent(G_Node * v) { G_StackPushImpl(g_state, Parent, parent, G_Node *, v) }
internal G_Bucket * g_push_bucket(G_Bucket * v) { G_StackPushImpl(g_state, Bucket, bucket, G_Bucket *, v) }
internal G_Scene * g_push_scene(G_Scene * v) { G_StackPushImpl(g_state, Scene, scene, G_Scene *, v) }
internal G_NodeFlags g_push_flags(G_NodeFlags v) { G_StackPushImpl(g_state, Flags, flags, G_NodeFlags, v) }
internal G_Node * g_pop_parent(void) { G_StackPopImpl(g_state, Parent, parent) }
internal G_Bucket * g_pop_bucket(void) { G_StackPopImpl(g_state, Bucket, bucket) }
internal G_Scene * g_pop_scene(void) { G_StackPopImpl(g_state, Scene, scene) }
internal G_NodeFlags g_pop_flags(void) { G_StackPopImpl(g_state, Flags, flags) }
internal G_Node * g_set_next_parent(G_Node * v) { G_StackSetNextImpl(g_state, Parent, parent, G_Node *, v) }
internal G_Bucket * g_set_next_bucket(G_Bucket * v) { G_StackSetNextImpl(g_state, Bucket, bucket, G_Bucket *, v) }
internal G_Scene * g_set_next_scene(G_Scene * v) { G_StackSetNextImpl(g_state, Scene, scene, G_Scene *, v) }
internal G_NodeFlags g_set_next_flags(G_NodeFlags v) { G_StackSetNextImpl(g_state, Flags, flags, G_NodeFlags, v) }
