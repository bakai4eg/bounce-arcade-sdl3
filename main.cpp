#define SDL_MAIN_HANDLED // Отключаем автоматический перехват функции main библиотекой SDL

#ifdef _WIN32

#endif

// Подключение основных модулей SDL3 для графики, работы с окнами и обработки ввода
#include <SDL3/SDL.h>
// Подключение расширения SDL_mixer для работы со звуковой картой, эффектами и музыкой
#include <SDL_mixer.h>

// Подключение стандартных библиотек C++ для работы с текстом, файлами и математикой
#include <iostream>   // Ввод и вывод данных в поток
#include <string>     // Работа со строковым типом данных string
#include <vector>     // Динамические массивы для хранения списков объектов
#include <fstream>    // Чтение и запись текстовых файлов (карты, рекорды)
#include <cmath>      // Математические функции (модуль числа, округление)
#include <algorithm>  // Готовые алгоритмы (например, сортировка списков)

using namespace std;  // Используем стандартное пространство имен C++, чтобы не писать std::

// Функция для удобного вывода текста на экран через встроенный технический шрифт SDL3
void DrawTextGUI(SDL_Renderer* renderer, float x, float y, string text, float scale, SDL_Color color = { 255, 255, 255, 255 }) {
    SDL_SetRenderScale(renderer, scale, scale);                           // Временно меняем масштаб графического движка
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // Устанавливаем выбранный цвет для текста
    SDL_RenderDebugText(renderer, x / scale, y / scale, text.c_str());   // Рисуем текст с поправкой на текущий масштаб
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);                             // Сбрасываем масштаб обратно к исходному (1:1)
}

// =============================================================================
// МЕНЕДЖЕРЫ РЕСУРСОВ (ЗАГРУЗКА ГРАФИКИ И ЗВУКА)
// =============================================================================

// Класс для управления текстурами (картинками в видеопамяти)
class TextureManager {
private:
    SDL_Renderer* renderer; // Указатель на движок отрисовки (рендерер) окна

    // Указатели на структуры картинок, загруженных в память видеокарты
    SDL_Texture* texWall;   // Текстура для блоков стен
    SDL_Texture* texBall;   // Текстура для главного героя (шарика)
    SDL_Texture* texSpike;  // Текстура для опасных шипов
    SDL_Texture* texRing;   // Текстура для собираемых колец
    SDL_Texture* texFinish; // Текстура для финишного блока
    SDL_Texture* texBg;     // Текстура для заднего фона уровня

    // Внутренняя функция для загрузки одной картинки с диска
    SDL_Texture* loadSingleTexture(string path, bool makeTransparent) {
        SDL_Surface* surface = SDL_LoadBMP(path.c_str()); // Загружаем файл BMP в оперативную память
        if (surface == NULL) return NULL;                  // Если файла нет или он сломан, выходим из функции

        if (makeTransparent) {
            // Находим на картинке чисто розовый цвет (255, 0, 255) и делаем его полностью прозрачным
            SDL_SetSurfaceColorKey(surface, true, SDL_MapSurfaceRGB(surface, 255, 0, 255));
        }

        // Переносим готовую картинку из оперативной памяти напрямую в память видеокарты
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface); // Удаляем временную копию из оперативки, чтобы избежать утечек памяти
        return texture;              // Возвращаем готовую текстуру
    }

public:
    // Конструктор: сохраняет ссылку на рендерер и обнуляет все указатели
    TextureManager(SDL_Renderer* ren) : renderer(ren) {
        texWall = NULL; texBall = NULL; texSpike = NULL;
        texRing = NULL; texFinish = NULL; texBg = NULL;
    }

    // Деструктор: автоматически очищает видеопамять при закрытии игры
    ~TextureManager() {
        if (texWall) SDL_DestroyTexture(texWall);     // Удаляем текстуру стены
        if (texBall) SDL_DestroyTexture(texBall);     // Удаляем текстуру игрока
        if (texSpike) SDL_DestroyTexture(texSpike);   // Удаляем текстуру шипов
        if (texRing) SDL_DestroyTexture(texRing);     // Удаляем текстуру кольца
        if (texFinish) SDL_DestroyTexture(texFinish); // Удаляем текстуру финиша
        if (texBg) SDL_DestroyTexture(texBg);         // Удаляем текстуру фона
    }

    // Загрузка всех основных картинок игры при старте
    void loadAllTextures() {
        texWall = loadSingleTexture("wall.bmp", false);    // Стены (прозрачность не нужна)
        texBall = loadSingleTexture("ball.bmp", true);     // Шарик (убираем розовый фон вокруг круга)
        texSpike = loadSingleTexture("spike.bmp", true);   // Шипы (убираем розовый фон вокруг треугольников)
        texRing = loadSingleTexture("ring.bmp", true);     // Кольцо (делаем внутреннюю и внешнюю часть прозрачной)
        texFinish = loadSingleTexture("finish.bmp", true); // Финишная дверь
    }

    // Динамическая загрузка фона (вызывается при старте конкретного уровня)
    void loadBackground(string path) {
        if (texBg) SDL_DestroyTexture(texBg);   // Если старый фон уже был загружен — удаляем его
        texBg = loadSingleTexture(path, false); // Загружаем новую картинку заднего плана
    }

    // Функции-геттеры для безопасного получения текстур из других классов игры
    SDL_Texture* getWall() { return texWall; }
    SDL_Texture* getBall() { return texBall; }
    SDL_Texture* getSpike() { return texSpike; }
    SDL_Texture* getRing() { return texRing; }
    SDL_Texture* getFinish() { return texFinish; }
    SDL_Texture* getBg() { return texBg; }
};

// Класс для управления звуковой картой и проигрывания аудиофайлов
class AudioManager {
private:
    MIX_Mixer* mixer; // Главный объект аудиомикшера SDL

    // Указатели на звуковые файлы, загруженные в память
    MIX_Audio* bgMusic;  // Файл длинной фоновой музыки уровней
    MIX_Audio* sndJump;  // Короткий звук прыжка шарика
    MIX_Audio* sndRing;  // Короткий звук сбора золотого кольца
    MIX_Audio* sndDeath; // Короткий звук лопания шарика при гибели

    // Виртуальные независимые аудиодорожки (каналы воспроизведения)
    MIX_Track* trackMusic; // Канал исключительно для фоновой музыки
    MIX_Track* trackJump;  // Канал для звуков прыжка
    MIX_Track* trackRing;  // Канал для звуков колец
    MIX_Track* trackDeath; // Канал для звука смерти

    int musicVol; // Уровень громкости музыки (в процентах, от 0 до 100)
    int sfxVol;   // Уровень громкости звуковых эффектов (в процентах, от 0 до 100)

public:
    // Конструктор: запускает звуковую подсистему и создает каналы
    AudioManager() {
        MIX_Init(); // Включаем звуковой модуль

        // Подключаемся к стандартному аудиоустройству воспроизведения операционной системы
        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        trackMusic = MIX_CreateTrack(mixer); // Выделяем дорожку под музыку
        trackJump = MIX_CreateTrack(mixer);  // Выделяем дорожку под прыжки
        trackRing = MIX_CreateTrack(mixer);  // Выделяем дорожку под кольца
        trackDeath = MIX_CreateTrack(mixer); // Выделяем дорожку под гибель

        bgMusic = NULL; sndJump = NULL; sndRing = NULL; sndDeath = NULL; // Обнуляем указатели звуков
        musicVol = 50; // Громкость музыки по умолчанию — 50%
        sfxVol = 80;   // Громкость эффектов по умолчанию — 80%
    }

