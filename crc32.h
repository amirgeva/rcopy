#pragma once

#include <vector>
#include <cstdint>

class CRC32
{
	static const std::vector<uint32_t>& get_table()
	{
		static std::vector<uint32_t> l_table;
		if (l_table.empty())
		{
			l_table.resize(256, 0);
			uint32_t polynomial = 0xEDB88320;
			for (uint32_t i = 0; i < 256; ++i)
			{
				uint32_t c = i;
				for (size_t j = 0; j < 8; ++j)
				{
					if (c & 1) {
						c = polynomial ^ (c >> 1);
					}
					else {
						c >>= 1;
					}
				}
				l_table[i] = c;
			}
		}
		return l_table;
	}
public:
	static uint32_t calculate(const uint8_t* data, size_t size)
	{
		uint32_t c = 0xFFFFFFFF;
		const auto& table = get_table();
		for (size_t i = 0; i < size; ++i)
			c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
		return c ^ 0xFFFFFFFF;
	}
};
