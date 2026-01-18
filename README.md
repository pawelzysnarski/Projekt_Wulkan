# Projekt Wulkan (Volcano Simulator)

Projekt Wulkan to aplikacja C++/OpenGL symulująca erupcję wulkanu na podstawie danych wysokościowych (DEM) i profilu pogodowego. Program renderuje teren z pliku GeoTIFF oraz generuje cząstki wyrzutu. Ruch cząstek jest aktualizowany z uwzględnieniem wiatru, turbulencji, gęstości powietrza oraz grawitacji.

## Najważniejsze funkcje
- Wczytywanie mapy wysokości (DEM) oraz tekstury z plików GeoTIFF.
- Symulacja chmury erupcyjnej z różnymi typami materiału (popiół, lapille, bomby wulkaniczne).
- Dynamiczne warunki pogodowe na podstawie danych CSV lub profilu domyślnego.
- Podgląd 3D w czasie rzeczywistym z możliwością sterowania kamerą.

## Struktura repozytorium
- `Volcano_Sim/Volcano_Sim.sln` – rozwiązanie Visual Studio.
- `Volcano_Sim/Volcano_Sim/main.cpp` – punkt wejścia aplikacji i ustawienia ścieżek do danych.
- `Volcano_Sim/src` oraz `Volcano_Sim/include` – logika symulacji (DEM, pogoda, fizyka cząstek).
- `Volcano_Sim/geo` – przykładowe dane DEM i profile pogodowe.

## Wymagania
- Windows + Visual Studio 2022 (toolset v143) z obsługą C++17.
- OpenGL (systemowe biblioteki Windows).
- GDAL (nagłówki i biblioteki w ścieżkach kompilatora/linkera).
- NuGet: `glfw` 3.4.0 oraz `glm` 1.0.2 (przywracane automatycznie przez Visual Studio).

## Instalacja i budowanie
1. Otwórz `Volcano_Sim/Volcano_Sim.sln` w Visual Studio.
2. Uruchom przywracanie pakietów NuGet (glfw, glm).
3. Upewnij się, że GDAL jest zainstalowany i skonfigurowany w ustawieniach projektu (Include/Library Directories).
4. Zbuduj projekt `Volcano_Sim` w konfiguracji Debug lub Release (x64).

## Uruchomienie
1. Ustaw katalog roboczy na `Volcano_Sim/Volcano_Sim`, aby ścieżki `../geo/...` wskazywały poprawne dane.
2. Uruchom aplikację z Visual Studio lub z pliku wynikowego (np. `x64/Debug/Volcano_Sim.exe`).

## Konfiguracja danych wejściowych
W pliku `Volcano_Sim/Volcano_Sim/main.cpp` możesz zmienić:
- `demPath` – ścieżkę do pliku DEM (GeoTIFF).
- `weatherCSV` – ścieżkę do profilu pogodowego CSV.

Domyślne dane znajdują się w katalogu `Volcano_Sim/geo`.

## Sterowanie
- `ESC` – zakończenie symulacji.
- `Left Arrow` / `Right Arrow` (← / →) – obrót kamery wokół krateru.
- `Up Arrow` / `Down Arrow` (↑ / ↓) – wysokość kamery nad kraterem.
- `Numpad +` / `Numpad -` – przybliżenie/oddalenie (promień orbity).
- `Z` / `X` – zwiększanie/zmniejszanie skali wysokości terenu.

## Testy
Repozytorium nie zawiera zautomatyzowanych testów ani skryptów lint/build poza konfiguracją Visual Studio.
