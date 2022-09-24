#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace fs = std::filesystem;
using fs::path;

constexpr size_t CRC_BLOCK_SIZE = 67108864ULL; // 64 MB

struct FileEntry
{
	std::wstring			filepath;
	size_t					filesize=0;
	std::vector<uint32_t>	block_crcs;

	FileEntry(const path& p="") : filepath(p.wstring())
	{
		if (fs::exists(filepath))
		{
			init();
		}
	}

	void init()
	{
		filesize = fs::file_size(filepath);
		size_t block_count = (filesize + (CRC_BLOCK_SIZE - 1)) / CRC_BLOCK_SIZE;
		if (block_crcs.size() != block_count)
			block_crcs.resize(block_count, 0);
	}

	void resize(size_t new_size)
	{
		filesize = new_size;
		size_t block_count = (filesize + (CRC_BLOCK_SIZE - 1)) / CRC_BLOCK_SIZE;
		if (block_crcs.size() != block_count)
			block_crcs.resize(block_count, 0);
	}

	void save(std::ostream& os) const
	{
		uint32_t path_len = filepath.length();
		uint32_t crc_len = block_crcs.size();
		os.write((const char*)&path_len, sizeof(uint32_t));
		os.write((const char*)&crc_len, sizeof(uint32_t));
		os.write((const char*)&filesize, sizeof(size_t));
		os.write((const char*)filepath.c_str(), path_len * sizeof(wchar_t));
		if (!block_crcs.empty())
			os.write((const char*)&block_crcs[0], crc_len * sizeof(uint32_t));
	}

	bool load(std::istream& is)
	{
		uint32_t path_len;
		uint32_t crc_len;
		is.read((char*)&path_len, sizeof(uint32_t));
		if (is.gcount() != sizeof(uint32_t)) return false;
		is.read((char*)&crc_len, sizeof(uint32_t));
		if (is.gcount() != sizeof(uint32_t)) return false;
		is.read((char*)&filesize, sizeof(size_t));
		if (is.gcount() != sizeof(size_t)) return false;
		filepath.resize(path_len);
		block_crcs.resize(crc_len);
		is.read((char*)&filepath[0], path_len * sizeof(wchar_t));
		if (is.gcount() != (path_len*sizeof(wchar_t))) return false;
		if (!block_crcs.empty())
		{
			is.read((char*)&block_crcs[0], crc_len * sizeof(uint32_t));
			if (is.gcount() != (crc_len * sizeof(uint32_t))) return false;
		}
		return true;
	}
};

class Database
{
	path m_DBPath;
	std::unordered_map<path, FileEntry> m_Entries;

	bool load()
	{
		std::ifstream f(m_DBPath, std::ios::in | std::ios::binary);
		if (f.fail()) return false;
		uint32_t n;
		f.read((char*)&n, sizeof(uint32_t));
		for (uint32_t i = 0; i < n; ++i)
		{
			FileEntry e;
			if (e.load(f))
				m_Entries[e.filepath] = e;
			else
			{
				m_Entries.clear();
				return false;
			}
		}
		return true;
	}

public:
	Database(bool clear)
		: m_DBPath(fs::temp_directory_path() / "fdb")
	{
		if (!clear)
			load();
	}

	~Database()
	{
		save();
	}

	bool has_entry(const path& filepath)
	{
		return m_Entries.count(filepath) > 0;
	}

	void create_entry(const path& filepath)
	{
		m_Entries[filepath] = FileEntry(filepath);
	}

	FileEntry& get_entry(const path& filepath)
	{
		auto it = m_Entries.find(filepath);
		if (it == m_Entries.end()) throw std::runtime_error("Invalid DB query " + filepath.string());
		return it->second;
	}

	void save()
	{
		std::ofstream f(m_DBPath, std::ios::out | std::ios::binary);
		if (f.fail()) throw std::runtime_error("Cannot write DB");
		uint32_t n = m_Entries.size();
		f.write((const char*)&n, sizeof(uint32_t));
		for (const auto& [filepath, entry] : m_Entries)
			entry.save(f);
	}
};
