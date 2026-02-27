#include "../include/dem_loader.h"
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>
#include <cmath>
#include <limits>
#include <iostream>
#include <algorithm>

using namespace std;

DEMLoader::DEMLoader()
    : nx_(0), ny_(0), loaded_(false), hasNoData_(false), noDataVal_(0.0),
    hasColors_(false) {
    for (int i = 0; i < 6; i++) gt_[i] = 0.0;
}

DEMLoader::~DEMLoader() {}

bool DEMLoader::loadHeight(const string& path) {
    // Inicjalizacja biblioteki GDAL
    GDALAllRegister();

    // Otwarcie pliku DEM
    GDALDataset* ds = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
    if (!ds) {
        cerr << "DEMLoader: nie mozna otworzyc pliku wysokosci: " << path << "\n";
        return false;
    }

    // Sprawdzenie liczby pasm
    int bandCount = ds->GetRasterCount();
    if (bandCount < 1) {
        cerr << "DEMLoader: brak pasm w pliku wysokosci\n";
        GDALClose(ds);
        return false;
    }

    // Pobranie transformacji georeferencyjnej
    if (ds->GetGeoTransform(gt_) != CE_None) {
        for (int i = 0; i < 6; i++) gt_[i] = 0.0;
    }

    // Pobranie pierwszego pasma z danymi wysokoœciowymi
    GDALRasterBand* heightBand = ds->GetRasterBand(1);
    nx_ = heightBand->GetXSize();
    ny_ = heightBand->GetYSize();

    // Sprawdzenie wartoœci NoData
    int hasNo = 0;
    double nd = heightBand->GetNoDataValue(&hasNo);
    if (hasNo) {
        hasNoData_ = true;
        noDataVal_ = nd;
    }
    else {
        hasNoData_ = false;
        noDataVal_ = numeric_limits<double>::quiet_NaN();
    }

    // Alokacja pamiêci i odczyt danych wysokoœciowych
    data_.resize((size_t)nx_ * (size_t)ny_);
    CPLErr err = heightBand->RasterIO(GF_Read, 0, 0, nx_, ny_,
        data_.data(), nx_, ny_, GDT_Float32, 0, 0);

    if (err != CE_None) {
        cerr << "DEMLoader: blad RasterIO przy odczycie wysokosci\n";
        GDALClose(ds);
        return false;
    }

    GDALClose(ds);
    loaded_ = true;
    cout << "DEMLoader: wczytano wysokosci z " << path << "\n";
    cout << "  Wymiary: " << nx_ << " x " << ny_ << "\n";
    return true;
}

bool DEMLoader::loadColors(const string& path) {
    // Inicjalizacja biblioteki GDAL
    GDALAllRegister();

    // Otwarcie pliku z kolorami
    GDALDataset* ds = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
    if (!ds) {
        cerr << "DEMLoader: nie mozna otworzyc pliku kolorow: " << path << "\n";
        return false;
    }

    // Sprawdzenie liczby pasm
    int bandCount = ds->GetRasterCount();
    if (bandCount < 3) {
        cerr << "DEMLoader: plik kolorow ma za malo pasm (potrzeba minimum 3)\n";
        GDALClose(ds);
        return false;
    }

    // Sprawdzenie czy wymiary zgadzaj¹ siê z wczytanymi wczeœniej wysokoœciami
    GDALRasterBand* testBand = ds->GetRasterBand(1);
    int colorNx = testBand->GetXSize();
    int colorNy = testBand->GetYSize();

    if (loaded_ && (colorNx != nx_ || colorNy != ny_)) {
        cerr << "DEMLoader: wymiary pliku kolorow (" << colorNx << "x" << colorNy
            << ") nie zgadzaja sie z wysokosciami (" << nx_ << "x" << ny_ << ")\n";
        GDALClose(ds);
        return false;
    }

    // Jeœli wysokoœci nie by³y jeszcze wczytane, zapisz wymiary z pliku kolorów
    if (!loaded_) {
        nx_ = colorNx;
        ny_ = colorNy;
    }

    // Pobranie transformacji (jeœli nie by³a wczeœniej pobrana)
    if (!loaded_ || gt_[0] == 0.0 && gt_[1] == 0.0) {
        ds->GetGeoTransform(gt_);
    }

    // Pasma R, G, B (zak³adamy, ¿e s¹ w kolejnoœci 1,2,3)
    GDALRasterBand* rBand = ds->GetRasterBand(1);
    GDALRasterBand* gBand = ds->GetRasterBand(2);
    GDALRasterBand* bBand = ds->GetRasterBand(3);

    cout << "DEMLoader: typ pasma R = " << GDALGetDataTypeName(rBand->GetRasterDataType()) << "\n";
    cout << "DEMLoader: typ pasma G = " << GDALGetDataTypeName(gBand->GetRasterDataType()) << "\n";
    cout << "DEMLoader: typ pasma B = " << GDALGetDataTypeName(bBand->GetRasterDataType()) << "\n";

    // Alokacja pamiêci dla kolorów
    colors_.resize((size_t)nx_ * (size_t)ny_);

    // Bufory dla ka¿dego kana³u
    vector<unsigned char> rBuf((size_t)nx_ * (size_t)ny_);
    vector<unsigned char> gBuf((size_t)nx_ * (size_t)ny_);
    vector<unsigned char> bBuf((size_t)nx_ * (size_t)ny_);

    // Odczyt kana³ów
    CPLErr errR = rBand->RasterIO(GF_Read, 0, 0, nx_, ny_,
        rBuf.data(), nx_, ny_, GDT_Byte, 0, 0);
    CPLErr errG = gBand->RasterIO(GF_Read, 0, 0, nx_, ny_,
        gBuf.data(), nx_, ny_, GDT_Byte, 0, 0);
    CPLErr errB = bBand->RasterIO(GF_Read, 0, 0, nx_, ny_,
        bBuf.data(), nx_, ny_, GDT_Byte, 0, 0);

    if (errR == CE_None && errG == CE_None && errB == CE_None) {
        for (size_t i = 0; i < colors_.size(); i++) {
            colors_[i] = { rBuf[i], gBuf[i], bBuf[i] };
        }
        hasColors_ = true;
        cout << "DEMLoader: wczytano kolory z " << path << "\n";
        cout << "  Probka: R=" << (int)rBuf[0] << " G=" << (int)gBuf[0] << " B=" << (int)bBuf[0] << "\n";
    }
    else {
        cerr << "DEMLoader: blad przy odczycie kolorow\n";
        GDALClose(ds);
        return false;
    }

    GDALClose(ds);
    loaded_ = true;
    return true;
}

