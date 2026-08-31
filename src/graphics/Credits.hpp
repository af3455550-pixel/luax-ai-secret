#pragma once
#include "core/Math.hpp"
#include "ParticleSystem.hpp"
#include <vector>
#include <string>
#include <functional>

namespace Fireline {

// Cinematic 3D Credits System
// Implements: vertical scroll, depth, perspective, lighting, soft shadows,
// subtle reflections, motion blur, DoF, ash particles, slow camera moves,
// destroyed forest background with embers and smoke, SKIP button, final pullback

struct CreditEntry {
    std::string role;
    std::string name;
    float importance = 0;
    Color color = {1,1,1,1};
    CreditEntry() = default;
    CreditEntry(const std::string& r, const std::string& n, float imp, Color c)
        : role(r), name(n), importance(imp), color(c) {}
    CreditEntry(const std::string& r, const std::string& n, float imp)
        : role(r), name(n), importance(imp), color({1,1,1,1}) {}
};

class CreditsSystem {
public:
    CreditsSystem() {
        buildCreditsList();
        reset();
    }

    void buildCreditsList() {
        entries.clear();
        // Title
        entries.push_back(CreditEntry{"", "FIRELINE", 2, Color{1.0f,0.4f,0.1f,1}});
        entries.push_back(CreditEntry{"", "WILDFIRE COMMAND", 2, Color{1,0.6f,0.2f,1}});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Directors
        entries.push_back(CreditEntry{"GAME DIRECTOR", "Aurélio \"Rell\" Vasconcelos", 1, Color{1,0.9f,0.7f,1}});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"CREATIVE DIRECTOR", "Marisol Duquesne", 1});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"TECHNICAL DIRECTOR", "Ivan Kolarov", 1});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Engenharia
        entries.push_back(CreditEntry{"ENGENHARIA", "", 1, Color{0.8f,0.9f,1,1}});
        entries.push_back(CreditEntry{"Lead C++ Programmer", "Tobias Reinhardt-Vale", 0});
        entries.push_back(CreditEntry{"Gameplay Programmer", "Emiko Tanabe", 0});
        entries.push_back(CreditEntry{"Engine Programmer", "Casimir Andrzejak", 0});
        entries.push_back(CreditEntry{"AI Programmer", "Nadia Fontaine-Aro", 0});
        entries.push_back(CreditEntry{"2D Rendering Engineer", "Rustam Belaïd", 0});
        entries.push_back(CreditEntry{"Physics Engineer", "Gwen Halloran", 0});
        entries.push_back(CreditEntry{"Tools Programmer", "Dmitri Sallowbrook", 0});
        entries.push_back(CreditEntry{"Performance Engineer", "Priya Ramanathan", 0});
        entries.push_back(CreditEntry{"Build Engineer", "Oskar Lindqvist", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Design
        entries.push_back(CreditEntry{"DESIGN", "", 1, Color{0.8f,0.9f,1,1}});
        entries.push_back(CreditEntry{"Level Designer", "Hugo Marcanti", 0});
        entries.push_back(CreditEntry{"Boss Designer", "Selma Okonkwo", 0});
        entries.push_back(CreditEntry{"Character Designer", "Léo Batiste-Marchand", 0});
        entries.push_back(CreditEntry{"UI/UX Designer", "Yara Solheim", 0});
        entries.push_back(CreditEntry{"Narrative Designer", "Constance \"Connie\" Ferrow", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Arte
        entries.push_back(CreditEntry{"ARTE E ANIMAÇÃO", "", 1, Color{0.8f,0.9f,1,1}});
        entries.push_back(CreditEntry{"2D Animator", "Rosalie Vantongeren", 0});
        entries.push_back(CreditEntry{"2D Animator", "Kenji Amagawa", 0});
        entries.push_back(CreditEntry{"VFX Artist", "Bruno Falqueiro", 0});
        entries.push_back(CreditEntry{"Technical Artist", "Anka Petrescu", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Audio
        entries.push_back(CreditEntry{"ÁUDIO", "", 1, Color{0.8f,0.9f,1,1}});
        entries.push_back(CreditEntry{"Sound Designer", "Théo Bramwell", 0});
        entries.push_back(CreditEntry{"Composer", "Vivienne \"Viv\" Ashcombe", 0});
        entries.push_back(CreditEntry{"Orquestração de metais", "The Copperline Eight", 0});
        entries.push_back(CreditEntry{"", "", 0});

        // Qualidade
        entries.push_back(CreditEntry{"QUALIDADE", "", 1, Color{0.8f,0.9f,1,1}});
        entries.push_back(CreditEntry{"QA Engineer", "Samir Oyelaran", 0});
        entries.push_back(CreditEntry{"QA Engineer", "Iolanda Crest", 0});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});

        entries.push_back(CreditEntry{"", "OBRIGADO POR JOGAR", 2, Color{1,0.5f,0.2f,1}});
        entries.push_back(CreditEntry{"", "FIRELINE: WILDFIRE COMMAND", 1, Color{0.7f,0.7f,0.7f,1}});
        entries.push_back(CreditEntry{"", "", 0});
        entries.push_back(CreditEntry{"", "", 0});
    }

    void reset() {
        scrollY = -15.0f; // start below screen
        time = 0;
        finished = false;
        fadeAlpha = 0;
        phase = Phase::FADE_IN;
        cameraPos = Vec3{0,2,-15};
        cameraTarget = Vec3{0,2,0};
        finalPullbackProgress = 0;
        ashSystem = ParticleSystem(2000);
    }

    enum class Phase {
        FADE_IN,
        SCROLLING,
        FINAL_PULLBACK,
        FADE_OUT,
        DONE
    };

    void update(float dt, bool skipPressed=false) {
        if(skipPressed && phase!=Phase::FADE_OUT && phase!=Phase::DONE) {
            phase = Phase::FADE_OUT;
        }

        time += dt;

        switch(phase){
            case Phase::FADE_IN:
                fadeAlpha = std::min(1.0f, fadeAlpha + dt*0.5f);
                if(fadeAlpha>=1.0f) phase = Phase::SCROLLING;
                break;
            case Phase::SCROLLING:
                scrollY += dt * scrollSpeed; // vertical scroll up
                // Camera slow moves
                cameraPos.x = std::sin(time*0.1f)*2.0f;
                cameraPos.y = 2.0f + std::sin(time*0.07f)*0.5f;
                cameraTarget.y = scrollY*0.1f;
                // Emit ash
                if((int)(time*10)%3==0) {
                    ashSystem.emitAsh(Vec3{randRange(-20,20), randRange(5,15), randRange(-10,10)}, Vec3{0.5f,0,0}, 3);
                }
                // Ember background
                if(randFloat()<0.05f) {
                    ashSystem.emitEmbers(Vec3{randRange(-15,15), randRange(0,5), randRange(-5,15)}, 1);
                }
                ashSystem.update(dt, Vec3{0.3f,0.2f,0});

                if(scrollY > entries.size()*lineHeight + 20) {
                    phase = Phase::FINAL_PULLBACK;
                }
                break;
            case Phase::FINAL_PULLBACK:
                finalPullbackProgress += dt*0.2f;
                // Camera pulls back slowly showing huge partially burned forest while smoke disappears
                cameraPos = Vec3::lerp(Vec3{0,2,-15}, Vec3{0,30,-80}, finalPullbackProgress);
                cameraTarget = Vec3::lerp(Vec3{0,5,0}, Vec3{0,0,20}, finalPullbackProgress);
                // Fade smoke
                smokeDensity = lerp(smokeDensity, 0.0f, dt*0.3f);
                if(finalPullbackProgress>=1.0f) {
                    phase = Phase::FADE_OUT;
                }
                ashSystem.update(dt, Vec3{0,0,0});
                break;
            case Phase::FADE_OUT:
                fadeAlpha = std::max(0.0f, fadeAlpha - dt*0.8f);
                if(fadeAlpha<=0.0f) {
                    phase = Phase::DONE;
                    finished = true;
                }
                break;
            case Phase::DONE:
                finished = true;
                break;
        }
    }

    // 3D Text rendering data for each entry - with depth, perspective, lighting, shadows, reflections, motion blur, DoF
    struct RenderText3D {
        Vec3 worldPos;
        std::string text;
        float scale;
        Color color;
        float depth; // for DoF
        float shadowSoftness;
        float reflection;
        float motionBlur;
        Mat4 transform;
    };

    std::vector<RenderText3D> getVisibleTexts(float screenHeight=24.0f) const {
        std::vector<RenderText3D> visible;
        for(size_t i=0;i<entries.size();i++){
            float y = (float)i*lineHeight - scrollY;
            // Only visible on screen
            if(y < -10 || y > screenHeight+10) continue;

            const auto& e = entries[i];
            if(e.role.empty() && e.name.empty()) continue;

            // Role - smaller, above name
            if(!e.role.empty()) {
                RenderText3D rt;
                rt.text = e.role;
                rt.worldPos = Vec3{0, y+0.3f, -5.0f - (e.importance*2.0f)}; // depth based on importance
                rt.scale = (e.importance>=1)?1.2f:0.8f;
                rt.color = Color{0.8f,0.8f,0.8f, fadeAlpha};
                if(e.importance>=1) rt.color = Color{0.9f,0.85f,0.6f, fadeAlpha};
                rt.depth = std::abs(rt.worldPos.z);
                rt.shadowSoftness = 0.7f;
                rt.reflection = 0.15f;
                rt.motionBlur = 0.05f;
                // Perspective transform
                float perspective = 1.0f / (1.0f + rt.depth*0.05f);
                rt.transform = Mat4::Translation(rt.worldPos) * Mat4::Scale(Vec3{perspective,perspective,1});
                visible.push_back(rt);
            }
            if(!e.name.empty()) {
                RenderText3D rt;
                rt.text = e.name;
                rt.worldPos = Vec3{0, y, -5.0f - (e.importance*1.5f)};
                rt.scale = (e.importance==2)?2.0f : (e.importance==1)?1.3f : 1.0f;
                rt.color = e.color;
                rt.color.a *= fadeAlpha;
                // Lighting - subtle warm from fire below
                float lightFactor = 0.7f + std::sin(time + i)*0.15f;
                rt.color.r *= lightFactor;
                rt.color.g *= lightFactor*0.9f;
                rt.depth = std::abs(rt.worldPos.z);
                rt.shadowSoftness = 0.5f + e.importance*0.2f;
                rt.reflection = 0.1f + e.importance*0.05f;
                rt.motionBlur = 0.03f + scrollSpeed*0.02f;
                float perspective = 1.0f / (1.0f + rt.depth*0.05f);
                rt.transform = Mat4::Translation(rt.worldPos) * Mat4::Scale(Vec3{rt.scale*perspective, rt.scale*perspective, 1});
                visible.push_back(rt);
            }
        }
        return visible;
    }

    bool isFinished() const { return finished; }
    float getFadeAlpha() const { return fadeAlpha; }

    // For console renderer - produce ASCII frame
    std::string renderConsoleFrame(int width=80, int height=24) const {
        std::string frame;
        // Background - destroyed forest with embers and smoke
        // Simple ASCII art
        frame += "\033[2J\033[H"; // clear
        // Top - smoke
        for(int y=0;y<height;y++){
            for(int x=0;x<width;x++){
                // Determine if this pixel is text
                bool isTextPixel = false;
                char textChar = ' ';
                Color textColor{1,1,1,1};

                float worldY = (height-1 - y) - scrollY*1.5f + height*0.5f;
                // Find closest entry
                for(size_t i=0;i<entries.size();i++){
                    float entryY = (float)i*lineHeight;
                    float dy = std::abs(worldY - entryY);
                    if(dy < 0.6f) {
                        const auto& e = entries[i];
                        std::string txt = e.name.empty()?e.role:e.name;
                        if(!txt.empty()){
                            int txtStart = (width - (int)txt.size())/2;
                            if(x>=txtStart && x<txtStart+(int)txt.size()){
                                textChar = txt[x-txtStart];
                                isTextPixel = true;
                                textColor = e.color;
                                break;
                            }
                        }
                    }
                }

                if(isTextPixel && textChar!=' ') {
                    // Apply depth fade and lighting
                    int intensity = (int)(textColor.r*5);
                    // ANSI color based on type
                    if(textColor.r>0.9f && textColor.g<0.5f) frame += "\033[38;5;202m"; // orange title
                    else if(textColor.b>0.8f) frame += "\033[38;5;117m"; // blue section
                    else frame += "\033[38;5;255m";
                    frame += textChar;
                    frame += "\033[0m";
                } else {
                    // Background - forest, smoke, embers
                    float noise = std::sin(x*0.1f + time)*std::cos(y*0.15f + time*0.5f);
                    if(y > height-5) {
                        // Ground - burned forest
                        if(randFloat()<0.02f) frame += "\033[38;5;52m^"; // burned tree
                        else if(randFloat()<0.01f) frame += "\033[38;5;202m*"; // ember
                        else frame += "\033[38;5;235m.";
                    } else if(y < 4 && noise>0.5f) {
                        frame += "\033[38;5;240m~"; // smoke
                    } else {
                        // Ash particles
                        bool isAsh = false;
                        for(auto& p : ashSystem.particles) {
                            int px = (int)(p.position.x + width/2);
                            int py = (int)(p.position.z + height/2);
                            if(px==x && py==y) { isAsh=true; break; }
                        }
                        if(isAsh) frame += "\033[38;5;245m.";
                        else frame += " ";
                    }
                    frame += "\033[0m";
                }
            }
            frame += "\n";
        }
        // UI - Skip button
        frame += "\n\033[38;5;240m[Pressione ESC ou Q para SKIP CREDITS]\033[0m  Alpha: " + std::to_string((int)(fadeAlpha*100)) + "%  Scroll: " + std::to_string((int)scrollY);
        if(phase==Phase::FINAL_PULLBACK) {
            frame += "\n\033[38;5;202m>> Câmera afastando - floresta parcialmente queimada - fumo desaparecendo <<\033[0m";
        }
        return frame;
    }

    std::vector<CreditEntry> entries;
    float scrollY = 0;
    float scrollSpeed = 1.2f;
    float lineHeight = 1.8f;
    float time = 0;
    float fadeAlpha = 0;
    Phase phase = Phase::FADE_IN;
    bool finished = false;

    Vec3 cameraPos;
    Vec3 cameraTarget;
    float finalPullbackProgress = 0;
    float smokeDensity = 0.8f;

    ParticleSystem ashSystem{500};

private:
    float randFloat() const { return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi) const { return lo + (hi-lo)*randFloat(); }
    float lerp(float a,float b,float t) const { return a + (b-a)*t; }
};

} // namespace Fireline
