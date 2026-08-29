# LunaPassport Patcher

Win32 GUI для Windows XP: один `.exe`, который настраивает LunaPassport lab на машине.

Патчит автоматически:

- `hosts` (`passport-staging.<domain>`, `memberservices-staging.<domain>`, `register.passport.com`)
- реестр Passport (`Internet Settings\Passport`)
- корневой CA Let's Encrypt (ISRG Root X1, встроен в exe)

## Сборка (Windows)

Нужно только:

1. [CMake](https://cmake.org/download/)
2. MinGW-w64 **i686 (32-bit) + MSVCRT** — не x86_64 и не UCRT

**Важно для Windows XP:** exe должен быть 32-bit. Если собрать 64-bit MinGW (как WinLibs UCRT x86_64), на XP появится ошибка «не является приложением Win32».

Автоустановка тулчейна:

```bat
setup-toolchain.bat
build.bat
```

`setup-toolchain.bat` скачает WinLibs i686 GCC 12.4 MSVCRT в `tools\mingw32\`.

Дальше — двойной клик или из cmd:

```bat
build.bat
```

Готовый файл: `build\lunapassport-patcher.exe`

Никаких DLL, .NET и установщиков не нужно — всё статически в одном exe.

### Если MinGW ещё не установлен

Скачайте MinGW-w64 **i686 MSVCRT** (32-bit), распакуйте, добавьте `bin` в PATH.

Или запустите `setup-toolchain.bat` — он скачает подходящую сборку автоматически.

**Не подходит для XP:** x86_64 (64-bit) и UCRT runtime (Windows 7+).

Проверка:

```bat
g++ --version
cmake --version
```

## Использование на Windows XP

1. Скопируйте `lunapassport-patcher.exe` на XP.
2. Запустите **от имени администратора**.
3. Введите:
   - **IP сервера** (где Traefik)
   - **Базовый домен** (например `lunastore.app`)
4. Нажмите **Применить**.
5. Перезапустите Internet Explorer 6.
6. Откройте `https://passport-staging.<domain>/static/netpass/index.html`

Тестовый аккаунт LunaPassport: `test@example.com` / `testpass`

## Что делает патч

Из домена `lunastore.app` строятся:

- `passport-staging.lunastore.app`
- `memberservices-staging.lunastore.app`

В `hosts` добавляется блок:

```text
# BEGIN LunaPassport
<IP> passport-staging.<domain> memberservices-staging.<domain> register.passport.com
# END LunaPassport
```

Перед изменением создаётся backup: `hosts.lunapassport.bak`

## Ручная сборка (если нужно)

```bat
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Требования

- Windows XP SP3+ (или новее)
- Права администратора
- Изолированная lab-сеть
