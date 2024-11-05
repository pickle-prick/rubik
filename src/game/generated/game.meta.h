// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef GAME_META_H
#define GAME_META_H

typedef enum G_IconKind
{
G_IconKind_Null,
G_IconKind_FolderOpenOutline,
G_IconKind_FolderClosedOutline,
G_IconKind_FolderOpenFilled,
G_IconKind_FolderClosedFilled,
G_IconKind_FileOutline,
G_IconKind_FileFilled,
G_IconKind_Play,
G_IconKind_PlayStepForward,
G_IconKind_Pause,
G_IconKind_Stop,
G_IconKind_Info,
G_IconKind_WarningSmall,
G_IconKind_WarningBig,
G_IconKind_Unlocked,
G_IconKind_Locked,
G_IconKind_LeftArrow,
G_IconKind_RightArrow,
G_IconKind_UpArrow,
G_IconKind_DownArrow,
G_IconKind_Gear,
G_IconKind_Pencil,
G_IconKind_Trash,
G_IconKind_Pin,
G_IconKind_RadioHollow,
G_IconKind_RadioFilled,
G_IconKind_CheckHollow,
G_IconKind_CheckFilled,
G_IconKind_LeftCaret,
G_IconKind_RightCaret,
G_IconKind_UpCaret,
G_IconKind_DownCaret,
G_IconKind_UpScroll,
G_IconKind_DownScroll,
G_IconKind_LeftScroll,
G_IconKind_RightScroll,
G_IconKind_Add,
G_IconKind_Minus,
G_IconKind_Thread,
G_IconKind_Threads,
G_IconKind_Machine,
G_IconKind_CircleFilled,
G_IconKind_X,
G_IconKind_Refresh,
G_IconKind_Undo,
G_IconKind_Redo,
G_IconKind_Save,
G_IconKind_Window,
G_IconKind_Target,
G_IconKind_Clipboard,
G_IconKind_Scheduler,
G_IconKind_Module,
G_IconKind_XSplit,
G_IconKind_YSplit,
G_IconKind_ClosePanel,
G_IconKind_StepInto,
G_IconKind_StepOver,
G_IconKind_StepOut,
G_IconKind_Find,
G_IconKind_Palette,
G_IconKind_Thumbnails,
G_IconKind_Glasses,
G_IconKind_Binoculars,
G_IconKind_List,
G_IconKind_Grid,
G_IconKind_QuestionMark,
G_IconKind_Person,
G_IconKind_Briefcase,
G_IconKind_Dot,
G_IconKind_COUNT,
} G_IconKind;

typedef enum RD_ThemeColor
{
RD_ThemeColor_Null,
RD_ThemeColor_Text,
RD_ThemeColor_TextPositive,
RD_ThemeColor_TextNegative,
RD_ThemeColor_TextNeutral,
RD_ThemeColor_TextWeak,
RD_ThemeColor_Cursor,
RD_ThemeColor_CursorInactive,
RD_ThemeColor_Focus,
RD_ThemeColor_Hover,
RD_ThemeColor_DropShadow,
RD_ThemeColor_DisabledOverlay,
RD_ThemeColor_DropSiteOverlay,
RD_ThemeColor_InactivePanelOverlay,
RD_ThemeColor_SelectionOverlay,
RD_ThemeColor_HighlightOverlay,
RD_ThemeColor_HighlightOverlayError,
RD_ThemeColor_BaseBackground,
RD_ThemeColor_BaseBackgroundAlt,
RD_ThemeColor_BaseBorder,
RD_ThemeColor_MenuBarBackground,
RD_ThemeColor_MenuBarBackgroundAlt,
RD_ThemeColor_MenuBarBorder,
RD_ThemeColor_FloatingBackground,
RD_ThemeColor_FloatingBackgroundAlt,
RD_ThemeColor_FloatingBorder,
RD_ThemeColor_ImplicitButtonBackground,
RD_ThemeColor_ImplicitButtonBorder,
RD_ThemeColor_PlainButtonBackground,
RD_ThemeColor_PlainButtonBorder,
RD_ThemeColor_PositivePopButtonBackground,
RD_ThemeColor_PositivePopButtonBorder,
RD_ThemeColor_NegativePopButtonBackground,
RD_ThemeColor_NegativePopButtonBorder,
RD_ThemeColor_NeutralPopButtonBackground,
RD_ThemeColor_NeutralPopButtonBorder,
RD_ThemeColor_ScrollBarButtonBackground,
RD_ThemeColor_ScrollBarButtonBorder,
RD_ThemeColor_TabBackground,
RD_ThemeColor_TabBorder,
RD_ThemeColor_TabBackgroundInactive,
RD_ThemeColor_TabBorderInactive,
RD_ThemeColor_CodeDefault,
RD_ThemeColor_CodeSymbol,
RD_ThemeColor_CodeType,
RD_ThemeColor_CodeLocal,
RD_ThemeColor_CodeRegister,
RD_ThemeColor_CodeKeyword,
RD_ThemeColor_CodeDelimiterOperator,
RD_ThemeColor_CodeNumeric,
RD_ThemeColor_CodeNumericAltDigitGroup,
RD_ThemeColor_CodeString,
RD_ThemeColor_CodeMeta,
RD_ThemeColor_CodeComment,
RD_ThemeColor_CodeLineNumbers,
RD_ThemeColor_CodeLineNumbersSelected,
RD_ThemeColor_LineInfoBackground0,
RD_ThemeColor_LineInfoBackground1,
RD_ThemeColor_LineInfoBackground2,
RD_ThemeColor_LineInfoBackground3,
RD_ThemeColor_LineInfoBackground4,
RD_ThemeColor_LineInfoBackground5,
RD_ThemeColor_LineInfoBackground6,
RD_ThemeColor_LineInfoBackground7,
RD_ThemeColor_Thread0,
RD_ThemeColor_Thread1,
RD_ThemeColor_Thread2,
RD_ThemeColor_Thread3,
RD_ThemeColor_Thread4,
RD_ThemeColor_Thread5,
RD_ThemeColor_Thread6,
RD_ThemeColor_Thread7,
RD_ThemeColor_ThreadUnwound,
RD_ThemeColor_ThreadError,
RD_ThemeColor_Breakpoint,
RD_ThemeColor_CacheLineBoundary,
RD_ThemeColor_COUNT,
} RD_ThemeColor;

