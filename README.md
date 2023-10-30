# DumbWinAFL
Build:
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" DumpWinAFL.sln /property:Configuration=Release

# Arguments
Теперь DumpBinAfl.exe необходимо передавать аргументы, их формат:
  1. -r <command> -основная команда запуска;
  2. -с -указывается, если вместо @@ необходимо в качественаргумента подафать строку, а не файл;
  3. -d <command> -указывается, если перед запуском основной команды (-r), необходимо запустить паралельный процесс (запускается только один раз;
  4. -t - если необходима задержка перед запуском осной команды, работает только вместе с аргументом -d

Пример:

DumpBinAfl.exe -r "my.exe @@" -c -d "new.exe -version" -t 4000
