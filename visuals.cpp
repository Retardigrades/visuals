#define SDL_MAIN_HANDLED
#include <iostream>
#include <c:/code/visuals/SDL2-2.0.5/include/SDL.h>
#include "network.hpp"
#include <thread>
#include "CivetServer.h"

class Visuals : public CivetHandler {
public:
    Visuals();
    int main(int argc, char* argv[]);

private:
    void send(const std::vector<unsigned int>& buffer);
    void fill(std::vector<unsigned int>& buffer);

    bool handleGet(CivetServer* server, mg_connection* conn);

    int m_width = 25;
    int m_height = 20;
    int m_fps = 60;

    Network m_network;
    CivetServer* m_server = nullptr;
};

Visuals::Visuals()
{
    // start webserver
    std::string path = "";
    std::cout << "starting webserver: " << path << std::endl;
    const char* options[] = { "document_root", path.c_str(),
                              "listening_ports", "8080",
                              "num_threads", "4",
                              nullptr
                            };
    m_server = new CivetServer(options);
    m_server->addHandler("/set", this);
}

bool Visuals::handleGet(CivetServer* server, mg_connection* conn)
{
    std::string uri = mg_get_request_info(conn)->uri;
    
    if (uri == "/set") {
        std::string message;
        std::string fps;
        if (CivetServer::getParam(conn, "fps", fps, 0)) {
            int fpsi = atoi(fps.c_str());
            if (fpsi)
                m_fps = fpsi;
            message = "fps=" + std::to_string(m_fps);
        }
        message = "HTTP/1.1 200 OK\r\n\r\n" + message;
        mg_printf(conn, message.c_str());
    }

    return true;
}

void Visuals::send(const std::vector<unsigned int>& buffer)
{
    std::vector<char> out;
    out.resize(m_width * m_height * 3);

    for (int x=0, i=0; x<m_width; x++) {
        if ((x & 1) == 0) {
            for (int y=0; y<m_height; y++, i+=3) {
                unsigned int col = buffer[y * m_width + x];
                out[i+0] = (col & 0x0000ff) >> 0;
                out[i+1] = (col & 0x00ff00) >> 8;
                out[i+2] = (col & 0xff0000) >> 16;
            }
        } else {
            for (int y=m_height-1; y>=0; y--, i+=3) {
                unsigned int col = buffer[y * m_width + x];
                out[i+0] = (col & 0x0000ff) >> 0;
                out[i+1] = (col & 0x00ff00) >> 8;
                out[i+2] = (col & 0xff0000) >> 16;
            }
        }
    }
    m_network.send(out);
}

void Visuals::fill(std::vector<unsigned int>& buffer)
{
    buffer.resize(m_width * m_height);
    for (int y=0; y<m_height; y++) {
        for (int x=0; x<m_width; x++) {

        }
    }
}

int Visuals::main(int argc, char* argv[])
{
/*    if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 ) {
    }
    Mix_Music* music = Mix_LoadMUS( "beat.wav" );*/

    std::vector<unsigned int> buffer;
    m_network.connect("192.168.17.1", 7890);
    while (true) {
        buffer.clear();
        fill(buffer);
        try {
            send(buffer);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000/m_fps));
        } catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }
    }
    
#if 0
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Visuals", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (!win){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_RendererInfo rendererInfo;
    if (SDL_GetRenderDriverInfo(0, &rendererInfo)) {
        SDL_DestroyWindow(win);
        std::cout << "SDL_GetRenderDriverInfo Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }


/*    SDL_Surface* bmp = SDL_CreateRGBSurface(0, width, height, 1, 0xff0000, 0xff00, 0xff00, 0xff000000);
    if (!bmp){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }*/


/*    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, bmp);
//    SDL_FreeSurface(bmp);
    if (!tex){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }*/
    for (unsigned int i=0; i<rendererInfo.num_texture_formats; i++)
        std::cout << "texture format: " << rendererInfo.texture_formats[i] << std::endl;
    
    SDL_Texture* tex = SDL_CreateTexture(renderer, rendererInfo.texture_formats[0], SDL_TEXTUREACCESS_STREAMING, width, height);

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT){
                quit = true;
            }        
        }

/*        if (!SDL_LockSurface(bmp)) {
            for (int y=0; y<bmp->h; y++)
                for (int x=0; x<bmp->w; x++) {
                    unsigned int* pixels = static_cast<unsigned int*>(bmp->pixels);
                    pixels[y*bmp->w+x] = 0xff0000ff;
                }
            SDL_UnlockSurface(bmp);
        }*/
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
#endif
    return 0;
}

int main(int argc, char* argv[])
{
    Visuals visuals;
    visuals.main(argc, argv);
    return 0;
}
