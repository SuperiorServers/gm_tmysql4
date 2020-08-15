#ifndef TMYSQL4_RESULT
#define TMYSQL4_RESULT

#include <string>
#include <vector>

class PStatement;

typedef std::tuple<void*, unsigned int> ResultCell;

class Result
{
public:
	Result(MYSQL* mysql);
	Result(PStatement* pStmt);

	void PopulateLuaTable(lua_State* state, bool useNumbers);

private:
	const char*			m_strError;
	int					m_iError;
	double				m_iLastID;
	double				m_iAffected;

	void	Resize(int colCount, int rowCount);

	std::vector<const char*>				m_columnNames;
	std::vector<int>						m_columnTypes;
	std::vector<std::vector<ResultCell>>	m_rows;
	std::vector<std::vector<bool>>			m_nullRowValues;
};

typedef std::vector<Result*> Results;

#endif