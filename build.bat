@echo off

set projdir=w:\talkien

if not exist %projdir%\build mkdir %projdir%\build
pushd %projdir%\build

::
:: Compiler flags
::

:: Preproc flags
:: -DINTERNAL_BUILD : is it debug build?
set cplflags=-DINTERNAL_BUILD=1

:: Optimization flags
:: -Oi : generate intrinsic funcs
:: -Od : disable optimization
set cplflags=%cplflags% -Oi -Od

:: Preprocessor output
:: set cplflags=%cplflags% -P

:: Code gen flags
:: -fp:fast : fast floating-point behavior
:: -Gm-     : disable minimal rebuild
:: -GR-     : disable run-time type info (RTTI)
:: -EHa-    : diasble exception handling
:: -MTd     : put multithreaded debug CRT into .obj
:: -Z7      : debug info format, put info into .obj
set cplflags=%cplflags% -fp:fast -Gm- -GR- -EHa- -MTd -Z7

:: Misc flags
:: -nologo : suppress the banner
:: -FC     : display full path for source codes
:: -WX     : treat warnings as errors
:: -W4     : level of warning
:: -wd4100 : unreferenced formal parameter
:: -wd4201 : nonstandard extension used: nameless struct/union
:: -wd4505 : unreferenced local function has been removed
:: -wd4189 : local variable is initialized but not referenced
:: -wd4101 : unreferenced local variable 
:: -wd4127 : conditional expression is constant
set cplflags=%cplflags% -nologo -FC -WX -W4 -wd4100 -wd4201 -wd4505 -wd4189 -wd4101 -wd4127

::
:: Linker flags
::

:: -nologo         : suppress the banner
:: -incremental:no : disable incremental linking
:: -opt:ref        : eliminate unreferenced funcs and data
set linkflags=-nologo -incremental:no -opt:ref

::
:: Build
::

del *.pdb > NUL 2> NUL
echo "waiting for msvc to finish writing to the pdb file..." > talkien.lock.dll

cl %cplflags% %projdir%\src\talkien.cpp -LD -link %linkflags% -PDB:talkien_%random%.pdb 

del talkien.lock.dll

cl %cplflags% %projdir%\src\win32_talkien.cpp -link %linkflags%

popd
