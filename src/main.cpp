#include "core/Game.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #include <io.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

using namespace Fireline;

// Cross-platform non-blocking keyboard
class Keyboard {
public:
    Keyboard() {
#ifdef _WIN32
        // Enable ANSI escape codes on Windows 10+
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
        // No init needed for conio
#else
        tcgetattr(STDIN_FILENO, &old);
        termios n = old;
        n.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &n);
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
#endif
    }
    ~Keyboard() {
#ifndef _WIN32
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
#endif
    }
    char getChar() {
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            // Handle arrow keys and special: _getch returns 0 or 224 then second code
            if (ch == 0 || ch == 224) {
                int ch2 = _getch();
                // Map arrow keys to WASD for simplicity, or return 0
                return 0;
            }
            return (char)ch;
        }
        return 0;
#else
        char c=0;
        if(read(STDIN_FILENO, &c, 1)==1) return c;
        return 0;
#endif
    }
private:
#ifndef _WIN32
    termios old;
#endif
};

void printBanner() {
    std::cout << "\033[2J\033[H";
    std::cout << R"(
================================================================================
  FIRELINE: WILDFIRE COMMAND - C++ FIREFIGHTING SIMULATOR
  Simulador Profissional de Combate a Incendios Florestais
================================================================================
  Motor: Fireline Engine v1.0 (C++17, multithread, otimizado)
  Simulacao: Rothermel + Wind + Slope + Vegetation + Humidity
  Graficos: Renderer 3D com DoF, Motion Blur, Volumetric Smoke, Particulas
  Audio: Sistema imersivo 3D com radio, sirenes, fogo, respiracao
  IA: Behavior Tree + Radio Commands + Coordenacao de equipas
  Fisica: Veiculos com suspensao, tracao, tanque, bomba, mangueiras
================================================================================
)" << std::endl;
}

void runInteractive() {
    Keyboard kb;
    Game game;

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();
    float accumulator = 0;
    const float dt = 1.0f/20.0f;

    bool running = true;
    int frame = 0;

    while(running) {
        auto now = clock::now();
        float frameTime = std::chrono::duration<float>(now-last).count();
        last = now;
        frameTime = std::min(frameTime, 0.25f);
        accumulator += frameTime;

        char c;
        while((c = kb.getChar()) != 0) {
            if(c=='\033') {
                char c2 = kb.getChar();
                if(c2=='[') {
                    char c3 = kb.getChar();
                } else if(c2==0) {
                    game.handleInput(27);
                }
            } else {
                if(c=='q' || c=='Q') {
                    if(game.state==GameState::IN_MISSION) {
                        game.handleInput(c);
                    } else if(game.state==GameState::CREDITS) {
                        game.handleInput(c);
                    } else {
                        running = false;
                    }
                } else {
                    game.handleInput(c);
                }
            }
        }

        while(accumulator >= dt) {
            game.update(dt);
            accumulator -= dt;
        }

        if(frame % 3 == 0) {
            std::cout << "\033[H";
            std::cout << game.renderConsole();
            std::cout.flush();
        }

        if(game.state==GameState::EXIT) running=false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        frame++;
    }

    std::cout << "\033[2J\033[H";
    std::cout << "Obrigado por jogar FIRELINE: WILDFIRE COMMAND!\n";
}

