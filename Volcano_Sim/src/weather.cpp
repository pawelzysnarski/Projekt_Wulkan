#include "../include/weather.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cpl_port.h>
using namespace std;

vector<WeatherSample> WeatherDataLoader::LoadCSV(const string& file) {
    vector<WeatherSample> samples;

    ifstream f(file);
    if (!f.is_open()) {
        cerr << "WeatherDataLoader: Nie mozna otworzyc pliku: " << file << "\n";
        return samples;
    }

    string line;

    if (!getline(f, line)) {
        cerr << "WeatherDataLoader: Plik pusty\n";
        return samples;
    }

    if (!getline(f, line)) {
        cerr << "WeatherDataLoader: Brak linii z naglowkami\n";
        return samples;
    }

    int lineNum = 2;
    int samplesLoaded = 0;

    while (getline(f, line)) {
        lineNum++;
        if (line.empty()) continue;

        stringstream ss(line);
        string val;
        WeatherSample s;

        try {
            getline(ss, val, ',');
            if (val.empty()) continue;

            getline(ss, val, ',');
            double wind_speed_kmh = stod(val);
            double wind_speed_ms = wind_speed_kmh * 1000.0 / 3600.0;

            getline(ss, val, ',');
            double wind_dir_deg = stod(val);

            double wind_dir_rad = wind_dir_deg * M_PI / 180.0;
            s.wind_u = wind_speed_ms * sin(wind_dir_rad);
            s.wind_v = wind_speed_ms * cos(wind_dir_rad);

            getline(ss, val, ',');
            getline(ss, val, ',');

            getline(ss, val, ',');
            s.temperature = stod(val);

            getline(ss, val, ',');
            s.humidity = stod(val);

            getline(ss, val, ',');
            s.pressure = stod(val) * 100.0;

            s.altitude = 1176.0;

            samples.push_back(s);
            samplesLoaded++;

        }
        catch (const exception& e) {
            cerr << "WeatherDataLoader: Blad w linii " << lineNum
                << ": " << e.what() << "\n";
            continue;
        }
    }

    if (samples.empty()) {
        for (int i = 0; i <= 20; i++) {
            double altitude = i * 500.0;
            WeatherSample ws;
            ws.altitude = altitude;
            ws.temperature = 15.0 - (altitude * 0.0065);
            ws.pressure = 101325.0 * exp(-altitude / 8500.0);
            ws.humidity = max(0.0, 50.0 - (altitude * 0.002));
            ws.wind_u = 2.0 + (altitude * 0.001);
            ws.wind_v = 1.0 + (altitude * 0.0005);
            samples.push_back(ws);
        }
    }
    else {
        sort(samples.begin(), samples.end(),
            [](const WeatherSample& a, const WeatherSample& b) {
                return a.altitude < b.altitude;
            });
    }

    cout << "WeatherDataLoader: Zaladowano " << samples.size() << " probek pogodowych\n";
    return samples;
}

WeatherSample Weather::interpolateForAltitude(double altitude) const {
    if (weatherProfile.empty()) {
        return WeatherSample{ altitude, wind_u, wind_v, temperature, pressure, humidity };
    }

    auto it_upper = lower_bound(weatherProfile.begin(), weatherProfile.end(), altitude,
        [](const WeatherSample& ws, double alt) { return ws.altitude < alt; });

    if (it_upper == weatherProfile.begin()) {
        return *it_upper;
    }
    if (it_upper == weatherProfile.end()) {
        return weatherProfile.back();
    }

    auto it_lower = prev(it_upper);
    double alt_lower = it_lower->altitude;
    double alt_upper = it_upper->altitude;

    if (alt_upper - alt_lower < 1e-6) {
        return *it_lower;
    }

    double t = (altitude - alt_lower) / (alt_upper - alt_lower);

    WeatherSample result;
    result.altitude = altitude;
    result.wind_u = it_lower->wind_u + t * (it_upper->wind_u - it_lower->wind_u);
    result.wind_v = it_lower->wind_v + t * (it_upper->wind_v - it_lower->wind_v);
    result.temperature = it_lower->temperature + t * (it_upper->temperature - it_lower->temperature);
    result.pressure = it_lower->pressure + t * (it_upper->pressure - it_lower->pressure);
    result.humidity = it_lower->humidity + t * (it_upper->humidity - it_lower->humidity);

    return result;
}

