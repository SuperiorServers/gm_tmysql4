#!/bin/sh
GIT_COMMIT="$(git rev-parse --short HEAD)"
if [ "$GIT_COMMIT" != "command not found" ]; then
  echo "#define MODULE_GIT_COMMIT \"$GIT_COMMIT\"" > ../../src/generated_git_commit_hash.h
else
  echo "#define MODULE_GIT_COMMIT \"GIT_NOT_INSTALLED\"" > ../../src/generated_git_commit_hash.h
fi