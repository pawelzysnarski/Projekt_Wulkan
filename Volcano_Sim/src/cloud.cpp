#define _USE_MATH_DEFINES
#include <cmath>
#include "../include/cloud.h"
#include "../include/formulas.h"
#include <random>
#include <algorithm>

using namespace std;

Cloud::Cloud() : weatherSystem(nullptr) {}

Cloud::Cloud(Weather* weather) : weatherSystem(weather) {}

Cloud::~Cloud() {}

void Cloud::setWeatherSystem(Weather* weather) {
    weatherSystem = weather;
}

static mt19937& rng() {
    static thread_local mt19937 gen((random_device())());
    return gen;
}

void Cloud::generateParticles(size_t N,
    double crater_x, double crater_y, double crater_z,
    double crater_radius,
    double min_speed, double max_speed,
    double min_diameter, double max_diameter,
    int choice)
{
    uniform_real_distribution<double> ang(0.0, 2.0 * M_PI);
    uniform_real_distribution<double> ang2(0.0, 0.1 * M_PI);
    uniform_real_distribution<double> r01(0.0, 1.0);
    uniform_real_distribution<double> spd(min_speed, max_speed);
    uniform_real_distribution<double> diam(min_diameter, max_diameter);

    this->particles.reserve(this->particles.size() + N);

    for (size_t i = 0; i < N; ++i) {
        double u = r01(rng());
        double r = crater_radius * sqrt(u);
        double theta = ang(rng());

        double px = crater_x + r * cos(theta);
        double py = crater_y + r * sin(theta);
        double pz = crater_z;

        double speed = spd(rng());
        double phi = ang2(rng());

        double vx = speed * sin(phi) * cos(theta);
        double vy = speed * sin(phi) * sin(theta);
        double vz = speed * cos(phi);

        double d = diam(rng());

        MaterialType type;
        switch (choice) {
        case 0: type = MaterialType::H2O; break;
        case 1: type = MaterialType::CO2; break;
        case 2: type = MaterialType::SO2; break;
        case 3: type = MaterialType::HCl; break;
        case 4: type = MaterialType::HF; break;
        case 5: type = MaterialType::CO; break;
        case 6: type = MaterialType::VolcanicAsh; break;
        case 7: type = MaterialType::Lapilli; break;
        case 8: type = MaterialType::VolcanicBomb; break;
        case 9: type = MaterialType::VolcanicGlass; break;
        }

        Materia m(px, py, pz, vx, vy, vz,
            MaterialDensity[static_cast<int>(type)], d, type);
        particles.push_back(m);
    }
}
void Cloud::update(double dt, double airDensity,
    double wind_u, double wind_v,
    vector<Materia>& vec, vector<Materia>& vec2,
    const DEMLoader& dem, double wind_w,
    double turbulence) {
    uniform_real_distribution<double> turb(-1.0, 1.0);

    for (auto& p : particles) {
        double wu = wind_u;
        double wv = wind_v;
        double ww = wind_w;

        if (weatherSystem != nullptr) {
            double temp, pres, hum;
            weatherSystem->getWeatherAtAltitude(p.position_z, wu, wv, temp, pres, hum);
            airDensity = weatherSystem->CalculateAirDensity();
            wu += weatherSystem->GenerateTurbulence();
            wv += weatherSystem->GenerateTurbulence();
            ww += weatherSystem->GenerateTurbulence();
        }
        else {
            wu += (turb(rng()) * turbulence);
            wv += (turb(rng()) * turbulence);
            ww += (turb(rng()) * turbulence);
        }

        double rel_vx = p.vel_x - wu;
        double rel_vy = p.vel_y - wv;
        double rel_vz = p.vel_z - ww;

        double Fx = 0.0, Fy = 0.0, Fz = 0.0;
        physics::dragForceVector(rel_vx, rel_vy, rel_vz, p, airDensity, Fx, Fy, Fz);

        double Fg = physics::sphereGravity(p);
        double Fb = physics::sphereBuoyancyForce(p, airDensity);

        double total_fx = Fx;
        double total_fy = Fy;
        double total_fz = -Fg + Fb + Fz;

        double inv_m = 1.0 / (p.mass() + 1e-12);
        double ax = total_fx * inv_m;
        double ay = total_fy * inv_m;
        double az = total_fz * inv_m;

        p.vel_x += ax * dt;
        p.vel_y += ay * dt;
        p.vel_z += az * dt;

        p.position_x += p.vel_x * dt;
        p.position_y += p.vel_y * dt;
        p.position_z += p.vel_z * dt;
    }

    auto it = std::remove_if(this->particles.begin(), this->particles.end(),
        [&](const Materia& p) {
            if (p.position_z <= dem.getGroundZ(p.position_x, p.position_y)) {
                vec.push_back(p);
                return true;
            }
            if (p.position_z >= 100000) {
                vec2.push_back(p);
                return true;
            }
            return false;
        });
    this->particles.erase(it, this->particles.end());
}