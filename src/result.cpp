#include "main.h"
#include "result.h"

using namespace GarrysMod::Lua;

void Result::PopulateLuaTable(lua_State* state, bool useNumbers)
{

	bool status = m_iError == 0;

	LUA->PushBool(status);
	LUA->SetField(-2, "status");
	if (!status) {
		LUA->PushString(m_strError.c_str());
		LUA->SetField(-2, "error");
		LUA->PushNumber(m_iError);
		LUA->SetField(-2, "errorid");
	}
	else {
		LUA->PushNumber(m_iAffected);
		LUA->SetField(-2, "affected");
		LUA->PushNumber(m_iLastID);
		LUA->SetField(-2, "lastid");

		LUA->CreateTable();

		MYSQL_RES* result = m_pResult;

		// no result to push, continue, this isn't fatal
		if (result == NULL)
		{
			LUA->SetField(-2, "data");
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		MYSQL_FIELD* fields = mysql_fetch_fields(result);

		int rowid = 1;

		while (row)
		{
			unsigned int field_count = mysql_num_fields(result);
			unsigned long* lengths = mysql_fetch_lengths(result);

			// black magic warning: we use a temp and assign it so that we avoid consuming all the temp objects and causing horrible disasters
			LUA->CreateTable();
			int resultrow = LUA->ReferenceCreate();

			LUA->PushNumber(rowid++);
			LUA->ReferencePush(resultrow);
			LUA->ReferenceFree(resultrow);

			for (unsigned int i = 0; i < field_count; i++)
			{
				if (useNumbers == true)
					LUA->PushNumber(i + 1);

				if (row[i] == NULL)
					LUA->PushNil();
				else if (IS_NUM(fields[i].type) && fields[i].type != MYSQL_TYPE_LONGLONG)
					LUA->PushNumber(atof(row[i]));
				else
					LUA->PushString(row[i], lengths[i]);

				if (useNumbers == true)
					LUA->SetTable(-3);
				else
					LUA->SetField(-2, fields[i].name);
			}

			LUA->SetTable(-3);

			row = mysql_fetch_row(result);
		}

		LUA->SetField(-2, "data");
	}
}