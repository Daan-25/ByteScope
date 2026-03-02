#include "ui.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

static const int kBlockSizes[] = {64, 128, 256, 512, 1024};
static const int kNumBlockSizes = 5;

static int current_block_size(const AppState& s) {
    return kBlockSizes[s.block_size_idx];
}

// ── Recompute metrics & strings when files change or settings change ──
static void recompute(AppState& s) {
    int bs = current_block_size(s);

    if (s.file_loaded[0] && !s.file_loaded[1]) {
        s.diff_mode = false;
        s.single_metrics = compute_metrics(s.files[0].data.data(),
                                           s.files[0].size, bs);
        s.strings[0] = extract_all_strings(s.files[0].data.data(),
                                           s.files[0].size, s.min_str_len);
    } else if (s.file_loaded[0] && s.file_loaded[1]) {
        s.diff_mode = true;
        s.single_metrics = compute_metrics(s.files[0].data.data(),
                                           s.files[0].size, bs);
        s.diff_metrics = compute_diff_metrics(s.files[0].data.data(), s.files[0].size,
                                              s.files[1].data.data(), s.files[1].size, bs);
        s.strings[0] = extract_all_strings(s.files[0].data.data(),
                                           s.files[0].size, s.min_str_len);
        s.strings[1] = extract_all_strings(s.files[1].data.data(),
                                           s.files[1].size, s.min_str_len);
        s.diff_summary.old_size = s.files[0].size;
        s.diff_summary.new_size = s.files[1].size;
        s.diff_summary.old_sha256 = s.files[0].sha256_hex;
        s.diff_summary.new_sha256 = s.files[1].sha256_hex;
        s.diff_summary.match = (s.files[0].sha256_hex == s.files[1].sha256_hex);
        s.diff_summary.differing_bytes = count_diff_bytes(
            s.files[0].data.data(), s.files[0].size,
            s.files[1].data.data(), s.files[1].size);
        s.top_changed = top_changed_blocks(s.diff_metrics);
    }
    s.selected_block = -1;
    s.needs_recompute = false;
}

void app_init(AppState& /*state*/) {
    // Nothing special needed at init time.
}

void app_drop_files(AppState& state, int count, const char** paths) {
    for (int i = 0; i < count && i < 2; ++i) {
        std::strncpy(state.path_buf[i], paths[i], sizeof(state.path_buf[i]) - 1);
        state.path_buf[i][sizeof(state.path_buf[i]) - 1] = '\0';
        state.loader.request_load(paths[i], i);
    }
}

// ── Color helpers ─────────────────────────────────────────────────────
static ImU32 entropy_color(float ent) {
    // 0 = dark blue (uniform/zeros), 8 = bright yellow (random)
    float t = ent / 8.0f;
    float r = t;
    float g = std::min(1.0f, t * 1.5f);
    float b = 1.0f - t;
    return IM_COL32(int(r*255), int(g*255), int(b*255), 255);
}

static ImU32 avg_byte_color(float avg) {
    float t = avg / 255.0f;
    return IM_COL32(int(t*255), int((1.f-t)*180), int((1.f-t)*255), 255);
}

static ImU32 diff_overlay_color(float ratio) {
    int a = int(ratio * 220);
    return IM_COL32(255, 40, 40, a);
}

// ── Top bar ───────────────────────────────────────────────────────────
static void draw_top_bar(AppState& s) {
    ImGui::BeginChild("##topbar", ImVec2(0, 40), true);
    ImGui::PushItemWidth(250);

    ImGui::Text("Old:");
    ImGui::SameLine();
    ImGui::InputText("##path0", s.path_buf[0], sizeof(s.path_buf[0]));
    ImGui::SameLine();
    if (ImGui::Button("Load##0") && s.path_buf[0][0] != '\0') {
        s.loader.request_load(s.path_buf[0], 0);
    }
    ImGui::SameLine(); ImGui::Text("  ");
    ImGui::SameLine();

    ImGui::Text("New:");
    ImGui::SameLine();
    ImGui::InputText("##path1", s.path_buf[1], sizeof(s.path_buf[1]));
    ImGui::SameLine();
    if (ImGui::Button("Load##1") && s.path_buf[1][0] != '\0') {
        s.loader.request_load(s.path_buf[1], 1);
    }

    if (s.diff_mode) {
        ImGui::SameLine(); ImGui::Text("  ");
        ImGui::SameLine();
        if (ImGui::Button("Export Diff")) {
            export_diff_ranges(s.export_path, s.diff_metrics, current_block_size(s));
        }
    }

    ImGui::PopItemWidth();
    ImGui::EndChild();
}

