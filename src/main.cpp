#include <cmath>
#include <iostream>

#include "open_sans_semibold.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

constexpr int TRUE_MAX_ITERS = 16384;
constexpr int INIT_ITERS = 64;

int main() {
    // OpenGL settings we request for:
    auto start = std::chrono::steady_clock::now();
    sf::ContextSettings settings;
    settings.depthBits = 24;   // 24-bit depth buffer
    settings.stencilBits = 8;  // 8-bit stencil buffer
    // Request OpenGL 2.1 for compatibility with both SFML/Graphics AND macOS:
    //  - SFML's Graphics module implementation relies on legacy fixed-function pipeline features
    //     of OpenGL, which means only up to OpenGL 2.1. Any newer version requires Compatibility
    //     profile (i.e., maintaining legacy features).
    //  - macOS has deprecated OpenGL support, meaning for anything newer than 2.1 (i.e., 3.0 and
    //      newer), only Core profile is supported, i.e., REMOVING legacy fixed-function features.
    // As a result, if you want to both support macOS AND use SFML's Graphics features (e.g.,
    // sf::RenderWindow), you can use at latest OpenGL 2.1, which is a nightmare.
    settings.majorVersion = 2;
    settings.minorVersion = 1;
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Shaders", sf::State::Windowed, settings);
    // Query the settings we actually received (may be different):
    const auto& actualSettings = window.getSettings();
    std::cout << "OpenGL version " << actualSettings.majorVersion << "."
              << actualSettings.minorVersion << " flags: " << actualSettings.attributeFlags << '\n';
    // Fullscreen triangle, as described in the prep.
    sf::VertexArray fullscreenTriangle(sf::PrimitiveType::Triangles, 3);
    fullscreenTriangle[0].position = {-1, -1};  // bottom-left
    fullscreenTriangle[1].position = {3, -1};   // bottom-right
    fullscreenTriangle[2].position = {-1, 3};   // top-left
    fullscreenTriangle[0].texCoords = {0, 0};
    fullscreenTriangle[1].texCoords = {2, 0};
    fullscreenTriangle[2].texCoords = {0, 2};
    // load vertex and fragment shaders:
    sf::Shader shader(std::filesystem::path{"main.vert"}, std::filesystem::path{"main.frag"});
    // ... other shaders can go here...
    sf::Shader shaderBack(std::filesystem::path{"main.vert"}, std::filesystem::path{"copy.frag"});

    sf::RenderTexture renderTextureBack({1280, 720});
    sf::RenderTexture renderTextureFront({1280, 720});
    renderTextureFront.setSmooth(true);
    renderTextureBack.setSmooth(true);
    sf::RenderTexture* front = &renderTextureFront;
    sf::RenderTexture* back  = &renderTextureBack;


    sf::Font openSansFont(EmbeddedFonts::OpenSans_SemiBold_ttf,
                          EmbeddedFonts::OpenSans_SemiBold_ttf_len);
    sf::Text cText(openSansFont);
    sf::Text cTextShadow(openSansFont);
    cText.setCharacterSize(14);
    cText.setFillColor(sf::Color::White);
    cText.setPosition({10, 10});
    cTextShadow.setCharacterSize(14);
    cTextShadow.setFillColor(sf::Color(0, 0, 0, 130));
    cTextShadow.setPosition({10, 10});
    cTextShadow.setOutlineColor(sf::Color(0, 0, 0, 130));
    cTextShadow.setOutlineThickness(3);
    sf::Vector2f c{-0.039, 0.68};
    sf::Vector2f prevC{-0.039, 0.68};

    float worldUnitsPerPixel = 1.f / window.getSize().y;

    float prevWorldUnitsPerPixel = 1.f / window.getSize().y;

    float prevCamWorldUnitsPerPixel = 1.f / window.getSize().y;

    sf::Vector2f camCenterWorld{0.f, 0.f};
    sf::Vector2f prevCamCenterWorld{0.f, 0.f};
    int maxIters = INIT_ITERS;

    bool firstFrame = true;

    int i = -1;
    while (window.isOpen()) {
        ++i;
        bool moved = false;

        while (auto e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                return 0;
            } else if (const auto* keyPressed = e->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scan::Q) {
                    return 0;
                } else if (keyPressed->scancode == sf::Keyboard::Scan::Escape) {
                    return 0;
                }
            } else if (const auto* scrolled = e->getIf<sf::Event::MouseWheelScrolled>()) {
                worldUnitsPerPixel *= std::pow(1.01, scrolled->delta);
                moved = true;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W)) {
            c.y += 0.001;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S)) {
            
            c.y -= 0.001;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
            c.x -= 0.001;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {

            c.x += 0.001;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up)) {
            camCenterWorld.y += 2 * worldUnitsPerPixel;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down)){
            camCenterWorld.y -= 2 * worldUnitsPerPixel;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left)) {
            camCenterWorld.x -= 2 * worldUnitsPerPixel;
            moved = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) {
            camCenterWorld.x += 2 * worldUnitsPerPixel;
            moved = true;
        }
        if (moved) {
            maxIters = INIT_ITERS;
        } else {
            maxIters = std::min(maxIters * 2, TRUE_MAX_ITERS);
        }
        // TODO set shader uniforms here...
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<float> diff = end - start;
        sf::Vector2f windowSize{1280.f, 720.f};

        shader.setUniform("c",c);
        shader.setUniform("firstFrame",firstFrame);

        shader.setUniform("MAX_ITERS",maxIters);
        shader.setUniform("TRUE_MAX_ITER",TRUE_MAX_ITERS);
        shader.setUniform("windowSizePixels",windowSize);

        shader.setUniform("camCenterWorld",camCenterWorld);
        shader.setUniform("camWorldUnitsPerPixel",worldUnitsPerPixel);

        //Previous Uniforms:
        shader.setUniform("prevC",prevC);
        shader.setUniform("prevCamCenterWorld",prevCamCenterWorld);
        shader.setUniform("prevCamWorldUnitsPerPixel",prevCamWorldUnitsPerPixel);



        shader.setUniform("prevFrame", renderTextureBack.getTexture());

        renderTextureFront.clear();
        renderTextureFront.draw(fullscreenTriangle, &shader);
        renderTextureFront.display();

        shaderBack.setUniform("toCopy", renderTextureFront.getTexture());
        window.clear();
        window.draw(fullscreenTriangle, &shader);

        // this might change as you finish more parts!
        cText.setString("c = (" + std::to_string(c.x) + ", " + std::to_string(c.y) + ")");
        cTextShadow.setString("c = (" + std::to_string(c.x) + ", " + std::to_string(c.y) + ")");
        window.draw(cTextShadow);
        window.draw(cText);
        window.display();

        std::swap(front, back);
        // TODO (reprojection)... update "old" states
        firstFrame = false;
        prevCamCenterWorld = camCenterWorld;
        prevC = c;
        prevCamWorldUnitsPerPixel  = worldUnitsPerPixel;


    }
    return 0;
}
