#include "stdafx.h"
#include "core/file_io.h"

static FILE* openFile(const char* path, const char* mode)
{
#if defined(_MSC_VER)
	FILE* fp = nullptr;
	fopen_s(&fp, path, mode);
	return fp;
#else
	return fopen(path, mode);
#endif
}

std::vector<uint8_t> FileReadBytes(const char* path)
{
	FILE* fp = openFile(path, "rb");
	if (!fp)
	{
		return {};
	}

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return {};
	}

	long size = ftell(fp);
	if (size <= 0)
	{
		fclose(fp);
		return {};
	}

	rewind(fp);

	std::vector<uint8_t> result(static_cast<size_t>(size));
	size_t readBytes = fread(result.data(), 1, static_cast<size_t>(size), fp);
	fclose(fp);

	if (static_cast<long>(readBytes) != size)
	{
		return {};
	}

	return result;
}

std::string FileReadString(const char* path)
{
	auto bytes = FileReadBytes(path);
	if (bytes.empty())
	{
		return {};
	}
	return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

bool FileWriteBytes(const char* path, const void* data, size_t size)
{
	FILE* fp = openFile(path, "wb");
	if (!fp)
	{
		return false;
	}

	size_t written = fwrite(data, 1, size, fp);
	fclose(fp);
	return written == size;
}