// ── Left panel ────────────────────────────────────────────────────────
static void draw_left_panel(AppState& s) {
    ImGui::BeginChild("##leftpanel", ImVec2(240, 0), true);
    ImGui::Text("Settings");
    ImGui::Separator();

    ImGui::Text("Block size:");
    const char* bs_labels[] = {"64","128","256","512","1024"};
    if (ImGui::Combo("##bsize", &s.block_size_idx, bs_labels, kNumBlockSizes))
        s.needs_recompute = true;

    ImGui::Text("Min string len:");
    if (ImGui::InputInt("##minstr", &s.min_str_len)) {
        if (s.min_str_len < 1) s.min_str_len = 1;
        s.needs_recompute = true;
    }

    if (!s.diff_mode)
        ImGui::Checkbox("Color by entropy", &s.color_by_entropy);

    if (ImGui::Button("Fit to window")) {
        s.zoom = 1.0f; s.pan_x = 0.f; s.pan_y = 0.f;
    }

    ImGui::Separator();
    ImGui::Text("Stats");

    if (s.file_loaded[0]) {
        ImGui::Text("Old: %s", s.files[0].path.c_str());
        ImGui::Text("Size: %zu bytes", s.files[0].size);
        ImGui::TextWrapped("SHA256: %.16s...", s.files[0].sha256_hex.c_str());
    }
    if (s.file_loaded[1]) {
        ImGui::Separator();
        ImGui::Text("New: %s", s.files[1].path.c_str());
        ImGui::Text("Size: %zu bytes", s.files[1].size);
        ImGui::TextWrapped("SHA256: %.16s...", s.files[1].sha256_hex.c_str());
    }
    if (s.diff_mode) {
        ImGui::Separator();
        ImGui::Text("Match: %s", s.diff_summary.match ? "YES" : "NO");
        ImGui::Text("Differing bytes: %zu", s.diff_summary.differing_bytes);
    }

    // Loading indicators
    for (int i = 0; i < 2; ++i) {
        if (s.loader.loading(i)) {
            ImGui::Separator();
            ImGui::Text("Loading slot %d: %.0f%%", i, s.loader.progress(i) * 100.f);
        }
        auto err = s.loader.error(i);
        if (!err.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,80,80,255));
            ImGui::TextWrapped("%s", err.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
}

// ── Heatmap rendering ─────────────────────────────────────────────────
static void draw_heatmap(AppState& s) {
    const auto& metrics = s.diff_mode ? s.diff_metrics : s.single_metrics;
    if (metrics.empty()) {
        ImGui::Text("No file loaded. Drag & drop a file or enter a path above.");
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float heatmapH = avail.y * 0.7f;
    ImGui::BeginChild("##heatmap_area", ImVec2(avail.x, heatmapH), true,
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Calculate grid dimensions
    int num_blocks = (int)metrics.size();
    int cols = std::max(1, (int)std::sqrt((double)num_blocks * canvas_size.x / canvas_size.y));
    int rows = (num_blocks + cols - 1) / cols;

    float base_cell = std::min(canvas_size.x / cols, canvas_size.y / rows);
    float cell = base_cell * s.zoom;

    // Handle zoom
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered()) {
        if (io.MouseWheel != 0.f) {
            float old_zoom = s.zoom;
            s.zoom *= (io.MouseWheel > 0) ? 1.15f : (1.f / 1.15f);
            s.zoom = std::max(0.1f, std::min(s.zoom, 50.f));
            // Zoom toward mouse
            float mx = io.MousePos.x - canvas_pos.x;
            float my = io.MousePos.y - canvas_pos.y;
            s.pan_x = mx - (mx - s.pan_x) * (s.zoom / old_zoom);
            s.pan_y = my - (my - s.pan_y) * (s.zoom / old_zoom);
        }
        // Pan with middle mouse or right-drag
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            s.pan_x += io.MouseDelta.x;
            s.pan_y += io.MouseDelta.y;
        }
    }

    // Clip to visible area
    dl->PushClipRect(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

    // Draw cells
    for (int i = 0; i < num_blocks; ++i) {
        int col = i % cols;
        int row = i / cols;
        float x0 = canvas_pos.x + s.pan_x + col * cell;
        float y0 = canvas_pos.y + s.pan_y + row * cell;
        float x1 = x0 + cell - 1;
        float y1 = y0 + cell - 1;

        // Frustum cull
        if (x1 < canvas_pos.x || x0 > canvas_pos.x + canvas_size.x ||
            y1 < canvas_pos.y || y0 > canvas_pos.y + canvas_size.y)
            continue;

        ImU32 color;
        if (s.diff_mode) {
            color = entropy_color(metrics[i].entropy);
            // Overlay diff ratio
            if (metrics[i].diff_ratio > 0.f) {
                dl->AddRectFilled(ImVec2(x0,y0), ImVec2(x1,y1), color);
                dl->AddRectFilled(ImVec2(x0,y0), ImVec2(x1,y1),
                                  diff_overlay_color(metrics[i].diff_ratio));
                continue;
            }
        } else {
            color = s.color_by_entropy ? entropy_color(metrics[i].entropy)
                                       : avg_byte_color(metrics[i].avg_byte);
        }
        dl->AddRectFilled(ImVec2(x0,y0), ImVec2(x1,y1), color);

        // Highlight selected
        if (i == s.selected_block) {
            dl->AddRect(ImVec2(x0,y0), ImVec2(x1,y1), IM_COL32(255,255,0,255), 0.f, 0, 2.f);
        }
    }

    // Click to select
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        float mx = io.MousePos.x - canvas_pos.x - s.pan_x;
        float my = io.MousePos.y - canvas_pos.y - s.pan_y;
        int col = (int)(mx / cell);
        int row = (int)(my / cell);
        if (col >= 0 && col < cols && row >= 0) {
            int idx = row * cols + col;
            if (idx >= 0 && idx < num_blocks)
                s.selected_block = idx;
        }
    }

    dl->PopClipRect();
    ImGui::EndChild();
}

// ── Entropy chart (ImPlot) ────────────────────────────────────────────
static void draw_entropy_chart(AppState& s) {
    const auto& metrics = s.diff_mode ? s.diff_metrics : s.single_metrics;
    if (metrics.empty()) return;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.y < 60) return;

    if (ImPlot::BeginPlot("Entropy per block", ImVec2(avail.x, avail.y), ImPlotFlags_NoTitle)) {
        ImPlot::SetupAxes("Block", "Entropy", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        // Build xs/ys arrays
        s.chart_xs.resize(metrics.size());
        s.chart_ys.resize(metrics.size());
        for (size_t i = 0; i < metrics.size(); ++i) {
            s.chart_xs[i] = float(i);
            s.chart_ys[i] = metrics[i].entropy;
        }
        ImPlot::PlotLine("Entropy", s.chart_xs.data(), s.chart_ys.data(), (int)metrics.size());

        // Highlight selected block
        if (s.selected_block >= 0 && s.selected_block < (int)metrics.size()) {
            float sx = float(s.selected_block);
            float sy = metrics[s.selected_block].entropy;
            ImPlot::PlotScatter("Selected", &sx, &sy, 1);
        }

        // Click to jump
        if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0)) {
            ImPlotPoint pt = ImPlot::GetPlotMousePos();
            int idx = (int)std::round(pt.x);
            if (idx >= 0 && idx < (int)metrics.size())
                s.selected_block = idx;
        }

        ImPlot::EndPlot();
    }
}

