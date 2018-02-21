#define NOMINMAX
#include <iostream>
#include <fstream>
#include <random>
#include <mutex>

#include "network.hpp"
#include <thread>
#include <memory>
#include "CivetServer.h"
#include "sound.hpp"
#include "color.hpp"
#include <array>
#include <algorithm>
#include <atomic>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "color_utils.hpp"

#include "effect_data.hpp"
#include "effect.hpp"
#include "color_maker.hpp"
#include "raindrop_effect.hpp"
#include "rotation_effect.hpp"
#include "line_effect.hpp"
#include "plasma_effect.hpp"
#include "circle_effect.hpp"
#include "ocean_effect.hpp"
#include "wave_effect.hpp"
#include "stars_effect.hpp"
#include "fill_effect.hpp"

/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/

#pragma pack(1)
struct MotionData {
    unsigned int timestamp;
    int16_t w, x, y, z;
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
    int m_fps = 35;
    std::string m_host = "127.0.0.1";
    std::string m_hostMotion = "192.168.1.112";
    int m_port = 7000;
    int m_portControl = 7001;
    int m_portMotion = 7002;
    int m_portMotionControl = 7001;
    float m_brightness = 0.1f;
    std::vector<int> m_leds;
    std::atomic<bool> m_do_fft;

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
    std::mutex m_soundMutex;
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
    m_server->addHandler("/music/seek/forward", this);
    m_server->addHandler("/music/seek/backward", this);
    m_server->addHandler("/music/beat/toggle", this);
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

    m_do_fft.exchange(false);
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
    HttpResponse response(conn);

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
	response.set_message(message);
    }
    if (uri == "/music/next") {
        std::lock_guard<std::mutex> lock(m_soundMutex);
        m_sound.stop(m_streamID);
        m_music = (m_music + 1) % m_musicFiles.size();
        std::cout << "Play: " << m_musicFiles[m_music] << std::endl;
        m_streamID = m_sound.play(m_musicFiles[m_music], true, m_volume);
    }
    if (uri == "/music/previous") {
        std::lock_guard<std::mutex> lock(m_soundMutex);
        m_sound.stop(m_streamID);
        m_music = (m_music + m_musicFiles.size() - 1) % m_musicFiles.size();
        std::cout << "Play: " << m_musicFiles[m_music] << std::endl;
        m_streamID = m_sound.play(m_musicFiles[m_music], true, m_volume);
    }
    if (uri == "/music/seek/forward") {
        std::lock_guard<std::mutex> lock(m_soundMutex);
        std::cout << "Seek forward" << std::endl;
        m_sound.seekSeconds(m_streamID, 20);
    }
    if (uri == "/music/seek/backward") {
        std::lock_guard<std::mutex> lock(m_soundMutex);
        std::cout << "Seek backward" << std::endl;
        m_sound.seekSeconds(m_streamID, -20);
    }
    if (uri == "/music/volup") {
        m_volume += 0.05f;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        m_sound.setVolume(m_streamID, m_volume);
    }
    if (uri == "/music/voldown") {
        m_volume -= 0.05f;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        m_sound.setVolume(m_streamID, m_volume);
    }
    if (uri == "/music/beat/toggle") {
        m_do_fft.exchange(! m_do_fft.load());
    }
    if (uri == "/effect/next") {
        m_currentEffect = (m_currentEffect + 1) % m_effects.size();
    }
    if (uri == "/effect/previous") {
        m_currentEffect = (m_currentEffect + m_effects.size() - 1) % m_effects.size();
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
    float xs = float(motionData.x) / 16384.0f;
    float ys = float(motionData.y) / 16384.0f;
    float zs = float(motionData.z) / 16384.0f;
    float ws = float(motionData.w) / 16384.0f;

    return RotationData(
        atan2f(2.0f * (xs*ys + ws*zs), 1.0f-2.0f*(ws*ws + xs*xs)),  // psi
        -asinf(2.0f * xs*zs + 2.0f * ws*ys),                         // theta
        atan2f(2.0f * ys*zs - 2.0f * ws*xs, 2.0f*ws*ws + 2.0f*zs*zs - 1.0f));  // phi
}

