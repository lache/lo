#pragma once

int load_ktx_hw_or_sw_memory(const char* b, int* width, int* height, const char* tex_atlas_filename);
int load_ktx_hw_or_sw(const char* tex_atlas_filename, int* width, int* height);