typedef enum RD_ThemePreset
{
RD_ThemePreset_DefaultDark,
RD_ThemePreset_DefaultLight,
RD_ThemePreset_VSDark,
RD_ThemePreset_VSLight,
RD_ThemePreset_SolarizedDark,
RD_ThemePreset_SolarizedLight,
RD_ThemePreset_HandmadeHero,
RD_ThemePreset_FourCoder,
RD_ThemePreset_FarManager,
RD_ThemePreset_COUNT,
} RD_ThemePreset;

typedef struct G_ParentNode G_ParentNode; struct G_ParentNode{G_ParentNode *next; G_Node * v;};
typedef struct G_BucketNode G_BucketNode; struct G_BucketNode{G_BucketNode *next; G_Bucket * v;};
typedef struct G_SceneNode G_SceneNode; struct G_SceneNode{G_SceneNode *next; G_Scene * v;};
typedef struct G_FlagsNode G_FlagsNode; struct G_FlagsNode{G_FlagsNode *next; G_NodeFlags v;};
typedef struct G_SeedNode G_SeedNode; struct G_SeedNode{G_SeedNode *next; G_Key v;};
typedef struct G_PathNode G_PathNode; struct G_PathNode{G_PathNode *next; String8 v;};
#define G_DeclStackNils \
struct\
{\
G_ParentNode parent_nil_stack_top;\
G_BucketNode bucket_nil_stack_top;\
G_SceneNode scene_nil_stack_top;\
G_FlagsNode flags_nil_stack_top;\
G_SeedNode seed_nil_stack_top;\
G_PathNode path_nil_stack_top;\
}
#define G_InitStackNils(state) \
state->parent_nil_stack_top.v = 0;\
state->bucket_nil_stack_top.v = 0;\
state->scene_nil_stack_top.v = 0;\
state->flags_nil_stack_top.v = 0;\
state->seed_nil_stack_top.v = g_nil_seed;\
state->path_nil_stack_top.v = str8(0,0);\

