# Отчёт о проверке

## Статус

Локальная реализация и повторные проверки после уточнения поведения `ws-err` завершены. Выполнены задачи 1.1, 1.2, 1.3, 1.4, 2.1 и 2.2. Задачи 2.3, 3.1 и 3.2 остаются открытыми, потому что отдельного разрешения на npm-публикацию нет. Права npm не проверялись, публикация не выполнялась.

Отчёт описывает локальный пакет, а не опубликованный SDK. Установка архива в отдельный consumer не заменяет проверку точной версии из registry. Change не готов к архивации.

## Источники и версия

- Репозиторий: `/home/caiiiycuk/js-dos/emulators`, ветка `8.xx`.
- Исходный commit: `23c3849d971311262a1370303844146d42b76b9a`, проверен через Git. Исправление находится в незакоммиченных изменениях, отдельного commit исправленной версии пока нет.
- Исходный патч: `../js-dos/mcp/emulators-audio-lifecycle.patch`.
- SHA-256 патча: `d55d13fc32c0b0e87eda5e36efddbbc8eb61f23c1b73a83c2bd17f0943d65807`, проверен командой `sha256sum`. `git apply --check` прошёл до переноса.
- Версия: `emulators@8.5.3`, следующая свободная patch-версия после `8.5.2` на момент проверки `npm view`. Версия записана в `package.json`, английская запись добавлена в `CHANGELOG.md`.
- Штатная production-сборка создала `Build.version = "8.5.3 (888a2a9f0785f8bdf2d5ce4676fccc2b)"` в `src/build.ts`.

SHA-256 исходных файлов после переноса:

```text
1249c677bea6e1ffb424acd0f7c70d4288b769f3fe07c09e473d221ed6d38f8b  src/dos/dosbox/ts/audio-worklet.ts
08930c40026568f433f6bf1826a812be77d7babd6dca76c859f72ce38c544267  src/dos/dosbox/ts/direct.ts
54577715bf7ca283b02d9a6ed77869d21cdbd4e3fd93161614ec443cb2e39c02  src/dos/dosbox/ts/worker.ts
```

## Сохранение аудио после сообщений об ошибках

Повторная проверка выявила регрессию перенесённого патча: `worker.onerror` вызывал `transportLayer.exit()`, который принудительно завершал worker до ответа `ws-exit`. Настоящий `CommandInterface.exit()` после такой ошибки оставался незавершённым. Ошибка Worker не обязательно прекращает его работу.

По уточнению пользователя из `onerror` удалены и очистка аудио, и флаг отказа запуска. Событие только передаётся прежним сообщением `ws-err`. Даже при событии во время `await createAudioPort()` создание transport продолжается. Аудио освобождается при `exit()` и отклонении инициализации, но не по сообщению об ошибке.

Регрессионный тест до последнего исправления падал: при `Worker.onerror` счётчик закрытий был равен 1 вместо 0. После исправления проходят 16 lifecycle-проверок. Настоящий `CommandInterfaceOverTransportLayer` получает `ws-err` с обычным тегом и тегом `panic` после готовности backend в direct и worker. Сообщения доходят до `events().onMessage()` с типом `error`, но контекст, порт, узел и обработчики жестов сохраняются. Последующий exit завершает Promise, однократно сообщает о завершении и освобождает аудио. Отдельно проверены `Worker.onerror` во время создания аудио и работы, включая выключенный worklet.

В direct аналогичной регрессии не обнаружено. Ошибки `instantiate` и синхронного `callMain` очищают ресурсы до создания CI и сохраняют исходную ошибку. Штатная очистка выполняется после `ws-exit`, поэтому не прерывает ожидаемый CI ответ. Дополнительных изменений в `direct.ts` не потребовалось.

## Выполненные команды

