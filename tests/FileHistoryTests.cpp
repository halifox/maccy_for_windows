#include "ClipboardRules.h"
#include "Database.h"
#include "HistoryRenderer.h"
#include "PreviewDecoder.h"

#include <shlobj.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

ClipboardFormatData Files(const std::vector<std::wstring> &paths) {
    DROPFILES header{};
    header.pFiles = sizeof(header);
    header.fWide = TRUE;
    std::wstring list;
    for (const auto &path : paths) {
        list += path;
        list += L'\0';
    }
    list += L'\0';
    ClipboardFormatData data{L"CF_HDROP", CF_HDROP};
    data.bytes.resize(sizeof(header) + list.size() * sizeof(wchar_t));
    std::memcpy(data.bytes.data(), &header, sizeof(header));
    std::memcpy(data.bytes.data() + sizeof(header), list.data(), list.size() * sizeof(wchar_t));
    return data;
}

ClipboardSnapshot Capture(ClipboardFormatData data, bool image = false) {
    ClipboardSnapshot capture;
    capture.fingerprint = L"test-files";
    capture.title = L"truncated title";
    capture.preview = L"truncated path";
    capture.has_files = true;
    capture.has_image = image;
    capture.data.push_back(std::move(data));
    return capture;
}

void CheckFileMetadata() {
    const std::vector<std::wstring> paths{
        L"C:\\目录\\archive; backup.zip",
        L"C:\\" + std::wstring(5000, L'a') + L"\\photo.png",
    };
    auto files = Files(paths);
    Check(ClipboardRules::DecodeDropPaths(files.bytes) == paths, "File boundaries were lost");
    Database database(L":memory:");
    database.SaveClipboard(Capture(files, true), 10);
    const auto history = database.SearchHistory(L"", 0, 0, false);
    Check(history.size() == 1, "File history was not saved");
    const auto &item = history.front();
    Check(item.file_paths == paths && item.data.empty(), "Metadata must contain complete paths only");
    AppSettings settings;
    HistoryRenderer renderer(settings);
    Check(renderer.DisplayText(item) == L"archive; backup.zip; photo.png", "History must show filenames");
    Check(!DecodePreviewBitmap(item, 20, 20, {}), "Multiple files should retain path preview");
    const auto preview = database.GetItem(item.id, PayloadMode::Preview);
    const auto full = database.GetItem(item.id, PayloadMode::Full);
    const auto metadata = database.GetItem(item.id);
    Check(preview && preview->file_paths == paths, "Preview paths were truncated");
    Check(metadata && metadata->file_paths == paths, "Single item metadata lost paths");
    Check(full && full->data.size() == 1 && full->data.front().bytes == files.bytes,
        "Original file clipboard data must be preserved for paste");
    database.TogglePin(item.id, L"a", true);
    const auto pinned = database.GetPinnedItems();
    Check(pinned.size() == 1 && pinned.front().file_paths == paths, "Pinned file paths were lost");
    files.bytes.resize(sizeof(DROPFILES) - 1);
    Check(ClipboardRules::DecodeDropPaths(files.bytes).empty(), "Truncated file list was accepted");

    DROPFILES header{};
    header.pFiles = sizeof(header);
    const char ansi[] = "C:\\data\\report.docx\0";
    std::vector<unsigned char> bytes(sizeof(header) + sizeof(ansi));
    std::memcpy(bytes.data(), &header, sizeof(header));
    std::memcpy(bytes.data() + sizeof(header), ansi, sizeof(ansi));
    Check(ClipboardRules::DecodeDropPaths(bytes) == std::vector<std::wstring>{L"C:\\data\\report.docx"},
        "ANSI file lists must remain supported");
}

struct TemporaryFile {
    std::filesystem::path path;
    TemporaryFile() {
        wchar_t directory[MAX_PATH]{};
        wchar_t filename[MAX_PATH]{};
        Check(GetTempPathW(MAX_PATH, directory) != 0, "Cannot find temporary directory");
        Check(GetTempFileNameW(directory, L"mcy", 0, filename) != 0, "Cannot create temporary file");
        path = filename;
    }
    ~TemporaryFile() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
};

void CheckImagePreview() {
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = 4;
    info.biHeight = 2;
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biSizeImage = 24;
    BITMAPFILEHEADER file{};
    file.bfType = 0x4D42;
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + info.biSizeImage;
    std::vector<unsigned char> pixels(info.biSizeImage, 128);
    TemporaryFile image;
    {
        std::ofstream output(image.path, std::ios::binary);
        output.write(reinterpret_cast<const char *>(&file), sizeof(file));
        output.write(reinterpret_cast<const char *>(&info), sizeof(info));
        output.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
        Check(output.good(), "Cannot write test image");
    }
    Database database(L":memory:");
    database.SaveClipboard(Capture(Files({image.path.wstring()})), 10);
    const auto history = database.SearchHistory(L"", 0, 0, false);
    const auto item = database.GetItem(history.front().id, PayloadMode::Preview);
    auto bitmap = DecodePreviewBitmap(*item, 2, 2, {});
    Check(bitmap && bitmap->handle && bitmap->width == 2 && bitmap->height == 1,
        "Image files must preview with aspect ratio preserved");
    AppSettings settings;
    HistoryRenderer renderer(settings);
    Check(renderer.DisplayText(*item) == image.path.filename().wstring(), "Image files must show filenames");
    std::stop_source cancelled;
    cancelled.request_stop();
    Check(!DecodePreviewBitmap(*item, 2, 2, cancelled.get_token()), "Cancelled decoding must stop");

    ClipboardItem screenshot;
    screenshot.has_image = true;
    ClipboardFormatData dib{L"CF_DIB", CF_DIB};
    dib.bytes.resize(sizeof(info) + pixels.size());
    std::memcpy(dib.bytes.data(), &info, sizeof(info));
    std::memcpy(dib.bytes.data() + sizeof(info), pixels.data(), pixels.size());
    screenshot.data.push_back(std::move(dib));
    Check(DecodePreviewBitmap(screenshot, 2, 2, {}).has_value(), "Direct image preview regressed");

    std::filesystem::remove(image.path);
    Check(!DecodePreviewBitmap(*item, 2, 2, {}), "Missing images must fall back to paths");
    { std::ofstream output(image.path); output << "ordinary document"; }
    Check(!DecodePreviewBitmap(*item, 2, 2, {}), "Non-image files must retain path preview");
}

} // namespace

int main() {
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(result)) return 1;
    int exit_code = 0;
    try {
        CheckFileMetadata();
        CheckImagePreview();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        exit_code = 1;
    }
    CoUninitialize();
    return exit_code;
}