int Visuals::main(int argc, char* argv[])
{
    std::string datadir = "data";

    std::string inputConfig;
    try {
        namespace po = boost::program_options;
        po::options_description podesc_generic("Generic Options");
        podesc_generic.add_options()
            ("config", po::value<std::string>(&inputConfig), "config file")
            ("help,h", "this help screen")
            ("version,v", "output version info")
        ;

        po::options_description podesc_config("Configuration");
        podesc_config.add_options()
            ("music,m", po::value<std::string>(&datadir)->default_value(datadir), "data directory")
            ("led-host,h", po::value<std::string>(&m_host)->default_value(m_host), "led host")
            ("led-port,p", po::value<int>(&m_port)->default_value(m_port), "led port")
            ("led-controlport,c", po::value<int>(&m_portControl)->default_value(m_portControl), "led control port")
            ("motion-port,g", po::value<int>(&m_portMotion)->default_value(m_portMotion), "motion port")
            ("fps,f", po::value<int>(&m_fps)->default_value(m_fps), "motion port")
            ("volume,V", po::value<float>(&m_volume)->default_value(m_volume), "initial volume")
        ;

        po::options_description podesc_cmdline;
        podesc_cmdline.add(podesc_generic).add(podesc_config);

        po::options_description podesc_file;
        podesc_file.add(podesc_config);

        po::options_description podesc_visible;
        podesc_visible.add(podesc_generic).add(podesc_config);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(podesc_cmdline).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: visuals [options]" << std::endl;
            std::cout << podesc_visible << std::endl;
            return 3;
        }

        if (vm.count("config")) {
            std::ifstream ifs(inputConfig.c_str());
            po::store(po::parse_config_file(ifs, podesc_file), vm);
            po::notify(vm);
        }

		std::cout << "Serving from: " << datadir << std::endl;
		std::cout << "Sending to: " << m_host << ":" << m_port << std::endl;
		std::cout << "Control at: " << m_portControl << std::endl;
		std::cout << "Motion from: " << m_portMotion << std::endl;
		std::cout << "Current FPS: " << m_fps << std::endl;

    } catch (std::exception& e) {
        std::cerr << "visuals: usage error: " << e.what() << std::endl;
        return 2;
    }

    datadir += '/';
    try {
        for (auto i = boost::filesystem::directory_iterator(datadir);
						i != boost::filesystem::directory_iterator();
						++i) {
            if (i->path().extension() == ".ogg") {
                m_musicFiles.push_back(i->path().string());
                std::cout << "music file found: " << i->path() << std::endl;
            }
        }
    } catch (const boost::filesystem::filesystem_error& ex) {
        std::cout << ex.what() << std::endl;
    }
    std::sort(m_musicFiles.begin(), m_musicFiles.end());

    EffectBuffer buffer;
    m_network.connect(m_host, m_port);
    //std::cout << "sockname: " << m_network.getSockName() << std::endl;
    m_networkControl.connect(m_host, m_portControl);

    std::atomic<MotionData> amd;

    Color3 test(0.2, 0.3, 0.4);
    std::cout << RGBtoHSV(HSVtoRGB(test)).r << std::endl;
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

    std::atomic<BeatEffectData> beat_data;

    auto beatThread = std::thread([this, &beat_data]() {
        auto sleep = std::chrono::milliseconds(1000 / (m_fps * 2));
        while (true) {
            if (m_do_fft.load()) {
                std::this_thread::sleep_for(sleep);
                beat_data = m_sound.getBeatData(m_streamID, int(m_time * 1000.0));
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                beat_data = BeatEffectData(0, 0, 0);
            }
        }
    });

    // start first music
    if (m_musicFiles.size())
        m_streamID = m_sound.play(m_musicFiles[0], true, m_volume);

    m_time = 0;
    std::chrono::steady_clock::time_point last_tp = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<Effect>> effects;

    effects.push_back(
            std::make_shared<AddEffect>(
                std::make_shared<RaindropEffect>(m_width, m_time), 0.7f));
    effects.push_back(
            std::make_shared<AddEffect>(
                std::make_shared<LineEffect>(Point(0, 0), Point(24, 19), Color3(1, 1, 1)), 1.0f));
    effects.push_back(
                std::make_shared<RotationEffect>());
    effects.push_back(
            std::make_shared<AddEffect>(
                std::make_shared<PlasmaEffect>(), 0.07f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));


    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
        std::make_shared<LineEffect>(Point(0, 0), Point(24, 19), Color3(1, 1, 1)));
    effects.push_back(
            std::make_shared<RotationEffect>());
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<ExtendingCircleEffect>(3.0f, HSVtoRGB(Color3(rand() / RAND_MAX, 1.0f, 1.0f)), 3, m_time), 1.0f));
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<ExplodingCircleEffect>(6, 1.2, m_time), 1.0));
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<ExtendingCircleEffect>(3.0f, HSVtoRGB(Color3(rand() / RAND_MAX, 1.0f, 1.0f)), 3.5, m_time), 1.0f));
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<ExplodingCircleEffect>(8, 2, m_time), 1.0));
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<ExtendingCircleEffect>(4.0f, HSVtoRGB(Color3(rand() / RAND_MAX, 1.0f, 1.0f)), 4, m_time), 1.0f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));

    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
        std::make_shared<LineEffect>(Point(0, 0), Point(24, 19), Color3(1, 1, 1)));
    effects.push_back(
            std::make_shared<RotationEffect>());
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<TopDownWaveEffect>(2, false), 1.0f));
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<TopDownWaveEffect>(3, true), 1.0f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));

    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
        std::make_shared<LineEffect>(Point(0, 0), Point(24, 19), Color3(1, 1, 1)));
    effects.push_back(
            std::make_shared<RotationEffect>());
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<DisolvingCircleFieldEffect>(35), 1.0f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));

    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
        std::make_shared<OceanEffect>());
    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<StarsEffect>(std::make_shared<RandomColorMaker>(), Rect(Point(0, 0), Point(24, 12)), 8), 1.0f));
    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<StarsEffect>(std::make_shared<ConstColorMaker>(Color3(1, 1, 1)), Rect(Point(0, 0), Point(24, 10)), 5), 1.0f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));

    m_effects.push_back(effects);

    effects.clear();
    effects.push_back(
        std::make_shared<LineEffect>(Point(0, 0), Point(24, 19), Color3(1, 1, 1)));
    effects.push_back(
            std::make_shared<RotationEffect>());
    effects.push_back(
            std::make_shared<AddEffect>(
        std::make_shared<FallingLineEffect>(5.0), 1.0f));

    effects.push_back(
        std::make_shared<AddEffect>(
            std::make_shared<FillEffect>(std::make_shared<KickColorIntensity>(Color3(1,1,1))), 0.2f));

    m_effects.push_back(effects);

    while (true) {
    m_time += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_tp).count() / 1000.0f;

        MotionData md = amd;
        EffectState state(m_time, motion(md), beat_data);

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
    
    return 0;
}

int main(int argc, char* argv[])
{
    Visuals visuals;
    return visuals.main(argc, argv);
}
