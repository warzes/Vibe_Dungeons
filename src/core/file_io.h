#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Portably read an entire file into a byte vector. Returns empty vector on failure.
[[nodiscard]] std::vector<uint8_t> FileReadBytes(const char* path);

// Portably read an entire text file into a string. Returns empty string on failure.
[[nodiscard]] std::string FileReadString(const char* path);

// Portably write bytes to a file. Returns true on success.
bool FileWriteBytes(const char* path, const void* data, size_t size);