| Команда | Результат |
| --- | --- |
| `yarn run eslint src --ext ts,tsx --max-warnings 0` | Прошла |
| `yarn run eslint test --ext ts,tsx --max-warnings 0` | Прошла |
| `yarn run tsc --noEmit` | Прошла |
| `yarn run tsc --noEmit --target esnext --module commonjs --moduleResolution node --strict --skipLibCheck --esModuleInterop test/src/browser-audio-lifecycle.ts` | Прошла отдельная проверка типов браузерного теста |
| `yarn test:node:build` | Прошла, 84 теста, включая 16 lifecycle-тестов |
| `yarn run gulp production` | Прошла, созданы wrapper, WASM и декларации |
| `yarn test:browser` | Прошла, включая lifecycle для четырёх backend и существующий suite |
| `npm pack --json --pack-destination /tmp/opencode` | Создан архив стандартным способом |
| `git diff --check` | Прошла |
| `openspec validate fix-audio-worklet-lifecycle --strict --json` | Прошла, ошибок нет |

Заключительные `test:node:build`, production и браузерный suite выполнены последовательно, без изменения `dist` во время браузерной проверки. Логи находятся в `/tmp/opencode/audio-lifecycle-{node-build,production,browser}-verified.log`.

Браузерный тест находится в `test/src/browser-audio-lifecycle.ts` и вызывается из обычного `test/src/browser-runner.ts`. Он использует native AudioContext и AudioWorkletNode, а не их моки. Создание первого экземпляра выполняется через CDP с `userGesture: false`, потому что Playwright `page.evaluate()` активирует страницу. Проверяются отсутствие предыдущей активации и состояние `suspended`.

Для каждого из `dosboxDirect`, `dosboxWorker`, `dosboxXDirect`, `dosboxXWorker` проверены остановка до первого жеста и два последующих запуска. После каждого stop все контексты достигают `closed`, каждый узел отключается один раз, а pointerdown и keydown не вызывают resume старых контекстов. При следующем запуске все предыдущие контексты остаются закрытыми.

Для обоих transport отправляется `ws-err` через существующий канал backend, а для worker дополнительно отправляется синтетический `ErrorEvent` настоящему Worker после запуска. Проверяются доставка сообщения в CI, сохранение контекста в `running`, отсутствие отключения узла и преждевременного `terminate`, затем завершение настоящего `ci.exit()` и закрытие контекста. Скрипт установленного Worker не подменяется. Сценарий проверяет обработчик восстанавливаемой ошибки, а не аварийное завершение процесса Chromium.

## Сверка со сценариями спецификации

| Сценарий | Подтверждение |
| --- | --- |
| Остановка работающего экземпляра | Node-проверки port.close, disconnect и удаления обработчиков; native-браузерная проверка состояния `closed` для всех четырёх backend |
| Остановка до первого пользовательского жеста | Native-браузерный тест подтверждает начальный `suspended`, затем `closed` и отсутствие resume после жестов |
| Ошибка загрузки worklet или создания узла | Node-тесты подтверждают закрытие контекста и возврат `undefined`; оба transport сохраняют fallback без audio port |
| Ошибка инициализации backend | Node-тесты покрывают direct instantiate, callMain и обе попытки worker install; исходная ошибка callMain сохраняется |
| Сообщение ws-err работающего backend | Node-тесты для direct и worker проверяют доставку обычного и panic-сообщения в CI без очистки; native-браузерный тест проверяет сохранение работающего аудио |
| Ошибка worker во время создания аудио | Node-тест подтверждает успешную установку и сохранение позднего контекста до последующего exit |
| Ошибка работающего worker | Node-тест подтверждает сохранение ресурсов и ws-err на границе transport; настоящие CI и native Worker сохраняют аудио и возможность завершения через ws-exit |
| Повторное завершение | Node-тесты подтверждают однократную очистку при повторном exit и после worker error; повторный CI exit не повторяет событие exit |
| Цикл создания, остановки и повторного создания | Native-браузерная проверка повторного запуска проходит для четырёх backend |
| Использование стандартного пакета | Локальный npm-архив проверен; требование опубликованной версии остаётся заблокировано задачей 2.3 |
| Проверка обратной совместимости | Node и браузерный suite прошли; DSP, публичный API и протокол не менялись |

