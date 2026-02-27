#include "../include/formulas.h"
#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

namespace physics {

    double maxRange(double v0, double angle_rad) {
        return (v0 * v0) * sin(2.0 * angle_rad) / g;
    }

    double flyingTime(double v0, double angle_rad) {
        return 2.0 * v0 * sin(angle_rad) / g;
    }

    double sphereGravity(const Materia& m) {
        return m.mass() * g;
    }

    double sphereBuoyancyForce(const Materia& m, double airDensity) {
        return airDensity * m.volume() * g;
    }

    double stokesDragMagnitude(double v_rel, const Materia& m) {
        double r = m.radius();
        return 6.0 * M_PI * airViscosity * r * v_rel;
    }

    double quadraticDragMagnitude(double v_rel, const Materia& m, double airDensity, double Cd) {
        double r = m.radius();
        double area = M_PI * r * r;
        return 0.5 * airDensity * v_rel * v_rel * Cd * area;
    }

    double dragMagnitude(double v_rel, const Materia& m, double airDensity) {
        double d = m.diameter;
        double rho_air = airDensity;
        double Re = (rho_air * fabs(v_rel) * d) / airViscosity + 1e-12;
        if (Re < 1.0) {
            return stokesDragMagnitude(v_rel, m);
        }
        else {
            return quadraticDragMagnitude(v_rel, m, airDensity);
        }
    }

    void dragForceVector(double rel_vx, double rel_vy, double rel_vz, const Materia& m, double airDensity,
        double& out_fx, double& out_fy, double& out_fz) {
        double vrel = sqrt(rel_vx * rel_vx + rel_vy * rel_vy + rel_vz * rel_vz);
        if (vrel < 1e-12) {
            out_fx = out_fy = out_fz = 0.0;
            return;
        }
        double mag = dragMagnitude(vrel, m, airDensity);
        out_fx = -mag * (rel_vx / vrel);
        out_fy = -mag * (rel_vy / vrel);
        out_fz = -mag * (rel_vz / vrel);
    }
}