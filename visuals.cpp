#define NOMINMAX
#include <iostream>
#include <random>

#include "network.hpp"
#include <thread>
#include <memory>
#include "CivetServer.h"
#include "sound.hpp"
#include "color.hpp"
#include <array>
#include <algorithm>
#include <atomic>

/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/

#pragma pack(1)
struct MotionData {
    unsigned int timestamp;
    short w, x, y, z;
    unsigned short counter;
};
#pragma pack()

class Visuals : public CivetHandler {
public:
    Visuals();
    int main(int argc, char* argv[]);

private:
    void send(const std::vector<unsigned int>& buffer);
    void fill(std::vector<unsigned int>& buffer);
    void rotate(std::vector<unsigned int>& buffer, float rot);
    void motion(const MotionData& motionData);
    void anim(std::vector<unsigned int>& buffer, std::vector<char> image, float time);
    void setBrightness(float brightness);

    bool handleGet(CivetServer* server, mg_connection* conn);
    bool getParamString(mg_connection* conn, const std::string& name, std::string& result, size_t occurance);
    bool getParamFloat(mg_connection* conn, const std::string& name, float& result, size_t occurance);
    bool getParamInt(mg_connection* conn, const std::string& name, int& result, size_t occurance);

    void add(std::vector<unsigned int>& result, const std::vector<unsigned int>& buf1, const std::vector<unsigned int>& buf2, float a = 0.5f, float b = 0.5f);
    void effectRaindrops(std::vector<unsigned int>& buffer);
    void effectLines(std::vector<unsigned int>& buffer, int x1, int y1, int x2, int y2, const Color3& color);
    void effectPlasma(std::vector<unsigned int>& buffer);

    int m_width = 25;
    int m_height = 20;
    int m_roof = 10;
    int m_fps = 60;
    std::string m_host = "192.168.1.111";
    std::string m_hostMotion = "192.168.1.112";
    int m_port = 7000;
    int m_portControl = 7001;
    int m_portMotion = 7000;
    int m_portMotionControl = 7001;
    float m_brightness = 0.1f;
    std::vector<int> m_leds;
    float m_x, m_y, m_z; // motion controller euler angles

    Network m_network;
    Network m_networkControl;
    std::unique_ptr<CivetServer> m_server;
    Sound m_sound;
    int m_streamID = 0;
    int m_music = 0;
    float m_volume = 1.0f;
    std::vector<std::string> m_musicFiles;
    double m_time;
};

template<class T>
T fmod2(T x, T y)
{
    if (x >= T(0))
        return std::fmod(x, y);
    return std::fmod(x, y) + y;
}

Color3 saturate(const Color3& col)
{
    return Color3(
        std::max(0.0f, std::min(1.0f, col.r)),
        std::max(0.0f, std::min(1.0f, col.g)),
        std::max(0.0f, std::min(1.0f, col.b))
        );
}
/*
Color3 hue(float h)
{
    float r = abs(h * 6 - 3) - 1;
    float g = 2 - abs(h * 6 - 2);
    float b = 2 - abs(h * 6 - 4);
    return saturate(Color3(r, g, b));
}

Color3 HSVtoRGB(const Color3& hsv)
{
    return Color3(((hue(hsv.r) - 1.0f) * hsv.g + 1.0f) * hsv.b);
}
*/
Color3 HSVtoRGB(const Color3& hsv)
{
    Color3 out;
    float h = hsv.r;
    float s = hsv.g;
    float v = hsv.b;
    if (s == 0.0f)
    {
        // gray
        out.r = out.g = out.b = v;
        return out;
    }

    h = fmodf(h, 1.0f) / (60.0f/360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out.r = v; out.g = t; out.b = p; break;
    case 1: out.r = q; out.g = v; out.b = p; break;
    case 2: out.r = p; out.g = v; out.b = t; break;
    case 3: out.r = p; out.g = q; out.b = v; break;
    case 4: out.r = t; out.g = p; out.b = v; break;
    case 5: default: out.r = v; out.g = p; out.b = q; break;
    }
    return out;
}

class Effect {
public:
    virtual ~Effect() {}
    virtual void fill(std::vector<unsigned int>& buffer, double time);
};

class EffectRaindrops : public Effect {
    void fill(std::vector<unsigned int>& buffer, double time) override
    {
    
    }
};

