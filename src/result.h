#ifndef TMYSQL4_RESULT
#define TMYSQL4_RESULT

#include <string.h>
#include <vector>

class PStatement;

class Result
{
public:
	Result(int errNo, const char* errStr) : m_iError(errNo), m_strError(errStr) { }
	Result(MYSQL* mysql);
	Result(PStatement* pStmt);
	~Result();

	void PopulateLuaTable(lua_State* state, bool useNumbers);

private:
	const char*			m_strError;
	int					m_iError;
	double				m_iLastID;
	double				m_iAffected;
	int					m_rowCount = 0;
	int					m_colCount = 0;

	void	Resize(int colCount, int rowCount);

	std::string*	m_columnNames;
	int*			m_columnTypes;
	std::string**	m_rows;
	bool**			m_nullRowValues;
};

typedef std::vector<Result*> Results;

#endif