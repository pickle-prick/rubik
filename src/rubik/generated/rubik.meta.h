// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef RUBIK_META_H
#define RUBIK_META_H

typedef enum RK_IconKind
{
RK_IconKind_Null,
RK_IconKind_FolderOpenOutline,
RK_IconKind_FolderClosedOutline,
RK_IconKind_FolderOpenFilled,
RK_IconKind_FolderClosedFilled,
RK_IconKind_FileOutline,
RK_IconKind_FileFilled,
RK_IconKind_Play,
RK_IconKind_PlayStepForward,
RK_IconKind_Pause,
RK_IconKind_Stop,
RK_IconKind_Info,
RK_IconKind_WarningSmall,
RK_IconKind_WarningBig,
RK_IconKind_Unlocked,
RK_IconKind_Locked,
RK_IconKind_LeftArrow,
RK_IconKind_RightArrow,
RK_IconKind_UpArrow,
RK_IconKind_DownArrow,
RK_IconKind_Gear,
RK_IconKind_Pencil,
RK_IconKind_Trash,
RK_IconKind_Pin,
RK_IconKind_RadioHollow,
RK_IconKind_RadioFilled,
RK_IconKind_CheckHollow,
RK_IconKind_CheckFilled,
RK_IconKind_LeftCaret,
RK_IconKind_RightCaret,
RK_IconKind_UpCaret,
RK_IconKind_DownCaret,
RK_IconKind_UpScroll,
RK_IconKind_DownScroll,
RK_IconKind_LeftScroll,
RK_IconKind_RightScroll,
RK_IconKind_Add,
RK_IconKind_Minus,
RK_IconKind_Thread,
RK_IconKind_Threads,
RK_IconKind_Machine,
RK_IconKind_CircleFilled,
RK_IconKind_X,
RK_IconKind_Refresh,
RK_IconKind_Undo,
RK_IconKind_Redo,
RK_IconKind_Save,
RK_IconKind_Window,
RK_IconKind_Target,
RK_IconKind_Clipboard,
RK_IconKind_Scheduler,
RK_IconKind_Module,
RK_IconKind_XSplit,
RK_IconKind_YSplit,
RK_IconKind_ClosePanel,
RK_IconKind_StepInto,
RK_IconKind_StepOver,
RK_IconKind_StepOut,
RK_IconKind_Find,
RK_IconKind_Palette,
RK_IconKind_Thumbnails,
RK_IconKind_Glasses,
RK_IconKind_Binoculars,
RK_IconKind_List,
RK_IconKind_Grid,
RK_IconKind_QuestionMark,
RK_IconKind_Person,
RK_IconKind_Briefcase,
RK_IconKind_Dot,
RK_IconKind_COUNT,
} RK_IconKind;

