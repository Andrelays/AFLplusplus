<div align="center">

# Фаззинг проекта с использованием [Afl++](https://github.com/AFLplusplus/AFLplusplus/tree/stable) и кастомных мутаций.
</div>

## Оглавление
- [Описание](#описание)
  - [Задание](#задание)
- [Ход работы](#ход-работы)
  - [Выбор проекта](#выбор-проекта)
  - [Сборка проекта](#сборка-проекта)
  - [Начальный набор данных](#начальный-набор-данных)
  - [Фаззинг без кастомных мутаций](#фаззинг-без-кастомных-мутаций)
  - [Написание кастомной мутации](#написание-кастомной-мутации)
    - [Описание работы мутации](#описание-работы-мутации)
  - [Запуск нескольких синхронизированных инстансов](#запуск-нескольких-синхронизированных-инстансов-с-кастомной-мутацией)
  - [Сравнение покрытия](#сравнение-покрытия)
    - [afl-plot](#afl-plot)
    - [afl-cov](#afl-cov)
- [Итоги и выводы](#итоги-и-выводы)

## Описание
В этой работе я занимался фаззингом проекта [fpng](https://github.com/richgel999/fpng), который обрабатывает файловый формат .PNG. Для фаззинга я использовал Afl++, а также написал кастомные мутации, адаптированные под формат .PNG и учитывающие специфику выбранного проекта.

### Задание:

1. Необходимо выбрать C/C++ open-source проект, работающий с каким-нибудь файловым форматом.

2. Собрать качественный начальный корпус входных данных для фаззинга.

3. Воспользовавшись фаззером Afl++, профаззить выбранный проект.

4. Написать кастомную мутацию для Afl++, учитывающую формат файла и специфику выбранного проекта.

5. Запустить несколько инстансов Afl++ с кастомной мутацией и обеспечить синхронизацию между ними.

6. Запустить мини-эксперимент: Три инстанса Afl++, каждый в своем режиме мутации: custom-only, default-only, custom+default. Построить графики прироста покрытия (можно воспользоваться afl-plot).

7. Подготовить небольшой отчет в формате ридми по проделанной работе: прикрепить результаты с графиками, техническую карту предложенной мутации.

## Ход работы

### Выбор проекта:

В качестве проекта я выбрал [fpng](https://github.com/richgel999/fpng). Проект fpng(Fast PNG) - это библиотека для быстрого чтения и записи файлов формата PNG, написанная на языках программирования C/C++. Она разработана для обеспечения высокой производительности и эффективности при работе с изображениями PNG. Библиотека поддерживает различные операции, такие как сжатие, декомпрессия, конвертация и манипуляция данными изображений.

Исследование проводилось в тестовом режиме. В этом режиме тестовая программа сначала сжимает изображение, а затем распаковывает вывод fpng с помощью `lodepng`, `stb_image` и декодера fpng для проверки сжатых данных.

### Сборка проекта:
Собирать программу мы будем сразу с использованием компиляторов: `afl-clang-fast` и `afl-clang-fast++`. Для сборки программы в тестовом режиме необходимо:

```bash
git clone --depth=1 https://github.com/richgel999/fpng.git
cd ./fpng
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=</path/to/AFLplusplus/afl-clang-fast>\
      -DCMAKE_CXX_COMPILER=</path/to/AFLplusplus/afl-clang-fast++>\
      -DSSE=1 ..
make
```
Для запуска тестового режима из директории fpng:
```bash
./bin/fpng_test <image_filename.png>
```

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/output_fpng_test_mode.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> Пример работы fpng </figcaption>
</div>
</figure>

### Начальный набор данных:

Начальный набор данных я взял из репозитория [fuzzing-corpus](https://github.com/strongcourage/fuzzing-corpus/tree/master/png).
> [!NOTE]
> Я брал только некоторые примеры, так как их список оказался слишком обширным. Даже после применения инструмента `afl-cmin`, `afl-fuzz` выводил предупреждение: `[!] WARNING: You probably have far too many input files! Consider trimming down.`

К исходному набору данных я применил два инструмента:

  - `afl-cmin` - удаляет из набора данные, не увеличивающие покрытие в тестируемой программе:

```bash
afl-cmin -i INPUTS -o INPUTS_UNIQUE -- bin/target -someopt @@
```

- `afl-tmin` - минимизирует все входные файлы в наборе:

```bash
mkdir input
cd INPUTS_UNIQUE
for i in *; do
  afl-tmin -i "$i" -o "../input_testcases/$i" -- bin/target @@
done
```
[Начальный корпус моих входных данных на гитхабе: input_testcases](https://github.com/Andrelays/AFLplusplus/tree/stable/custom_mutators/png_mutator/input_testcases).

### Фаззинг без кастомных мутаций:
`afl-fuzz` — это инструмент для фаззинга, который помогает находить ошибки в программах путем подачи разнообразных входных данных и отслеживания результатов. Он автоматически генерирует и модифицирует тестовые примеры, чтобы выявить неочевидные проблемы.

Перед запуском `afl-fuzz` я выполнил `sudo afl-system-config` - это перенастраивает систему на оптимальную скорость. А также установил  `AFL_SKIP_CPUFREQ=1`, чтобы запустить фаззинг без root прав.
```bash
sudo afl-system-config
export AFL_SKIP_CPUFREQ=1
```
После этого я запустил `afl-fuzz` с исползованием начального корпус входных данных.
```bash
afl-fuzz -i input_testcases -o output_default_only -- bin/target @@
```
<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/afl_fuzz_default_only_screen.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> Вывод afl-fuzz в консоль </figcaption>
</div>
</figure>

Фаззинг длился 20 минут, примерно через это время afl-fuzz прекращал активно находить новые пути в исследуемой программе, что можно проследить по графику.

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/edges_default_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> График покрытия, сгенерированный afl-plot </figcaption>
</div>
</figure>


<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/afl_cov_default_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> Отчет, сгенерированный с помощью afl-cov </figcaption>
</div>
</figure>

> [!NOTE]
>  про то, как работать с [afl-plot](#afl-plot) и [afl-cov](#afl-cov) будет рассказано дальше.

### Написание кастомной мутации:
Следующим пунктом стало написание своей мутации, которая должна быть ориентирована на формат PNG, что приведёт к ускорению тестирования или/и увеличению покрытия исследуемой программы. Более подробно, про то, как устроены кастомные мутации можно прочитать на ГитХабе проекта [AFLplusplus](https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/custom_mutators.md).

#### Описание работы мутации:

**Входные данные:**
`in_buf` - массив в который записан исходный файл.
`buf_size` - размер этого массива.
`max_size` - максимальный размер выходного массива.

**Выходные данные:**
`out_buf` - массив, в котором записан мутированный файл.

**Возвращаемое значение:**
`mutated_size` - размер этого массива.


- Если `buf_size` меньше 8 байт, что соответствует размеру сигнатуры PNG, то `in_buf` просто записывается в `out_buf` и функция мутации возвращает `buf_size`.
- Иначе первые 8 байт этого буффера копируется в `out_buf` и функция мутации начинает по чанкам считывать входной массив. Здесь возможны следующие варианты, которые выполняются с 33% вероятностью:
   - Перед считыванием следующего чанка будет сгенерирован случайный новый чанк и добавлен в выходной массив.
   - Чанк из `in_buf` вместо записи будет просто пропущен.
   - У считанного чанка будут заменена область `data` на случайную.
   - У считанного чанка `type` заменится на случайный, в том числе возможно на несуществующий.

После полного считывания входного массива и записи его в `out_buf` он обрезается до размера `max_size` и функция мутации возвращает min(`max_size`, `mutated_size`). На этом работа мутатора завершена.
### Запуск нескольких синхронизированных инстансов с кастомной мутацией:
Для того, чтобы запустить `afl-fuzz` с кастомной мутацией, необходимо передать путь до неё с помощью переменной окружения `AFL_CUSTOM_MUTATOR_LIBRARY`:
```bash
AFL_CUSTOM_MUTATOR_LIBRARY="./mutate_png.so" afl-fuzz -i input_testcases/ -o output_custom_png -- ~/fpng/bin/fpng_test @@
```
Чтобы запустить нескольких синхронизированных инстансов. Необходимо указать главный среди них с помощью опции `-M main-$HOSTNAME`, также необходимо установить `AFL_FINAL_SYNC=1`. Вторичные инстансы указываются с помощью опции `-S variant`. Каждой записи -M/-S необходимо уникальное имя (которое может быть любым), однако для всех инстансов должно использоваться одно и то же расположение выходного каталога.

Я запустил 1 основной и 3 вторичных инстанса и т.к. для каждого вторичного должна быть вариация, то:
- первый фаззил цель с опцией `AFL_DISABLE_TRIM=1`.
- второй с `AFL_DISABLE_TRIM=1` и `-P explore`.
- третий с `-a binary`.
### Сравнение покрытия:

Если необходимо, чтобы применялись только кастомные мутации, то это также необходимо указать с помощью переменной окружения `AFL_CUSTOM_MUTATOR_ONLY`:
```bash
AFL_CUSTOM_MUTATOR_ONLY=1 AFL_CUSTOM_MUTATOR_LIBRARY="./mutate_png.so" afl-fuzz -i input_testcases/ -o output_custom_png_only -- ~/fpng/bin/fpng_test @@
```
Перед запуском каждого инстанса я применял `unset` к этим переменным.

Каждый из трех инстансов: **Custom and default**, **Custom only**, **Default only** работал на протяжении 20 минут. Для их сравнения я использовал инструменты `afl-plot` и `afl-cov`.
#### afl-plot:
Для использования `afl-plot`:
```bash
afl-plot <path/to/afl-fuzz/output> <path/to/afl-plot/output>
```

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/edges_custom_and_default.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 1. График покрытия <U> Custom and default </U>. Максимальное покрытие: 4183 </figcaption>
</div>
</figure>

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/edges_custom_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 2. График покрытия <U> Custom only </U>. Максимальное покрытие: 4160 </figcaption>
</div>
</figure>

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/edges_default_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 3. График покрытия <U> Default only </U>. Максимальное покрытие: 4140 </figcaption>
</div>
</figure>

Как видно из графиков самым быстрым и эффективным по покрытию является инстанс **Custom and default**, на втором месте **Custom only** и на третьем **Default only**.

У нас получилось получить прирост в покрытии и скорости по сравнению с дефолтными мутациями.

#### [afl-cov](https://github.com/vanhauser-thc/afl-cov):
`afl-cov` использует файлы тестовых случаев, созданные фаззером `afl-fuzz`, для генерации результатов покрытия кода тестируемой программы с помощью `gcov`.

Для начала необходимо скопировать ещё несобранный тестируемый проект в другую директорию:
```bash
git clone --depth=1 https://github.com/richgel999/fpng.git
```
и скомпилировать его ещё раз используя те же компиляторы `afl-clang-fast` и `afl-clang-fast++`, как было описано в пункте [сборка проекта](#сборка-проекта), но добавив к флагам `CMAKE_C_FLAGS` и `CMAKE_CXX_FLAGS` - `-fprofile-arcs`, `-ftest-coverage` и `-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION`.


А также к `CMAKE_EXE_LINKER_FLAGS` флаг `--coverage`.

После чего можно использовать инструмент `afl-cov`:
```bash
./afl-cov --clang --overwrite -d <path/to/afl-fuzz/output>\
--coverage-cmd "<path/to/fpng_cov/bin/fpng_test> @@"\
--code-dir <path/to/fpng_cov/build/CMakeFiles/fpng_test.dir> --enable-branch-coverage
```
"path/to/fpng_cov" - путь к копии исследуемого проекта, собранного с флагами, необходимыми для работы `afl-cov`.

Подробнее про флаги запуска и сборку проекта можно найти на гитхабе [afl-cov](https://github.com/vanhauser-thc/afl-cov).

На выходе мы получаем подробный отчёт, в котором показано, какие блоки кода были покрыты за время фаззинга. Также можно посмотреть, какие тестовые случаи увеличили покрытие и каких новых функций/строк/веток кода они достигли, что позволяет эффективнее писать мутации.

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/afl_cov_custom_and_default.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 1. Отчёт afl-cov <U> Custom and default </U> </figcaption>
</div>
</figure>

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/afl_cov_custom_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 2. Отчёт afl-cov <U> Custom only </U> </figcaption>
</div>
</figure>

<figure>
<img src="https://github.com/Andrelays/AFLplusplus/blob/stable/custom_mutators/png_mutator/pictures/afl_cov_default_only.png?raw=true" width = 100%>
<div align="center">
<figcaption style="text-align:center"> 3. Отчёт afl-cov <U> Default only </U> </figcaption>
</div>
</figure>

## Итоги и выводы
В результате выполнения задания я научился работать с фаззером `Afl++` для тестирования проектов и писать свои собственные мутации. Также я работал с инструментам `afl-plot` и `afl-cov`, которые помогли мне визуализировать работу фаззера. У меня получилось с помощью своих мутаций улучшить работу фаззера: увеличить покрытие и ускорить его работу.
