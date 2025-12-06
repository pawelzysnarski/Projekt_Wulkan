#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>

struct WeatherSample {
    double altitude;      // [m]
    double wind_u;        // sk³adowa U wiatru (E-W) [m/s]
    double wind_v;        // sk³adowa V wiatru (N-S) [m/s]
    double temperature;   // [°C]
    double pressure;      // [Pa]
    double humidity;      // [%]
};

class WeatherDataLoader {
public:
    static std::vector<WeatherSample> LoadCSV(const std::string& file);
};

class Weather {
private:
    std::vector<WeatherSample> weatherProfile;
    double currentAltitude;
    WeatherSample interpolatedWeather;

    WeatherSample interpolateForAltitude(double altitude) const;

public:
    double altitude;
    double wind_u;
    double wind_v;
    double temperature;
    double pressure;
    double humidity;
    double turbulence;

    Weather();
    Weather(double alt, double u, double v, double temp, double pres, double hum, double turb);

    bool loadWeatherProfile(const std::string& csvFile);
    void updateForAltitude(double alt);

    double GenerateTurbulence() const;
    void GetWindVector(double& out_x, double& out_y) const;
    double CalculateAirDensity() const;

    void AffectParticle(
        double& vel_x,
        double& vel_y,
        double& vel_z,
        double particle_density
    ) const;

    void getWeatherAtAltitude(double alt, double& out_wind_u, double& out_wind_v,
        double& out_temp, double& out_pres, double& out_hum) const;
};