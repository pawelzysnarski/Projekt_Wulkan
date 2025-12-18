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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <streambuf>
using namespace std;

struct NullBuffer : streambuf { int overflow(int c) override { return c; } };
void Wait(int milliseconds) { this_thread::sleep_for(chrono::milliseconds(milliseconds)); }

int main() {
    srand((unsigned)time(NULL));

    float angle = 0.0f;
    string demPath = "../geo/vesuvius_final.tiff";
    string weatherCSV = "../geo/open-meteo-40.81N14.44E1176m.csv";
    float orbitRadius = 8000.0f;
    Weather weatherSystem;

    cout << "Ladowanie danych pogodowych z: " << weatherCSV << "\n";
    bool weatherLoaded = false;
    {
        NullBuffer nb; streambuf* oldOut = cout.rdbuf(); streambuf* oldErr = cerr.rdbuf();
        cout.rdbuf(&nb); cerr.rdbuf(&nb);
        weatherLoaded = weatherSystem.loadWeatherProfile(weatherCSV);
        cout.rdbuf(oldOut); cerr.rdbuf(oldErr);
    }
    if (!weatherLoaded) {
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
    double pxSizeY_abs = abs(pxSizeY);

    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
    if (!ds) { cerr << "Nie mozna otworzyc TIFF do RGB: " << demPath << "\n"; return 1; }

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
        for (size_t i = 0; i < (size_t)nx * (size_t)ny; ++i) { tex[3 * i + 0] = r[i]; tex[3 * i + 1] = g[i]; tex[3 * i + 2] = b[i]; }
    }
    else {
        vector<unsigned char> gray((size_t)nx * (size_t)ny);
        GDALRasterBand* b = ds->GetRasterBand(1);
        b->RasterIO(GF_Read, 0, 0, nx, ny, gray.data(), nx, ny, GDT_Byte, 0, 0);
        for (size_t i = 0; i < (size_t)nx * (size_t)ny; ++i) { tex[3 * i + 0] = gray[i]; tex[3 * i + 1] = gray[i]; tex[3 * i + 2] = gray[i]; }
    }
    GDALClose(ds);

    if (!glfwInit()) { cerr << "glfwInit failed\n"; return 1; }

    int winW = nx, winH = ny;
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* vm = glfwGetVideoMode(mon);
    int maxW = (int)(vm->width * 0.9);
    int maxH = (int)(vm->height * 0.9);
    double scale = 1.0;
    if (winW > maxW || winH > maxH) {
        double sW = (double)maxW / (double)winW;
        double sH = (double)maxH / (double)winH;
        scale = min(sW, sH);
        winW = (int)floor(winW * scale);
        winH = (int)floor(winH * scale);
    }

    GLFWwindow* window = glfwCreateWindow(winW, winH, "Eruption Simulator - Dynamic Weather System", nullptr, nullptr);
    if (!window) { cerr << "glfwCreateWindow failed\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
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
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    double centerX = (minX + maxX) * 0.5;
    double centerY = (minY + maxY) * 0.5;
    double craterX = centerX;
    double craterY = centerY;

    double minElev = 1e15, maxElev = -1e15;
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            double gx = minX + x * pxSizeX;
            double gy = minY + y * pxSizeY_abs;
            double z = dem.getGroundZ(gx, gy);
            if (!isnan(z)) { if (z < minElev) minElev = z; if (z > maxElev) maxElev = z; }
        }
    }
    double craterZRaw = dem.getGroundZ(craterX, craterY);

    double zScale = 15.0;
    double baseZ = craterZRaw;

    float camHeightOverCrater = 4000.0f;
    float camZ = (float)((craterZRaw - baseZ) * zScale + camHeightOverCrater);

    weatherSystem.updateForAltitude(craterZRaw);
    cout << "Warunki poczatkowe w kraterze (wysokosc " << craterZRaw << " m):\n";
    cout << "  Temperatura: " << weatherSystem.temperature << "°C\n";
    cout << "  Wiatr U: " << weatherSystem.wind_u << " m/s\n";
    cout << "  Wiatr V: " << weatherSystem.wind_v << " m/s\n";
    cout << "  Cisnienie: " << weatherSystem.pressure << " Pa\n";
    cout << "  Wilgotnosc: " << weatherSystem.humidity << "%\n";
    cout << "  Gestosc powietrza: " << weatherSystem.CalculateAirDensity() << " kg/m3\n";

    int particleCount = (int)rand() % 1000 + 2000;
    int particlesPerFrame;
    int holdParticlesCount = 0;
    Cloud* cloud = new Cloud(&weatherSystem);
    bool isActive = true;
    vector<Materia> particlesOnEarth;
    vector<Materia> particlesOverflow;

    cout << "\nSymulacja rozpoczyna sie. Nacisnij ESC aby zakonczyc.\n";
    cout << "Liczba czastek: " << particleCount << "\n";

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    GLfloat lightDiffuse[4] = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat lightAmbient[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glShadeModel(GL_SMOOTH);

    static int frameCounter = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }

        int w, h; glfwGetFramebufferSize(window, &w, &h);
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;
        glViewport(0, 0, w, h);

        float aspect = (float)w / (float)h;
        if (!isfinite(aspect) || aspect < 1e-6f) aspect = 1e-6f;

        float fovY = glm::radians(55.0f);
        float zNear = 5.0f;
        float zFar = 2000000.0f;

        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glm::mat4 proj = glm::perspective(fovY, aspect, zNear, zFar);
        glLoadMatrixf(glm::value_ptr(proj));
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();

        double orbitX = cos(angle) * orbitRadius;
        double orbitY = sin(angle) * orbitRadius;

        glm::vec3 eye((float)(craterX + orbitX), (float)(craterY + orbitY), camZ);
        glm::vec3 target((float)craterX, (float)craterY, (float)((craterZRaw - baseZ) * zScale));
        glm::vec3 up(0.0f, 0.0f, 1.0f);

        glm::mat4 view = glm::lookAt(eye, target, up);
        glLoadMatrixf(glm::value_ptr(view));

        GLfloat lightPos[4] = { (GLfloat)craterX, (GLfloat)craterY, (GLfloat)(camZ + 2000.0f), 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);

        for (int y = 0; y < ny - 1; y++) {
            for (int x = 0; x < nx - 1; x++) {
                double x0 = minX + x * pxSizeX;
                double x1 = minX + (x + 1) * pxSizeX;

                double y0 = minY + y * pxSizeY_abs;
                double y1 = minY + (y + 1) * pxSizeY_abs;

                double z00r = dem.getGroundZ(x0, y0);
                double z10r = dem.getGroundZ(x1, y0);
                double z11r = dem.getGroundZ(x1, y1);
                double z01r = dem.getGroundZ(x0, y1);

                double z00 = (z00r - baseZ) * zScale;
                double z10 = (z10r - baseZ) * zScale;
                double z11 = (z11r - baseZ) * zScale;
                double z01 = (z01r - baseZ) * zScale;

                float u0 = (float)x / (float)(nx - 1);
                float u1 = (float)(x + 1) / (float)(nx - 1);
                float v0 = (float)y / (float)(ny - 1);
                float v1 = (float)(y + 1) / (float)(ny - 1);

                glm::vec3 v00((float)x0, (float)y0, (float)z00);
                glm::vec3 v10((float)x1, (float)y0, (float)z10);
                glm::vec3 v01((float)x0, (float)y1, (float)z01);
                glm::vec3 n = glm::normalize(glm::cross(v10 - v00, v01 - v00));

                glBegin(GL_QUADS);
                glColor3f(1.f, 1.f, 1.f);
                glNormal3f(n.x, n.y, n.z);
                glTexCoord2f(u0, v0); glVertex3f((float)x0, (float)y0, (float)z00);
                glTexCoord2f(u1, v0); glVertex3f((float)x1, (float)y0, (float)z10);
                glTexCoord2f(u1, v1); glVertex3f((float)x1, (float)y1, (float)z11);
                glTexCoord2f(u0, v1); glVertex3f((float)x0, (float)y1, (float)z01);
                glEnd();
            }
        }

        glDisable(GL_TEXTURE_2D);

        glDisable(GL_LIGHTING);
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f((float)craterX, (float)craterY, (float)((craterZRaw - baseZ) * zScale));
        glEnd();
        glEnable(GL_LIGHTING);

        for (int i = 0; i < (int)particlesOnEarth.size(); i++) {
            Materia& p = particlesOnEarth[i];
            glPointSize((GLfloat)(p.diameter * 4000.0));
            glBegin(GL_POINTS);
            glColor3f(0.0f, 0.0f, 0.0f);
            float pz = (float)((p.position_z - baseZ) * zScale);
            glVertex3f((float)p.position_x, (float)p.position_y, pz);
            glEnd();
        }

        if (isActive) {
            particlesPerFrame = (int)rand() % 30 + 10;
            int particlesToAdd = min(particlesPerFrame, particleCount - holdParticlesCount);
            if (particlesToAdd > 0) {
                cloud->generateParticles(particlesToAdd, craterX, craterY, craterZRaw, 30.0, 40.0, 80.0, 0.0005, 0.002, (int)floor(rand() % 10));
                holdParticlesCount += particlesToAdd;
            }
            else { isActive = false; }
        }

        for (int i = 0; i < (int)cloud->particles.size(); i++) {
            Materia& p = cloud->particles[i];
            glDisable(GL_LIGHTING);
            glPointSize((GLfloat)(p.diameter * 4000.0));
            glBegin(GL_POINTS);
            float heightFactor = (float)min(1.0, max(0.0, (p.position_z - craterZRaw + 500.0) / 1000.0));
            glColor3f(0.2f + heightFactor * 0.3f, 0.2f + heightFactor * 0.3f, 0.2f + heightFactor * 0.3f);
            float pz = (float)((p.position_z - baseZ) * zScale);
            glVertex3f((float)p.position_x, (float)p.position_y, pz);
            glEnd();
            glEnable(GL_LIGHTING);
        }

        double wind_u, wind_v;
        weatherSystem.GetWindVector(wind_u, wind_v);
        double updraft = max(0.0, (weatherSystem.temperature - 15.0) * 0.2)*100;
        cloud->update(0.05, weatherSystem.CalculateAirDensity(), wind_u, wind_v, particlesOnEarth, particlesOverflow, dem, updraft, weatherSystem.turbulence * 0.5);

        for (auto it = particlesOnEarth.begin(); it != particlesOnEarth.end();) {
            bool out = (it->position_x < minX || it->position_x > maxX || it->position_y < minY || it->position_y > maxY);
            double gz = dem.getGroundZ(it->position_x, it->position_y);
            if (out || isnan(gz)) {
                particlesOverflow.push_back(*it);
                it = particlesOnEarth.erase(it);
            }
            else {
                it->position_z = gz;
                ++it;
            }
        }

        if (frameCounter % 50 == 0) {
            cout << "Frame " << frameCounter << ": ";
            if (isActive) { cout << "Wulkan aktywny. Nowych czastek: " << particlesPerFrame << "\n"; }
            else { cout << "Wulkan nieaktywny.\n"; }
            cout << "W powietrzu: " << cloud->particles.size()
                << ", Na ziemi: " << particlesOnEarth.size()
                << ", Poza atmosfera: " << particlesOverflow.size()
                << ", Pozna obszarem: " << holdParticlesCount - particlesOnEarth.size() - cloud->particles.size()-particlesOverflow.size()
                << ", Ogolem: " << particleCount << endl;
            double avgHeight = 0, minHeight = 1e9, maxHeight = -1e9; int validCount = 0;
            for (auto& p : cloud->particles) {
                if (!isnan(p.position_z) && p.position_z > 0 && p.position_z < 100000) {
                    avgHeight += p.position_z; if (p.position_z < minHeight) minHeight = p.position_z; if (p.position_z > maxHeight) maxHeight = p.position_z; validCount++;
                }
            }
            if (validCount > 0) { avgHeight /= validCount; }
            cout << "Czastek: " << cloud->particles.size() << "/" << particleCount
                << ", Wysokosc: " << avgHeight << "m"
                << ", Wiatr: (" << fixed << setprecision(2) << wind_u << ", " << wind_v << ") m/s"
                << ", Temp: " << weatherSystem.temperature << "°C" << endl;
        }
        frameCounter++;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camHeightOverCrater += 100.0f, camZ = (float)((craterZRaw - baseZ) * zScale + camHeightOverCrater);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camHeightOverCrater = max(500.0f, camHeightOverCrater - 100.0f), camZ = (float)((craterZRaw - baseZ) * zScale + camHeightOverCrater);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) angle -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) angle += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) orbitRadius = max(2000.0f, orbitRadius - 100.0f);
        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) orbitRadius += 100.0f;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) zScale += 0.5;
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) zScale = max(1.0, zScale - 0.5);

        glfwSwapBuffers(window);
        Wait(100);
    }

    if (texId) glDeleteTextures(1, &texId);
    glfwDestroyWindow(window);
    glfwTerminate();

    delete cloud;

    cout << "\nSymulacja zakonczona.\n";
    cout << "Liczba czastek, ktore spadly na ziemie: " << particlesOnEarth.size() << "\n";
    cout << "Liczba czastek, ktore opuscily atmosfere: " << particlesOverflow.size() << "\n";
	cout << "Liczba czastek pozostalych w powietrzu: " << cloud->particles.size() << "\n";
	cout << "Liczba czastek poza obszarem DEM: " << holdParticlesCount - particlesOnEarth.size() - cloud->particles.size() - particlesOverflow.size() << "\n";
	cout << "Laczna liczba czastek: " << holdParticlesCount << "\n";
    return 0;
}