#pragma once

class MemoryMapper {
public:
	class Map {
	public:
		Map(void *data, int length)
		 : m_data(data), m_length(length)
		{ }
		virtual ~Map() { };

		void *getData(void)
		{
			return m_data;
		}

		int   getLength(void)
		{
			return m_length;
		}
	private:
		void *m_data;
		int   m_length;
	};

	static Map *map(const char *fname);
	static void unmap(Map *map);
};
