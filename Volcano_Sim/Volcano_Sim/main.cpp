#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "../include/dem_loader.h"
#include "../include/materia.h"
#include "../include/cloud.h"
#include "../include/weather.h"
#include "../include/formulas.h"
#include <gdal_priv.h>
#include <thread>
#include <chrono>
#include <ctime>

using namespace std;

void Wait(int milliseconds) {
    this_thread::sleep_for(chrono::milliseconds(milliseconds));
}

int main() {
    srand(time(NULL));

    string demPath = "../geo/vesuvius_final.tiff";
    string weatherCSV = "../geo/open-meteo-40.81N14.44E1176m.csv";

    Weather weatherSystem;

    cout << "Ladowanie danych pogodowych z: " << weatherCSV << "\n";
    if (!weatherSystem.loadWeatherProfile(weatherCSV)) {
        cout << "Nie mozna zaladowac danych pogodowych. Uzywam domyslnych warunkow.\n";
        weatherSystem = Weather(0, 2.0, 1.0, 15.0, 101300, 50, 0.1);
    }
    else {
        cout << "Dane pogodowe zaladowane pomyslnie.\n";
    }

    DEMLoader dem;
    if (!dem.load(demPath)) {
        cerr << "Nie mozna wczytac DEM: " << demPath << "\n";
        return 1;
    }

    int nx = dem.width();
    int ny = dem.height();

    const double* gt = dem.geoTransform();
    double originX = gt[0];
    double originY = gt[3];
    double pxSizeX = gt[1];
    double pxSizeY = gt[5];

    double minX = originX;
    double maxY = originY;
    double maxX = originX + nx * pxSizeX;
    double minY = originY + ny * pxSizeY;

    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
    if (!ds) {
        cerr << "Nie mozna otworzyc TIFF do RGB: " << demPath << "\n";
        return 1;
    }

    vector<unsigned char> tex((size_t)nx * (size_t)ny * 3, 0);
    GDALRasterBand* b1 = ds->GetRasterBand(1);
    GDALRasterBand* b2 = ds->GetRasterBand(2);
    GDALRasterBand* b3 = ds->GetRasterBand(3);

    if (b1 && b2 && b3) {
        vector<unsigned char> r((size_t)nx * (size_t)ny),
            g((size_t)nx * (size_t)ny),
            b((size_t)nx * (size_t)ny);

        b1->RasterIO(GF_Read, 0, 0, nx, ny, r.data(), nx, ny, GDT_Byte, 0, 0);
        b2->RasterIO(GF_Read, 0, 0, nx, ny, g.data(), nx, ny, GDT_Byte, 0, 0);
        b3->RasterIO(GF_Read, 0, 0, nx, ny, b.data(), nx, ny, GDT_Byte, 0, 0);

        for (size_t i = 0; i < (size_t)nx * (size_t)ny; ++i) {
            tex[3 * i + 0] = r[i];
            tex[3 * i + 1] = g[i];
            tex[3 * i + 2] = b[i];
        }
    }
    else {
        vector<unsigned char> gray((size_t)nx * (size_t)ny);
        GDALRasterBand* b = ds->GetRasterBand(1);
        b->RasterIO(GF_Read, 0, 0, nx, ny, gray.data(), nx, ny, GDT_Byte, 0, 0);

        for (size_t i = 0; i < (size_t)nx * (size_t)ny; ++i) {
            tex[3 * i + 0] = gray[i];
            tex[3 * i + 1] = gray[i];
            tex[3 * i + 2] = gray[i];
        }
    }
    GDALClose(ds);

    if (!glfwInit()) {
        cerr << "glfwInit failed\n";
        return 1;
    }

    int winW = nx;
    int winH = ny;

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* vm = glfwGetVideoMode(mon);
    int maxW = (int)(vm->width * 0.9);
    int maxH = (int)(vm->height * 0.9);

    double scale = 1.0;
    if (winW > maxW || winH > maxH) {
        double sW = (double)maxW / (double)winW;
        double sH = (double)maxH / (double)winH;
        scale = std::min(sW, sH);
        winW = (int)std::floor(winW * scale);
        winH = (int)std::floor(winH * scale);
    }

    GLFWwindow* window = glfwCreateWindow(winW, winH, "Eruption Simulator - Dynamic Weather System", nullptr, nullptr);
    if (!window) {
        cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);

    GLuint texId = 0;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nx, ny, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.data());

    double craterX = (minX + maxX) * 0.5;
    double craterY = (minY + maxY) * 0.5;
    double craterZ = dem.getGroundZ(craterX, craterY) + 500.0;

    weatherSystem.updateForAltitude(craterZ);
    cout << "Warunki poczatkowe w kraterze (wysokosc " << craterZ << "m):\n";
    cout << "  Temperatura: " << weatherSystem.temperature << "°C\n";
    cout << "  Wiatr U: " << weatherSystem.wind_u << " m/s\n";
    cout << "  Wiatr V: " << weatherSystem.wind_v << " m/s\n";
    cout << "  Cisnienie: " << weatherSystem.pressure << " Pa\n";
    cout << "  Wilgotnosc: " << weatherSystem.humidity << "%\n";
    cout << "  Gestosc powietrza: " << weatherSystem.CalculateAirDensity() << " kg/m3\n";

    int particleCount = (int)rand() % 2000 + 5000;
    int particlesPerFrame;
    int holdParticlesCount=0;
    Cloud* cloud = new Cloud(&weatherSystem);
	bool isActive = true;
    vector<Materia> particlesOnEarth;
	vector<Materia> particlesOverflow;

    cout << "\nSymulacja rozpoczyna sie. Nacisnij ESC aby zakonczyc.\n";
    cout << "Liczba czastek: " << particleCount << "\n";

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        double dataAspect = (maxX - minX) / (maxY - minY);
        double winAspect = (double)w / (double)h;
        double vx0 = minX, vx1 = maxX, vy0 = minY, vy1 = maxY;

        if (winAspect > dataAspect) {
            double cx = (minX + maxX) * 0.5;
            double halfY = (maxY - minY) * 0.5;
            double halfX = halfY * winAspect;
            vx0 = cx - halfX;
            vx1 = cx + halfX;
        }
        else {
            double cy = (minY + maxY) * 0.5;
            double halfX = (maxX - minX) * 0.5;
            double halfY = halfX / winAspect;
            vy0 = cy - halfY;
            vy1 = cy + halfY;
        }

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(vx0, vx1, vy0, vy1, -1000, 1000);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);

        glBegin(GL_QUADS);
        glColor3f(1.f, 1.f, 1.f);
        glTexCoord2f(0.0f, 1.0f); glVertex3d(minX, minY, 0.0);
        glTexCoord2f(1.0f, 1.0f); glVertex3d(maxX, minY, 0.0);
        glTexCoord2f(1.0f, 0.0f); glVertex3d(maxX, maxY, 0.0);
        glTexCoord2f(0.0f, 0.0f); glVertex3d(minX, maxY, 0.0);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        glPointSize(8.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3d(craterX, craterY, 10.0);
        glEnd();
        glDisable(GL_DEPTH_TEST);
        for (int i = 0; i < particlesOnEarth.size(); i++) {
            Materia& p = particlesOnEarth[i];
            glPointSize(4.0f);
            glBegin(GL_POINTS);
            glColor3f(0.0f, 0.0f, 0.0f);
            glVertex3d(p.position_x, p.position_y, p.position_z);
            glEnd();
        }
        if (isActive) {

            particlesPerFrame = (int)rand() % 30 + 1;
            int particlesToAdd = min(particlesPerFrame, particleCount - holdParticlesCount);
            if (particlesToAdd > 0) {
                cloud->generateParticles(particlesToAdd,
                    craterX, craterY, craterZ,
                    30.0,
                    150.0, 200.0,
                    0.0005, 0.002,
                    (int)floor(rand() % 10));

                holdParticlesCount += particlesToAdd;
            }
            else {
                isActive = false;
            }

        }
        for (int i = 0; i < cloud->particles.size(); i++) {
            Materia& p = cloud->particles[i];
            glPointSize((GLfloat)(p.diameter * 2000.0));
            glBegin(GL_POINTS);
            float heightFactor = min(1.0f, max(0.0f, (float)(p.position_z - craterZ + 500.0f) / 1000.0f));
            glColor3f(0.2f + heightFactor * 0.3f,
                0.2f + heightFactor * 0.3f,
                0.2f + heightFactor * 0.3f);
            glVertex3d(p.position_x, p.position_y, p.position_z);
            glEnd();
        }
        glEnable(GL_DEPTH_TEST);
        double wind_u, wind_v;
        weatherSystem.GetWindVector(wind_u, wind_v);

        cloud->update(0.05, weatherSystem.CalculateAirDensity(),
            wind_u, wind_v,
            particlesOnEarth,particlesOverflow, dem,
            0.0, weatherSystem.turbulence * 0.5);
        if (cloud->particles.size() > 0||holdParticlesCount==particleCount) {
            double avgHeight = 0;
            double minHeight = 1e9, maxHeight = -1e9;
            int validCount = 0;
            for (auto& p : cloud->particles) {
                if (!isnan(p.position_z) && p.position_z > 0 && p.position_z < 100000) {
                    avgHeight += p.position_z;
                    if (p.position_z < minHeight) minHeight = p.position_z;
                    if (p.position_z > maxHeight) maxHeight = p.position_z;
                    validCount++;
                }
            }
            if (validCount > 0) {
                avgHeight /= validCount;

                if (avgHeight < -100) {
                    cout << "UWAGA: Srednia wysokosc ujemna: " << avgHeight
                        << " min: " << minHeight << " max: " << maxHeight << "\n";
                }

                weatherSystem.updateForAltitude(avgHeight);

                static int frameCounter = 0;
                if (frameCounter % 50 == 0) {
                    cout << "Frame " << frameCounter << ": ";
                    if (isActive) {
						cout << "Wulkan aktywny. Nowych czastek: "<<particlesPerFrame<< "\n";
                        cout << "W powietrzu: " << cloud->particles.size()
                            << ", Na ziemi: " << particlesOnEarth.size()
                            << ", W kosmosie: " << particlesOverflow.size()
                            << "/" << particleCount << endl;
                    }
                    cout << "Czastek: " << cloud->particles.size() << "/" << particleCount;
                    cout << ", Wysokosc: " << avgHeight << "m";
                    cout << ", Wiatr: (" << fixed << setprecision(2) << wind_u
                        << ", " << wind_v << ") m/s";
                    cout << ", Temp: " << weatherSystem.temperature << "°C";
                    cout << endl;
                }
                frameCounter++;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        Wait(0);
    }

    if (texId) glDeleteTextures(1, &texId);
    glfwDestroyWindow(window);
    glfwTerminate();

    delete cloud;

    cout << "\nSymulacja zakonczona.\n";
    cout << "Liczba czastek, ktore spadly na ziemie: " << particlesOnEarth.size() << "\n";
	cout << "Liczba czastek, ktore opuscily atmosfere: " << particlesOverflow.size() << "\n";
    return 0;
}