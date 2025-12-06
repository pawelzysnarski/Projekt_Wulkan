#include "../include/dem_loader.h"
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <cmath>
#include <limits>
#include <iostream>

using namespace std;

DEMLoader::DEMLoader() {
    for (int i = 0; i < 6; i++) gt_[i] = 0.0;
}

DEMLoader::~DEMLoader() {}

bool DEMLoader::load(const string& path) {
    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
    if (!ds) {
        cerr << "DEMLoader: nie mozna otworzyc pliku: " << path << "\n";
        return false;
    }
    if (ds->GetRasterCount() < 1) {
        cerr << "DEMLoader: brak pasma rastrowego\n";
        GDALClose(ds);
        return false;
    }
    ds->GetGeoTransform(gt_);
    GDALRasterBand* band = ds->GetRasterBand(1);
    nx_ = band->GetXSize();
    ny_ = band->GetYSize();

    int hasNo = 0;
    double nd = band->GetNoDataValue(&hasNo);
    if (hasNo) {
        hasNoData_ = true;
        noDataVal_ = nd;
    }
    else {
        hasNoData_ = false;
        noDataVal_ = numeric_limits<double>::quiet_NaN();
    }

    data_.resize((size_t)nx_ * (size_t)ny_);
    CPLErr err = band->RasterIO(GF_Read, 0, 0, nx_, ny_,
        data_.data(), nx_, ny_, GDT_Float32,
        0, 0);
    if (err != CE_None) {
        cerr << "DEMLoader: blad RasterIO\n";
        GDALClose(ds);
        return false;
    }
    GDALClose(ds);
    loaded_ = true;
    return true;
}

int DEMLoader::width() const { return nx_; }
int DEMLoader::height() const { return ny_; }
const double* DEMLoader::geoTransform() const { return gt_; }

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
        if (x < 0 || x >= nx_ || y < 0 || y >= ny_) return numeric_limits<double>::quiet_NaN();
        float v = data_[y * nx_ + x];
        if (hasNoData_ && v == (float)noDataVal_) return numeric_limits<double>::quiet_NaN();
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
    if (!loaded_) return numeric_limits<double>::quiet_NaN();
    double px, py;
    if (!geoToPixel(geoX, geoY, px, py)) return numeric_limits<double>::quiet_NaN();
    if (px < -0.5 || py < -0.5 || px > nx_ - 0.5 || py > ny_ - 0.5) return numeric_limits<double>::quiet_NaN();
    double val = bilinearInterp(px, py);
    return val;
}