#pragma once
#include "materia.h"

namespace physics {
    constexpr double g = 9.80665;
    constexpr double airViscosity = 1.8e-5;

    double maxRange(double v0, double angle_rad);
    double flyingTime(double v0, double angle_rad);
    double sphereGravity(const Materia& m);
    double sphereBuoyancyForce(const Materia& m, double airDensity = 1.225);
    double stokesDragMagnitude(double v_rel, const Materia& m);
    double quadraticDragMagnitude(double v_rel, const Materia& m, double airDensity = 1.225, double Cd = 0.47);
    double dragMagnitude(double v_rel, const Materia& m, double airDensity = 1.225);
    void dragForceVector(double rel_vx, double rel_vy, double rel_vz, const Materia& m, double airDensity,
        double& out_fx, double& out_fy, double& out_fz);
}