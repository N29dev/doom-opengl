# doom-opengl

Минимальная среда для разработки OpenGL на C++17 с GLFW и GLAD.

## Требования

- Windows 10/11
- Visual Studio 2022 с workload **Desktop development with C++**
- CMake 3.20 или новее
- Git в `PATH` — нужен CMake для загрузки GLFW и GLAD через FetchContent

## Сборка через PowerShell

Из корня репозитория:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Первый запуск скачает зависимости в `build/_deps`. Их не нужно добавлять в Git.

Запуск:

```powershell
.\build\Debug\doom_opengl.exe
```

Нажмите `Esc`, чтобы закрыть окно. При успешном запуске в консоли будет выведена версия OpenGL.

## Visual Studio

Можно открыть каталог репозитория через **File → Open → Folder**. Visual Studio обнаружит `CMakeLists.txt`, настроит CMake-проект и загрузит зависимости при конфигурации. В качестве startup item выберите `doom_opengl`.

Для ручной генерации решения используйте команду выше, затем откройте `build\DoomOpenGL.sln`.

## Структура

- `CMakeLists.txt` — сборка, OpenGL и FetchContent-зависимости.
- `src/main.cpp` — окно GLFW и инициализация GLAD.
