
# ⚡ ESPBOT v2.0 — ESP8266 WiFi WebRobot

[![Platform](https://img.shields.io/badge/Platform-ESP8266-blue.svg)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-2.0-orange.svg)]()
[![Arduino](https://img.shields.io/badge/Arduino-IDE%202.x-blue.svg)](https://www.arduino.cc/)
[![Build](https://img.shields.io/badge/Build-Passing-success.svg)]()

<p align="center">
  <img src="docs/espbot_banner.png" alt="ESPBOT" width="600"/>
</p>

**ESPBOT v2.0** — современная Hi-Tech прошивка для ESP8266, превращающая микроконтроллер в полноценного WiFi-робота на драйвере L298N с веб-интерфейсом, авторизацией, OTA-обновлениями и CLI-консолью через Serial.

> 🤖 Управляй роботом прямо из браузера через точку доступа ESP8266!

---

## 🌟 Возможности

- 📡 **WiFi Access Point** — ESP8266 сам раздаёт WiFi (не требует роутера)
- 🔐 **HTTP Basic Auth** — защита веб-интерфейса логином/паролем (`admin`/`admin`)
- 🚀 **OTA обновления** — прошивка "по воздуху" через Arduino IDE
- ⚙️ **L298N Driver** — поддержка драйвера с ШИМ-управлением скоростью (ENA/ENB)
- 💾 **EEPROM хранение** — все настройки сохраняются между перезагрузками
- 📟 **Serial CLI** — 8 команд для конфигурации без веб-интерфейса
- 🔒 **Секреты через `secrets.h`** — пароли не коммитятся в git
- 📱 **Адаптивный Hi-Tech UI** — glassmorphism дизайн для смартфонов и ПК
- 🎮 **Hold-to-Move** — удержание кнопки для движения (как в консольных играх)
- 📊 **ADC датчик A0** — подключение аналоговых сенсоров (ИК, УЗ, освещённость)
- 🎛️ **ШИМ регулятор** — плавная настройка мощности моторов 0-1023

---

## 🔄 Сравнение с v1.0

| Возможность | v1.0 (оригинал) | v2.0 (ESPBOT) |
|---|---|---|
| **Режим WiFi** | STA (клиент роутера) | **AP (точка доступа)** |
| **Пины моторов** | `{15, 13, 12, 14}` | **`{5, 13, 12, 14}`** |
| **ШИМ управление** | ❌ (только HIGH/LOW) | ✅ **ENA + ENB** |
| **Авторизация** | ❌ | ✅ **HTTP Basic Auth** |
| **OTA обновления** | ❌ | ✅ **ArduinoOTA** |
| **Сохранение настроек** | ❌ | ✅ **EEPROM с маркером** |
| **Serial CLI** | ❌ | ✅ **8 команд** |
| **Секреты в git** | ⚠️ hardcoded | ✅ **`secrets.h`** |
| **Дизайн UI** | Базовый Hi-Tech | 🎨 **Glassmorphism + градиенты** |

---

## 📋 Содержание

- [Требования](#-требования)
- [Схема подключения](#-схема-подключения-l298n)
- [Установка](#-установка)
- [Первый запуск](#-первый-запуск)
- [Веб-интерфейс](#-веб-интерфейс)
- [Serial CLI](#-serial-cli-консоль)
- [OTA обновление](#-ota-обновление)
- [EEPROM структура](#-eeprom-структура)
- [FAQ](#-faq)
- [Автор](#-автор)

---

## 🔧 Требования

### Аппаратные
| Компонент | Назначение |
|---|---|
| **ESP8266** (NodeMCU/Wemos D1 Mini) | Микроконтроллер |
| **L298N** | Драйвер двигателей |
| **2x DC мотора** | Привод колёс |
| **Li-ion 2S (7.4V)** | Питание моторов |
| **Power Bank 5V** | Питание ESP8266 |

### Программные
- **Arduino IDE 2.x** или выше
- **ESP8266 Board Package** (версия 3.x)
- **Библиотеки:** `ESP8266WiFi`, `ESP8266WebServer`, `ArduinoOTA` (встроены в пакет)

---

## 🔌 Схема подключения L298N

```text
╔═══════════════════════════════════════════════════════╗
║                 ESPBOT v2.0 PINOUT                    ║
╠═══════════════════════════════════════════════════════╣
║  ESP8266 GPIO    │  D-Pin  │  L298N     │  Функция   ║
╠═══════════════════════════════════════════════════════╣
║  GPIO 5          │  D1     │  IN1       │  Лев.Вперёд║
║  GPIO 13         │  D7     │  IN2       │  Лев.Назад ║
║  GPIO 12         │  D6     │  IN3       │  Пр.Вперёд ║
║  GPIO 14         │  D5     │  IN4       │  Пр.Назад  ║
║  GPIO 4          │  D2     │  ENA       │  Лев.ШИМ   ║
║  GPIO 0          │  D3     │  ENB       │  Пр.ШИМ    ║
║  GND             │  GND    │  GND       │  Общий     ║
║  3.3V / 5V       │  3V3    │  5V        │  Логика    ║
║  —               │  —      │  12V       │  Питание М ║
╠═══════════════════════════════════════════════════════╣
║  A0 (ADC)        │  A0     │  —         │  Сенсор    ║
╚═══════════════════════════════════════════════════════╝
```

> ⚠️ **Важно:** Снимите перемычки с ENA и ENB на L298N, чтобы ШИМ работал корректно!

---

## 📥 Установка

### 1. Клонировать репозиторий
```bash
git clone https://github.com/saikttech/espbot-v2.git
cd espbot-v2
```

### 2. Настроить секреты
```bash
cp secrets.example.h secrets.h
# Отредактируйте secrets.h своими значениями
nano secrets.h
```

### 3. Настроить Arduino IDE
1. **File → Preferences → Additional Boards Manager URLs:**
   ```text
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. **Tools → Board → Boards Manager** → Установить `esp8266` by ESP8266 Community
3. **Tools → Board:** → `NodeMCU 1.0 (ESP-12E Module)`
4. **Tools → Flash Size:** → `4MB (FS:1MB OTA:~1019KB)`

### 4. Прошить
Откройте `ESPBOT_v2.ino` и нажмите **Upload** (⬆️).

---

## 🚀 Первый запуск

После прошивки ESP8266 автоматически создаст WiFi точку доступа:

```text
╔══════════════════════════════════╗
║   ESPBOT v2.0 — HI-TECH CTRL     ║
║   (c) sa | juniorgenius.ru/it    ║
╚══════════════════════════════════╝
[EEPROM] Initialized with secrets.h values
[WIFI] AP 'ESPBOT' started
[WIFI] IP: 192.168.4.1
[HTTP] Server started
[SYS] Type 'help' for CLI
```

### 🔑 Значения по умолчанию (из `secrets.example.h`)

| Параметр | Значение |
|---|---|
| **SSID** | `ESPBOT` |
| **Пароль WiFi** | `12345678` |
| **IP адрес** | `192.168.4.1` |
| **Веб-логин** | `admin` |
| **Веб-пароль** | `admin` |
| **OTA пароль** | `espbot` |

---

## 🌐 Веб-интерфейс

1. Подключитесь к WiFi **ESPBOT**
2. Откройте браузер: **http://192.168.4.1**
3. Введите логин `admin` / пароль `admin`

### Управление
- **▲ FWD** — движение вперёд (удерживайте!)
- **▼ BWD** — движение назад
- **◀ L / R ▶** — повороты
- **PWM слайдер** — регулировка скорости 0-1023
- **ADC A0** — показания аналогового сенсора в реальном времени

> 💡 Кнопки работают по принципу **HOLD → MOVE | RELEASE → STOP**

---

## 📟 Serial CLI Консоль

Откройте Serial Monitor на скорости **115200 бод** и введите `help`:

```text
========= ESPBOT CLI =========
help          - This help
status        - System status
pinout        - Pin configuration
login <u> <p> - Change web credentials
wifi <s> <p>  - Change WiFi AP (reboot)
backup        - EEPROM hex dump
speed <0-1023>- Set motor PWM
reboot        - Restart ESP
==============================
```

### Примеры команд

```bash
# Посмотреть текущие настройки
> status

# Изменить WiFi точку доступа
> wifi MyRobot 87654321

# Сменить веб-логин
> login user pass123

# Установить скорость 50%
> speed 512

# Сделать дамп EEPROM
> backup

# Перезагрузить
> reboot
```

---

## 📡 OTA Обновление

Обновляйте прошивку без USB-кабеля!

### Настройка в Arduino IDE:
1. **Tools → Port** → выберите `ESPBOT at 192.168.4.1`
2. **Tools → Upload Using:** → `OTA`
3. Введите пароль: `espbot`
4. Нажмите **Upload**

> ⚠️ Во время OTA моторы автоматически останавливаются для безопасности!

---

## 💾 EEPROM Структура

Настройки хранятся в EEPROM (512 байт) с маркером валидности `0xAA`.

```cpp
struct Settings {
  char wifiSSID[32];  // Имя WiFi AP
  char wifiPass[64];  // Пароль WiFi
  char webUser[32];   // Логин веб-интерфейса
  char webPass[32];   // Пароль веб-интерфейса
  uint8_t valid;      // Маркер 0xAA (защита от мусора)
};
```

### Дамп EEPROM
```bash
> backup
==== EEPROM BACKUP (512 bytes) ====
45 53 50 42 4F 54 00 00 00 00 00 00 00 00 00 00  ESPBOT..........
31 32 33 34 35 36 37 38 00 00 00 00 00 00 00 00  12345678........
...
==================================
```

---

## ❓ FAQ

### ❔ Моторы не реагируют на команды
- Проверьте перемычки на ENA/ENB (должны быть сняты)
- Убедитесь, что подключён контакт GND между ESP и L298N
- Проверьте `> pinout` в CLI

### ❔ Не могу подключиться к WiFi AP
- Проверьте, что пароль `12345678` (8 символов минимум)
- Выполните `> reboot` в CLI

### ❔ OTA не работает
- Убедитесь, что выбран правильный порт в Arduino IDE
- Пароль OTA: `espbot` (по умолчанию)
- Проверьте, что размер прошивки < 500KB

### ❔ Сбросить настройки к заводским
Временно измените в `loadSettings()` условие проверки маркера:
```cpp
if (true) { // вместо settings.valid != VALID_MARKER
```
Перезагрузите — значения из `secrets.h` будут записаны заново.

---

## 📂 Структура проекта

```text
espbot-v2/
├── ESP8266_L298N_wifi_v2.0.ino # Основной код прошивки
├── secrets.h                   # 🔒 Ваши секреты (НЕ в git)
├── secrets.example.h           # 📋 Пример для репозитория
├── README.md                   # Этот файл
├── .gitignore                  # Игнорируемые файлы
├── LICENSE                     # MIT лицензия
└── docs/
    ├── espbot_banner.png       # Баннер проекта
    ├── web_ui_screenshot.png   # Скриншот веб-UI
    └── wiring_diagram.png      # Схема подключения
```

---

## 🛣️ Roadmap

- [ ] Поддержка камеры ESP32-CAM
- [ ] Виртуальный джойстик (nipple.js)
- [ ] Телеметрия в реальном времени (WebSocket)
- [ ] PID-регулятор скорости
- [ ] Веб-интерфейс на Vue.js
- [ ] Голосовое управление (Web Speech API)
- [ ] Мульти-робот система (mesh network)
- [ ] Поддержка гироскопа MPU6050

---

## 👨‍💻 Автор

**© dsaru**  
🌐 [www.juniorgenius.ru/it](http://www.juniorgenius.ru/bugulma)

### Поддержать проект
Если проект оказался полезен, поставьте ⭐ на GitHub!

---

## 📄 Лицензия

Этот проект распространяется под лицензией **MIT** — свободное использование с сохранением уведомления об авторстве.

```text
MIT License

Copyright (c) 2026 dsaru | www.juniorgenius.ru/bugulma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## 🔗 Ссылки

- 📚 [ESP8266 Arduino Core Documentation](https://arduino-esp8266.readthedocs.io/)
- 📚 [L298N Datasheet](https://www.st.com/resource/en/datasheet/l298.pdf)
- 🎓 [Junior Genius IT — обучение робототехнике](http://www.juniorgenius.ru/it)

---

<p align="center">
  <b>Made with ⚡ by ESPBOT Team</b><br>
  <sub>© sa | www.juniorgenius.ru/bugulma | 2026</sub>
</p>


