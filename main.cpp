#include <memory>
#include "db.h"
#include "crc32.h"
#include <cxx/cmdline.h>
#include <cxx/bar.h>

std::unique_ptr<Database> DB;
bool g_Recursive = false;
bool g_ClearDB = false;

class Failure : std::runtime_error
{
public:
	Failure() : std::runtime_error("Failure") {}
};

class Copier
{
	FileEntry&				m_Source;
	FileEntry&				m_Destination;
	std::ifstream			m_Input;
	std::fstream			m_Output;
	size_t					m_ActualOutputSize;
	std::vector<uint8_t>	m_Data;

	void copy_block(size_t offset, size_t block_size, bool already_read)
	{
		if (!already_read)
		{
			if (m_Data.size() < block_size)
				m_Data.resize(block_size);
			m_Input.seekg(offset);
			m_Input.read((char*)&m_Data[0], block_size);
			if (m_Input.gcount() != block_size) throw Failure();
		}
		if (m_Output.tellp() != offset)
			m_Output.seekp(offset);
		m_Output.write((const char*)&m_Data[0], block_size);
		m_Output.flush();
	}

	template<typename S>
	uint32_t read_block_crc(S& stream, size_t offset, size_t size)
	{
		if (m_Data.size() < size) m_Data.resize(size);
		stream.seekg(offset);
		if (stream.tellg()!=offset) throw Failure();
		stream.read((char*)&m_Data[0], size);
		if (stream.gcount() != size) throw Failure();
		return CRC32::calculate(&m_Data[0], size);
	}

	static std::ios_base::openmode target_flags(const path& filepath)
	{
		std::ios_base::openmode mode = (std::ios::out | std::ios::binary);
		if (fs::exists(filepath)) 
			mode |= std::ios::in;
		return mode;
	}

	void reopen_destination()
	{
		m_Output.close();
		m_Output.open(m_Destination.filepath, target_flags(m_Destination.filepath));
	}
public:
	Copier(FileEntry& source, FileEntry& destination)
		: m_Source(source)
		, m_Destination(destination)
		, m_Input(source.filepath, std::ios::in | std::ios::binary)
		, m_Output(destination.filepath, target_flags(destination.filepath))
		, m_Data(CRC_BLOCK_SIZE)
	{
	}

	bool copy()
	{
		std::wcout << L"Copying " << m_Source.filepath;
		std::cout << std::endl;
		try
		{
			size_t actual_output_size = m_Destination.filesize;
			if (m_Destination.filesize != m_Source.filesize)
				m_Destination.resize(m_Source.filesize);
			size_t offset = 0;
			size_t i = 0;
			size_t fail_count = 0;
			cxx::ProgressBar bar;
			while (i < m_Source.block_crcs.size())
			{
				bar.set_progress(int(i), int(m_Source.block_crcs.size()));
				size_t block_size = std::min((m_Source.filesize - offset), CRC_BLOCK_SIZE);
				bool already_read = false;
				if (m_Source.block_crcs[i] == 0)
				{
					m_Source.block_crcs[i] = read_block_crc(m_Input, offset, block_size);
					DB->save();
					already_read = true;
				}
				if (m_Source.block_crcs[i] == m_Destination.block_crcs[i])
				{
					++i;
					offset += block_size;
					continue;
				}
				copy_block(offset, block_size, already_read);
				// verify
				reopen_destination();
				if (m_Source.block_crcs[i] == read_block_crc(m_Output, offset, block_size))
				{
					// Successful block copy
					m_Destination.block_crcs[i] = m_Source.block_crcs[i];
					DB->save();
					++i;
					offset += block_size;
				}
				else
				{
					std::cerr << "CRC mismatch.  Retry..." << std::endl;
					if (++fail_count >= 5)
					{
						std::cerr << "Failed" << std::endl;
						return false;
					}
				}
			}
			bar.clear();
			return true;
		}
		catch (const Failure&)
		{}
		return false;
	}
};


void copy_file(const path& source, const path& destination)
{
	if (!DB->has_entry(destination))
		DB->create_entry(destination);
	if (!DB->has_entry(source))
		DB->create_entry(source);
	for (int retry_count = 0; retry_count < 5; ++retry_count)
	{
		Copier c(DB->get_entry(source), DB->get_entry(destination));
		if (c.copy()) break;
	}
}

void copy_directory(const path& source, const path& destination)
{
	for (auto const& dir_entry : fs::directory_iterator{ source })
	{
		path target = destination / dir_entry.path().filename();
		if (g_Recursive && dir_entry.is_directory())
		{
			fs::create_directories(target);
			copy_directory(dir_entry.path(), target);
		}
		else if (dir_entry.is_regular_file())
		{
			::copy_file(dir_entry.path(), target);
		}
	}
}

COMMAND_LINE_OPTION(r, false, "Recursive")
{
	g_Recursive = true;
}

COMMAND_LINE_OPTION(c, false, "Clear cache")
{
	g_ClearDB = true;
}

int main(int argc, char* argv[])
{
	PROCESS_COMMAND_LINE_P("<source> <destination>", 2, 2);
	try
	{
		DB = std::make_unique<Database>(g_ClearDB);
		path source, destination;
		*cmd >> source >> destination;
		bool src_dir = fs::is_directory(source);
		bool dst_dir = fs::is_directory(destination);
		if (src_dir && !dst_dir)
		{
			std::cout << "Cannot copy directory onto a file.\n";
			return 2;
		}
		if (!src_dir && dst_dir)
		{
			destination = destination / source.filename();
			dst_dir = false;
		}
		if (src_dir) copy_directory(source, destination);
		else ::copy_file(source, destination);
		DB.reset();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