typedef enum RK_ThemeColor
{
RK_ThemeColor_Null,
RK_ThemeColor_Text,
RK_ThemeColor_TextPositive,
RK_ThemeColor_TextNegative,
RK_ThemeColor_TextNeutral,
RK_ThemeColor_TextWeak,
RK_ThemeColor_Cursor,
RK_ThemeColor_CursorInactive,
RK_ThemeColor_Focus,
RK_ThemeColor_Hover,
RK_ThemeColor_DropShadow,
RK_ThemeColor_DisabledOverlay,
RK_ThemeColor_DropSiteOverlay,
RK_ThemeColor_InactivePanelOverlay,
RK_ThemeColor_SelectionOverlay,
RK_ThemeColor_HighlightOverlay,
RK_ThemeColor_HighlightOverlayError,
RK_ThemeColor_BaseBackground,
RK_ThemeColor_BaseBackgroundAlt,
RK_ThemeColor_BaseBorder,
RK_ThemeColor_MenuBarBackground,
RK_ThemeColor_MenuBarBackgroundAlt,
RK_ThemeColor_MenuBarBorder,
RK_ThemeColor_FloatingBackground,
RK_ThemeColor_FloatingBackgroundAlt,
RK_ThemeColor_FloatingBorder,
RK_ThemeColor_ImplicitButtonBackground,
RK_ThemeColor_ImplicitButtonBorder,
RK_ThemeColor_PlainButtonBackground,
RK_ThemeColor_PlainButtonBorder,
RK_ThemeColor_PositivePopButtonBackground,
RK_ThemeColor_PositivePopButtonBorder,
RK_ThemeColor_NegativePopButtonBackground,
RK_ThemeColor_NegativePopButtonBorder,
RK_ThemeColor_NeutralPopButtonBackground,
RK_ThemeColor_NeutralPopButtonBorder,
RK_ThemeColor_ScrollBarButtonBackground,
RK_ThemeColor_ScrollBarButtonBorder,
RK_ThemeColor_TabBackground,
RK_ThemeColor_TabBorder,
RK_ThemeColor_TabBackgroundInactive,
RK_ThemeColor_TabBorderInactive,
RK_ThemeColor_CodeDefault,
RK_ThemeColor_CodeSymbol,
RK_ThemeColor_CodeType,
RK_ThemeColor_CodeLocal,
RK_ThemeColor_CodeRegister,
RK_ThemeColor_CodeKeyword,
RK_ThemeColor_CodeDelimiterOperator,
RK_ThemeColor_CodeNumeric,
RK_ThemeColor_CodeNumericAltDigitGroup,
RK_ThemeColor_CodeString,
RK_ThemeColor_CodeMeta,
RK_ThemeColor_CodeComment,
RK_ThemeColor_CodeLineNumbers,
RK_ThemeColor_CodeLineNumbersSelected,
RK_ThemeColor_LineInfoBackground0,
RK_ThemeColor_LineInfoBackground1,
RK_ThemeColor_LineInfoBackground2,
RK_ThemeColor_LineInfoBackground3,
RK_ThemeColor_LineInfoBackground4,
RK_ThemeColor_LineInfoBackground5,
RK_ThemeColor_LineInfoBackground6,
RK_ThemeColor_LineInfoBackground7,
RK_ThemeColor_Thread0,
RK_ThemeColor_Thread1,
RK_ThemeColor_Thread2,
RK_ThemeColor_Thread3,
RK_ThemeColor_Thread4,
RK_ThemeColor_Thread5,
RK_ThemeColor_Thread6,
RK_ThemeColor_Thread7,
RK_ThemeColor_ThreadUnwound,
RK_ThemeColor_ThreadError,
RK_ThemeColor_Breakpoint,
RK_ThemeColor_CacheLineBoundary,
RK_ThemeColor_COUNT,
} RK_ThemeColor;

typedef enum RK_ThemePreset
{
RK_ThemePreset_DefaultDark,
RK_ThemePreset_DefaultLight,
RK_ThemePreset_VSDark,
RK_ThemePreset_VSLight,
RK_ThemePreset_SolarizedDark,
RK_ThemePreset_SolarizedLight,
RK_ThemePreset_HandmadeHero,
RK_ThemePreset_FourCoder,
RK_ThemePreset_FarManager,
RK_ThemePreset_COUNT,
} RK_ThemePreset;

typedef enum RK_SettingCode
{
RK_SettingCode_HoverAnimations,
RK_SettingCode_PressAnimations,
RK_SettingCode_FocusAnimations,
RK_SettingCode_TooltipAnimations,
RK_SettingCode_MenuAnimations,
RK_SettingCode_ScrollingAnimations,
RK_SettingCode_TabWidth,
RK_SettingCode_MainFontSize,
RK_SettingCode_CodeFontSize,
RK_SettingCode_COUNT,
} RK_SettingCode;