    // Деструктор: выключает звуки и освобождает память звуковой карты
    ~AudioManager() {
        if (trackMusic) MIX_DestroyTrack(trackMusic); // Удаляем звуковую дорожку музыки
        if (trackJump) MIX_DestroyTrack(trackJump);   // Удаляем звуковую дорожку прыжка
        if (trackRing) MIX_DestroyTrack(trackRing);   // Удаляем звуковую дорожку колец
        if (trackDeath) MIX_DestroyTrack(trackDeath); // Удаляем звуковую дорожку смерти

        if (bgMusic) MIX_DestroyAudio(bgMusic);   // Удаляем из памяти аудиофайл музыки
        if (sndJump) MIX_DestroyAudio(sndJump);   // Удаляем из памяти аудиофайл прыжка
        if (sndRing) MIX_DestroyAudio(sndRing);   // Удаляем из памяти аудиофайл кольца
        if (sndDeath) MIX_DestroyAudio(sndDeath); // Удаляем из памяти аудиофайл смерти

        if (mixer) MIX_DestroyMixer(mixer); // Закрываем системный микшер звуков
        MIX_Quit();                         // Полностью выгружаем звуковую библиотеку из памяти
    }

    // Загрузка звуковых эффектов в формате .wav
    void loadSounds() {
        sndJump = MIX_LoadAudio(mixer, "jump.wav", true);   // Параметр true: полностью распаковывает звук в память,
        sndRing = MIX_LoadAudio(mixer, "ring.wav", true);   // чтобы он проигрывался мгновенно и без задержек
        sndDeath = MIX_LoadAudio(mixer, "death.wav", true); // при наступлении игрового события

        MIX_SetTrackAudio(trackJump, sndJump);   // Прикрепляем аудиофайл прыжка к его дорожке
        MIX_SetTrackAudio(trackRing, sndRing);   // Прикрепляем аудиофайл кольца к его дорожке
        MIX_SetTrackAudio(trackDeath, sndDeath); // Прикрепляем аудиофайл смерти к его дорожке
        setSFXVolume(sfxVol);                    // Применяем настройки громкости к каналам спецэффектов
    }

    // Запуск фонового трека
    void playMusic(string path) {
        if (bgMusic != NULL) {
            MIX_DestroyAudio(bgMusic); // Если уже играет какая-то музыка — удаляем старый файл трека
            bgMusic = NULL;
        }

        // Загружаем новый трек. Использование false включает потоковое чтение файла частями (экономит ОЗУ)
        bgMusic = MIX_LoadAudio(mixer, path.c_str(), false);
        if (bgMusic != NULL) {
            MIX_SetTrackAudio(trackMusic, bgMusic); // Привязываем фоновый трек к музыкальному каналу
            setMusicVolume(musicVol);               // Выставляем громкость для музыки
            MIX_PlayTrack(trackMusic, 0);           // Запускаем воспроизведение трека с начала
            MIX_SetTrackLoops(trackMusic, -1);      // Указываем значение -1, чтобы музыка играла бесконечно по кругу
        }
    }

    // Функции для мгновенной активации звуковых эффектов
    void playJump() { MIX_PlayTrack(trackJump, 0); }   // Играть звук прыжка шарика
    void playRing() { MIX_PlayTrack(trackRing, 0); }   // Играть звук подбора кольца
    void playDeath() { MIX_PlayTrack(trackDeath, 0); } // Играть звук взрыва шарика

    // Метод изменения громкости музыки (переводит проценты 0-100 в коэффициент звуковой карты 0.0-1.0)
    void setMusicVolume(int v) {
        musicVol = v;                   // Запоминаем новое значение процентов
        float gain = musicVol / 100.0f; // Вычисляем дробное значение для микшера
        MIX_SetTrackGain(trackMusic, gain); // Передаем коэффициент громкости на музыкальный трек
    }

    // Метод изменения громкости звуковых эффектов
    void setSFXVolume(int v) {
        sfxVol = v;                   // Запоминаем значение процентов
        float gain = sfxVol / 100.0f; // Вычисляем дробный коэффициент
        MIX_SetTrackGain(trackJump, gain);  // Меняем громкость канала прыжков
        MIX_SetTrackGain(trackRing, gain);  // Меняем громкость канала колец
        MIX_SetTrackGain(trackDeath, gain); // Меняем громкость канала смертей
    }

    // Геттеры для отображения текущих процентов громкости в меню настроек
    int getMusicVolume() { return musicVol; }
    int getSFXVolume() { return sfxVol; }
};

// =============================================================================
// СИСТЕМА ТАБЛИЦЫ РЕКОРДОВ
// =============================================================================

// Структура, объединяющая имя игрока и его время прохождения уровня в одну запись
struct Record {
    string playerName; // Текстовое имя (никнейм) игрока
    float bestTime;    // Итоговое время прохождения уровня в секундах
};

// Функция-компаратор: задает правило сортировки (кто прошел быстрее — тот находится выше в списке)
bool compareRecords(const Record& a, const Record& b) {
    return a.bestTime < b.bestTime; // Сравниваем время двух результатов
}

// Класс для загрузки, сохранения и обработки таблиц рекордов
class RecordsManager {
private:
    vector<Record> level1_Records; // Список лучших результатов для Уровня 1
    vector<Record> level2_Records; // Список лучших результатов для Уровня 2
    vector<Record> level3_Records; // Список лучших результатов для Уровня 3
    string filename = "records.txt"; // Имя текстового файла, где физически хранятся рекорды на диске

    // Вспомогательная функция для быстрого получения нужного списка по индексу уровня
    vector<Record>& getListByLevel(int levelIndex) {
        if (levelIndex == 0) return level1_Records;
        if (levelIndex == 1) return level2_Records;
        return level3_Records;
    }

public:
    // Конструктор: автоматически считывает рекорды с диска при старте приложения
    RecordsManager() {
        loadRecords();
    }

    // Чтение файла рекордов
    void loadRecords() {
        level1_Records.clear(); // Полностью очищаем старые массивы в памяти
        level2_Records.clear();
        level3_Records.clear();

        ifstream file(filename); // Открываем файл для чтения данных
        if (file.is_open()) {
            int lvl;         // Временная переменная для номера уровня
            string name;     // Временная переменная для имени
            float time;      // Временная переменная для времени
            while (file >> lvl >> name >> time) { // Читаем данные строчка за строчкой до конца файла
                Record r;
                r.playerName = name;
                r.bestTime = time;
                getListByLevel(lvl).push_back(r); // Распределяем запись в соответствующий массив уровня
            }
            file.close(); // Закрываем открытый текстовый файл

            // Сортируем списки всех трех уровней: лучшие результаты перемещаются в начало списков
            sort(level1_Records.begin(), level1_Records.end(), compareRecords);
            sort(level2_Records.begin(), level2_Records.end(), compareRecords);
            sort(level3_Records.begin(), level3_Records.end(), compareRecords);
        }
    }

    // Запись таблицы результатов обратно в файл на диске
    void saveRecords() {
        ofstream file(filename); // Открываем (или перезаписываем) файл для записи данных
        if (file.is_open()) {
            // Записываем сначала все строки первого уровня, затем второго, затем третьего
            for (size_t i = 0; i < level1_Records.size(); i++) file << 0 << " " << level1_Records[i].playerName << " " << level1_Records[i].bestTime << "\n";
            for (size_t i = 0; i < level2_Records.size(); i++) file << 1 << " " << level2_Records[i].playerName << " " << level2_Records[i].bestTime << "\n";
            for (size_t i = 0; i < level3_Records.size(); i++) file << 2 << " " << level3_Records[i].playerName << " " << level3_Records[i].bestTime << "\n";
            file.close(); // Закрываем файл после успешной записи
        }
    }

