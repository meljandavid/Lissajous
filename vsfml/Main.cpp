#include "imgui/imgui.h"
#include "imgui/imgui-SFML.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include<cmath>
#include<list>
#include<iostream>
#include<fstream>
#include<random>

std::string vtos(const ImVec2& v) {
    return std::to_string(v.x) + "," + std::to_string(v.y);
}

struct Snake {
    std::list<sf::Vector2f> segments;
    int TAIL = 50;
    int MAXSIZE = 100.f;
    float SPEED = 1.f;
    int VEL[2] = { 3, 2 };
    float color[3] = { .0f, 1.f, 1.f };
    sf::CircleShape shape;
    
    Snake() {
        shape.setRadius(MAXSIZE);
    }

    void setColor(float r, float g, float b) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
    }

    void setVel(int vx, int vy) {
        VEL[0] = vx;
        VEL[1] = vy;
    }

    void update(float elapsed, const sf::Vector2f& ps) {
        shape.setFillColor(sf::Color(255*color[0], 255*color[1], 255*color[2]));

        sf::Vector2f newpos = { std::sin(VEL[0] * elapsed * SPEED) * ps.x / 3 + ps.x / 2 - shape.getRadius(),
            std::sin(VEL[1] * elapsed * SPEED) * ps.y / 3 + ps.y / 2 - shape.getRadius() };

        segments.push_back(newpos);
        while (segments.size() > TAIL) {
            segments.pop_front();
        }
    }

    void draw(sf::RenderTexture& texture) {
        int idx = segments.size() - 1;
        for (auto it = segments.rbegin(); it != segments.rend(); it++) {
            float s = MAXSIZE * ((idx + 1.f) / segments.size());
            double multi = 255 * std::pow(.98f, idx);
            shape.setFillColor(sf::Color(
                color[0] * multi,
                color[1] * multi,
                color[2] * multi
            ));
            shape.setRadius(s);
            shape.setOrigin({ s,s });
            shape.setPosition(*it);
            texture.draw(shape);
            idx--;
        }
    }
};

