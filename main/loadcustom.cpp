#include "loadcustom.hpp"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <vector>

#include "../support/error.hpp"

namespace
{
	// See readme.md for a description of the file structure!
	
	// The "file magic" occurs at the very start of the file and serves as an
	// identifier for this type of files. "Magic" codes such as these are very
	// common in many file formats. You can find a list of some of these at
	//   https://en.wikipedia.org/wiki/List_of_file_signatures
	// 
	// There are a few considerations when creating a file magic. If the file
	// is a binary file (not text), it is good practice to include a NUL byte
	// ('\0') or a similarly unprintable character early, preferable in the
	// first 4 or 8 bytes. This prevents the file from being misidentified as
	// text. It is probably a good idea to use a file magic that is a multiple
	// of 4 or 8 bytes in length, to help with alignment further down the file
	// (this is more important for memory mapped IO than for stream-based IO).
	// The first 4 or 8 bytes should be as unique as possible, as some
	// utilities may only consider those. It's probably also a good idea to
	// avoid any common existing magics (e.g. starting the file with "\177ELF"
	// is less-than-optimal).
	char kFileMagic[16] = "\0COMP3811mesh00";

	void fread_( void* aPtr, std::size_t, std::FILE* );

	struct FileDeleter
	{
		~FileDeleter();
		std::FILE* file;
	};
}

SimpleMeshData load_simple_binary_mesh(char const* aPath)
{
	std::FILE* fin = std::fopen(aPath, "rb");
	if (!fin) {
		throw Error("load_simple_binary_mesh(): Unable to open '%s' for reading", aPath);
	}

	FileDeleter fd{ fin };

	auto readAndVerifyMagic = [&]() {
		char magicBuffer[sizeof(kFileMagic)];
		fread_(magicBuffer, sizeof(kFileMagic), fin);
		if (std::memcmp(magicBuffer, kFileMagic, sizeof(kFileMagic)) != 0) {
			throw Error("'%s': not a COMP3811 mesh\n", aPath);
		}
		};

	readAndVerifyMagic();

	std::uint32_t metadata[2];
	auto readMetadata = [&]() { fread_(metadata, sizeof(metadata), fin); };
	readMetadata();

	std::vector<std::uint32_t> indexBuffer(metadata[1]);
	auto readIndices = [&]() {
		fread_(indexBuffer.data(), metadata[1] * sizeof(std::uint32_t), fin);
		};
	readIndices();

	SimpleMeshData resultMesh;
	auto reserveMeshData = [&](std::size_t size) {
		resultMesh.positions.reserve(size);
		resultMesh.colors.reserve(size);
		resultMesh.normals.reserve(size);
		};
	reserveMeshData(metadata[1]);

	std::vector<Vec3f> buffer(metadata[0]);

	auto populateData = [&](std::vector<Vec3f>& destination) {
		fread_(buffer.data(), buffer.size() * sizeof(Vec3f), fin);
		for (const auto& idx : indexBuffer) {
			destination.emplace_back(buffer[idx]);
		}
		};

	populateData(resultMesh.positions);
	populateData(resultMesh.colors);
	populateData(resultMesh.normals);

	return resultMesh;
}


namespace
{
	void fread_(void* aPtr, std::size_t aBytes, std::FILE* aFile)
	{
		auto* bytePtr = static_cast<std::byte*>(aPtr);

		auto checkError = [&](std::size_t remainingBytes) {
			if (auto err = std::ferror(aFile); err) {
				throw Error("fread_(): error while reading %zu bytes : %d", remainingBytes, err);
			}
			if (std::feof(aFile)) {
				throw Error("fread_(): unexpected EOF (%zu still to read)", remainingBytes);
			}
			};

		for (;;) {
			if (aBytes == 0) break;

			std::size_t bytesRead = std::fread(bytePtr, 1, aBytes, aFile);

			if (bytesRead == 0) {
				checkError(aBytes);
			}
			else {
				aBytes -= bytesRead;
				bytePtr += bytesRead;
			}
		}
	}

	FileDeleter::~FileDeleter()
	{
		if( file ) std::fclose( file );
	};
}

