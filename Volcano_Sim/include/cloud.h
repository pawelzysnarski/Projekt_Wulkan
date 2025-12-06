#pragma once

#include <vector>
#include "materia.h"
#include "dem_loader.h"
#include "weather.h"  

class Cloud {
public:
    std::vector<Materia> particles;
    Weather* weatherSystem;  

    Cloud();
    Cloud(Weather* weather);  
    ~Cloud();

    void setWeatherSystem(Weather* weather);  

    void generateParticles(size_t N,
        double crater_x, double crater_y, double crater_z,
        double crater_radius,
        double min_speed, double max_speed,
        double min_diameter, double max_diameter,
        int choice = 0);

    void update(double dt, double airDensity,
        double wind_u, double wind_v,
        std::vector<Materia>& particlesOnEart,
        const DEMLoader& dem,
        double wind_w = 0.0, double turbulence = 0.0);

    void clear() { particles.clear(); }
};