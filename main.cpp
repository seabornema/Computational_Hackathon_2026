#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>



double dmod(double a,double b) {
    if(a>b){
        return a-b;
    }else if(a<0){
        return a+b;
    }else{
    return a;
    }
}

using namespace std; 
  

const int win_size = 800;
const double dt = 0.02;

const double box_size = 4.0;

const double velo_range = 0;
const double pos_range = 0;
const int n =200;


struct Vector2 { double x, y; };

struct Particle {
    Vector2 pos, velo;
    sf::Color col = sf::Color::Blue;
    void evolve() {
      pos.x+= velo.x*dt;
      pos.y += velo.y*dt;
    }
};

double random_range(double a, double b) {
    return a + (b - a) * (double(rand()) / RAND_MAX);
}


std::vector<Particle> particles;

void initialize_scene(int n,vector<Particle>& particles,double pos_range,double velo_range) {
    for(int i = 0; i < n; i++) {

    double xpos= random_range(-pos_range,pos_range);
    double ypos = random_range(-pos_range,pos_range);
    double xvelo = random_range(-velo_range,velo_range);
    double yvelo = random_range(-velo_range,velo_range);
    particles.push_back({{xpos, ypos},{xvelo, yvelo}});
    }
}



void spawn_particle(int n,vector<Particle>& particles,Vector2 point,Vector2 pos_range,Vector2 velo_range) {
    for(int i = 0; i < n; i++) {
    double xpos= point.x + random_range(-pos_range.x,pos_range.x);
    double ypos = point.y + random_range(-pos_range.y,pos_range.y);
    double xvelo = random_range(-velo_range.x,velo_range.x);
    double yvelo = random_range(-velo_range.y,velo_range.y);
    particles.push_back({{xpos, ypos},{xvelo, yvelo}});
    }
}


void full_sweep(int n) {
    for (Particle &p : particles) p.evolve();
}

int main() {
    srand(time(0));
    initialize_scene(n,particles,box_size,0.1);
    sf::RenderWindow window(sf::VideoMode(win_size, win_size), "Particles");

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e))
            if (e.type == sf::Event::Closed) window.close();

        full_sweep(n);
        window.clear(sf::Color::Black);
        for (Particle &p : particles) {
            sf::CircleShape circle(2);
            circle.setPosition(p.pos.x * (win_size / box_size),
                               p.pos.y * (win_size / box_size));
            window.draw(circle);
           
        }
  
        spawn_particle(1,particles,
            {1.0,1.0},  //spawnpoint
            {0.1,0.1},  //spawn range x and y
            {1.0,1.0}  //velocity range x and y
            );


        window.display();
        sf::sleep(sf::milliseconds(1));
    }
}

