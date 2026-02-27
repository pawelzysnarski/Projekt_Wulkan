#pragma once

enum class MaterialType {
    H2O,
    CO2,
    SO2,
    HCl,
    HF,
    CO,
    VolcanicAsh,
    Lapilli,
    VolcanicBomb,
    VolcanicGlass
};
static const char* MaterialTypeS[] = {
    "H2O",
    "CO2",
    "SO2",
    "HCl",
    "HF",
    "CO",
    "Volcanic Ash",
    "Lapilli",
    "Volcanic Bomb",
    "Volcanic Glass"
};
static const double MaterialDensity[] = {
    0.6,
    1.98,
    2.62,
    1.49,
    0.98,
    1.25,
    2400.0,
    2600.0,
    2700.0,
    2500.0
};
static const int ParticleColor[10][3] = {
    {255,0,0},
    {0,255,0},
    {0,0,255},
    {255,255,0},
    {255,0,255},
    {0,255,255},
    {128,128,128},
    {139,69,19},
    {105,105,105},
    {169,169,169}
};

class Materia {
public:
    double position_x = 0.0;
    double position_y = 0.0;
    double position_z = 0.0;
    double vel_x = 0.0;
    double vel_y = 0.0;
    double vel_z = 0.0;
    double density = 2500.0;
    double diameter = 200e-6;
    MaterialType type = MaterialType::VolcanicAsh;

    Materia() = default;
    Materia(double x, double y, double z, double vx, double vy, double vz, double dens, double diam, MaterialType t);

    double radius() const;
    double volume() const;
    double mass() const;

    void changePosition(double dx, double dy, double dz);
    void setVelocity(double vx, double vy, double vz);
};