#define G_DeclStacks \
struct\
{\
struct { G_ParentNode *top; G_Node * bottom_val; G_ParentNode *free; B32 auto_pop; } parent_stack;\
struct { G_BucketNode *top; G_Bucket * bottom_val; G_BucketNode *free; B32 auto_pop; } bucket_stack;\
struct { G_SceneNode *top; G_Scene * bottom_val; G_SceneNode *free; B32 auto_pop; } scene_stack;\
struct { G_FlagsNode *top; G_NodeFlags bottom_val; G_FlagsNode *free; B32 auto_pop; } flags_stack;\
struct { G_SeedNode *top; G_Key bottom_val; G_SeedNode *free; B32 auto_pop; } seed_stack;\
struct { G_PathNode *top; String8 bottom_val; G_PathNode *free; B32 auto_pop; } path_stack;\
}
#define G_InitStacks(state) \
state->parent_stack.top = &state->parent_nil_stack_top; state->parent_stack.bottom_val = 0; state->parent_stack.free = 0; state->parent_stack.auto_pop = 0;\
state->bucket_stack.top = &state->bucket_nil_stack_top; state->bucket_stack.bottom_val = 0; state->bucket_stack.free = 0; state->bucket_stack.auto_pop = 0;\
state->scene_stack.top = &state->scene_nil_stack_top; state->scene_stack.bottom_val = 0; state->scene_stack.free = 0; state->scene_stack.auto_pop = 0;\
state->flags_stack.top = &state->flags_nil_stack_top; state->flags_stack.bottom_val = 0; state->flags_stack.free = 0; state->flags_stack.auto_pop = 0;\
state->seed_stack.top = &state->seed_nil_stack_top; state->seed_stack.bottom_val = g_nil_seed; state->seed_stack.free = 0; state->seed_stack.auto_pop = 0;\
state->path_stack.top = &state->path_nil_stack_top; state->path_stack.bottom_val = str8(0,0); state->path_stack.free = 0; state->path_stack.auto_pop = 0;\

#define G_AutoPopStacks(state) \
if(state->parent_stack.auto_pop) { g_pop_parent(); state->parent_stack.auto_pop = 0; }\
if(state->bucket_stack.auto_pop) { g_pop_bucket(); state->bucket_stack.auto_pop = 0; }\
if(state->scene_stack.auto_pop) { g_pop_scene(); state->scene_stack.auto_pop = 0; }\
if(state->flags_stack.auto_pop) { g_pop_flags(); state->flags_stack.auto_pop = 0; }\
if(state->seed_stack.auto_pop) { g_pop_seed(); state->seed_stack.auto_pop = 0; }\
if(state->path_stack.auto_pop) { g_pop_path(); state->path_stack.auto_pop = 0; }\

internal G_Node *                   g_top_parent(void);
internal G_Bucket *                 g_top_bucket(void);
internal G_Scene *                  g_top_scene(void);
internal G_NodeFlags                g_top_flags(void);
internal G_Key                      g_top_seed(void);
internal String8                    g_top_path(void);
internal G_Node *                   g_bottom_parent(void);
internal G_Bucket *                 g_bottom_bucket(void);
internal G_Scene *                  g_bottom_scene(void);
internal G_NodeFlags                g_bottom_flags(void);
internal G_Key                      g_bottom_seed(void);
internal String8                    g_bottom_path(void);
internal G_Node *                   g_push_parent(G_Node * v);
internal G_Bucket *                 g_push_bucket(G_Bucket * v);
internal G_Scene *                  g_push_scene(G_Scene * v);
internal G_NodeFlags                g_push_flags(G_NodeFlags v);
internal G_Key                      g_push_seed(G_Key v);
internal String8                    g_push_path(String8 v);
internal G_Node *                   g_pop_parent(void);
internal G_Bucket *                 g_pop_bucket(void);
internal G_Scene *                  g_pop_scene(void);
internal G_NodeFlags                g_pop_flags(void);
internal G_Key                      g_pop_seed(void);
internal String8                    g_pop_path(void);
internal G_Node *                   g_set_next_parent(G_Node * v);
internal G_Bucket *                 g_set_next_bucket(G_Bucket * v);
internal G_Scene *                  g_set_next_scene(G_Scene * v);
internal G_NodeFlags                g_set_next_flags(G_NodeFlags v);
internal G_Key                      g_set_next_seed(G_Key v);
internal String8                    g_set_next_path(String8 v);
C_LINKAGE_BEGIN
extern String8 g_icon_kind_text_table[69];
extern String8 rd_theme_preset_display_string_table[9];
extern String8 rd_theme_preset_code_string_table[9];
extern String8 rd_theme_color_version_remap_old_name_table[22];
extern String8 rd_theme_color_version_remap_new_name_table[22];
extern Vec4F32 rd_theme_preset_colors__default_dark[76];
extern Vec4F32 rd_theme_preset_colors__default_light[76];
extern Vec4F32 rd_theme_preset_colors__vs_dark[76];
extern Vec4F32 rd_theme_preset_colors__vs_light[76];
extern Vec4F32 rd_theme_preset_colors__solarized_dark[76];
extern Vec4F32 rd_theme_preset_colors__solarized_light[76];
extern Vec4F32 rd_theme_preset_colors__handmade_hero[76];
extern Vec4F32 rd_theme_preset_colors__four_coder[76];
extern Vec4F32 rd_theme_preset_colors__far_manager[76];
extern Vec4F32* rd_theme_preset_colors_table[9];
extern String8 rd_theme_color_display_string_table[76];
extern String8 rd_theme_color_cfg_string_table[76];

C_LINKAGE_END

#endif // GAME_META_H