    // Добавление нового рекорда в таблицу по окончании игры
    void addRecord(int levelIndex, string playerName, float time) {
        if (playerName == "") playerName = "Player"; // Если игрок ничего не ввел, даем ему имя по умолчанию

        Record r;
        r.playerName = playerName;
        r.bestTime = time;

        vector<Record>& targetList = getListByLevel(levelIndex); // Получаем ссылку на нужный массив результатов
        targetList.push_back(r);                                 // Добавляем новую запись в конец списка
        sort(targetList.begin(), targetList.end(), compareRecords); // Пересортировываем список с учетом новой записи

        // Ограничиваем таблицу: если результатов стало больше 5, удаляем все что ниже пятого места
        if (targetList.size() > 5) {
            targetList.resize(5);
        }
        saveRecords(); // Автоматически перезаписываем обновленный файл на диске
    }

    // Очистка таблицы рекордов конкретного уровня
    void clearLevelRecords(int levelIndex) {
        getListByLevel(levelIndex).clear(); // Очищаем динамический массив в памяти
        saveRecords();                      // Записываем пустой результат в файл на диске
    }

    // Метод получения копии списка рекордов уровня для вывода их на экран в меню
    vector<Record> getRecords(int levelIndex) {
        return getListByLevel(levelIndex);
    }
};

// =============================================================================
// ЭЛЕМЕНТЫ ИНТЕРФЕЙСА (ПОЛЬЗОВАТЕЛЬСКИЙ UI)
// =============================================================================

// Класс для создания интерактивных графических кнопок в меню
class UIButton {
private:
    SDL_FRect rect; // Координаты (x, y) и физические размеры (w, h) кнопки на экране
    string text;    // Надпись, отображаемая поверх кнопки
    int tag;        // Дополнительный числовой идентификатор (например, номер уровня для кнопки)

public:
    // Конструктор кнопки: инициализирует её положение, размеры, текст и тег
    UIButton(float x, float y, float w, float h, string btnText, int btnTag = -1) {
        rect.x = x; rect.y = y; rect.w = w; rect.h = h;
        text = btnText;
        tag = btnTag;
    }

    // Метод отрисовки кнопки на экране
    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 70, 130, 180, 255); // Задаем цвет заливки кнопки (стальной синий)
        SDL_RenderFillRect(renderer, &rect);                 // Закрашиваем внутреннюю область кнопки

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Задаем цвет контура кнопки (белый)
        SDL_RenderRect(renderer, &rect);                     // Рисуем рамку по границам прямоугольника

        DrawTextGUI(renderer, rect.x + 10, rect.y + 15, text, 1.5f); // Рисуем текст кнопки со смещением внутрь
    }

    // Функция проверки: кликнул ли пользователь мышкой по кнопке
    bool isClicked(float mouseX, float mouseY) {
        // Проверяем, входят ли координаты курсора мыши в границы прямоугольника кнопки (AABB-тест)
        if (mouseX >= rect.x && mouseX <= rect.x + rect.w) {
            if (mouseY >= rect.y && mouseY <= rect.y + rect.h) {
                return true; // Курсор мыши находится внутри кнопки, клик засчитан
            }
        }
        return false; // Клик мимо кнопки
    }

    int getTag() { return tag; } // Возврат тега кнопки
};

// =============================================================================
// ИГРОВЫЕ ОБЪЕКТЫ (ИЕРАРХИЯ КЛАССОВ ООП)
// =============================================================================

// Абстрактный базовый класс для абсолютно всех объектов на игровой карте
class GameObject {
protected:
    float x, y;          // Мировые координаты объекта на карте уровня
    float width, height; // Размеры хитбокса (физической модели) объекта
    SDL_Texture* texture; // Указатель на графическую картинку объекта

public:
    // Конструктор базового объекта
    GameObject(float startX, float startY, float w, float h) : x(startX), y(startY), width(w), height(h), texture(NULL) {}
    virtual ~GameObject() {} // Виртуальный деструктор для корректного удаления классов-наследников

    // Чисто виртуальная функция отрисовки: каждый наследник обязан реализовать её сам
    virtual void draw(SDL_Renderer* renderer, float cameraX, float cameraY) = 0;

    // Вспомогательные функции для получения физических параметров объекта
    SDL_FRect getRect() { return { x, y, width, height }; } // Возвращает прямоугольник для проверки столкновений
    void setTexture(SDL_Texture* tex) { texture = tex; }    // Привязывает картинку к объекту
    float getX() { return x; }
    float getY() { return y; }
    float getWidth() { return width; }
    float getHeight() { return height; }
};

// Класс твердой стены (препятствие, от которого игрок отталкивается)
class Wall : public GameObject {
public:
    Wall(float x, float y, float w, float h) : GameObject(x, y, w, h) {}
    void draw(SDL_Renderer* renderer, float cameraX, float cameraY) override {
        SDL_FRect dest = { x - cameraX, y - cameraY, width, height }; // Считаем экранную позицию с учетом сдвига камеры
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, &dest); // Отрисовываем картинку блока стены
        }
        else {
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // Если картинка не загрузилась — рисуем серый квадрат
            SDL_RenderFillRect(renderer, &dest);
        }
    }
};

// Класс смертоносных шипов (убивают игрока при касании)
class Spike : public GameObject {
public:
    Spike(float x, float y, float w, float h) : GameObject(x, y, w, h) {}
    void draw(SDL_Renderer* renderer, float cameraX, float cameraY) override {
        SDL_FRect dest = { x - cameraX, y - cameraY, width, height }; // Переводим координаты из мировых в экранные
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, &dest); // Рисуем текстуру шипов
        }
        else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);     // Запасной вариант: красный блок
            SDL_RenderFillRect(renderer, &dest);
        }
    }
};

// Класс финишных дверей (активируется, когда собраны все кольца)
class Finish : public GameObject {
public:
    Finish(float x, float y, float w, float h) : GameObject(x, y, w, h) {}
    void draw(SDL_Renderer* renderer, float cameraX, float cameraY) override {
        SDL_FRect dest = { x - cameraX, y - cameraY, width, height }; // Рассчитываем положение двери относительно камеры
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, &dest); // Отрисовываем картинку финиша
        }
        else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);     // Запасной вариант: зеленый блок
            SDL_RenderFillRect(renderer, &dest);
        }
    }
};

// Класс золотого кольца (игрок должен собирать их для прохождения)
class Ring : public GameObject {
private:
    bool isCollected; // Флаг состояния: подобрано кольцо игроком или еще нет

public:
    Ring(float x, float y, float size) : GameObject(x, y, size, size), isCollected(false) {}
    void draw(SDL_Renderer* renderer, float cameraX, float cameraY) override {
        if (isCollected) return; // Если кольцо уже подобрано, полностью игнорируем его отрисовку
        SDL_FRect dest = { x - cameraX, y - cameraY, width, height }; // Переводим координаты под экран игрока
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, &dest); // Выводим графику кольца
        }
        else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);   // Запасной вариант: желтый квадрат
            SDL_RenderFillRect(renderer, &dest);
        }
    }
    void collect() { isCollected = true; }   // Переключение флага при взятии объекта
    bool getCollected() { return isCollected; } // Узнать текущий статус кольца
};

