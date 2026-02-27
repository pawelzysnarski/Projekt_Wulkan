#ifndef DEM_LOADER_H
#define DEM_LOADER_H

#include <string>
#include <vector>
#include <utility>

class DEMLoader {
public:
    struct Color {
        unsigned char r, g, b;
    };

    DEMLoader();
    ~DEMLoader();

    // G³ówne metody wczytuj¹ce
    bool loadHeight(const std::string& path);   // wczytuje tylko wysokoœæ (1 pasmo)
    bool loadColors(const std::string& path);   // wczytuje tylko kolory (3+ pasm)
    bool load(const std::string& path);          // kompatybilnoœæ wsteczna

    // Informacje
    int width() const;
    int height() const;
    const double* geoTransform() const;
    bool isLoaded() const;
    bool hasColors() const;

    // Pobieranie danych
    double getGroundZ(double geoX, double geoY) const;
    bool getGroundColor(double geoX, double geoY,
        unsigned char& r, unsigned char& g, unsigned char& b) const;
    const Color* getColor(int x, int y) const;
    const Color* getColorAtPixel(double px, double py) const;

    // Dodatkowe
    std::pair<double, double> getHeightRange() const;
    double pixelSizeX() const;
    double pixelSizeY() const;
    bool geoToPixel(double gx, double gy, double& px, double& py) const;

private:
    double bilinearInterp(double px, double py) const;

    int nx_, ny_;
    std::vector<float> data_;        // dane wysokoœciowe
    std::vector<Color> colors_;      // dane kolorów
    double gt_[6];
    bool loaded_;
    bool hasNoData_;
    double noDataVal_;
    bool hasColors_;
};

#endif // DEM_LOADER_H