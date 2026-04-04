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

const double world_size = 4.0;

const double velo_range = 0;
const double pos_range = 0;
const double h = 1.0; // interaction radius
//const int n =200;


struct Vector2 { double x, y; };

struct Particle {
    Vector2 pos, velo,force;
    sf::Color col = sf::Color::Blue;
    void evolve() {
      if(pos.x > 3.0 || pos.x < 1.0){
        if(pos.y < 3.0) velo.x *= -1.0;
      }
      if(pos.y < 1.0) {
        velo.y *= -1.0;
      }

      velo.x += force.x*dt;
      velo.y += force.y*dt;

      pos.x+= velo.x*dt;
      pos.y += velo.y*dt;
    }
};


vector<Particle> particles;
int cell_number = (floor(world_size/h)+1);
const vector<vector<Particle>> empty_cells(cell_number*cell_number,vector<Particle>(0));
vector<vector<Particle>> cells(cell_number,vector<Particle>(0));


int cell_index(Vector2 position) {
  return (cell_number)*floor(position.y/h) + floor(position.x/h); 
}


void fill_cells() {
  cells = empty_cells;
  for(Particle &p : particles) {
      int ci = cell_index(p.pos);
      if(ci >= cell_number*cell_number || ci <0) continue;
    cells[ci].push_back(p);
  }
}




vector<Particle> neighbors_from_index(int cell_index) {
  vector<Particle> temp;
  for(int i = 0;i < 9;i++) {
  int j = (i%3);
  int k = cell_number*((i/3));
  if(k+j < 0 || k+j >= cell_number*cell_number) continue;
  vector<Particle> current = cells[k+j];
  temp.insert(temp.end(),current.begin(),current.end());
  }
  return temp;
}
double k =0.001;
Vector2 force_between_particles(Particle& p1,Particle& p2){
  double delx = p1.pos.x - p2.pos.x;
  double dely = p1.pos.y - p2.pos.y;
  double mag = (delx*delx + dely*dely);
  if(mag < 0.1) return {0,0};
 return {k*delx/mag,k*dely/mag};
}


double random_range(double a, double b) {
    return a + (b - a) * (double(rand()) / RAND_MAX);
}



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
    for (Particle &p : particles) {
      int ci = cell_index(p.pos);
      if(ci >= cell_number*cell_number || ci <0) continue;
      vector<Particle> neighbors = neighbors_from_index(ci);
     for(Particle &p2 : neighbors) {
       Vector2 current_force = force_between_particles(p,p2);
       p.force.x += current_force.x;
       p.force.y += current_force.y;
     }
    }

    for (Particle &p : particles) {
      p.evolve();
      p.force = {0,0};
    }
}

vector<sf::Vector2f> rocket_body_points = {{1.0*(win_size / world_size),3.0*(win_size / world_size)},{1.0*(win_size / world_size),1.0*(win_size / world_size)},{3.0*(win_size / world_size),1.0*(win_size / world_size)},{3.0*(win_size / world_size),3.0*(win_size / world_size)}};

int main() {
    srand(time(0));
    sf::RenderWindow window(sf::VideoMode(win_size, win_size), "Particles");
    fill_cells();
    


    sf::VertexArray path(sf::LineStrip, 4);
    for (int i = 0; i < 4; ++i) {
        path[i].position  = rocket_body_points[i];
        path[i].color     = sf::Color::White;
    }






    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e))
            if (e.type == sf::Event::Closed) window.close();
        fill_cells();
        full_sweep(particles.size());
        window.clear(sf::Color::Black);


        window.draw(path);

        for (Particle &p : particles) {
            sf::CircleShape circle(2);
            circle.setPosition(p.pos.x * (win_size / world_size),
                               p.pos.y * (win_size / world_size));
            window.draw(circle);
           
        }
  
        spawn_particle(1,particles,
            {2.0,2.0},  //spawnpoint
            {0.5,0.5},  //spawn range x and y
            {0.0,0.0}  //velocity range x and y
            );


        window.display();
        sf::sleep(sf::milliseconds(1));
    }
}