// Класс управляемого игрока (красный Bounce-шарик)
class Player : public GameObject {
private:
    float speedX;  // Скорость движения шарика по горизонтали (ось X)
    float speedY;  // Скорость движения шарика по вертикали (ось Y)
    bool onGround; // Флаг: стоит ли шарик твердо на земле/платформе

public:
    Player(float startX, float startY, float size) : GameObject(startX, startY, size, size) {
        speedX = 0; speedY = 0; // На старте шарик полностью неподвижен
        onGround = false;       // Шарик начинает движение в состоянии падения
    }

    // Математический обсчет движения физики игрока
    void update(float dt) {
        speedY = speedY + 800 * dt; // Постоянно прибавляем силу гравитации к вертикальной скорости
        x = x + speedX * dt;        // Изменяем координату X на величину горизонтальной скорости
        y = y + speedY * dt;        // Изменяем координату Y на величину вертикальной скорости
    }

    void draw(SDL_Renderer* renderer, float cameraX, float cameraY) override {
        SDL_FRect dest = { x - cameraX, y - cameraY, width, height }; // Вычисляем позицию шарика на мониторе
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, &dest); // Отрисовываем картинку катящегося шарика
        }
        else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);     // Запасной вариант: чисто красный квадрат
            SDL_RenderFillRect(renderer, &dest);
        }
    }

    // Функция прыжка вверх
    void jump() {
        if (onGround) {     // Прыгать можно исключительно в том случае, если мы стоим на твердой поверхности
            speedY = -450;  // Задаем резкий импульс скорости вверх (минус по оси Y)
            onGround = false; // Сразу снимаем флаг земли, так как шарик взлетает
        }
    }

    // Функции управления горизонтальной скоростью шарика
    void moveLeft() { speedX = -250; }  // Движение влево (минус по X)
    void moveRight() { speedX = 250; }  // Движение вправо (плюс по X)
    void stopX() { speedX = 0; }        // Игрок отпустил кнопки ходьбы — останавливаемся по горизонтали
    void stopY() { speedY = 0; }        // Обнуление вертикальной скорости при ударах
    void land() {
        onGround = true; // Шарик успешно приземлился на платформу
        speedY = 0;      // Сбрасываем скорость падения в ноль
    }

    // Принудительное изменение координат (например, при спавне или выталкивании из стен)
    void setX(float newX) { x = newX; }
    void setY(float newY) { y = newY; }
    float getSpeedY() { return speedY; } // Проверка текущего направления падения/взлета
};

// =============================================================================
// ГЛАВНЫЙ ИГРОВОЙ ДВИЖОК (МЕНЕДЖЕР ВСЕХ СИСТЕМ)
// =============================================================================

class GameEngine {
private:
    SDL_Window* window;       // Указатель на окно операционной системы
    SDL_Renderer* renderer;   // Указатель на графический движок отрисовки
    TextureManager* textures; // Модуль управления текстурами картинок
    AudioManager* audio;       // Модуль управления аудиосистемой
    RecordsManager recordsManager; // Объект управления базой рекордов

    bool isRunning; // Флаг работы главного цикла игры (false — полностью закрывает приложение)
    int state;      // Текущее состояние игры: 0=Главное меню, 1=Выбор уровня, 2=Геймплей, 3=Пауза, 4=Победа, 5=Поражение, 6=Ввод имени, 7=Экран уровней рекордов, 8=Просмотр Топ-5, 9=Настройки

    vector<GameObject*> staticObjects; // Массив всех блоков на текущей карте (стены, шипы, кольца)
    Player* player;                     // Указатель на живой объект игрока
    Finish* finishBlock;               // Ссылка на блок финиша для проверки условий победы

    float cameraX, cameraY; // Мировые координаты положения камеры слежения
    float mapWidth, mapHeight; // Физические размеры текущей карты в пикселях
    float startX, startY;     // Начальные координаты спавна игрока при старте или смерти

    int totalRings;     // Сколько всего колец раскидано на текущей карте уровня
    int collectedRings; // Сколько колец игрок уже успешно подобрал
    int lives;          // Текущее количество жизней шарика
    float timeElapsed;  // Сколько секунд уже идет игра на уровне
    float timeLimit;    // Предельный лимит времени на прохождение уровня (в секундах)

    string levels[3] = { "level1.txt", "level2.txt", "level3.txt" }; // Список имен файлов с картами уровней
    int currentLevelIndex;      // Индекс уровня, в который сейчас играет пользователь (0, 1 или 2)
    int recordViewingLevelIndex; // Индекс уровня, чью таблицу рекордов мы сейчас изучаем в меню
    string inputPlayerName;     // Строка, куда посимвольно записывается вводимое имя игрока