bool DEMLoader::load(const string& path) {
    // Dla kompatybilnoœci wstecznej - próbuje zgadn¹æ co to za plik
    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
    if (!ds) return false;

    int bandCount = ds->GetRasterCount();
    GDALClose(ds);

    if (bandCount == 1) {
        // Pojedyncze pasmo - prawdopodobnie wysokoœæ
        return loadHeight(path);
    }
    else if (bandCount >= 3) {
        // Wielopasmowy - prawdopodobnie kolory
        return loadColors(path);
    }

    return false;
}

int DEMLoader::width() const {
    return nx_;
}

int DEMLoader::height() const {
    return ny_;
}

const double* DEMLoader::geoTransform() const {
    return gt_;
}

bool DEMLoader::hasColors() const {
    return hasColors_;
}

const DEMLoader::Color* DEMLoader::getColor(int x, int y) const {
    if (!hasColors_ || x < 0 || x >= nx_ || y < 0 || y >= ny_) return nullptr;
    return &colors_[y * nx_ + x];
}

const DEMLoader::Color* DEMLoader::getColorAtPixel(double px, double py) const {
    int x = (int)round(px);
    int y = (int)round(py);
    return getColor(x, y);
}

bool DEMLoader::geoToPixel(double gx, double gy, double& px, double& py) const {
    double a = gt_[1], b = gt_[2], c = gt_[4], d = gt_[5];
    double det = a * d - b * c;
    if (abs(det) < 1e-12) return false;

    double dx = gx - gt_[0];
    double dy = gy - gt_[3];

    px = (d * dx - b * dy) / det;
    py = (-c * dx + a * dy) / det;
    return true;
}

double DEMLoader::bilinearInterp(double px, double py) const {
    if (!loaded_) return numeric_limits<double>::quiet_NaN();

    int x0 = (int)floor(px);
    int y0 = (int)floor(py);
    double fx = px - x0;
    double fy = py - y0;

    auto sample = [&](int x, int y) -> double {
        if (x < 0 || x >= nx_ || y < 0 || y >= ny_)
            return numeric_limits<double>::quiet_NaN();
        float v = data_[y * nx_ + x];
        if (hasNoData_ && v == (float)noDataVal_)
            return numeric_limits<double>::quiet_NaN();
        return (double)v;
        };

    double v00 = sample(x0, y0);
    double v10 = sample(x0 + 1, y0);
    double v01 = sample(x0, y0 + 1);
    double v11 = sample(x0 + 1, y0 + 1);

    if (isnan(v00) || isnan(v10) || isnan(v01) || isnan(v11)) {
        int xn = (int)round(px);
        int yn = (int)round(py);
        double nn = sample(xn, yn);
        return nn;
    }

    double v0 = v00 * (1.0 - fx) + v10 * fx;
    double v1 = v01 * (1.0 - fx) + v11 * fx;
    return v0 * (1.0 - fy) + v1 * fy;
}

double DEMLoader::getGroundZ(double geoX, double geoY) const {
    if (!loaded_ || data_.empty()) return numeric_limits<double>::quiet_NaN();

    double px, py;
    if (!geoToPixel(geoX, geoY, px, py))
        return numeric_limits<double>::quiet_NaN();

    if (px < -0.5 || py < -0.5 || px > nx_ - 0.5 || py > ny_ - 0.5)
        return numeric_limits<double>::quiet_NaN();

    return bilinearInterp(px, py);
}

bool DEMLoader::getGroundColor(double geoX, double geoY, unsigned char& r,
    unsigned char& g, unsigned char& b) const {
    if (!loaded_ || !hasColors_) return false;

    double px, py;
    if (!geoToPixel(geoX, geoY, px, py)) return false;

    const Color* col = getColorAtPixel(px, py);
    if (!col) return false;

    r = col->r;
    g = col->g;
    b = col->b;
    return true;
}

pair<double, double> DEMLoader::getHeightRange() const {
    if (!loaded_ || data_.empty())
        return { numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN() };

    double minH = numeric_limits<double>::max();
    double maxH = -numeric_limits<double>::max();

    for (float val : data_) {
        if (hasNoData_ && val == (float)noDataVal_) continue;
        minH = min(minH, (double)val);
        maxH = max(maxH, (double)val);
    }

    return { minH, maxH };
}

double DEMLoader::pixelSizeX() const {
    return gt_[1];
}

double DEMLoader::pixelSizeY() const {
    return abs(gt_[5]);
}

bool DEMLoader::isLoaded() const {
    return loaded_;
}