typedef struct RK_ParentNode RK_ParentNode; struct RK_ParentNode{RK_ParentNode *next; RK_Node * v;};
typedef struct RK_BucketNode RK_BucketNode; struct RK_BucketNode{RK_BucketNode *next; RK_Bucket * v;};
typedef struct RK_SceneNode RK_SceneNode; struct RK_SceneNode{RK_SceneNode *next; RK_Scene * v;};
typedef struct RK_FlagsNode RK_FlagsNode; struct RK_FlagsNode{RK_FlagsNode *next; RK_NodeFlags v;};
typedef struct RK_PathNode RK_PathNode; struct RK_PathNode{RK_PathNode *next; String8 v;};
typedef struct RK_MeshCacheTableNode RK_MeshCacheTableNode; struct RK_MeshCacheTableNode{RK_MeshCacheTableNode *next; RK_MeshCacheTable * v;};
#define RK_DeclStackNils \
struct\
{\
RK_ParentNode parent_nil_stack_top;\
RK_BucketNode bucket_nil_stack_top;\
RK_SceneNode scene_nil_stack_top;\
RK_FlagsNode flags_nil_stack_top;\
RK_PathNode path_nil_stack_top;\
RK_MeshCacheTableNode mesh_cache_table_nil_stack_top;\
}
#define RK_InitStackNils(state) \
state->parent_nil_stack_top.v = 0;\
state->bucket_nil_stack_top.v = 0;\
state->scene_nil_stack_top.v = 0;\
state->flags_nil_stack_top.v = 0;\
state->path_nil_stack_top.v = str8(0,0);\
state->mesh_cache_table_nil_stack_top.v = 0;\

#define RK_DeclStacks \
struct\
{\
struct { RK_ParentNode *top; RK_Node * bottom_val; RK_ParentNode *free; B32 auto_pop; } parent_stack;\
struct { RK_BucketNode *top; RK_Bucket * bottom_val; RK_BucketNode *free; B32 auto_pop; } bucket_stack;\
struct { RK_SceneNode *top; RK_Scene * bottom_val; RK_SceneNode *free; B32 auto_pop; } scene_stack;\
struct { RK_FlagsNode *top; RK_NodeFlags bottom_val; RK_FlagsNode *free; B32 auto_pop; } flags_stack;\
struct { RK_PathNode *top; String8 bottom_val; RK_PathNode *free; B32 auto_pop; } path_stack;\
struct { RK_MeshCacheTableNode *top; RK_MeshCacheTable * bottom_val; RK_MeshCacheTableNode *free; B32 auto_pop; } mesh_cache_table_stack;\
}
#define RK_InitStacks(state) \
state->parent_stack.top = &state->parent_nil_stack_top; state->parent_stack.bottom_val = 0; state->parent_stack.free = 0; state->parent_stack.auto_pop = 0;\
state->bucket_stack.top = &state->bucket_nil_stack_top; state->bucket_stack.bottom_val = 0; state->bucket_stack.free = 0; state->bucket_stack.auto_pop = 0;\
state->scene_stack.top = &state->scene_nil_stack_top; state->scene_stack.bottom_val = 0; state->scene_stack.free = 0; state->scene_stack.auto_pop = 0;\
state->flags_stack.top = &state->flags_nil_stack_top; state->flags_stack.bottom_val = 0; state->flags_stack.free = 0; state->flags_stack.auto_pop = 0;\
state->path_stack.top = &state->path_nil_stack_top; state->path_stack.bottom_val = str8(0,0); state->path_stack.free = 0; state->path_stack.auto_pop = 0;\
state->mesh_cache_table_stack.top = &state->mesh_cache_table_nil_stack_top; state->mesh_cache_table_stack.bottom_val = 0; state->mesh_cache_table_stack.free = 0; state->mesh_cache_table_stack.auto_pop = 0;\