int main() {
    sf::VideoMode videoMode = sf::VideoMode::getDesktopMode();
    // sf::RenderWindow window(videoMode, "ImGuiSFML", sf::Style::Fullscreen);
    sf::RenderWindow window(sf::VideoMode(960, 640), "Lissajous");
    window.setVerticalSyncEnabled(true);
    sf::RenderTexture texture;
    texture.create(200, 200);
    ImGui::SFML::Init(window, false);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->ClearFonts();
    io.Fonts->AddFontFromFileTTF("Roboto-Black.ttf", 15.f);
    ImGui::SFML::UpdateFontTexture();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImVec2 panelSize = window.getView().getSize();
    float elapsed = 0.f;
    float fpsdt = 0.f;
    int frames = 0;
    int FPS=0;

    std::list<Snake> snakes;
    
    std::ifstream in("snakes.txt");
    if (in) {
        std::string cmd;
        while (in >> cmd) {
            if (cmd == "NewSnake") {
                snakes.push_back(Snake());
            }
            else if (cmd == "Color") {
                in >> snakes.back().color[0] >> snakes.back().color[1] >> snakes.back().color[2];
            }
            else if (cmd == "Size") {
                in >> snakes.back().MAXSIZE;
            }
            else if (cmd == "Tail") {
                in >> snakes.back().TAIL;
            }
            else if (cmd == "Velocity") {
                in >> snakes.back().VEL[0] >> snakes.back().VEL[1];
            }
            else if (cmd == "Speed") {
                in >> snakes.back().SPEED;
            }
            else std::cout << "Unknown property found\n";
        }

        in.close();
    }
    else { // gen snakes
        Snake snake;
        snake.setColor(0.f, 1.f, 1.f);
        snake.setVel(3, 2);
        snake.SPEED = .4f;
        snakes.push_back(snake);
        snake.setColor(1.f, 0.f, 0.f);
        snake.setVel(3, 5);
        snake.MAXSIZE = 60.f;
        snake.TAIL = 60;
        snake.SPEED = .2f;
        snakes.push_back(snake);
        snake.setColor(1.f, 1.f, 0.f);
        snake.setVel(7, 2);
        snake.MAXSIZE = 40.f;
        snake.TAIL = 30;
        snake.SPEED = .1f;
        snakes.push_back(snake);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist_speed(0.1f, .5f);
    std::uniform_real_distribution<> dist_color(0.f, 1.f);
    std::uniform_int_distribution<> dist_vel(1, 10);
    std::uniform_int_distribution<> dist_size(10, 250);
    std::uniform_int_distribution<> dist_tail(5, 200);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
        }

        sf::Time deltaTime = deltaClock.restart();
        elapsed += deltaTime.asSeconds();
        ImGui::SFML::Update(window, deltaTime);

        frames++;
        fpsdt += deltaTime.asSeconds();

        if (fpsdt > 1.f) {
            FPS = frames;
            fpsdt = 0.f;
            frames = 0.f;
        }

        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        static bool dockspaceOpen = true;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close")) window.close();
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();

        ImGui::ShowDemoWindow();

        ImGui::Begin("Modify");
        if (ImGui::CollapsingHeader("Debug log")) {
            ImGui::Text(std::string("FPS:" + std::to_string(FPS)).c_str());
            ImGui::Text(vtos(panelSize).c_str());
        }
        if (ImGui::CollapsingHeader("Edit nodes", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add new")) {
                Snake snake;
                snake.setColor(dist_color(gen), dist_color(gen), dist_color(gen));
                snake.setVel(dist_vel(gen), dist_vel(gen));
                snake.MAXSIZE = dist_size(gen);
                snake.TAIL = dist_tail(gen);
                snake.SPEED = dist_speed(gen);
                snakes.push_front(snake);
            }

            ImGui::BeginChild("EditSnakes", { 0,0 }, true);
            int id = 0;
            for (auto it = snakes.begin(); it != snakes.end(); it++) {
                std::string sid = std::to_string(id);
                if (ImGui::Button(std::string("Delete this##" + sid).c_str())) {
                    snakes.erase(it++);
                    it--;
                    continue;
                }
                Snake& snake = *it;
                ImGui::ColorEdit3(std::string("Circles color##" + sid).c_str(), snake.color);
                ImGui::SliderInt(std::string("Tail Length##" + sid).c_str(), &(snake.TAIL), dist_tail.min(), dist_tail.max());
                ImGui::SliderInt(std::string("Head Size##" + sid).c_str(), &(snake.MAXSIZE), dist_size.min(), dist_tail.max());
                ImGui::SliderInt2(std::string("Velocity##" + sid).c_str(), snake.VEL, dist_vel.min(), dist_vel.max());
                ImGui::SliderFloat(std::string("Speed##" + sid).c_str(), &(snake.SPEED), .1f, 2.f);
                ImGui::Separator();
                id++;
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Render View");

        ImVec2 ps = ImGui::GetContentRegionAvail();

        if (ps.x != panelSize.x || ps.y != panelSize.y) {
            texture.create(ps.x, ps.y);
        }
        panelSize = ps;

        for (Snake& snake : snakes) {
            snake.update(elapsed, panelSize);
        }

        texture.clear(sf::Color::White);
        for (Snake& snake : snakes) {
            snake.draw(texture);
        }
        texture.display();

        ImGui::Image(texture);
        ImGui::End();
        ImGui::PopStyleVar();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    std::ofstream out("snakes.txt");
    for (const Snake& snake : snakes) {
        out << "NewSnake" << std::endl;
        out << "Color " << snake.color[0] << " " << snake.color[1] << " " << snake.color[2] << std::endl;
        out << "Size " << snake.MAXSIZE << std::endl;
        out << "Tail " << snake.TAIL << std::endl;
        out << "Velocity " << snake.VEL[0] << " " << snake.VEL[1] << std::endl;
        out << "Speed " << snake.SPEED << std::endl;
    }
    out.close();

    ImGui::SFML::Shutdown();

    return 0;
}