// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

C_LINKAGE_BEGIN
String8 r_tex2d_format_display_string_table[9] =
{
str8_lit_comp("R8"),
str8_lit_comp("RG8"),
str8_lit_comp("RGBA8"),
str8_lit_comp("BGRA8"),
str8_lit_comp("R16"),
str8_lit_comp("RGBA16"),
str8_lit_comp("R32"),
str8_lit_comp("RG32"),
str8_lit_comp("RGBA32"),
};

U8 r_tex2d_format_bytes_per_pixel_table[9] =
{
1,
2,
4,
4,
2,
8,
4,
8,
16,
};

String8 r_resource_kind_display_string_table[3] =
{
str8_lit_comp("Static"),
str8_lit_comp("Dynamic"),
str8_lit_comp("Stream "),
};

String8 r_tex2d_sample_kind_display_string_table[2] =
{
str8_lit_comp("Nearest"),
str8_lit_comp("Linear"),
};

String8 r_pass_kind_display_string_table[7] =
{
str8_lit_comp("Rect"),
str8_lit_comp("Blur"),
str8_lit_comp("Noise"),
str8_lit_comp("Edge"),
str8_lit_comp("Crt"),
str8_lit_comp("Geo2D"),
str8_lit_comp("Geo3D"),
};

U8 r_pass_kind_batch_table[7] =
{
1,
0,
0,
0,
0,
1,
1,
};

U64 r_pass_kind_params_size_table[7] =
{
sizeof(R_PassParams_Rect),
sizeof(R_PassParams_Blur),
sizeof(R_PassParams_Noise),
sizeof(R_PassParams_Edge),
sizeof(R_PassParams_Crt),
sizeof(R_PassParams_Geo2D),
sizeof(R_PassParams_Geo3D),
};

C_LINKAGE_END

