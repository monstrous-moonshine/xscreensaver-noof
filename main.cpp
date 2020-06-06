#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

#define N_SHAPES 7

class hsv_color {
    float hsv[3] = {0, 0, 0};
public:
    hsv_color& operator += (hsv_color&& that);
    hsv_color  operator * (float s);
    void clip_hue();
    void clip_saturation() { hsv[1] = hsv[1] < 0 ? 0 : (hsv[1] > 1 ? 1 : hsv[1]); }
    void clip_value()      { hsv[2] = hsv[2] < 0 ? 0 : (hsv[2] > 1 ? 1 : hsv[2]); }
    float& H() { return hsv[0]; }
    float& S() { return hsv[1]; }
    float& V() { return hsv[2]; }
};

hsv_color& hsv_color::operator += (hsv_color&& that) {
    for (int i = 0; i < 3; i++) hsv[i] += that.hsv[i];
    return *this;
}

hsv_color hsv_color::operator * (float s) {
    hsv_color result = *this;
    for (int i = 0; i < 3; i++) result.hsv[i] *= s;
    return result;
}

void hsv_color::clip_hue() {
    while (hsv[0] < 0)    hsv[0] += 360;
    while (hsv[0] >= 360) hsv[0] -= 360;
}

class vec2 {
    float arr[2] = {0, 0};
public:
    vec2& operator *= (float s);
    vec2& operator += (vec2&& that);
    vec2 operator * (float s);
    vec2 operator - (vec2& that);
    float& operator [] (int i);
    float norm2();
};

vec2& vec2::operator *= (float s) {
    arr[0] *= s;
    arr[1] *= s;
    return *this;
}

vec2& vec2::operator += (vec2&& that) {
    arr[0] += that.arr[0];
    arr[1] += that.arr[1];
    return *this;
}

vec2 vec2::operator * (float s) {
    vec2 result = *this;
    result *= s;
    return result;
}

vec2 vec2::operator - (vec2& that) {
    vec2 result = *this;
    result.arr[0] -= that.arr[0];
    result.arr[1] -= that.arr[1];
    return result;
}

float& vec2::operator [] (int i) {
    // don't do bounds check
    return arr[i];
}

float vec2::norm2() {
    return arr[0] * arr[0] + arr[1] * arr[1];
}

class noof {
    std::vector<vec2> pos;
    std::vector<vec2> dir;
    // std::vector<vec2> acc;
    std::vector<sf::Color> rgb;
    std::vector<hsv_color> hsv;
    std::vector<hsv_color> hpr;
    std::vector<float> ang;
    std::vector<float> spn;
    std::vector<float> sca;
    std::vector<float> geep;
    std::vector<float> peep;
    // std::vector<float> speedsq;
    std::vector<float> num_blades;

    float ht, wd;
    
    float delta = 0;
    
    float threshold = 1.6; // controls the plotting frequency

    int tko = 0;
    
    sf::RenderWindow& window;
    
    constexpr static float blade_ratio[] =
    {
        /* nblades = 2..7 */
        0.0, 0.0, 3.00000, 1.73205, 1.00000, 0.72654, 0.57735, 0.48157,
        /* 8..13 */
        0.41421, 0.36397, 0.19076, 0.29363, 0.26795, 0.24648,
        /* 14..19 */
        0.22824, 0.21256, 0.19891, 0.18693, 0.17633, 0.16687,
    };

    void init_shape(int i);
    void draw_leaf(int l);
    void motion_update(int t, float delta);
    void color_update(int i, float delta);
    void gravity(float fx);
    bool out_of_bound_x(int t);
    bool out_of_bound_y(int t);
public:
    noof(sf::RenderWindow& window);
    void draw_noof(float delta);
};

noof::noof(sf::RenderWindow& window) :
    pos(N_SHAPES),
    dir(N_SHAPES),
    // acc(N_SHAPES),
    rgb(N_SHAPES),
    hsv(N_SHAPES),
    hpr(N_SHAPES),
    ang(N_SHAPES),
    spn(N_SHAPES),
    sca(N_SHAPES),
    geep(N_SHAPES),
    peep(N_SHAPES),
    // speedsq(N_SHAPES),
    num_blades(N_SHAPES),
    window(window)
{
    sf::Vector2u window_size = window.getSize();
    wd = window_size.x;
    ht = window_size.y;
    std::cout << "width: " << wd << ", height: " << ht << "\n";
    for (int i = 0; i < N_SHAPES; i++) {
        init_shape(i);
    }
}

