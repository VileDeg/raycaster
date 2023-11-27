cmake -S . -B build -G "Unix Makefiles"
Set-Location .\build
make
./rayc