Visuals::Visuals()
{
    // start webserver
    std::string path = "data";
    std::cout << "starting webserver: " << path << std::endl;
    const char* options[] = { "document_root", path.c_str(),
                              "listening_ports", "8080",
                              "num_threads", "4",
                              nullptr
                            };
    m_server.reset(new CivetServer(options));
    m_server->addHandler("/set", this);
    m_server->addHandler("/music/volup", this);
    m_server->addHandler("/music/voldown", this);
    m_server->addHandler("/music/next", this);
    m_server->addHandler("/music/previous", this);

    // init led lookup table
/*    for (int x=0, i=0; x < m_width; x++) {
        int off = (x / 5) % 2;
        for (int y=0; y < m_height; y++, i++) {
            if ((x + off) % 2 == 0)
                m_leds.push_back(x * m_height + y);
            else
                m_leds.push_back(x * m_height + (m_height - 1) - y);                
        }
    }*/

    m_leds = { 0, 25, 50, 75, 100, 125, 150, 175, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475 };
    for (int x=1; x<25; x++) {
        for (int y=0; y<20; y++) {
            if ((x % 2) == ((x / 5) % 2))
                m_leds.push_back(m_leds[y] + x);
            else
                m_leds.push_back(m_leds[19-y] + x);
        }
    }

    m_musicFiles = {
        "compo.ogg"
    };

}

bool Visuals::getParamString(mg_connection* conn, const std::string& name, std::string& result, size_t occurance)
{
    std::string results;
    if (CivetServer::getParam(conn, name.c_str(), results, 0)) {
        if (!results.empty())
            result = results;
    }
    return false;
}

bool Visuals::getParamFloat(mg_connection* conn, const std::string& name, float& result, size_t occurance)
{
    std::string results;
    if (CivetServer::getParam(conn, name.c_str(), results, 0)) {
        float resultf = static_cast<float>(atof(results.c_str()));
        if (resultf) {
            result = resultf;
            return true;
        }
    }
    return false;
}

bool Visuals::getParamInt(mg_connection* conn, const std::string& name, int& result, size_t occurance)
{
    std::string results;
    if (CivetServer::getParam(conn, name.c_str(), results, 0)) {
        int resulti = atoi(results.c_str());
        if (resulti) {
            result = resulti;
            return true;
        }
    }
    return false;
}

