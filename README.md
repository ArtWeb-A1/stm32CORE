# stm32CORE Project

### Общая информация
Архитектура **stm32CORE** спроектирована с акцентом на оптимизацию памяти и использование целочисленных типов минимально необходимой разрядности (от `int8_t` / `uint8_t` и выше в зависимости от задач). При разработке приложений строго рекомендуется выбирать типы данных с учетом этих ограничений.

### Аппаратное обеспечение
* [STM32H562VGT6](https://www.aliexpress.com/item/1005009873111564.html?spm=a2g0o.order_list.order_list_main.25.612f1802ZgOg8A) (со слотом для MicroSD)
* [TFT-дисплей 480x320](https://www.aliexpress.com/item/32985467436.html?spm=a2g0o.order_list.order_list_main.5.612f1802ZgOg8A) (с сенсорным экраном)

### Планируемая система установки приложений *(в разработке)*
> **Обратите внимание:** Описанные ниже утилиты на данный момент находятся на стадии проектирования и **еще не реализованы**.

Установку приложений в **stm32CORE** планируется осуществлять с помощью следующих инструментов:

* **stmd** (*Download*) — утилита для загрузки пакетов из официального репозитория (в будущем — с поддержкой пакетов сообщества).
* **stmdm** (*Download Manager*) — менеджер загрузок для управления скачиванием.
* **stmuz** (*UnZip*) — инструмент для распаковки пакетов и их инсталляции в систему.

В дальнейшем также планируется разработка открытого API для создания пользовательских репозиториев (mirror lists) и сторонних установщиков.

### Disclaimer

This is a personal hobby project developed for educational and experimental purposes. The project is provided "AS IS", without warranties of any kind, to the extent permitted by applicable law.

This project may involve radio frequency transmission and configurable RF parameters, including frequency and transmission power.

**Radio Frequency Notice:** Users are solely responsible for ensuring that their use, configuration, and modification of this project comply with applicable radio frequency regulations in their country, including the European Union, Ireland, and the Russian Federation.

The author is not responsible, to the maximum extent permitted by applicable law, for unlawful transmissions, radio interference, regulatory violations, or damages resulting from user modifications or misuse of the project.

The ability to configure or modify a radio parameter does not imply that its use is legally permitted.

This project is provided for lawful experimentation only. Nothing in this disclaimer excludes or limits liability where such exclusion or limitation is prohibited by applicable law.

**License:** See `LICENSE` for the software licensing terms.
