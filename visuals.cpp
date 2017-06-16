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
#include "fftw3.h"
#include "color_utils.hpp"

#include "effect.hpp"
#include "raindrop_effect.hpp"
#include "rotation_effect.hpp"
#include "line_effect.hpp"
#include "plasma_effect.hpp"
#include "circle_effect.hpp"

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
    void send(const EffectBuffer& buffer);
    void fill(EffectBuffer& buffer, std::vector<std::shared_ptr<Effect>>& effects, const EffectState& state);
    RotationData motion(const MotionData& motionData);
    void setBrightness(float brightness);

    bool handleGet(CivetServer* server, mg_connection* conn);
    bool getParamString(mg_connection* conn, const std::string& name, std::string& result, size_t occurance);
    bool getParamFloat(mg_connection* conn, const std::string& name, float& result, size_t occurance);
    bool getParamInt(mg_connection* conn, const std::string& name, int& result, size_t occurance);


    int m_width = 25;
    int m_height = 20;
    int m_roof = 10;
    int m_fps = 60;
    std::string m_host = "127.0.0.1";
    std::string m_hostMotion = "192.168.1.112";
    int m_port = 7000;
    int m_portControl = 7001;
    int m_portMotion = 7002;
    int m_portMotionControl = 7001;
    float m_brightness = 0.1f;
    std::vector<int> m_leds;

    Network m_network;
    Network m_networkControl;
    std::unique_ptr<CivetServer> m_server;
    Sound m_sound;
    int m_streamID = 0;
    int m_music = 0;
    float m_volume = 1.0f;
    std::vector<std::string> m_musicFiles;
    double m_time;
    std::vector<std::vector<std::shared_ptr<Effect>>> m_effects;
    int m_currentEffect = 0;
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
    m_server->addHandler("/effect/next", this);
    m_server->addHandler("/effect/previous", this);

    // init led lookup table
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
    if (uri == "/effect/next") {
        m_currentEffect = (m_currentEffect + 1) % m_effects.size();
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    if (uri == "/effect/previous") {
        m_currentEffect = (m_currentEffect + m_effects.size() - 1) % m_effects.size();
        mg_printf(conn,"HTTP/1.1 200 OK\r\n\r\n");
    }
    return true;
}

void Visuals::send(const EffectBuffer& buffer)
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

void Visuals::fill(EffectBuffer& buffer, std::vector<std::shared_ptr<Effect>>& effects, const EffectState& state)
{
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(0, 0xffffff);

    for (auto effect : effects) {
        effect->fill(buffer, state);
    }
}

void Visuals::setBrightness(float brightness)
{
    std::vector<char> control;
    control.push_back(2);
    control.push_back(static_cast<char>(brightness * 255.0f));
    m_networkControl.send(control);
}

RotationData Visuals::motion(const MotionData& motionData)
{
    float xs = motionData.x / 16384.0f;
    float ys = motionData.y / 16384.0f;
    float zs = motionData.z / 16384.0f;
    float ws = motionData.w / 16384.0f;

    return RotationData(
        atan2(2*(xs*ys + ws*zs), 1-2*(ws*ws + xs*xs)),  // psi
        -asin(2*xs*zs + 2*ws*ys),                         // theta
        atan2(2*ys*zs - 2*ws*xs, 2*ws*ws + 2*zs*zs - 1));  // phi
}

// Create a hamming window of windowLength samples in buffer
// stand so in : http://blog.bjornroche.com/2012/07/frequency-detection-using-fft-aka-pitch.html
#if 0
#define PI 3.141592653589793238f
    void hamming(int windowLength, float *buffer) {

      for (int i = 0; i < windowLength; i++) {

        buffer[i] = 0.54 - (0.46 * cos(2 * PI * (i / ((windowLength - 1) * 1.0))));

      }
  for (int i = 0; i < windowLength; i++) {

    buffer[i] = 0.54 - (0.46 * cos(2 * PI * (i / ((windowLength - 1) * 1.0))));

  }
}

#endif
int Visuals::main(int argc, char* argv[])
{
 

    EffectBuffer buffer;
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
    //FFT Gelöt
    //stand so in der fftw doku
#if 0
    int fft_num_in;
    double *in;
    fftw_complex *out;
    fftw_plan p;
    out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex));
    //FFTW_ESTIMATE scheint die Flag zu sein.. not sure
    p = fftw_plan_dft_r2c_1d(fft_num_in, in, out, FFTW_ESTIMATE);
    fftw_execute(p);
    float *converted_out;
    //TODO: irgendwie hier fft**_complex in float umbasteln
    hamming(fft_num_in, converted_out);
#endif
    

    //END FFT Gelöt
    m_time = 0;
    std::chrono::steady_clock::time_point last_tp = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<Effect>> effects;
    effects.push_back(
            std::make_shared<AddEffect>(
                std::make_shared<PlasmaEffect>(), 0.2f));
    effects.push_back(
            std::make_shared<AddEffect>(
                std::make_shared<RaindropEffect>(m_width, m_time), 0.7f));
    effects.push_back(
            std::make_shared<LineEffect>(Point(0, 0), Point(0, 19), Color3(1, 1, 1)));
    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
            std::make_shared<AddEffect>(
		std::make_shared<ExtendingCircleEffect>(3, Color3(1, 1, 1), 4, m_time), 1.0));
    effects.push_back(
            std::make_shared<AddEffect>(
		std::make_shared<ExplodingCircleEffect>(6, 1, m_time), 1.0));
    effects.push_back(
            std::make_shared<RotationEffect>());
    m_effects.push_back(effects);

    while (true) {
    m_time += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_tp).count() / 1000.0f;

        MotionData md = amd;
        EffectState state(m_time, motion(md));

        buffer.clear();
        fill(buffer, m_effects[m_currentEffect], state);
        try {
            send(buffer);
            auto sleep = std::chrono::milliseconds(1000/m_fps);
            last_tp = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(sleep);
        } catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }
    }
#if 0
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
#endif
    return 0;

}

int main(int argc, char* argv[])
{
    Visuals visuals;
    return visuals.main(argc, argv);
}
