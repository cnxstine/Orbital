#pragma once

/**
 * @file utils/BMPWriter.hpp
 * @brief Minimal BMP file writer utility.
 *
 * Writes an uncompressed 24-bit BMP from a packed RGB byte buffer.
 * Rows are stored bottom-to-top per the BMP specification.
 *
 * Usage:
 *   std::vector<uint8_t> rgb(width * height * 3);
 *   // ... fill rgb ...
 *   BMPWriter::Save("output.bmp", width, height, rgb.data());
 */

#include <cstdint>
#include <string>

namespace Orbital {

class BMPWriter {
public:
    BMPWriter() = delete;

    /**
     * @brief Write a 24-bit BMP file from a packed RGB pixel buffer.
     *
     * @param filepath   Destination path (parent directory must exist or will be
     *                   created by the caller).
     * @param width      Image width in pixels.
     * @param height     Image height in pixels.
     * @param rgbData    Pointer to width*height*3 bytes in top-to-bottom,
     *                   left-to-right, R-G-B order.
     * @return true on success, false on I/O failure.
     */
    static bool Save(const std::string& filepath,
                     int                width,
                     int                height,
                     const uint8_t*     rgbData);
};

} // namespace Orbital