Дополнительно проверено побайтовое совпадение `audioWorkletCode`, `src/emulators.ts`, `src/protocol/protocol.ts`, `test/src/test-audio-worklet.ts` и `test/src/test-protocol.ts` с исходным commit. Сохранена повторная установка worker без готового WASM-модуля с тем же audio port.

## Локальный npm-архив

Архив: `/tmp/opencode/emulators-8.5.3.tgz`.

Данные из `npm pack --json` и `sha256sum`:

```text
size: 9783532 bytes
unpackedSize: 29055584 bytes
SHA-256: 48de410b8d44f6f61f5681de8e7268215ead4d52b66dc861731aba2cf6983e71
npm shasum: f85de070e3b2dec66bb8da0f61912da4925fd919
npm integrity: sha512-paHJw3GFwkPj21dP2v/wcdBCHKRsiyJwWBZmDsGVEVEjHHa5KawVLL1lVNGkyXY03Dq8YVZRZdGqHrKPnUJtmg==
```

В архиве 38 файлов, включая `dist/emulators.js`, wrapper и WASM для DOSBox, DOSBox-X, debug, JSPI и libzip, `dist/types/emulators.d.ts` и `LICENSE`. В `.npmignore` исключены локальные инструменты, OpenSpec, talks, sockdrive и результаты браузерных проверок, которые попадали в пробный архив. Сами пользовательские файлы не изменены.

Архив установлен в `/tmp/opencode/audio-lifecycle-consumer-errors-preserved` командой:

```bash
npm install --prefix /tmp/opencode/audio-lifecycle-consumer-errors-preserved --ignore-scripts --no-audit --no-fund /tmp/opencode/emulators-8.5.3.tgz
node /tmp/opencode/verify-audio-lifecycle-package.cjs
```

Проверка подтвердила integrity установленного архива и побайтовое совпадение всех его файлов с production-сборкой. Native-браузерные lifecycle-сценарии прошли для обоих ядер, direct и worker, используя стандартные файлы установленного пакета. Лог: `/tmp/opencode/audio-lifecycle-installed-package-verified.log`. Hooks, правки `node_modules`, overrides и `JSDOS_TEST_EMULATORS_JS` не использовались.

## Блокер публикации и передача потребителю

Для задачи 2.3 нужно отдельное разрешение пользователя на npm-публикацию и наличие npm-доступа. После разрешения необходимо подтвердить commit исправления, пересобрать пакет при изменении release metadata и проверить доступность точной версии в registry.

Для задачи 3.1 нужно установить именно опубликованную версию и повторить native-браузерную проверку. После проверки следует дополнить отчёт registry integrity и commit опубликованного исправления, затем завершить задачу 3.2.

Для `add-jsdos-mcp` подготовлена кандидатная версия `emulators@8.5.3`. Пока она не опубликована, переходить на неё нельзя. Файлы соседнего js-dos и `add-jsdos-mcp` не изменялись, их задачи выполненными не объявлялись. Подмодуль `native/dosbox-x` сохранил существовавший до работы неотслеживаемый `.idea/`; другие изменения подмодуля не вносились.

`yarn test:browser:net` и проверка в других браузерах не выполнялись. Область проверки native lifecycle ограничена штатным Chromium-runner проекта.

Существующий формат сообщения, создаваемого обработчиком `Worker.onerror`, не менялся. В нём нет `sessionId`, поэтому `CommandInterfaceOverTransportLayer.onServerMessage()` отбрасывает такое сообщение. Этот отдельный дефект существовал до lifecycle-патча; проверка его сохранения относится к границе transport. Обычные backend-сообщения `ws-err` с `sessionId` проверяются через настоящий `ci.events().onMessage()` с типом `error`.
