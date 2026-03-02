#pragma once
#include "file_load.h"
#include "metrics.h"
#include "string_extract.h"
#include "diff.h"
#include <string>
#include <vector>

// All application state in one struct, driven by the render loop.
struct AppState {
    // File loader
    FileLoader loader;

    // Loaded file data
    FileData files[2];          // 0=old, 1=new
    bool     file_loaded[2] = {false, false};

    // Path input buffers
    char path_buf[2][512] = {};

    // Computed metrics
    std::vector<BlockMetrics>   single_metrics;
    std::vector<BlockMetrics>   diff_metrics;
    std::vector<ExtractedString> strings[2];

    // Settings
    int   block_size_idx = 2;   // index into {64,128,256,512,1024}
    int   min_str_len    = 4;
    bool  color_by_entropy = true; // false = color by avg byte value
    bool  diff_mode      = false;

    // Heatmap view
    float zoom   = 1.0f;
    float pan_x  = 0.0f;
    float pan_y  = 0.0f;
    bool  panning = false;
    float pan_start_x = 0.f;
    float pan_start_y = 0.f;

    // Selection
    int   selected_block = -1;

    // String filter
    char  str_filter[256] = {};

    // Diff summary
    DiffSummary          diff_summary;
    std::vector<ChangedBlock> top_changed;

    // Export path
    char export_path[512] = "diff_export.txt";

    // Recomputation flag
    bool needs_recompute = false;
};

// Initialize application (called once).
void app_init(AppState& state);

// Render one frame of the UI.
void app_render(AppState& state);

// Called from GLFW drop callback.
void app_drop_files(AppState& state, int count, const char** paths);