Weather::Weather() {
    altitude = 0;
    wind_u = 0;
    wind_v = 0;
    temperature = 15;
    pressure = 101300;
    humidity = 50;
    turbulence = 0.1;
    currentAltitude = 0;
    interpolatedWeather = WeatherSample{ 0, wind_u, wind_v, temperature, pressure, humidity };
}

Weather::Weather(double alt, double u, double v, double temp, double pres, double hum, double turb)
    : altitude(alt), wind_u(u), wind_v(v), temperature(temp),
    pressure(pres), humidity(hum), turbulence(turb),
    currentAltitude(alt) {
    interpolatedWeather = WeatherSample{ alt, u, v, temp, pres, hum };
}

bool Weather::loadWeatherProfile(const string& csvFile) {
    weatherProfile = WeatherDataLoader::LoadCSV(csvFile);
    if (weatherProfile.empty()) {
        cerr << "Weather: Nie udalo sie zaladowac profilu pogodowego\n";
        return false;
    }

    updateForAltitude(currentAltitude);
    cout << "Weather: Zaladowano profil pogodowy z " << weatherProfile.size() << " poziomami\n";
    return true;
}

void Weather::updateForAltitude(double alt) {
    if (alt < 0) alt = 0;
    if (alt > 20000) alt = 20000;

    currentAltitude = alt;

    if (weatherProfile.empty()) {
        interpolatedWeather.altitude = alt;
        interpolatedWeather.temperature = 15.0 - (alt * 0.0065);
        interpolatedWeather.pressure = 101325.0 * exp(-alt / 8500.0);
        interpolatedWeather.humidity = max(0.0, 50.0 - (alt * 0.002));
        interpolatedWeather.wind_u = 2.0 + (alt * 0.001);
        interpolatedWeather.wind_v = 1.0 + (alt * 0.0005);
    }
    else {
        interpolatedWeather = interpolateForAltitude(alt);
    }

    wind_u = interpolatedWeather.wind_u;
    wind_v = interpolatedWeather.wind_v;
    temperature = interpolatedWeather.temperature;
    pressure = interpolatedWeather.pressure;
    humidity = interpolatedWeather.humidity;

    altitude = alt;
}

double Weather::GenerateTurbulence() const {
    double r = (rand() % 1000) / 1000.0;
    return (r - 0.5) * 2.0 * turbulence;
}

void Weather::GetWindVector(double& out_x, double& out_y) const {
    out_x = wind_u + GenerateTurbulence();
    out_y = wind_v + GenerateTurbulence();
}

double Weather::CalculateAirDensity() const {
    const double R = 287.05;
    double T = temperature + 273.15;
    return pressure / (R * T);
}

void Weather::AffectParticle(double& vel_x, double& vel_y, double& vel_z, double particle_density) const {
    double wx, wy;
    GetWindVector(wx, wy);

    vel_x += wx * 0.1;
    vel_y += wy * 0.1;

    double air_dens = CalculateAirDensity();
    double drag = air_dens * 0.002;

    vel_x *= (1.0 - drag);
    vel_y *= (1.0 - drag);
    vel_z *= (1.0 - drag);

    double updraft = max(0.0, 1.0 - (altitude / 20000.0));
    double buoyancy = (air_dens - particle_density) * 0.05 * updraft;
    double gravity = -0.03 * (particle_density / 2500.0);
    double turb = GenerateTurbulence() * 0.005;

    vel_z += buoyancy + gravity + turb;

}

void Weather::getWeatherAtAltitude(double alt, double& out_wind_u, double& out_wind_v,
    double& out_temp, double& out_pres, double& out_hum) const {
    WeatherSample ws = interpolateForAltitude(alt);
    out_wind_u = ws.wind_u;
    out_wind_v = ws.wind_v;
    out_temp = ws.temperature;
    out_pres = ws.pressure;
    out_hum = ws.humidity;
}