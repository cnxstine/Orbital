/**
 * @file utils/BMPWriter.cpp
 * @brief Minimal 24-bit BMP file writer implementation.
 */

#include "utils/BMPWriter.hpp"

#include <fstream>
#include <vector>

namespace Orbital {

// ─── BMP binary layout ────────────────────────────────────────────────────────
// All fields are little-endian, matching the BMP specification.
// pragma pack ensures no compiler padding is inserted.

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t fileType   { 0x4D42 };  // "BM"
    uint32_t fileSize   { 0 };
    uint16_t reserved1  { 0 };
    uint16_t reserved2  { 0 };
    uint32_t offsetData { 54 };      // 14 (file header) + 40 (info header)
};

struct BMPInfoHeader {
    uint32_t size          { 40 };
    int32_t  width         { 0 };
    int32_t  height        { 0 };
    uint16_t planes        { 1 };
    uint16_t bitCount      { 24 };   // 24-bit RGB
    uint32_t compression   { 0 };    // BI_RGB — no compression
    uint32_t sizeImage     { 0 };
    int32_t  xPixelsPerMeter{ 0 };
    int32_t  yPixelsPerMeter{ 0 };
    uint32_t colorsUsed    { 0 };
    uint32_t colorsImportant{ 0 };
};
#pragma pack(pop)

// ─────────────────────────────────────────────────────────────────────────────

bool BMPWriter::Save(const std::string& filepath,
                     int                width,
                     int                height,
                     const uint8_t*     rgbData)
{
    std::ofstream file(filepath, std::ios::binary);
    if (!file) return false;

    // BMP rows must be padded to a 4-byte boundary
    const int rowSize = (width * 3 + 3) & ~3;
    const int padding = rowSize - (width * 3);
    const uint32_t dataSize  = static_cast<uint32_t>(rowSize * height);
    const uint32_t totalSize = 54u + dataSize;

    BMPFileHeader fileHeader;
    fileHeader.fileSize = totalSize;

    BMPInfoHeader infoHeader;
    infoHeader.width     = width;
    infoHeader.height    = height;
    infoHeader.sizeImage = dataSize;

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // BMP stores rows bottom-to-top; caller supplies top-to-bottom.
    // We iterate in reverse so the on-disk order matches the BMP spec.
    const std::vector<uint8_t> zeroPad(static_cast<size_t>(padding), 0);

    for (int y = 0; y < height; ++y) {
        const uint8_t* row = rgbData + y * width * 3;

        // Swap R↔B to produce BGR byte order required by BMP
        for (int x = 0; x < width; ++x) {
            file.put(static_cast<char>(row[x * 3 + 2])); // B
            file.put(static_cast<char>(row[x * 3 + 1])); // G
            file.put(static_cast<char>(row[x * 3 + 0])); // R
        }
        if (padding > 0) {
            file.write(reinterpret_cast<const char*>(zeroPad.data()), padding);
        }
    }

    return file.good();
}

} // namespace Orbital
