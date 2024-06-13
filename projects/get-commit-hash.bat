@echo off

WHERE git
IF %ERRORLEVEL% NEQ 0 (
	echo "Git not found"
	ECHO #define MODULE_GIT_COMMIT "GIT_NOT_INSTALLED" > ../../src/generated_git_commit_hash.h
) ELSE (
	FOR /F "tokens=1" %%g IN ('git rev-parse --short HEAD') do (
		ECHO #define MODULE_GIT_COMMIT "%%g" > ../../src/generated_git_commit_hash.h
		echo "Git found, commit %%g"
	)	
)