// ── Inspector panel ───────────────────────────────────────────────────
static void draw_inspector(AppState& s) {
    ImGui::Text("Inspector");
    ImGui::Separator();

    const auto& metrics = s.diff_mode ? s.diff_metrics : s.single_metrics;
    if (s.selected_block < 0 || s.selected_block >= (int)metrics.size()) {
        ImGui::Text("Click a block to inspect.");
        return;
    }

    const auto& bm = metrics[s.selected_block];
    ImGui::Text("Block #%d", s.selected_block);
    ImGui::Text("Offset: 0x%zX - 0x%zX", bm.offset, bm.offset + bm.length);
    ImGui::Text("Entropy: %.3f", bm.entropy);
    ImGui::Text("Avg byte: %.1f", bm.avg_byte);
    if (s.diff_mode)
        ImGui::Text("Diff ratio: %.1f%%", bm.diff_ratio * 100.f);

    ImGui::Separator();
    ImGui::Text("Hex preview (old):");

    // Show hex preview
    if (s.file_loaded[0] && bm.offset < s.files[0].size) {
        size_t end = std::min(bm.offset + bm.length, s.files[0].size);
        size_t len = end - bm.offset;
        const uint8_t* data = s.files[0].data.data() + bm.offset;
        int per_row = 16;
        for (size_t i = 0; i < len; i += per_row) {
            char line[128];
            int pos = 0;
            pos += std::snprintf(line + pos, sizeof(line) - pos, "%04zX: ", i);
            for (int j = 0; j < per_row && (i+j) < len; ++j) {
                if (s.diff_mode && s.file_loaded[1]) {
                    size_t abs_off = bm.offset + i + j;
                    uint8_t ob = data[i+j];
                    uint8_t nb = (abs_off < s.files[1].size) ? s.files[1].data[abs_off] : 0;
                    if (ob != nb) {
                        // highlight different bytes by color
                        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%02X", ob);
                        ImGui::SameLine(0, 0);
                        ImGui::Text(" ");
                        ImGui::SameLine(0, 0);
                        continue;
                    }
                }
                pos += std::snprintf(line + pos, sizeof(line) - pos, "%02X ", data[i+j]);
            }
            ImGui::TextUnformatted(line, line + pos);
        }
    }

    if (s.diff_mode && s.file_loaded[1]) {
        ImGui::Separator();
        ImGui::Text("Hex preview (new):");
        if (bm.offset < s.files[1].size) {
            size_t end = std::min(bm.offset + bm.length, s.files[1].size);
            size_t len = end - bm.offset;
            const uint8_t* data = s.files[1].data.data() + bm.offset;
            int per_row = 16;
            for (size_t i = 0; i < len; i += per_row) {
                char line[128];
                int pos = 0;
                pos += std::snprintf(line + pos, sizeof(line) - pos, "%04zX: ", i);
                for (int j = 0; j < per_row && (i+j) < len; ++j)
                    pos += std::snprintf(line + pos, sizeof(line) - pos, "%02X ", data[i+j]);
                ImGui::TextUnformatted(line, line + pos);
            }
        }
    }
}