#define RK_AutoPopStacks(state) \
if(state->parent_stack.auto_pop) { rk_pop_parent(); state->parent_stack.auto_pop = 0; }\
if(state->bucket_stack.auto_pop) { rk_pop_bucket(); state->bucket_stack.auto_pop = 0; }\
if(state->scene_stack.auto_pop) { rk_pop_scene(); state->scene_stack.auto_pop = 0; }\
if(state->flags_stack.auto_pop) { rk_pop_flags(); state->flags_stack.auto_pop = 0; }\
if(state->path_stack.auto_pop) { rk_pop_path(); state->path_stack.auto_pop = 0; }\
if(state->mesh_cache_table_stack.auto_pop) { rk_pop_mesh_cache_table(); state->mesh_cache_table_stack.auto_pop = 0; }\

internal RK_Node *                  rk_top_parent(void);
internal RK_Bucket *                rk_top_bucket(void);
internal RK_Scene *                 rk_top_scene(void);
internal RK_NodeFlags               rk_top_flags(void);
internal String8                    rk_top_path(void);
internal RK_MeshCacheTable *        rk_top_mesh_cache_table(void);
internal RK_Node *                  rk_bottom_parent(void);
internal RK_Bucket *                rk_bottom_bucket(void);
internal RK_Scene *                 rk_bottom_scene(void);
internal RK_NodeFlags               rk_bottom_flags(void);
internal String8                    rk_bottom_path(void);
internal RK_MeshCacheTable *        rk_bottom_mesh_cache_table(void);
internal RK_Node *                  rk_push_parent(RK_Node * v);
internal RK_Bucket *                rk_push_bucket(RK_Bucket * v);
internal RK_Scene *                 rk_push_scene(RK_Scene * v);
internal RK_NodeFlags               rk_push_flags(RK_NodeFlags v);
internal String8                    rk_push_path(String8 v);
internal RK_MeshCacheTable *        rk_push_mesh_cache_table(RK_MeshCacheTable * v);
internal RK_Node *                  rk_pop_parent(void);
internal RK_Bucket *                rk_pop_bucket(void);
internal RK_Scene *                 rk_pop_scene(void);
internal RK_NodeFlags               rk_pop_flags(void);
internal String8                    rk_pop_path(void);
internal RK_MeshCacheTable *        rk_pop_mesh_cache_table(void);
internal RK_Node *                  rk_set_next_parent(RK_Node * v);
internal RK_Bucket *                rk_set_next_bucket(RK_Bucket * v);
internal RK_Scene *                 rk_set_next_scene(RK_Scene * v);
internal RK_NodeFlags               rk_set_next_flags(RK_NodeFlags v);
internal String8                    rk_set_next_path(String8 v);
internal RK_MeshCacheTable *        rk_set_next_mesh_cache_table(RK_MeshCacheTable * v);
C_LINKAGE_BEGIN
extern String8 rk_icon_kind_text_table[69];
extern String8 rk_theme_preset_display_string_table[9];
extern String8 rk_theme_preset_code_string_table[9];
extern String8 rk_theme_color_version_remap_old_name_table[22];
extern String8 rk_theme_color_version_remap_new_name_table[22];
extern Vec4F32 rk_theme_preset_colors__default_dark[76];
extern Vec4F32 rk_theme_preset_colors__default_light[76];
extern Vec4F32 rk_theme_preset_colors__vs_dark[76];
extern Vec4F32 rk_theme_preset_colors__vs_light[76];
extern Vec4F32 rk_theme_preset_colors__solarized_dark[76];
extern Vec4F32 rk_theme_preset_colors__solarized_light[76];
extern Vec4F32 rk_theme_preset_colors__handmade_hero[76];
extern Vec4F32 rk_theme_preset_colors__four_coder[76];
extern Vec4F32 rk_theme_preset_colors__far_manager[76];
extern Vec4F32* rk_theme_preset_colors_table[9];
extern String8 rk_theme_color_display_string_table[76];
extern String8 rk_theme_color_cfg_string_table[76];
extern String8 rk_setting_code_display_string_table[9];
extern String8 rk_setting_code_lower_string_table[9];
extern RK_SettingVal rk_setting_code_default_val_table[9];
extern Rng1S32 rk_setting_code_s32_range_table[9];

C_LINKAGE_END

#endif // RUBIK_META_H