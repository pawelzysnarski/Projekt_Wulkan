#include "../include/materia.h"
#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

Materia::Materia(double x, double y, double z, double vx, double vy, double vz, double dens, double diam, MaterialType t)
    : position_x(x), position_y(y), position_z(z), vel_x(vx), vel_y(vy), vel_z(vz),
    density(dens), diameter(diam), type(t) {
}

double Materia::radius() const {
    return 0.5 * diameter;
}

double Materia::volume() const {
    double r = radius();
    return (4.0 / 3.0) * M_PI * r * r * r;
}

double Materia::mass() const {
    return density * volume();
}

void Materia::changePosition(double dx, double dy, double dz) {
    position_x += dx;
    position_y += dy;
    position_z += dz;
}

void Materia::setVelocity(double vx, double vy, double vz) {
    vel_x = vx;
    vel_y = vy;
    vel_z = vz;
}