#pragma once
#include <string>
#include <vector>

using namespace std;

class DEMLoader {
public:
    DEMLoader();
    ~DEMLoader();
    bool load(const string& path);
    double getGroundZ(double geoX, double geoY) const;
    int width() const;
    int height() const;
    const double* geoTransform() const;

private:
    bool geoToPixel(double gx, double gy, double& px, double& py) const;
    double bilinearInterp(double px, double py) const;

private:
    vector<float> data_;
    int nx_{ 0 }, ny_{ 0 };
    double gt_[6];
    double noDataVal_;
    bool hasNoData_{ false };
    bool loaded_{ false };
};