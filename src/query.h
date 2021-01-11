#pragma once

#include "databaseaction.h"

class Query : public DatabaseAction
{
public:
	Query(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false) : DatabaseAction(callback, callbackref, usenumbers), m_strQuery(query) { }
	std::string&		GetQuery(void) { return m_strQuery; }
private:
	std::string			m_strQuery;
};