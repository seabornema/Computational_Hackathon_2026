#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <omp.h>


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
const double dt = 0.002;

const double world_size = 12.0;

const double velo_range = 0;
const double pos_range = 0;
const double h = 0.6; // interaction radius
const double p0 = 0.0;
const double ylimit = 12.0;


const double engine_size = 2.0;

struct Vector2 { double x, y;
  Vector2(double x,double y) : x(x), y(y) {}
Vector2& scalar_mult(double a) {
  x *= a;
  y *= a;
  return *this;
}
Vector2& add(const Vector2& input) {
  x += input.x;
  y += input.y;
  return *this;
}
};


const Vector2 engine_center(world_size/2,world_size/2);

struct Particle {
    Vector2 pos, velo,force;
    int index;
    sf::Color col = sf::Color::Blue;
    bool should_die = false;
    Particle(Vector2 pos, Vector2 velo, int index) : pos(pos), velo(velo), index(index),force(0,0) {}
Particle() : pos(0,0), velo(0,0), force(0,0), index(0) {}
    void evolve() {
      if(pos.x > engine_center.x+engine_size || pos.x < engine_center.x - engine_size){
        if(pos.y < engine_center.y+engine_size) velo.x *= -1.0;
      }
      if(pos.y < engine_center.y-engine_size) {
        velo.y *= -1.0;
      }
      if(pos.y > ylimit) should_die = true;

      velo.x += force.x*dt;
      velo.y += force.y*dt;

      pos.x+= velo.x*dt;
      pos.y += velo.y*dt;
    }

};




const double k = 4.0;
const double mu = 30.0;





vector<Particle> particles;
vector<double> pressure_array;


int cell_number = (floor(world_size/h)+1);
const vector<vector<Particle*>> empty_cells(cell_number*cell_number,vector<Particle*>(0));
vector<vector<Particle*>> cells(cell_number*cell_number,vector<Particle*>(0));

int cell_index(Vector2 position) {
  return (cell_number)*floor(position.y/h) + floor(position.x/h); 
}


void fill_cells() {
  cells = empty_cells;
#pragma omp parallel for
  for(Particle &p : particles) {
      int ci = cell_index(p.pos);
      if(ci >= cell_number*cell_number || ci <0) continue;
#pragma omp critical
    cells[ci].push_back(&p);
  }
}

vector<Particle*> neighbors_from_index(int cell_index) {
  vector<Particle*> temp;
#pragma omp parallel for
  for(int i = 0;i < 9;i++) {
  int j = (i%3)-1;
  int k = cell_number*((i/3)-1);
  if(cell_index+k+j < 0 || k+j+cell_index >= cell_number*cell_number) continue;
  vector<Particle*> current = cells[cell_index+k+j];
  temp.insert(temp.end(),current.begin(),current.end());
  }
  return temp;
}


double pressure_kernel(Particle* p1, Particle* p2){ 
  double dx = p1->pos.x-p2->pos.x;
  double dy = p1->pos.y-p2->pos.y;
  double r = sqrt(dx*dx + dy*dy);
      return max(0.0,((15/(M_PI*pow(h,6))) * pow(h-r,3)));
}

double pressure(Particle* p) {
  double temp =0;
#pragma omp parallel for
  for(Particle *pj : neighbors_from_index(cell_index(p->pos))) {
    temp += pressure_kernel(p,pj);
  }
  return k*(temp-p0);
}

void calculate_pressures(vector<double>& pressure_array, vector<Particle>& particles) {
    size_t n = particles.size();
    pressure_array.resize(n); 

    #pragma omp parallel for
    for (size_t i = 0; i < n; ++i) {
        pressure_array[i] = pressure(&particles[i]);
        particles[i].index = i;
    }
}

Vector2 gradw(Vector2 position){ 
  double x = position.x;
  double y = position.y;
  double r = sqrt(x*x  +  y*y);

  if(r>h) return {0,0};

  double wx =  (-3 * x) * pow(h - r, 2)/r;  
  double wy =  (-3 * y) * pow(h - r, 2)/r;  
  
  return {wx,wy};
}

double laplaw(Vector2& position){
  double x = position.x;
  double y = position.y;
  return (45/(M_PI*pow(h,6)))*(h-sqrt(x*x + y*y)); 
}