bool noof::out_of_bound_x(int t) {
    return pos[t][0] < -wd * (1 + 2 * sca[t]) / ht && dir[t][0] < 0 ||
           pos[t][0] >  wd * (1 + 2 * sca[t]) / ht && dir[t][0] > 0;
}

bool noof::out_of_bound_y(int t) {
    return pos[t][1] < -(1 + 2 * sca[t]) && dir[t][1] < 0 ||
           pos[t][1] >  (1 + 2 * sca[t]) && dir[t][1] > 0;
}

void noof::init_shape(int i) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<float> distribution(0, 1);
    auto rand_unif = std::bind(distribution, generator);
    
    for (int k = 0; k < 2; k++) {
        pos[i][k] = (rand_unif() - 0.5) * 2;;
        dir[i][k] = (rand_unif() - 0.5) * 0.05;
        // acc[i][k] = (rand_unif() - 0.5) * 0.0002;
    }
    // speedsq[i] = dir[i].norm2();
    num_blades[i] = 2 + (int)(rand_unif() * 17);
    std::cerr << "leaf[" << i << "]: " << num_blades[i] << " blades.\n";
    ang[i] = rand_unif();
    spn[i] = (rand_unif() - 0.5) * 40.0 / (10 + num_blades[i]);
    sca[i] = (rand_unif() * 0.1 + 0.08);
    dir[i] *= sca[i];
    hsv[i].H() = rand_unif() * 360;
    hsv[i].S() = rand_unif() * 0.6 + 0.4;
    hsv[i].V() = rand_unif() * 0.7 + 0.3;
    hpr[i].H() = rand_unif() * 360 * 0.005;
    hpr[i].S() = rand_unif() * 0.03;
    hpr[i].V() = rand_unif() * 0.02;
    geep[i] = 0;
    peep[i] = 0.01 + rand_unif() * 0.2;
}

void noof::draw_leaf(int l) {
    float wobble;
    int blades = num_blades[l];
    float x = fabs(0.15 * cos(geep[l] * M_PI / 180.0) + 0.149 * cos(geep[l] * 5.12 * M_PI / 180.0));
    float y = fabs(0.10 * sin(geep[l] * M_PI / 180.0) + 0.099 * sin(geep[l] * 5.12 * M_PI / 180.0));
    
    if (y < 1e-3 && x > 2e-6 && ((tko & 0x1) == 0)) {
        init_shape(l);      /* let it become reborn as something else */
        tko++;
        std::cerr << "Leaf " << l << " reborn.\n";
        return;
    }
    {
        float w1 = sin(geep[l] * 15.3 * M_PI / 180.0);
        wobble = 3.0 + 2.0 * sin(geep[l] * 0.4 * M_PI / 180.0) + 3.94261 * w1;
    }

    // keep blade thin
    if (y > x * blade_ratio[blades]) y = x * blade_ratio[blades];

    for (int b = 0; b < blades; b++) {
        sf::Transform t;
        t.translate(wd / 2, ht / 2);
        t.scale(ht / 2, -ht / 2); // maybe make the second scale factor negative
        t.translate(pos[l][0], pos[l][1]);
        t.rotate(ang[l] + b * (360.0 / blades));
        t.scale(wobble * sca[l], wobble * sca[l]);
        
        sf::VertexArray leaf_poly(sf::TriangleStrip, 4);
        leaf_poly[0].position = sf::Vector2f(x * sca[l], 0);
        leaf_poly[1].position = sf::Vector2f(x, y);
        leaf_poly[2].position = sf::Vector2f(x, -y);
        leaf_poly[3].position = sf::Vector2f(0.3, 0);
        
        for (int i = 0; i < 4; i++) {
            leaf_poly[i].color = sf::Color::Black;
        }
        
        window.draw(leaf_poly, t);
        
        sf::VertexArray leaf_line(sf::LineStrip, 5);
        leaf_line[0].position = sf::Vector2f(x * sca[l], 0);
        leaf_line[1].position = sf::Vector2f(x, y);
        leaf_line[2].position = sf::Vector2f(0.3, 0);
        leaf_line[3].position = sf::Vector2f(x, -y);
        leaf_line[4].position = sf::Vector2f(x * sca[l], 0);
        
        for (int i = 0; i < 5; i++) {
            leaf_line[i].color = rgb[l];
        }
        
        window.draw(leaf_line, t);
    }
}