    // Списки динамических кнопок для каждого раздельного экрана интерфейса игры
    vector<UIButton*> menuButtons;
    vector<UIButton*> levelButtons;
    vector<UIButton*> pauseButtons;
    vector<UIButton*> settingsButtons;
    vector<UIButton*> recordButtons;

public:
    // Конструктор движка: запускает SDL, создает окно и генерирует кнопки интерфейса
    GameEngine() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); // Инициализируем видео- и аудио-подсистемы SDL
        window = SDL_CreateWindow("Bounce Arcade", 800, 600, 0); // Создаем окно размером 800 на 600 пикселей
        renderer = SDL_CreateRenderer(window, NULL);             // Подключаем графический рендерер к окну
        SDL_SetRenderLogicalPresentation(renderer, 800, 600, SDL_LOGICAL_PRESENTATION_LETTERBOX); // Включаем умное сохранение пропорций экрана

        textures = new TextureManager(renderer); // Создаем менеджер картинок
        textures->loadAllTextures();             // Сразу загружаем всю базовую графику в видеопамять

        audio = new AudioManager(); // Создаем менеджер звука
        audio->loadSounds();        // Загружаем спецэффекты прыжков и смертей
        audio->playMusic("music_menu.wav"); // Сходу запускаем расслабляющий трек для главного меню

        isRunning = true; // Запускаем флаг игры
        state = 0;        // При старте принудительно открываем экран Главного меню
        player = NULL; finishBlock = NULL; // Обнуляем ссылки на игровые сущности
        timeLimit = 60.0f; // Ставим ограничение раунда в 60 секунд

        // Наполнение кнопками экрана Главного меню (state 0)
        menuButtons.push_back(new UIButton(300, 200, 200, 40, "PLAY GAME"));
        menuButtons.push_back(new UIButton(300, 260, 200, 40, "RECORDS"));
        menuButtons.push_back(new UIButton(300, 320, 200, 40, "SETTINGS"));
        menuButtons.push_back(new UIButton(300, 380, 200, 40, "EXIT"));

        // Наполнение кнопками экрана Выбора уровней (state 1 и state 7)
        for (int i = 0; i < 3; i++) {
            levelButtons.push_back(new UIButton(300, 150 + i * 80, 200, 50, "Level " + to_string(i + 1), i));
        }

        // Наполнение кнопками экрана Паузы (state 3)
        pauseButtons.push_back(new UIButton(300, 250, 200, 50, "RESUME"));
        pauseButtons.push_back(new UIButton(300, 350, 200, 50, "MAIN MENU"));

        // Наполнение кнопками экрана Настроек звука (state 9)
        settingsButtons.push_back(new UIButton(250, 250, 50, 40, "-")); // Кнопка уменьшения музыки
        settingsButtons.push_back(new UIButton(500, 250, 50, 40, "+")); // Кнопка увеличения музыки
        settingsButtons.push_back(new UIButton(250, 350, 50, 40, "-")); // Кнопка уменьшения звуков sfx
        settingsButtons.push_back(new UIButton(500, 350, 50, 40, "+")); // Кнопка увеличения звуков sfx
        settingsButtons.push_back(new UIButton(300, 500, 200, 50, "BACK"));

        // Наполнение кнопками экрана просмотра таблицы рекордов (state 8)
        recordButtons.push_back(new UIButton(150, 500, 200, 40, "BACK"));
        recordButtons.push_back(new UIButton(440, 500, 220, 40, "CLEAR RECORDS"));
    }

    // Деструктор: подчищает за собой абсолютно всю выделенную динамическую память
    ~GameEngine() {
        clearLevel(); // Удаляем карту, если она была загружена
        delete textures; // Стираем текстуры
        delete audio;    // Отключаем аудиосистему

        // Удаляем из памяти структуры всех кнопок UI, чтобы освободить ОЗУ
        for (size_t i = 0; i < menuButtons.size(); i++) delete menuButtons[i];
        for (size_t i = 0; i < levelButtons.size(); i++) delete levelButtons[i];
        for (size_t i = 0; i < pauseButtons.size(); i++) delete pauseButtons[i];
        for (size_t i = 0; i < settingsButtons.size(); i++) delete settingsButtons[i];
        for (size_t i = 0; i < recordButtons.size(); i++) delete recordButtons[i];

        SDL_DestroyRenderer(renderer); // Уничтожаем графический рендерер
        SDL_DestroyWindow(window);     // Ломаем окно приложения
        SDL_Quit();                    // Полностью закрываем библиотеки SDL
    }

    // Полная зачистка текущего уровня перед выходом в меню или загрузкой новой карты
    void clearLevel() {
        for (size_t i = 0; i < staticObjects.size(); i++) {
            delete staticObjects[i]; // Очищаем по очереди память каждого блока на карте
        }
        staticObjects.clear(); // Обнуляем сам массив-контейнер указателей
        if (player) {
            delete player;    // Стираем объект игрока
            player = NULL;    // Зануляем указатель во избежание ошибок
        }
        finishBlock = NULL;   // Забываем указатель на финиш
    }

    // Метод парсинга и загрузки карты уровня из текстового файла формата .txt
    void loadLevel(int levelIndex) {
        clearLevel(); // Сначала очищаем остатки старого уровня
        currentLevelIndex = levelIndex; // Запоминаем текущий индекс выбранной локации
        string filename = levels[levelIndex]; // Извлекаем имя текстового файла карты

        ifstream file(filename); // Пробуем открыть файл уровня на чтение
        if (file.is_open() == false) return; // Если файла карты нет в папке, аварийно выходим

        audio->playMusic("music_game.wav"); // Включаем динамичную музыку для игрового процесса
        textures->loadBackground("bg_level" + to_string(levelIndex + 1) + ".bmp"); // Подгружаем картинку фона под этот уровень

        string line;
        int row = 0;         // Счетчик текущей строки карты
        float tileSize = 40.0f; // Базовый физический размер одного квадратного блока на экране
        mapWidth = 0;        // Сброс ширины карты
        totalRings = 0;      // Сброс счетчика колец уровня
        collectedRings = 0;  // Сброс счетчика собранных игроком колец
        lives = 3;           // Выдаем стандартные 3 жизни на уровень
        timeElapsed = 0.0f;  // Обнуляем таймер времени

        // Посимвольно читаем текстовый файл построчно
        while (getline(file, line)) {
            for (size_t col = 0; col < line.length(); col++) {
                float px = col * tileSize; // Вычисляем мировую X позицию блока на карте
                float py = row * tileSize; // Вычисляем мировую Y позицию блока на карте

                if (px + tileSize > mapWidth) mapWidth = px + tileSize; // Считаем общую максимальную ширину уровня

                if (line[col] == '#') { // Символ решетки — создаем блок твердой Стены
                    Wall* w = new Wall(px, py, tileSize, tileSize);
                    w->setTexture(textures->getWall()); // Навешиваем графику стены
                    staticObjects.push_back(w);          // Добавляем стену в общий вектор объектов
                }
                else if (line[col] == '*') { // Символ звездочки — создаем ловушку с Шипами
                    Spike* s = new Spike(px, py, tileSize, tileSize);
                    s->setTexture(textures->getSpike()); // Навешиваем графику шипов
                    staticObjects.push_back(s);
                }
                else if (line[col] == 'O') { // Символ буквы O — создаем золотое Кольцо
                    Ring* r = new Ring(px + 10, py + 10, 20); // Смещаем кольцо к центру ячейки
                    r->setTexture(textures->getRing());       // Навешиваем графику кольца
                    staticObjects.push_back(r);
                    totalRings++; // Увеличиваем счетчик необходимых для прохождения колец
                }
                else if (line[col] == 'X') { // Символ буквы X — создаем финишные Ворота
                    Finish* f = new Finish(px, py, tileSize, tileSize);
                    f->setTexture(textures->getFinish()); // Навешиваем графику финиша
                    staticObjects.push_back(f);
                    finishBlock = f; // Сохраняем прямую ссылку на финиш для обсчета триггера победы
                }
                else if (line[col] == '@') { // Символ собаки — это точка появления Игрока
                    player = new Player(px, py, 30); // Создаем игровой шар размером 30 пикселей
                    player->setTexture(textures->getBall()); // Навешиваем текстуру шарика
                    startX = px; startY = py; // Запоминаем точку старта для чекпоинта при смертях
                }
            }
            row++; // Переходим к чтению следующей строки текстового файла карты
        }
        file.close(); // Закрываем файл после парсинга карты
        mapHeight = row * tileSize; // Вычисляем общую финальную высоту карты уровня
        state = 2; // Принудительно переключаем состояние движка в режим непосредственной игры
    }

    // Обработка кликов мыши, нажатий кнопок клавиатуры и системных событий ОС
    void handleInput() {
        SDL_Event event; // Структура для записи входящего события
        while (SDL_PollEvent(&event)) { // Вытаскиваем события из системной очереди в цикле
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false; // Игрок нажал на крестик окна — завершаем игровой цикл
            }

            // Обработка кликов левой кнопки мыши
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mx, my;
                SDL_ConvertEventToRenderCoordinates(renderer, &event); // Пересчитываем координаты клика под масштаб 800х600
                mx = event.button.x; my = event.button.y;              // Получаем скорректированные координаты курсора

                if (state == 0) { // Клик в Главном меню
                    if (menuButtons[0]->isClicked(mx, my)) state = 1; // PLAY GAME -> выбор уровня
                    if (menuButtons[1]->isClicked(mx, my)) state = 7; // RECORDS -> выбор рекордов уровня
                    if (menuButtons[2]->isClicked(mx, my)) state = 9; // SETTINGS -> настройки громкости
                    if (menuButtons[3]->isClicked(mx, my)) isRunning = false; // EXIT -> закрыть игру
                }
                else if (state == 1) { // Клик на экране выбора уровня для игры
                    for (size_t i = 0; i < levelButtons.size(); i++) {
                        if (levelButtons[i]->isClicked(mx, my)) {
                            loadLevel(levelButtons[i]->getTag()); // Загружаем карту выбранного уровня
                            break;
                        }
                    }
                }
                else if (state == 3) { // Клик на экране Паузы
                    if (pauseButtons[0]->isClicked(mx, my)) state = 2; // RESUME -> продолжить игру
                    if (pauseButtons[1]->isClicked(mx, my)) {
                        state = 0; // MAIN MENU -> сбросить уровень и выйти в главное меню
                        audio->playMusic("music_menu.wav"); // Переключаем аудио на музыку меню
                    }
                }
                else if (state == 7) { // Клик на экране выбора уровня для просмотра рекордов
                    for (size_t i = 0; i < levelButtons.size(); i++) {
                        if (levelButtons[i]->isClicked(mx, my)) {
                            recordViewingLevelIndex = levelButtons[i]->getTag(); // Запоминаем выбранный уровень
                            state = 8; // Переходим на экран вывода Топ-5 результатов
                            break;
                        }
                    }
                }
                else if (state == 8) { // Клик на экране просмотра Топ-5 рекордов
                    if (recordButtons[0]->isClicked(mx, my)) state = 7; // BACK -> вернуться к списку уровней рекордов
                    if (recordButtons[1]->isClicked(mx, my)) recordsManager.clearLevelRecords(recordViewingLevelIndex); // Очистить рекорды
                }
                else if (state == 9) { // Клик в меню настроек громкости
                    if (settingsButtons[0]->isClicked(mx, my)) { // Музыка Минус
                        audio->setMusicVolume(audio->getMusicVolume() - 10);
                        if (audio->getMusicVolume() < 0) audio->setMusicVolume(0);
                    }
                    if (settingsButtons[1]->isClicked(mx, my)) { // Музыка Плюс
                        audio->setMusicVolume(audio->getMusicVolume() + 10);
                        if (audio->getMusicVolume() > 100) audio->setMusicVolume(100);
                    }
                    if (settingsButtons[2]->isClicked(mx, my)) { // Звуки Минус
                        audio->setSFXVolume(audio->getSFXVolume() - 10);
                        if (audio->getSFXVolume() < 0) audio->setSFXVolume(0);
                    }
                    if (settingsButtons[3]->isClicked(mx, my)) { // Звуки Плюс
                        audio->setSFXVolume(audio->getSFXVolume() + 10);
                        if (audio->getSFXVolume() > 100) audio->setSFXVolume(100);
                    }
                    if (settingsButtons[4]->isClicked(mx, my)) state = 0; // BACK -> назад в главное меню
                }
            }

            // Обработка одиночных нажатий на клавиши клавиатуры
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) { // Нажатие клавиши ESCAPE
                    if (state == 1 || state == 7 || state == 9) state = 0; // Возвращает в главное меню
                    else if (state == 8) state = 7;                        // Из рекордов возвращает к выбору уровня
                    else if (state == 2) state = 3;                        // Во время геймплея ставит игру на Паузу
                    else if (state == 3) state = 2;                        // В паузе снимает с нее
                    else if (state == 6) {                                 // При вводе имени отменяет сохранение рекорда
                        SDL_StopTextInput(window);
                        state = 0; audio->playMusic("music_menu.wav");
                    }
                }

                if (state == 4) { // Экран Победы
                    if (event.key.key == SDLK_RETURN) { // Клавиша ENTER запускает процесс сохранения рекорда
                        inputPlayerName = "";
                        SDL_StartTextInput(window); // Включаем встроенную подсистему текстового ввода ОС
                        state = 6;                  // Переводим экран в текстовый ввод имени
                    }
                }
                else if (state == 5) { // Экран Поражения (Game Over)
                    if (event.key.key == SDLK_RETURN) { // Клавиша ENTER возвращает в главное меню
                        state = 0; audio->playMusic("music_menu.wav");
                    }
                }
                else if (state == 6) { // Экран непосредственного набора текста имени
                    if (event.key.key == SDLK_BACKSPACE && inputPlayerName.length() > 0) {
                        inputPlayerName.pop_back(); // Нажатие стирания Backspace удаляет последнюю букву строки
                    }
                    else if (event.key.key == SDLK_RETURN) { // Клавиша ENTER сохраняет имя
                        SDL_StopTextInput(window); // Отключаем текстовый ввод ОС
                        recordsManager.addRecord(currentLevelIndex, inputPlayerName, timeElapsed); // Пишем рекорд в Топ-5
                        audio->playMusic("music_menu.wav"); // Включаем музыку меню
                        state = 0; // Уходим на главный экран
                    }
                }
            }

            // Системный перехват введенных символов с клавиатуры для набора имени игрока
            if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (state == 6) { // Проверяем, что мы в состоянии ввода имени, длина строки меньше 10 и это не пробел
                    if (inputPlayerName.length() < 10 && event.text.text[0] != ' ') {
                        inputPlayerName += event.text.text; // Приклеиваем нажатый символ к никнейму
                    }
                }
            }
        }

        // Непрерывный опрос зажатых кнопок движения во время геймплея (state 2)
        if (state == 2 && player != NULL) {
            const bool* keys = SDL_GetKeyboardState(NULL); // Берем массив состояний всех физических кнопок

            if (keys[SDL_SCANCODE_LEFT]) player->moveLeft();       // Нажата стрелка влево -> катимся влево
            else if (keys[SDL_SCANCODE_RIGHT]) player->moveRight(); // Нажата стрелка вправо -> катимся вправо
            else player->stopX();                                   // Ничего не нажато -> плавно останавливаемся по оси X

            if (keys[SDL_SCANCODE_SPACE]) { // Проверка нажатия кнопки ПРОБЕЛ для прыжка
                if (player->getSpeedY() == 0) audio->playJump(); // Если вертикальная скорость ноль (на земле) — издаем звук прыжка
                player->jump(); // Выполняем прыжок
            }
        }
    }

    // Алгоритмы обсчета столкновений (Коллизий) игрока с блоками на карте уровня
    void handleCollisions() {
        SDL_FRect pRect = player->getRect(); // Получаем хитбокс прямоугольника игрока

        // Проверяем игрока на пересечение со всеми статическими блоками карты в цикле
        for (size_t i = 0; i < staticObjects.size(); i++) {
            GameObject* obj = staticObjects[i];
            SDL_FRect objRect = obj->getRect(); // Получаем хитбокс очередного блока

            // Функция проверяет, пересекаются ли хитбоксы игрока и проверяемого блока
            if (SDL_HasRectIntersectionFloat(&pRect, &objRect)) {

                // Проверяем: является ли этот блок Твердой Стеной
                Wall* wall = dynamic_cast<Wall*>(obj);
                if (wall != NULL) {
                    // Вычисляем математические центры хитбоксов игрока и стены
                    float pCenterY = pRect.y + pRect.h / 2; float objCenterY = objRect.y + objRect.h / 2;
                    float pCenterX = pRect.x + pRect.w / 2; float objCenterX = objRect.x + objRect.w / 2;

                    // Находим глубину наложения (взаимного проникновения) объектов друг в друга по осям X и Y
                    float overlapX = (pRect.w + objRect.w) / 2 - abs(pCenterX - objCenterX);
                    float overlapY = (pRect.h + objRect.h) / 2 - abs(pCenterY - objCenterY);

                    if (overlapX > 0 && overlapY > 0) {
                        // Выталкиваем шарик по оси с НАИМЕНЬШИМ наложением, чтобы коллизия была корректной
                        if (overlapY < overlapX) {
                            if (pCenterY < objCenterY) { // Шарик упал на блок сверху
                                player->setY(objRect.y - pRect.h); // Ставим шарик строго на верхнюю грань блока
                                player->land();                    // Активируем приземление шарика (обнуляем скорость Y)
                            }
                            else { // Шарик ударился о блок снизу
                                player->setY(objRect.y + objRect.h); // Выталкиваем шарик под блок
                                player->stopY();                     // Обнуляем скорость взлета
                            }
                        }
                        else { // Выталкивание по горизонтали (боковые столкновения со стенами)
                            if (pCenterX < objCenterX) player->setX(objRect.x - pRect.w); // Выталкиваем влево от стены
                            else player->setX(objRect.x + objRect.w);                     // Выталкиваем вправо от стены
                            player->stopX(); // Гасим горизонтальную скорость шарика
                        }
                    }
                }

                // Проверяем: является ли этот блок Опасными Шипами
                Spike* spike = dynamic_cast<Spike*>(obj);
                if (spike != NULL) {
                    lives--;             // Отнимаем у игрока одну жизнь
                    audio->playDeath();  // Воспроизводим звук лопания/смерти шарика
                    player->setX(startX); // Сбрасываем координаты игрока на точку старта уровня
                    player->setY(startY);
                    player->stopX(); player->stopY(); // Полностью останавливаем шарик
                    if (lives <= 0) state = 5;         // Если жизней больше нет — переключаем стейт в Game Over
                }

                // Проверяем: является ли этот блок Золотым Кольцом
                Ring* ring = dynamic_cast<Ring*>(obj);
                if (ring != NULL) {
                    if (ring->getCollected() == false) { // Если кольцо еще не подобрано
                        ring->collect();                 // Переводим его в статус собранного (скрываем)
                        collectedRings++;                // Прибавляем очко в счетчик колец уровня
                        audio->playRing();               // Проигрываем приятный звук подбора кольца
                    }
                }
            }
        }

        // Проверка условия завершения уровня через триггер Финиша
        if (finishBlock != NULL && collectedRings >= totalRings) { // Финиш работает только если собраны абсолютно ВСЕ кольца
            SDL_FRect pRect = player->getRect();
            SDL_FRect fRect = finishBlock->getRect();
            if (SDL_HasRectIntersectionFloat(&pRect, &fRect)) { // Если шарик пересек двери финиша
                state = 4; // Переключаем игру в состояние Уровень Пройден (Победа)
            }
        }
    }

    // Обновление всей игровой логики (вызывается каждый кадр)
    void update(float dt) {
        if (state == 2) { // Логика обновляется только внутри активного игрового процесса
            player->update(dt);   // Обсчитываем физику перемещения и гравитации игрока
            handleCollisions();  // Проверяем и разрешаем столкновения со всеми блоками карты

            timeElapsed += dt; // Наращиваем таймер прошедшего времени на величину дельты кадра
            if (timeElapsed >= timeLimit) { // Если игрок не уложился в отведенный лимит времени
                lives--;             // Наказываем смертью и забираем одну жизнь
                audio->playDeath();  // Воспроизводим звук гибели
                timeElapsed = 0.0f;  // Сбрасываем таймер времени на ноль
                player->setX(startX); // Респавним игрока на начальной точке карты
                player->setY(startY);
                player->stopX(); player->stopY();
                if (lives <= 0) state = 5; // Проверяем на окончательное поражение (Game Over)
            }

            // Расчет логики плавного движения Камеры слежения за шариком
            cameraX = player->getX() - 400; // Центрируем камеру по горизонтали (800 / 2 = 400 пикселей)
            cameraY = player->getY() - 300; // Центрируем камеру по вертикали (600 / 2 = 300 пикселей)

            // Условия жесткой блокировки камеры, чтобы она не выходила за границы карты и не показывала пустоту
            if (cameraX < 0) cameraX = 0; // Левая граница уровня
            if (mapWidth > 800 && cameraX > mapWidth - 800) cameraX = mapWidth - 800; // Правая граница уровня
            if (cameraY < 0) cameraY = 0; // Верхняя граница уровня
            if (mapHeight > 600 && cameraY > mapHeight - 600) cameraY = mapHeight - 600; // Нижняя граница уровня
        }
    }

    // Метод вывода всей графики на экран монитора (вызывается каждый кадр)
    void draw() {
        SDL_SetRenderDrawColor(renderer, 30, 40, 60, 255); // Задаем темный цвет фона для очистки экрана
        SDL_RenderClear(renderer);                         // Полностью очищаем старый кадр из буфера

        if (state == 0) { // Отрисовка экрана Главного меню
            DrawTextGUI(renderer, 250, 100, "BOUNCE ARCADE", 4.0f, { 255, 215, 0, 255 }); // Рисуем золотой заголовок игры
            for (size_t i = 0; i < menuButtons.size(); i++) menuButtons[i]->draw(renderer); // Рисуем кнопки меню в цикле
            DrawTextGUI(renderer, 10, 570, "made by bakai4eg", 1.5f, { 150, 150, 150, 255 }); // Маленький водяной знак
        }
        else if (state == 1) { // Отрисовка экрана выбора уровня игры
            DrawTextGUI(renderer, 250, 80, "PLAY: SELECT LEVEL", 3.0f, { 255, 215, 0, 255 });
            for (size_t i = 0; i < levelButtons.size(); i++) levelButtons[i]->draw(renderer); // Выводим кнопки уровней
            DrawTextGUI(renderer, 300, 550, "Press ESC to return", 1.5f, { 150, 150, 150, 255 });
        }
        else if (state == 7) { // Отрисовка экрана выбора уровня для таблиц рекордов
            DrawTextGUI(renderer, 200, 80, "RECORDS: SELECT LEVEL", 3.0f, { 255, 215, 0, 255 });
            for (size_t i = 0; i < levelButtons.size(); i++) levelButtons[i]->draw(renderer);
            DrawTextGUI(renderer, 300, 550, "Press ESC to return", 1.5f, { 150, 150, 150, 255 });
        }
        else if (state == 8) { // Отрисовка экрана отображения Топ-5 результатов игроков
            DrawTextGUI(renderer, 150, 50, "TOP 5 - Level " + to_string(recordViewingLevelIndex + 1), 3.0f, { 255, 215, 0, 255 });
            vector<Record> recs = recordsManager.getRecords(recordViewingLevelIndex); // Запрашиваем записи рекордов из базы данных
            if (recs.size() == 0) {
                DrawTextGUI(renderer, 250, 200, "NO RECORDS YET!", 3.0f, { 200, 200, 200, 255 }); // Сообщение, если файл рекордов пуст
            }
            else {
                float currentY = 150.0f; // Начальная Y позиция для вывода первой строки рекорда
                for (size_t i = 0; i < recs.size(); i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d. %s - %.1fs", (int)i + 1, recs[i].playerName.c_str(), recs[i].bestTime); // Форматируем строку рекорда
                    DrawTextGUI(renderer, 200, currentY, buf, 2.5f); // Выводим строчку рекорда на экран
                    currentY += 60.0f; // Смещаем позицию вывода следующей строки ниже на 60 пикселей
                }
            }
            for (size_t i = 0; i < recordButtons.size(); i++) recordButtons[i]->draw(renderer); // Рисуем кнопки возврата и очистки рекордов
        }
        else if (state == 9) { // Отрисовка меню настроек звука и музыки
            DrawTextGUI(renderer, 300, 50, "SETTINGS", 4.0f, { 255, 215, 0, 255 });

            DrawTextGUI(renderer, 320, 200, "MUSIC VOLUME", 2.0f); // Текст громкости музыки
            settingsButtons[0]->draw(renderer);                      // Кнопка минус
            DrawTextGUI(renderer, 370, 250, to_string(audio->getMusicVolume()) + "%", 3.0f); // Вывод текущих процентов музыки
            settingsButtons[1]->draw(renderer);                      // Кнопка плюс

            DrawTextGUI(renderer, 340, 310, "SFX VOLUME", 2.0f);   // Текст громкости эффектов
            settingsButtons[2]->draw(renderer);                      // Кнопка минус
            DrawTextGUI(renderer, 370, 350, to_string(audio->getSFXVolume()) + "%", 3.0f); // Вывод текущих процентов эффектов
            settingsButtons[3]->draw(renderer);                      // Кнопка плюс

            settingsButtons[4]->draw(renderer); // Кнопка BACK (назад в меню)
        }
        else if (state == 6) { // Отрисовка экрана интерактивного набора имени игрока
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "YOUR TIME: %.1f s", timeElapsed); // Выводим время, за которое игрок прошел уровень
            DrawTextGUI(renderer, 200, 150, timeBuf, 3.0f, { 0, 255, 0, 255 });
            DrawTextGUI(renderer, 250, 250, "ENTER YOUR NAME:", 2.0f);

            SDL_FRect inputRect = { 250, 300, 300, 50 }; // Создаем прямоугольную рамку для поля ввода имени
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255); // Задаем черный цвет фона текстового поля
            SDL_RenderFillRect(renderer, &inputRect);           // Заливаем поле ввода черным
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Задаем белый цвет контура поля
            SDL_RenderRect(renderer, &inputRect);                 // Рисуем границы рамки поля

            string displayStr = inputPlayerName;
            if (SDL_GetTicks() % 1000 < 500) displayStr += "_"; // Раз в полсекунды дорисовываем мигающий курсор нижнего подчеркивания

            DrawTextGUI(renderer, 260, 310, displayStr, 2.5f, { 255, 255, 0, 255 }); // Отрисовываем сам набираемый текст желтым цветом
            DrawTextGUI(renderer, 250, 400, "Press ENTER to save", 1.5f, { 0, 255, 0, 255 });
            DrawTextGUI(renderer, 250, 440, "Press ESC to skip", 1.5f, { 255, 100, 100, 255 });
        }
        else if (state == 2 || state == 3 || state == 4 || state == 5) { // Отрисовка миров игровых уровней
            SDL_Texture* bg = textures->getBg(); // Получаем картинку фонового изображения
            if (bg) {
                SDL_FRect bgRect = { 0, 0, 800, 600 };
                SDL_RenderTexture(renderer, bg, NULL, &bgRect); // Выводим красивый задний фон на весь экран
            }
            else {
                SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Если файла фона нет — заливаем экран стандартным голубым небом
                SDL_RenderFillRect(renderer, NULL);
            }

            // Отрисовываем все статические блоки карты (стены, шипы, кольца) с учетом движения камеры
            for (size_t i = 0; i < staticObjects.size(); i++) {
                staticObjects[i]->draw(renderer, cameraX, cameraY);
            }
            if (player != NULL) player->draw(renderer, cameraX, cameraY); // Отрисовываем круглый шарик игрока

            // Расчет вывода таймера оставшегося времени на прохождение
            float timeLeft = timeLimit - timeElapsed;
            if (timeLeft < 0) timeLeft = 0; // Исключаем появление отрицательного времени

            SDL_Color timerColor = { 0, 0, 0, 255 }; // По умолчанию цвет таймера черный
            if (timeLeft <= 10.0f) timerColor = { 255, 0, 0, 255 }; // Если осталось меньше 10 секунд — перекрашиваем таймер в агрессивный красный цвет

            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "TIME LEFT: %.1f", timeLeft); // Форматируем текст оставшегося времени

            // Выводим классический игровой интерфейс (HUD) в левом верхнем углу
            DrawTextGUI(renderer, 10, 10, timeBuf, 2.0f, timerColor); // Рисуем таймер
            DrawTextGUI(renderer, 10, 40, "LIVES: " + to_string(lives), 2.0f, { 255, 0, 0, 255 }); // Рисуем количество жизней
            DrawTextGUI(renderer, 10, 70, "RINGS: " + to_string(collectedRings) + "/" + to_string(totalRings), 2.0f, { 255, 255, 0, 255 }); // Счёт колец

            if (state == 3) { // Отрисовка темного полупрозрачного оверлея для экрана Паузы
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); // Задаем черный цвет с высокой прозрачностью (альфа = 180)
                SDL_FRect overlay = { 0, 0, 800, 600 };
                SDL_RenderFillRect(renderer, &overlay);           // Покрываем весь экран полупрозрачной вуалью

                DrawTextGUI(renderer, 320, 150, "PAUSE", 4.0f); // Заголовок паузы
                for (size_t i = 0; i < pauseButtons.size(); i++) pauseButtons[i]->draw(renderer); // Рисуем кнопки управления паузой
            }
            else if (state == 4) { // Отрисовка зеленого полупрозрачного оверлея для экрана Победы
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); // Зеленый прозрачный цвет
                SDL_FRect overlay = { 0, 0, 800, 600 };
                SDL_RenderFillRect(renderer, &overlay);

                DrawTextGUI(renderer, 200, 250, "LEVEL COMPLETE!", 4.0f);
                DrawTextGUI(renderer, 250, 350, "Press ENTER to continue", 2.0f);
            }
            else if (state == 5) { // Отрисовка красного полупрозрачного оверлея для экрана Поражения (Game Over)
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); // Красный прозрачный цвет
                SDL_FRect overlay = { 0, 0, 800, 600 };
                SDL_RenderFillRect(renderer, &overlay);

                DrawTextGUI(renderer, 250, 250, "GAME OVER", 4.0f);
                DrawTextGUI(renderer, 250, 350, "Press ENTER to menu", 2.0f);
            }
        }

        SDL_RenderPresent(renderer); // Проявляем скрытый системный буфер кадра, плавно выводя готовую картинку на экран монитора
    }

    // Главный игровой цикл: запускает непрерывное сердцебиение игрового движка
    void start() {
        Uint64 lastTime = SDL_GetTicks(); // Засекаем точное системное время старта в миллисекундах
        while (isRunning) {               // Цикл крутится непрерывно до тех пор, пока переменная флаг равна true
            Uint64 current = SDL_GetTicks(); // Считываем текущее системное время в данном кадре
            float deltaTime = (current - lastTime) / 1000.0f; // Вычисляем точную дельту времени (сколько долей секунды процессор собирал прошлый кадр)
            lastTime = current; // Перезаписываем время для следующего шага цикла

            // Предохранитель от лагов компьютера: ограничиваем дельту 0.1 секундами.
            // Это нужно, чтобы при внезапном зависании ПК физика игры не сломалась и шарик не пролетел сквозь стены.
            if (deltaTime > 0.1f) deltaTime = 0.1f;

            handleInput();     // Опрашиваем устройства ввода (мышь, клавиатура, закрытие окна)
            update(deltaTime); // Передаем дельту времени в физический движок для симуляции перемещений
            draw();            // Отрисовываем обновленные кадры графики на монитор
        }
    }
};

// Главная точка входа в программу для операционной системы
int main(int argc, char* argv[]) {
    GameEngine game; // Выделяем память и запускаем конструктор нашего игрового движка GameEngine
    game.start();    // Активируем бесконечный игровой цикл работы приложения
    return 0;        // Возвращаем системе статус успешного завершения программы при закрытии окна игры
}