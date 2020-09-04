# gm_tmysql4 **4.3**

## Building
1. Pull the [asio](https://github.com/chriskohlhoff/asio/) submodule using `git submodule update --init`
2. Run the batch/shell file to generate project files
3. Build!

Compiling should work out of the box for both Visual Studio and GCC as long as you use the premake script

On Linux, make sure the compiler is using the C++17 standard library. You should be linking directly to libmariadbclient.a as well.
You may also need to install libssl-dev.

On Windows, make sure the module is compiling as multi-threaded DLL (/MD flag) and that mariadbclient.lib is being linked.

For any other issues, just verify all include directories are properly set up.

# Changes
### 4.35 -> 4.37 : No breaking changes
```
* References to databases and statements are now properly invalidated on disconnect/gc
* tmysql.GetTable() now generates a table instead of maintaining and reusing a Lua table
* Metamethods properly error when being ran on an invalidated database/statement (and when necessary, disconnected databases also)
```

### 4.3 -> 4.35 : No breaking changes
```
* Module now takes advantage of newer asio types
* Fixed a couple issues in source that prevented linux from compiling out of the box
* Added safeguards for gc being called twice and crashing the module
* Provided mariadbclient libs now have caching_sha2_password statically linked
  (this was accomplished by compiling mariadbclient from source, setting caching_sha2_password
  to static rather than dynamic (and using openssl instead of gnutls on linux))
  You'll have to follow this process as well if you prefer to compile libmariadbclient yourself as well.
```
### 4.2 -> 4.3 : Prepared Statements
```
Added:
* Database:Prepare(String query)
  MySQL prepared statements are parameterized queries.
  Write a query with ? in place of your input to recycle the query object
  This is much more efficient and faster than running a traditional query many times with different data
  Additionally, inputs do not need to be escaped for prepared statements
  Returns nil, string if there was a problem preparing the statement

* PreparedStatement:Run(..., Function callback, Object anything, Boolean ColumnNumbers)
  Write your parameters in the function call first,
  followed by an optional callback function, object to pass to the callback, and boolean switch for named/numbered columns

* PreparedStatement:GetArgCount()
  Returns the number of parameters that MySQL expects in order to run the statement
```
### 4.1 -> 4.2 : caching_sha2_password support
```
Connector/C is now based on MariaDB 10.5.4
* Oracle does not supply 32bit versions of MySQL's newest C API libraries.

Module will now search for and load MySQL plugins inside lua/bin.
* Rejoice, for now we can connect to MySQL 8 servers using caching_sha2_password!
  ~~(hint: copy caching_sha2_password.dll or caching_sha2_password.so into lua/bin and it'll just work)~~
  **Superceded in 4.35 - caching_sha2_password is now statically linked and copying files is no longer necessary

Removed:
* tmysql.flags.CLIENT_LONG_PASSWORD (obsolete now)
```

### 4 -> 4.1 : Breaks backwards compatibility
```
Renamed:
* tmysql.initalize -> tmysql.Connect  
* Database:Option -> Database:SetOption

All enums are now separated into the following tables:  
* tmysql.flags  
* tmysql.opts  
* tmysql.info  

Added:
* tmysql.Version

Removed:
* tmysql.PollAll -- You have to manually poll now - it's faster, by a lot. Trust me.
  (hint: we recommend using Dash (https://github.com/SuperiorServers/dash), it makes this sort of thing super easy)
```

# Documentation

#### Creating a connection
``` lua
Database connection, String error = tmysql.Connect( String hostname, String username, String password, String database, Number port, String unixSocketPath, Number ClientFlags, Function ConnectCallback)
```

#### Escaping a String
``` lua
String escaped = Database:Escape( String stuff )
```
#### Getting all active connections
``` lua
Table connections = tmysql.GetTable()
```
#### Starting a connection
``` lua
Database:Connect() -- Starts and connects the database for tmysql.Create
```
#### Connect Options
``` lua
Database:SetOption( tmysql.opts.MYSQL_OPT_ENUM, String option ) -- Sets a mysql_option for the connection. Use with tmysql.Create then call Connect() after you set the options you want.
```
#### Stopping a connection
``` lua
Database:Disconnect() -- Ends the connection for this database and calls all pending callbacks immediately. Any method calls to this database, from now on, will error.
```
#### Query data from database
``` lua
Database:Query( String query, Function callback, Object anything, Boolean ColumnNumbers )
```
#### Running a prepared statement
``` lua
Statement = Database:Prepare( String query )
Statement:Run( String/Number/Boolean/nil arguments..., Function callback, Object anything, Boolean ColumnNumbers )
```
#### Callback structure
``` lua
function callback( Table results )
```
#### Results structure
```
Results {
	[1] = {
		status = true/false
		error = the error string
		affected = number of rows affected by the query
		lastid = the index of the last row inserted by the query
		time = time elapsed since creating the query and receiving the callback (not strictly useful for query benchmarking)
		data = {
			[1] = {
				columnName1 = "some data 1"
				columnName2 = "more data 1"
			},
			[2] = {
				columnName1 = "some data 2"
				columnName2 = "more data 2"
			}
		}
	}
}
```
If status is false, data, affected, and lastid will be nil

## Examples
#### Standard data retrieval
``` lua
local function onPlayerCompleted( ply, results )
	print( "Query for player completed", ply )
	PrintTable( results[1].data )
end

Database:Query( "SELECT * FROM some_table", onPlayerCompleted, Player(1) )

local function onCompleted( results )
	print( "Query completed" )
	PrintTable( results[1].data )
end

Database:Query( "SELECT * FROM some_table", onCompleted )

function GM:OurMySQLCallback( results )
	print( results )
end

Database:Query( "SELECT * FROM some_table", GAMEMODE.OurMySQLCallback, GAMEMODE ) -- Call the gamemode function
```

#### Prepared query data retrieval
``` lua
local statement, err = Database:Prepare( "SELECT * FROM some_table WHERE steamid = ?" )

if (!statement)
	ErrorNoHalt(err)
	return
end

for k, v in ipairs( player.GetAll() ) do
	statement:Run(v:SteamID64(), onPlayerCompleted, v)
end
```

#### Multiple results
``` lua
local Database, error = tmysql.Connect("localhost", "root", "root", "test", 3306, nil, tmysql.flags.CLIENT_MULTI_STATEMENTS)

local function onCompleted( results )
	print( "Query completed" )
	PrintTable( results )
	print("1+5 = ", results[2].data["1+5"])
end

Database:Query( "SELECT * FROM test; SELECT 1+5;", onCompleted )
```

#### Custom client IP
``` lua
local Database, error = tmysql.Create("localhost", "root", "root", "test", 3306, nil, tmysql.flags.CLIENT_MULTI_STATEMENTS)
Database:Option(tmysql.opts.MYSQL_SET_CLIENT_IP, "192.168.1.123")
local status, error = Database:Connect()
```