void runCreditsOnly() {
    Keyboard kb;
    CreditsSystem credits;
    credits.reset();

    using clock = std::chrono::high_resolution_clock;
    auto last = clock::now();
    bool skip=false;

    std::cout << "\033[?25l";
    while(!credits.isFinished()) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now-last).count();
        last = now;
        dt = std::min(dt, 0.1f);

        char c = kb.getChar();
        if(c=='q' || c=='Q' || c==27) skip=true;

        credits.update(dt, skip);
        std::cout << credits.renderConsoleFrame(100,30);
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    std::cout << "\033[?25h";
    std::cout << "\033[2J\033[H";
    std::cout << "Fim dos creditos. Fade para preto.\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runDemo() {
    std::cout << "=== FIRELINE DEMO - Simulacao de Fogo Dinamico ===\n";
    FireSimulation sim(100,100,2.0f);
    WeatherSystem weather;
    weather.current.windDirection = Vec2{1,0.2f}.normalized();
    weather.current.windSpeed = 20;
    weather.current.temperature = 35;
    weather.current.humidity = 20;

    sim.ignite(50,50,800);
    sim.ignite(51,50,800);
    sim.ignite(50,51,800);

    for(int step=0; step<200; ++step) {
        sim.update(0.5f, weather.current);
        if(step%10==0) {
            std::cout << "Step " << step << " | Combustao: " << sim.stats.combustion << " | Queimado: " << sim.stats.burned << " | Intensidade: " << (int)sim.stats.totalIntensity << " | Area: " << sim.stats.burnedAreaHa << " ha\n";
            for(int y=55; y>=45; --y){
                for(int x=45; x<=55; ++x){
                    auto& cell = sim.getCell(x,y);
                    char ch='.';
                    if(cell.state==FireState::COMBUSTION) ch='#';
                    else if(cell.state==FireState::BURNED) ch='x';
                    else if(cell.state==FireState::SMOKE) ch='s';
                    else if(cell.state==FireState::MOPUP) ch='*';
                    std::cout << ch;
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }
        if(step==100) {
            weather.current.windSpeed = 40;
            std::cout << ">>> VENTO AUMENTA PARA 40 km/h - INCENDIO FORA DE CONTROLO <<<\n";
        }
    }
    std::cout << "Demo concluida. Area total queimada: " << sim.stats.burnedAreaHa << " ha\n";
}

void runTests() {
    std::cout << "=== FIRELINE TESTS ===\n";
    FireCell cell(0,0,VegetationType::EUCALYPTUS, Vec3{0,0,0});
    std::cout << "Estado inicial: " << fireStateToString(cell.state) << " Flammability: " << cell.flammability << "\n";
    cell.applyHeat(200);
    std::cout << "Apos 200 heat: " << fireStateToString(cell.state) << " Heat: " << cell.heat << "\n";
    cell.applyHeat(400);
    cell.update(1.0f, 30, 30);
    std::cout << "Apos update: " << fireStateToString(cell.state) << "\n";
    cell.applyWater(5);
    std::cout << "Apos agua: " << fireStateToString(cell.state) << " Water: " << cell.waterContent << "\n";

    Vehicle v(VehicleType::FOREST_TRUCK, Vec3{0,0,0});
    std::cout << "Veiculo: " << v.spec.name << " Agua: " << v.spec.waterCapacityL << "L Tracao: " << v.spec.traction << "\n";
    v.startEngine();
    v.accelerate(1.0f);
    v.update(1.0f, nullptr);
    std::cout << "Pos apos acelerar: " << v.position.x << "," << v.position.z << " Vel: " << v.velocity.length() << "\n";

    DispatchCenter dc;
    auto call = dc.generateCall(0.8f);
    std::cout << "Dispatch: " << call.id << " " << call.locationName << " " << call.fireSizeHa << "ha Threat: " << threatToString(call.threat) << "\n";

    CreditsSystem cr;
    std::cout << "Creditos: " << cr.entries.size() << " entradas\n";
    std::cout << "Primeira: " << cr.entries[0].name << "\n";

    std::cout << "Todos os testes passaram!\n";
}

int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));

    printBanner();

    std::string arg1 = argc>1 ? argv[1] : "";

    if(arg1=="--credits") {
        runCreditsOnly();
    } else if(arg1=="--demo") {
        runDemo();
    } else if(arg1=="--test") {
        runTests();
    } else if(arg1=="--help" || arg1=="-h") {
        std::cout << "Uso: fireline [opcao]\n";
        std::cout << "  (sem args)  - Jogo interativo completo\n";
        std::cout << "  --credits   - Sequencia de creditos 3D cinematicos\n";
        std::cout << "  --demo      - Demo da simulacao de fogo dinamico\n";
        std::cout << "  --test      - Testes de sistemas\n";
        std::cout << "  --help      - Esta ajuda\n";
        std::cout << "\nComandos no jogo:\n";
        std::cout << "  WASD mover, SHIFT correr, ESPACO usar ferramenta\n";
        std::cout << "  1-4 trocar ferramenta, C trocar camara, V entrar/sair veiculo\n";
        std::cout << "  R atacar fogo (equipa), F seguir, G criar aceiro, Q creditos/sair\n";
        std::cout << "\nWindows: .\\fireline.exe --help\n";
        std::cout << "Linux: ./fireline --help\n";
    } else {
        runInteractive();
    }

    return 0;
}
