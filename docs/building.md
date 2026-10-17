# Подключение и сборка

Команды выполняются из корня репозитория. Цели: Windows x64 и Linux x86_64. Ревизия движка и SHA256: [sdk-source.json](../sdk-source.json).

## Подключение

Добавьте `include/` в include path и подключите `mandagram_plugin.h` либо `mandagram_plugin.hpp` для C++-обёрток. SDK header-only, отдельная библиотека для линковки не нужна. Экспортируйте `mandagram_plugin_metadata` и `mandagram_plugin_init` через `PLUGIN_EXPORT` и `extern "C"`.

Пример: [plain_text.cpp](../examples/plain_text.cpp).

## Windows

Visual Studio C++ Build Tools с поддержкой C++17 и Windows SDK. Команды — в x64 Native Tools Command Prompt, не Git Bash.

```bat
mkdir build
cl /nologo /std:c++17 /O2 /EHsc /LD /Iinclude /Fobuild\ examples\plain_text.cpp /link /OUT:build\plain_text.dll
cl /nologo /std:c++17 /O2 /EHsc /LD /Iinclude /Fobuild\ examples\hello.cpp /link /OUT:build\hello.dll
cl /nologo /std:c++17 /O2 /EHsc /LD /Iinclude /Fobuild\ examples\chat_counter.cpp /link /OUT:build\chat_counter.dll
```

Плагины: `build/*.dll`; .lib/.exp/.obj не устанавливаются. Runtime компилятора должен быть доступен клиенту.

## Linux

GCC 9+ с C++17/std::filesystem и системный linker.

```sh
mkdir -p build
g++ -std=c++17 -O2 -fPIC -shared -Iinclude examples/plain_text.cpp -o build/plain_text.so
g++ -std=c++17 -O2 -fPIC -shared -Iinclude examples/hello.cpp -o build/hello.so
g++ -std=c++17 -O2 -fPIC -shared -Iinclude examples/chat_counter.cpp -o build/chat_counter.so
```

Плагины: `build/*.so`; это не Windows DLL.

## Поведение и ограничения

Plain Text удаляет точный префикс `.plain ` без аллокаций. Хост копирует возвращённую заимствованную подстроку в пределах вызова. Это не экранирование: результат с `{"@type":` может интерпретироваться как структурированный контент. Пустая строка не означает отмену отправки.

Hello пишет сообщение об инициализации. Chat Counter считает receive callbacks без хранения текста/ID; действие Show receive count выводит счётчик в журнал. Необязательный examples/config.txt (enabled=1 либо enabled=0) размещается в каталоге, возвращаемом get_storage_dir("chat_counter"). Отсутствующий конфиг включает счётчик.

C++-обёртки могут молча усекать строки фиксированными буферами; для контроля полноты используйте C API. JVM handles не являются адресами объектов. Не допускайте исключений через FFI.

ABI содержит 53 слота без согласования версии, shutdown/deinit и unregister. SDK должен соответствовать клиенту. Native-плагины работают внутри процесса, без песочницы; просмотр metadata может исполнить библиотеку. Не загружайте неизвестные DLL/SO. Для проверки в клиенте используйте отдельный portable root без рабочих аккаунтов. Source matched не означает runtime certified.

## Выполненная проверка

2026-10-17: все три примера собраны на Windows x64 (MSVC toolset 14.44.35207) и Linux x86_64 в WSL (GCC 16.2.1). DLL и SO загружены в отдельные временные C++ host-процессы с подключённым C++ SDK. Plain Text: два экспорта, метаданные, девять текстовых граничных случаев, заимствованный указатель, отказ регистрации и отсутствующая функция регистрации. Hello: сообщение инициализации. Chat Counter: подсчёт двух событий, выключенный и некорректный конфиг в изолированном временном каталоге. Заголовки и SHA256 сверены с закреплённой ревизией движка. Состав публикуемых исходников проверен на бинарники и шаблоны секретов. Загрузка в MandaGram не проверялась; данные клиента не использовались.