bool Visuals::handleGet(CivetServer* server, mg_connection* conn)
{
    std::string uri = mg_get_request_info(conn)->uri;
    
    if (uri == "/set") {
        std::string message;
        if (getParamInt(conn, "fps", m_fps, 0)) {
            message = "fps=" + std::to_string(m_fps);
        }
        if (getParamString(conn, "host", m_host, 0)) {
            message = "host=" + m_host;
        }
        if (getParamString(conn, "hostMotion", m_hostMotion, 0)) {
            message = "hostMotion=" + m_hostMotion;
        }
        if (getParamInt(conn, "port", m_port, 0)) {
            message = "port=" + std::to_string(m_port);
        }
        if (getParamInt(conn, "portMotionControl", m_portControl, 0)) {
            message = "portControl=" + std::to_string(m_portControl);
        }
        if (getParamInt(conn, "portMotion", m_portMotion, 0)) {
            message = "portMotion=" + std::to_string(m_portMotion);
        }
        if (getParamInt(conn, "portMotionControl", m_portMotionControl, 0)) {
            message = "portMotionControl=" + std::to_string(m_portMotionControl);
        }
        if (getParamFloat(conn, "brightness", m_brightness, 0)) {
            setBrightness(m_brightness);
            message = "brightness=" + std::to_string(m_brightness);
        }
        message = "HTTP/1.1 200 OK\r\n\r\n" + message;
        mg_printf(conn, message.c_str());
    }
    if (uri == "/music/next") {
        m_sound.stop(m_streamID);
        m_music = (m_music + 1) % m_musicFiles.size();
        m_streamID = m_sound.play(m_musicFiles[m_music]);
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    if (uri == "/music/previous") {
        m_sound.stop(m_streamID);
        m_music = (m_music + m_musicFiles.size() - 1) % m_musicFiles.size();
        m_streamID = m_sound.play(m_musicFiles[m_music]);
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    if (uri == "/music/volup") {
        m_volume += 0.1f;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        m_sound.setVolume(m_streamID, m_volume);
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    if (uri == "/music/voldown") {
        m_volume -= 0.1f;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        m_sound.setVolume(m_streamID, m_volume);
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    return true;
}

void Visuals::send(const std::vector<unsigned int>& buffer)
{
    std::vector<char> out;
    out.resize(m_width * m_height * 3);

    for (int i=0; i < m_width * m_height; i++) {
        unsigned int col = buffer[m_leds[i]];

        
        // cut away errors 0x030504
        if ((col & 0xff0000) >> 16 < 5)
            col = col & 0xff00ffff;
        if ((col & 0x00ff00) >> 8 < 5)
            col = col & 0xffff00ff;
        if ((col & 0x0000ff) >> 0 < 5)
            col = col & 0xffffff00;

        out[i * 3 + 0] = (col & 0xff0000) >> 16;
        out[i * 3 + 1] = (col & 0x00ff00) >> 8;
        out[i * 3 + 2] = (col & 0x0000ff) >> 0;
    }
    m_network.send(out, 0, 800);
    m_network.send(out, 800, 700);
}

float dot(float x1, float y1, float x2, float y2)
{
    return sqrt(x1*x2 + y1*y2);
}

float length(float x, float y)
{
    return sqrt(x*x + y*y);
}

void Visuals::effectPlasma(std::vector<unsigned int>& buffer)
{
    const float PI = 3.141592f;
    for (int y=0; y<m_height; y++)
        for (int x=0; x<m_width; x++) {
    
            float color1 = (sin(dot(x+1.0f, sin(m_time), y+1.0f, cos(m_time))*0.6f+m_time)+1.0f)/2.0f;
    
            float centerX = m_width/2.0f + m_width/2.0f*sin(-m_time);
            float centerY = m_height/2.0f + m_height/2.0f*cos(-m_time);
    
            float color2 = (cos(length(x - centerX, y - centerY)*0.3f)+1.0f)/2.0f;
    
            float color = (color1 + color2)/2.0f;

            float red	= (cos(PI*color/0.5f+m_time)+1.0f)/2.0f;
            float green	= (sin(PI*color/0.5f+m_time)+1.0f)/2.0f;
            float blue	= (sin(m_time)+1.0f)/2.0f;

            buffer[y * m_width + x] = Color3(red, green, blue);
        }
    
}

void Visuals::effectRaindrops(std::vector<unsigned int>& buffer)
{
    srand(0);
    std::array<int, 25> offsets;
    std::array<Color3, 25> colors;
    for (int x=0; x<m_width; x++) {
        offsets[x] = rand() % 10;
        colors[x] = Color3(((float)rand()/RAND_MAX) / 2 + 0.5f, ((float)rand()/RAND_MAX / 4) / 2 + 0.5f, ((float)rand()/RAND_MAX / 4) / 2 + 0.5f);
/*        colors[x].r = fmod(x / 25.0f, 1.0f); //(float)rand() / RAND_MAX / 2 + 0.5f;
        colors[x].g = fmod(x / 25.0f+0.33, 1.0f); // (float)rand() / RAND_MAX / 8 + 0.5f;
        colors[x].b = fmod(x / 25.0f+0.66, 1.0f); // (float)rand() / RAND_MAX / 8 + 0.5f;*/
        colors[x] = HSVtoRGB(Color3(x / 50.0f, 1.0f, 1.0f));
    }

    int t = m_time * m_fps / 10.0f;
    for (int y=0; y<m_height; y++) {
        for (int x=0; x<m_width; x++) {
            float frac = fmod(t, 1.0f);
            Color3 col1 = colors[x];
            col1 *= (fmod2((float)(y-t+offsets[x]), (float)m_height) / m_height - 0.5f) * 2.0f;
            buffer[y*m_width+x] = col1;
        }
    }
}

void Visuals::effectLines(std::vector<unsigned int>& buffer, int x1, int y1, int x2, int y2, const Color3& color)
{
    float dx = float(x2-x1)/float(y2-y1);
    float dy = float(y2-y1)/float(x2-x1);
    float len = length(x2-x1, y2-y1);

    if (dx < dy) {
        float x = x1;
        for (int y=y1; y<=y2; y++) {
            x += dx;
            buffer[y * m_width + x] = color;
        }
    } else {
        float y = y1;
        for (int x=x1; x<=x2; x++) {
            y += dy;
            buffer[(int)y * m_width + x] = color;
        }
    }
}

void Visuals::anim(std::vector<unsigned int>& buffer, std::vector<char> image, float time)
{
    for (int y=0; y<m_width; y++) {
        for (int x=0; x<m_height; x++) {
        }
    }
}

void Visuals::add(std::vector<unsigned int>& result, const std::vector<unsigned int>& buf1, const std::vector<unsigned int>& buf2, float a, float b)
{
    for (int i=0; i<result.size(); i++)
        result[i] = Color3(buf1[i]) * a + Color3(buf2[i]) * b;
}

void Visuals::rotate(std::vector<unsigned int>& buffer, float rot)
{
    rot = fmod2(rot / 3.141592f / 2.0f, 1.0f);
    std::cout << m_x << " " << " " << m_y << " " << " " << m_z << " " << rot << std::endl;
    std::vector<unsigned int> buf2;
    buf2.resize(buffer.size());
    for (int x=0; x<m_width; x++) {
        for (int y=0; y<m_height; y++) {
            float frac = fmod(rot * m_width, 1.0f);
            Color3 col1(buffer[y * m_width + (x + size_t(rot * m_width) + 0) % m_width]);
            Color3 col2(buffer[y * m_width + (x + size_t(rot * m_width) + 1) % m_width]);
            buf2[y * m_width + x] = /*col1 + (col2 - col1) * frac;*/ col1 * (1 - frac) + col2 * frac;
        }
    }

    std::copy(buf2.begin(), buf2.end(), buffer.begin());
}

void Visuals::fill(std::vector<unsigned int>& buffer)
{
    int t = m_time * m_fps;

    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(0, 0xffffff);

    buffer.resize(m_width * m_height);
    std::vector<unsigned int> buf1, buf2;
    buf1.resize(m_width * m_height);
    buf2.resize(m_width * m_height);
    effectRaindrops(buf1);
    effectPlasma(buf2);
    add(buffer, buf1, buf2, 0.0f, 0.1f);
    effectLines(buffer, 0, 0, 00, 19, Color3(1, 1, 1));
    rotate(buffer, m_x/*time / 10.0f*/);
}

void Visuals::setBrightness(float brightness)
{
    std::vector<char> control;
    control.push_back(2);
    control.push_back(static_cast<char>(brightness * 255.0f));
    m_networkControl.send(control);
}

void Visuals::motion(const MotionData& motionData)
{
    float xs = motionData.x / 16384.0f;
    float ys = motionData.y / 16384.0f;
    float zs = motionData.z / 16384.0f;
    float ws = motionData.w / 16384.0f;
    m_x = atan2(2*(xs*ys + ws*zs), 1-2*(ws*ws + xs*xs));  // psi
    m_y = -asin(2*xs*zs + 2*ws*ys);                         // theta
    m_z = atan2(2*ys*zs - 2*ws*xs, 2*ws*ws + 2*zs*zs - 1);  // phi
    /*  m_x = atan2(2*xs*ys - 2*ws*zs, 2*ws*ws + 2*xs*xs - 1);  // psi
    m_x = m_x + 3.141592f;*/
   // m_x = m_x + 3.141592f;
    //std::cout << motionData.timestamp << " " << xs << " " << ys << " " << zs << " " << ws << " " << m_x / 3.141592f * 180.0f << " " << m_y / 3.141592f * 180.0f << " " << m_z / 3.141592f * 180.0f << std::endl;
}

int Visuals::main(int argc, char* argv[])
{
    std::vector<unsigned int> buffer;
    m_network.connect(m_host, m_port);
    //std::cout << "sockname: " << m_network.getSockName() << std::endl;
    m_networkControl.connect(m_host, m_portControl);

    std::atomic<MotionData> amd;

    auto motionThread = std::thread([this, &amd]() {
        Network network;
        network.listen(m_portMotion);
        while (true) {
            try {
                auto result = network.recv();
                MotionData& motionData = reinterpret_cast<MotionData&>(*result.data());
                amd = motionData;
            } catch (const std::runtime_error& ex) {
                std::cout << ex.what() << std::endl;
            }
        }
    });
    
    m_streamID = m_sound.play("compo.ogg", true);
    m_time = 0;
    std::chrono::steady_clock::time_point last_tp = std::chrono::steady_clock::now();

    while (true) {
        m_time += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_tp).count() / 1000.0f;

        MotionData md = amd;
        motion(md);

        buffer.clear();
        fill(buffer);
        try {
            send(buffer);
            auto sleep = std::chrono::milliseconds(1000/m_fps);
            last_tp = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(sleep);
        } catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }
    }
    
    return 0;
}

int main(int argc, char* argv[])
{
    Visuals visuals;
    return visuals.main(argc, argv);
}