void viscous_force(Particle &p,vector<double>& pressure_array){
#pragma omp parallel for
    for(Particle *pj : neighbors_from_index(cell_index(p.pos))){
        if(pj == &p) continue;
        Vector2 diff_vel(pj->velo.x - p.velo.x, pj->velo.y - p.velo.y);
        Vector2 diff_pos(pj->pos.x - p.pos.x, pj->pos.y - p.pos.y);
        double r = sqrt(diff_pos.x*diff_pos.x + diff_pos.y*diff_pos.y);
        if(r == 0 || r > h) continue;
        Vector2 f(0,0);
        double lap = (h - r);
        f.add(diff_vel.scalar_mult(lap/(pressure_array[pj->index]/k  - p0)));
#pragma omp atomic
        p.force.add(f.scalar_mult(mu));
#pragma omp atmoic
        pj->force.add(f.scalar_mult(-1.0));
    }

}

void pressure_force(Particle& p,vector<double>& pressure_array) {
  double fx= 0;
  double fy = 0;
  double i_pressure = pressure_array[p.index];
  Vector2 pi_pos = p.pos;
#pragma omp parallel for
  for(Particle *pj : neighbors_from_index(cell_index(p.pos))) { 
    double j_pressure  = pressure_array[pj->index];
    if(pj->index == p.index) continue;
    double prefactor = (i_pressure + j_pressure)/(2*((j_pressure/k) - p0));
    Vector2 diff(p.pos.x - pj->pos.x, p.pos.y - pj->pos.y);
    Vector2 grad = gradw(diff);
    grad.scalar_mult(-prefactor);
#pragma omp atomic
    pj->force.add(grad.scalar_mult(-1.0));
  #pragma omp atomic
    p.force.add(grad.scalar_mult(-1.0));
  }
//  return temp_vector;
}


double random_range(double a, double b) {
    return a + (b - a) * (double(rand()) / RAND_MAX);
}

void spawn_particle(int n,vector<Particle>& particles,Vector2 point,double radius_range,Vector2 velo_range) {
    for(int i = 0; i < n; i++) {

  int index = particles.size();
    double theta = random_range(0,2*M_PI);
    double r = random_range(0,radius_range);
    double xpos= point.x + r*cos(theta);
    double ypos= point.y + r*sin(theta);
    double xvelo = random_range(-velo_range.x,velo_range.x);
    double yvelo = random_range(-velo_range.y,velo_range.y);
    Particle current({xpos,ypos},
                    {xvelo,yvelo},
                      index
                  );
    particles.push_back(current);
    }
}


void full_sweep(int n,vector<double>& pressure_array) {
#pragma omp parallel for
    for (Particle &p : particles) {
      int ci = cell_index(p.pos);
      if(ci >= cell_number*cell_number || ci <0) continue;
      vector<Particle*> neighbors = neighbors_from_index(ci);
      pressure_force(p,pressure_array);
      viscous_force(p,pressure_array);
    }
#pragma omp parallel for
    for (int i =0;i< particles.size();i++) {
      Particle &p = particles[i];
      if(p.should_die == true) {
        particles.erase(particles.begin() + i);
        continue;
        }  
      p.evolve();
      p.force = {0,0};
    }
}

vector<sf::Vector2f> rocket_body_points = {{(float)((engine_center.x-engine_size)*(win_size / world_size)),(float)((engine_center.y+engine_size)*(win_size / world_size))}
                                           ,{(float)((engine_center.x-engine_size)*(win_size / world_size)),(float)((engine_center.y-engine_size)*(win_size / world_size))}
                                           ,{(float)((engine_center.x+engine_size)*(win_size / world_size)),(float)((engine_center.y-engine_size)*(win_size / world_size))}
                                           ,{(float)((engine_center.x+engine_size)*(win_size / world_size)),(float)((engine_center.y+engine_size)*(win_size / world_size))}};




const double p_max_display = 1.0;

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
        window.clear(sf::Color::Black);


        spawn_particle(1,particles,
            {engine_center.x,engine_center.y - engine_size/2},  //spawnpoint
            0.1,  //spawn radius
            {0.0,0.0}  //velocity range x and y
            );
        fill_cells();
        calculate_pressures(pressure_array,particles);
        full_sweep(particles.size(),pressure_array);


        window.draw(path);

        for (Particle &p : particles) {
            sf::CircleShape circle(3);
            circle.setPosition(p.pos.x * (win_size / world_size),
                               p.pos.y * (win_size / world_size));
            sf::Uint8 intensity = (sf::Uint8)((clamp(abs(pressure_array[p.index]), 0.0, 255.0))/p_max_display);
            circle.setFillColor(sf::Color(intensity,0,255,255));
            window.draw(circle);
           
        }
  

        window.display();
        sf::sleep(sf::milliseconds(1));
    }
}