void noof::motion_update(int t, float delta) {
    if (out_of_bound_x(t)) dir[t][0] = -dir[t][0];
    if (out_of_bound_y(t)) dir[t][1] = -dir[t][1];
    
    pos[t] += dir[t] * delta;
    ang[t] += spn[t] * delta;
    geep[t] += peep[t] * delta;
    
    if (geep[t] > 360 * 5.0) {
        geep[t] -= 360 * 5.0;
    }
    
    if (ang[t] < 0.0) {
        ang[t] += 360.0;
    }
    
    if (ang[t] > 360.0) {
        ang[t] -= 360.0;
    }
}

void noof::color_update(int i, float delta) {
    float v = hsv[i].V();
    float s = hsv[i].S();
    float v_hpr = hpr[i].V();
    float s_hpr = hpr[i].S();
    
    if (s < 0.5 && s_hpr < 0 ||
        s > 1.0 && s_hpr > 0)
    {
        s_hpr = -s_hpr;
    }
    
    if (v < 0.4 && v_hpr < 0 ||
        v > 1.0 && v_hpr > 0)
    {
        v_hpr = -v_hpr;
    }
    
    hsv[i] += hpr[i] * delta;
    hsv[i].clip_value();
    
    std::vector<float> rgb_val(3);
    
    if (s < 0) {
        for (auto& val : rgb_val) {
            val = hsv[i].V();
        }
    } else {
        hsv[i].clip_hue();
        hsv[i].clip_saturation();
        float h = hsv[i].H() / 60;
        int hi = (int) h;
        float f = h - hi;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1 - f));
        switch (hi) {
        case 0:
            rgb_val = {v, t, p}; break;
        case 1:
            rgb_val = {q, v, p}; break;
        case 2:
            rgb_val = {p, v, t}; break;
        case 3:
            rgb_val = {p, q, v}; break;
        case 4:
            rgb_val = {t, p, v}; break;
        case 5:
            rgb_val = {v, p, q}; break;
        }
    }
    
    for (auto& val : rgb_val) val *= 255;
    
    rgb[i] = sf::Color(rgb_val[0], rgb_val[1], rgb_val[2]);
}

void noof::gravity(float fx) {
    for (int i = 0; i < N_SHAPES; i++) {
        for (int j = 0; j < i; j++) {
            float d2 = (pos[j] - pos[i]).norm2();
            if (d2 < 1e-6) d2 = 1e-5;
            if (d2 < 0.1) {
                vec2 v = pos[j] - pos[i];
                float z = 1e-8 * fx / d2;
                dir[i] += v * ( z * sca[j]);
                dir[j] += v * (-z * sca[i]);
            }
        }
    }
}

void noof::draw_noof(float delta) {
    this->delta += delta;
    gravity(-2);
    for (int i = 0; i < N_SHAPES; i++) {
        motion_update(i, delta);
        color_update(i, delta);
        if (this->delta > threshold) {
            draw_leaf(i);
        }
    }
    if (this->delta > threshold) this->delta -= threshold;
}

int main()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    
    sf::RenderWindow window(
        sf::VideoMode(800, 800),
        "Noof",
        sf::Style::Titlebar | sf::Style::Close,
        settings
    );
    
    noof my_noof(window);
    
    sf::Clock clock;
    sf::Time t1 = clock.getElapsedTime();
    
    window.clear();

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // window.clear();
        
        sf::Time t2 = clock.getElapsedTime();
        float delta = (t2 - t1).asSeconds();
        my_noof.draw_noof(delta * 40);
        t1 = t2;
        
        window.display();
    }

    return 0;
}
