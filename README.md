# DumbWinAFL
Build:
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" DumpWinAFL.sln /property:Configuration=Release

# CMD arguments fuzzing
для фаззинга аргументов коммандной строки необходимо добавить любой символ третьим аргументом
Пример:
DumpWinAFL.exe "example.exe @@" a
