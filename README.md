# gm_tmysql4

## Building
[Boost](http://www.boost.org/) is the only extra thing needed to compile this module. It takes several gigs of storage.

Download their latest release and unzip. Inside the folder there's a shell script called bootstrap. Run this with include directories as an argument. Visual Studio users get a free pass here as opening the developer command prompt will allow you to run bootstrap.bat right out and everything will be good to go. Once this process completes, run the b2 executable that gets created.

This will take a while. Once it completes, add the proper include and lib directories to the project and you're good to go.


## Documentation

##### Creating a connection
``` lua
Database connection, String error = tmysql.initialize( String hostname, String username, String password, String database, Number port, String unixSocketPath, Number ClientFlags, Function ConnectCallback)
```

##### Escaping a String
``` lua
String escaped = Database:Escape( String stuff )
```
##### Getting all active connections
``` lua
Table connections = tmysql.GetTable()
```
##### Starting a connection
``` lua
Database:Connect() -- Starts and connects the database for tmysql.Create
```
##### Connect Options
``` lua
Database:Option(MYSQL_OPT_ENUM, String option) -- Sets a mysql_option for the connection. Use with tmysql.Create then call Connect() after you set the options you want.
```
##### Stopping a connection
``` lua
Database:Disconnect() -- Ends the connection for this database and calls all pending callbacks immediately. Any method calls to this database, from now on, will error.
```
##### Query data from database
``` lua
Database:Query( String query, Function callback, Object anything, Boolean ColumnNumbers )
```
##### Callback structure
``` lua
function callback( Table results )
```
##### Results structure
``` lua
Results {
	[1] = {
		status = true/false,
		error = the error string,
		affected = number of rows affected by the query,
		lastid = the index of the last row inserted by the query,
		time = how long it took to complete,
		data = {
			{
				somefield = "some shit",
				otherfield = "other shit",
			}
		},
	},
}
```
If status is false, data, affected, and lastid will be nil

## Examples
##### Standard data retrieval
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
##### Multiple results
``` lua
local Database, error = tmysql.Connect("localhost", "root", "root", "test", 3306, nil, CLIENT_MULTI_STATEMENTS)

local function onCompleted( results )
	print( "Query completed" )
	PrintTable( results )
	print("1+5 = ", results[2].data["1+5"])
end

Database:Query( "SELECT * FROM test; SELECT 1+5;", onCompleted )
```

##### Custom client IP
``` lua
local Database, error = tmysql.Create("localhost", "root", "root", "test", 3306, nil, CLIENT_MULTI_STATEMENTS)
Database:Option(MYSQL_SET_CLIENT_IP, "192.168.1.123")
local status, error = Database:Connect()
```