// ── Strings list ──────────────────────────────────────────────────────
static void draw_strings_list(AppState& s) {
    ImGui::Separator();
    ImGui::Text("Strings");
    ImGui::InputText("Filter##str", s.str_filter, sizeof(s.str_filter));

    const auto& strs = s.strings[0];
    int bs = current_block_size(s);

    ImGui::BeginChild("##strlist", ImVec2(0, 0), true);
    for (size_t i = 0; i < strs.size(); ++i) {
        // Filter
        if (s.str_filter[0] != '\0') {
            if (strs[i].text.find(s.str_filter) == std::string::npos)
                continue;
        }
        char label[64];
        std::snprintf(label, sizeof(label), "0x%zX: ", strs[i].offset);
        // Truncate display
        std::string display = label + strs[i].text.substr(0, 60);
        if (strs[i].text.size() > 60) display += "...";

        if (ImGui::Selectable(display.c_str())) {
            // Jump to block
            int block_idx = (int)(strs[i].offset / bs);
            const auto& metrics = s.diff_mode ? s.diff_metrics : s.single_metrics;
            if (block_idx >= 0 && block_idx < (int)metrics.size()) {
                s.selected_block = block_idx;
                // Approximate centering in heatmap - not perfect but functional
            }
        }
    }
    ImGui::EndChild();
}

// ── Diff summary panel (right side, above inspector) ──────────────────
static void draw_diff_summary(AppState& s) {
    if (!s.diff_mode) return;

    ImGui::Text("Diff Summary");
    ImGui::Separator();
    ImGui::Text("Old size: %zu", s.diff_summary.old_size);
    ImGui::Text("New size: %zu", s.diff_summary.new_size);
    ImGui::Text("Match: %s", s.diff_summary.match ? "YES" : "NO");
    ImGui::Text("Diff bytes: %zu", s.diff_summary.differing_bytes);

    ImGui::Separator();
    ImGui::Text("Top changed blocks:");
    for (auto& cb : s.top_changed) {
        char label[64];
        std::snprintf(label, sizeof(label), "#%zu @0x%zX (%.1f%%)",
                      cb.index, cb.offset, cb.diff_ratio * 100.f);
        if (ImGui::Selectable(label))
            s.selected_block = (int)cb.index;
    }
    ImGui::Separator();
}

// ── Main render function ──────────────────────────────────────────────
void app_render(AppState& s) {
    // Poll loader results
    for (int i = 0; i < 2; ++i) {
        FileData fd;
        if (s.loader.poll_result(i, fd)) {
            s.files[i] = std::move(fd);
            s.file_loaded[i] = true;
            s.needs_recompute = true;
        }
    }
    if (s.needs_recompute) recompute(s);

    // Full-screen window
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("ByteScope", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoCollapse);

    // Top bar
    draw_top_bar(s);

    // Three-column layout
    float left_w = 240;
    float right_w = 320;
    float center_w = ImGui::GetContentRegionAvail().x - left_w - right_w - 16;
    if (center_w < 200) center_w = 200;

    // Left panel
    ImGui::BeginChild("##left", ImVec2(left_w, 0), false);
    draw_left_panel(s);
    ImGui::EndChild();

    ImGui::SameLine();

    // Center panel (heatmap + entropy chart)
    ImGui::BeginChild("##center", ImVec2(center_w, 0), false);
    draw_heatmap(s);
    draw_entropy_chart(s);
    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel (diff summary + inspector + strings)
    ImGui::BeginChild("##right", ImVec2(right_w, 0), false);
    draw_diff_summary(s);
    draw_inspector(s);
    ImGui::Separator();
    draw_strings_list(s);
    ImGui::EndChild();

    ImGui::End();
}
