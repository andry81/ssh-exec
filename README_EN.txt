* README_EN.txt
* 2024.04.21
* ssh-exec

1. DESCRIPTION
2. LICENSE
3. REPOSITORIES
4. PREREQUISITES
5. BUILD
6. AUTHOR

-------------------------------------------------------------------------------
1. DESCRIPTION
-------------------------------------------------------------------------------
Console application to execute commands (copy files) into remote hosts using
SSH and yaml config file as the input.

-------------------------------------------------------------------------------
2. LICENSE
-------------------------------------------------------------------------------
The MIT license (see included text file "license.txt" or
https://en.wikipedia.org/wiki/MIT_License)

-------------------------------------------------------------------------------
3. REPOSITORIES
-------------------------------------------------------------------------------
Primary:
  * https://github.com/andry81/ssh-exec/branches
    https://github.com/andry81/ssh-exec.git
First mirror:
  * https://sf.net/p/ssh-exec/ssh-exec/ci/master/tree
    https://svn.code.sf.net/p/ssh-exec/ssh-exec
Second mirror:
  * https://gitlab.com/andry81/ssh-exec/-/branches
    https://gitlab.com/andry81/ssh-exec.git

-------------------------------------------------------------------------------
4. PREREQUISITES
-------------------------------------------------------------------------------
Currently used these set of OS platforms, compilers, interpreters, modules,
IDE's, applications and patches to run with or from:

1. OS platforms:

* Windows 7+

2. C++11 compilers:

* (primary) Microsoft Visual C++ 2019+

3. IDE's:

* Microsoft Visual Studio 2019

-------------------------------------------------------------------------------
5. BUILD
-------------------------------------------------------------------------------
1. run: `build\ext\lib\rapidyaml\build-*.bat`
2. run: `build\ext\lib\libssh2\build-*.bat`
3. open and build: `build\msvc\ssh-exec.sln`

-------------------------------------------------------------------------------
6. AUTHOR
-------------------------------------------------------------------------------
Andrey Dibrov (andry at inbox dot ru)
