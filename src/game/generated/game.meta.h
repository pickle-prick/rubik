// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef GAME_META_H
#define GAME_META_H

typedef struct G_ParentNode G_ParentNode; struct G_ParentNode{G_ParentNode *next; G_Node * v;};
typedef struct G_BucketNode G_BucketNode; struct G_BucketNode{G_BucketNode *next; G_Bucket * v;};
typedef struct G_SceneNode G_SceneNode; struct G_SceneNode{G_SceneNode *next; G_Scene * v;};
typedef struct G_FlagsNode G_FlagsNode; struct G_FlagsNode{G_FlagsNode *next; G_NodeFlags v;};
#define G_DeclStackNils \
struct\
{\
G_ParentNode parent_nil_stack_top;\
G_BucketNode bucket_nil_stack_top;\
G_SceneNode scene_nil_stack_top;\
G_FlagsNode flags_nil_stack_top;\
}
#define G_InitStackNils(state) \
state->parent_nil_stack_top.v = 0;\
state->bucket_nil_stack_top.v = 0;\
state->scene_nil_stack_top.v = 0;\
state->flags_nil_stack_top.v = 0;\

#define G_DeclStacks \
struct\
{\
struct { G_ParentNode *top; G_Node * bottom_val; G_ParentNode *free; B32 auto_pop; } parent_stack;\
struct { G_BucketNode *top; G_Bucket * bottom_val; G_BucketNode *free; B32 auto_pop; } bucket_stack;\
struct { G_SceneNode *top; G_Scene * bottom_val; G_SceneNode *free; B32 auto_pop; } scene_stack;\
struct { G_FlagsNode *top; G_NodeFlags bottom_val; G_FlagsNode *free; B32 auto_pop; } flags_stack;\
}
#define G_InitStacks(state) \
state->parent_stack.top = &state->parent_nil_stack_top; state->parent_stack.bottom_val = 0; state->parent_stack.free = 0; state->parent_stack.auto_pop = 0;\
state->bucket_stack.top = &state->bucket_nil_stack_top; state->bucket_stack.bottom_val = 0; state->bucket_stack.free = 0; state->bucket_stack.auto_pop = 0;\
state->scene_stack.top = &state->scene_nil_stack_top; state->scene_stack.bottom_val = 0; state->scene_stack.free = 0; state->scene_stack.auto_pop = 0;\
state->flags_stack.top = &state->flags_nil_stack_top; state->flags_stack.bottom_val = 0; state->flags_stack.free = 0; state->flags_stack.auto_pop = 0;\

#define G_AutoPopStacks(state) \
if(state->parent_stack.auto_pop) { g_pop_parent(); state->parent_stack.auto_pop = 0; }\
if(state->bucket_stack.auto_pop) { g_pop_bucket(); state->bucket_stack.auto_pop = 0; }\
if(state->scene_stack.auto_pop) { g_pop_scene(); state->scene_stack.auto_pop = 0; }\
if(state->flags_stack.auto_pop) { g_pop_flags(); state->flags_stack.auto_pop = 0; }\

internal G_Node *                   g_top_parent(void);
internal G_Bucket *                 g_top_bucket(void);
internal G_Scene *                  g_top_scene(void);
internal G_NodeFlags                g_top_flags(void);
internal G_Node *                   g_bottom_parent(void);
internal G_Bucket *                 g_bottom_bucket(void);
internal G_Scene *                  g_bottom_scene(void);
internal G_NodeFlags                g_bottom_flags(void);
internal G_Node *                   g_push_parent(G_Node * v);
internal G_Bucket *                 g_push_bucket(G_Bucket * v);
internal G_Scene *                  g_push_scene(G_Scene * v);
internal G_NodeFlags                g_push_flags(G_NodeFlags v);
internal G_Node *                   g_pop_parent(void);
internal G_Bucket *                 g_pop_bucket(void);
internal G_Scene *                  g_pop_scene(void);
internal G_NodeFlags                g_pop_flags(void);
internal G_Node *                   g_set_next_parent(G_Node * v);
internal G_Bucket *                 g_set_next_bucket(G_Bucket * v);
internal G_Scene *                  g_set_next_scene(G_Scene * v);
internal G_NodeFlags                g_set_next_flags(G_NodeFlags v);
#endif // GAME_META_H
