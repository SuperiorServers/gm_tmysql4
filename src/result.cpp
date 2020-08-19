#include "main.h"
#include "result.h"
#include "statement.h"

using namespace GarrysMod::Lua;

Result::~Result()
{
}

Result::Result(MYSQL* mysql) :
	m_iError(mysql_errno(mysql)),
	m_strError(mysql_error(mysql))
{
	MYSQL_RES* pResult = mysql_store_result(mysql);

	m_iLastID = mysql_insert_id(mysql);
	m_iAffected = mysql_affected_rows(mysql);

	if (pResult == nullptr)
	{
		mysql_free_result(pResult);
		return;
	}

	int colCount = mysql_num_fields(pResult);
	int rowCount = mysql_num_rows(pResult);

	Resize(colCount, rowCount);

	for (int i = 0; i < colCount; i++)
	{
		auto field = mysql_fetch_field_direct(pResult, i);

		m_columnNames[i] = field->name;
		m_columnTypes[i] = field->type;
	}

	for (int i = 0; i < rowCount; i++)
	{
		auto row = mysql_fetch_row(pResult);
		unsigned long* len = mysql_fetch_lengths(pResult);
		for (int n = 0; n < colCount; n++)
		{
			if (row[n])
				m_rows[i][n] = std::string(row[n], len[n]);
			else
				m_nullRowValues[i][n] = true;
		}
	}

	mysql_free_result(pResult);
}

Result::Result(PStatement* pStmt, double LastID, double Affected) :
	m_iError(mysql_stmt_errno(pStmt->GetInternal())),
	m_strError(mysql_stmt_error(pStmt->GetInternal())),
	m_iLastID(LastID),
	m_iAffected(Affected)
{
	auto stmt = pStmt->GetInternal();

	mysql_stmt_store_result(stmt);
	MYSQL_RES* pResult = mysql_stmt_result_metadata(stmt);

	if (pResult == nullptr)
	{
		mysql_free_result(pResult);
		return;
	}

	int colCount = mysql_stmt_field_count(stmt);
	int rowCount = mysql_stmt_num_rows(stmt);
	Resize(colCount, rowCount);

	std::vector<MYSQL_BIND> binds(colCount);
	std::vector<std::vector<char>> buf;
	std::vector<unsigned long> len(colCount);
	std::vector<my_bool> nullValues(colCount);

	for (unsigned int i = 0; i < colCount; i++)
	{
		auto field = mysql_fetch_field_direct(pResult, i);

		m_columnNames[i] = field->name;
		m_columnTypes[i] = field->type;

		MYSQL_BIND& bind = binds[i];

		// I really don't like making prepped statements mimic the char array style of MYSQL_ROW
		// I'll probably write something someday to make more efficient use of the buffer types
		bind.buffer_type = MYSQL_TYPE_STRING;
		buf.emplace_back(field->max_length);
		bind.buffer = buf.back().data();
		bind.buffer_length = field->max_length;
		bind.length = &len[i];
		bind.is_null = &nullValues[i];
		bind.is_unsigned = 0;
	}

	auto bdata = binds.data();
	mysql_stmt_bind_result(stmt, bdata);

	for (unsigned int i = 0; i < rowCount; i++)
	{
		int res = mysql_stmt_fetch(stmt);
		if (res == 0)
		{
			for (unsigned int n = 0; n < colCount; n++)
			{
				if (!*(bdata[n].is_null) && bdata[n].buffer)
					m_rows[i][n] = std::string(static_cast<char*>(bdata[n].buffer), *bdata[n].length);
				else
					m_nullRowValues[i][n] = true;
			}
		}
		else
		{
			m_iError = mysql_stmt_errno(stmt);
			m_strError = mysql_stmt_error(stmt);
			break;
		}
	}

	mysql_free_result(pResult);
}

void Result::Resize(int colCount, int rowCount)
{
	m_columnNames.resize(colCount);
	m_columnTypes.resize(colCount);
	m_rows.resize(rowCount);
	m_nullRowValues.resize(rowCount);

	for (int i = 0; i < rowCount; i++)
	{
		m_rows[i].resize(colCount);
		m_nullRowValues[i].resize(colCount);
	}
}

void Result::PopulateLuaTable(lua_State* state, bool useNumbers)
{
	bool status = m_iError == 0;

	LUA->PushBool(status);
	LUA->SetField(-2, "status");
	if (!status) {
		LUA->PushString(m_strError);
		LUA->SetField(-2, "error");
		LUA->PushNumber(m_iError);
		LUA->SetField(-2, "errorid");
	}
	else {
		LUA->PushNumber(m_iAffected == ((uint64_t)-1) ? -1.0 : m_iAffected);
		LUA->SetField(-2, "affected");
		LUA->PushNumber(m_iLastID);
		LUA->SetField(-2, "lastid");

		LUA->CreateTable();

		// no rows to push, continue, this isn't fatal
		if (m_rows.size() == 0)
		{
			LUA->SetField(-2, "data");
			return;
		}

		int rowid = 1;
		for (unsigned int r = 0; r < m_rows.size(); r++)
		{
			// black magic warning: we use a temp and assign it so that we avoid consuming all the temp objects and causing horrible disasters
			LUA->CreateTable();
			int resultrow = LUA->ReferenceCreate();

			LUA->PushNumber(rowid++);
			LUA->ReferencePush(resultrow);
			LUA->ReferenceFree(resultrow);

			for (unsigned int c = 0; c < m_columnNames.size(); c++)
			{
				if (useNumbers == true)
					LUA->PushNumber(c + 1);

				if (m_nullRowValues[r][c])
					LUA->PushNil();
				else if (IS_NUM(m_columnTypes[c]) && m_columnTypes[c] != MYSQL_TYPE_LONGLONG)
					LUA->PushNumber(atof(m_rows[r][c].c_str()));
				else
					LUA->PushString(m_rows[r][c].c_str(), m_rows[r][c].length());

				if (useNumbers == true)
					LUA->SetTable(-3);
				else
					LUA->SetField(-2, m_columnNames[c].c_str());
			}

			LUA->SetTable(-3);
		}

		LUA->SetField(-2, "data");
	}
}
