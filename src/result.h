#ifndef TMYSQL4_RESULT
#define TMYSQL4_RESULT

#include <string>
#include <vector>

class Result
{
public:
	Result(MYSQL_RES* pResult, int iError, std::string strError, double iAffected, double iLastID) :
		m_pResult(pResult), m_iError(iError), m_strError(strError), m_iAffected(iAffected), m_iLastID(iLastID)
	{
	}

	~Result(void)
	{
		mysql_free_result(m_pResult);
	}

	void				PopulateLuaTable(lua_State* state, bool useNumbers);

private:
	std::string			m_strError;
	int					m_iError;
	double				m_iLastID;
	double				m_iAffected;
	MYSQL_RES*			m_pResult;
};

typedef std::vector<Result*> Results;

#endif