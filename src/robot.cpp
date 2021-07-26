#include "robot.h"
#define DT 1

TurtlebotBurger::TurtlebotBurger(float x, float y, float theta, float radius, float dt, std::string name) : Circle(x, y, radius){
    // Attributes
    this->dt = dt;
    this->theta = theta;
    this->inv_diameter = 1.0 / (2 * this->radius);
    this->diameter = this->radius * 2.0;
    this->name = name;
    this->model = "burger";

    // Lidar
    float frequency;
    this->lidar = new Lidar(frequency, new Point2d(x,y));
}

float TurtlebotBurger::x(){
    return this->xc;
}

float TurtlebotBurger::y(){
    return this->yc;
}

float TurtlebotBurger::get_theta(){
    return this->theta;
}

float TurtlebotBurger::get_dt(){
    return this->dt;
}

std::string TurtlebotBurger::tostring(){
    return "(TurtlebotBurger) Name: " + this->get_name() + " , Model: " + this->get_model() + " , " +
    Circle::tostring() + ", theta: " 
    + std::to_string(this->theta) 
    + " dt: " + std::to_string(this->dt)
    + ", with " + this->lidar->tostring() + "\n";
}

std::tuple<float, float, float> TurtlebotBurger::kinematics(float v, float w){
    float v_left = v + w * this->radius;
    float v_right = v - w * this->radius;
    float dd = (v_left + v_right) * 0.5;
    float dth = (v_left - v_right) * this->inv_diameter;
    
    return 
    {
        this->xc + dd * cos(this->theta + dth * 0.5) * this->dt,
        this->yc + dd * sin(this->theta + dth * 0.5) * this->dt,
        normalize_angle(this->theta + dth * this->dt)
    };
}

void TurtlebotBurger::move(float v, float w){
    // Compute position
    std::tuple<float,float,float> new_pose = this->kinematics(v,w);
    // Update position
    this->xc = std::get<0>(new_pose);
    this->yc = std::get<1>(new_pose);
    this->theta = std::get<2>(new_pose);
    return;
}

Lidar* TurtlebotBurger::get_lidar(){
    return this->lidar;
}

std::vector<float> Lidar::get_lasers(){
    return this->lasers;
}

std::string TurtlebotBurger::get_name(){
    return this->name;
}

std::string TurtlebotBurger::get_model(){
    return this->model;
}

void TurtlebotBurger::update_lidar_heavy(std::vector<RoundObstacle> round_obstacles, std::vector<Line> edges){
    Line laser(0,0,0,0);
    std::cout << "Laser before:" << laser.tostring()<<"\n";
    bool in_sight;
    std::tuple<bool, float, float, float, float> intersection;
    float min_distance;
    float distance;

    // Go through all rays
    //for (int ray=0; ray < this->lidar->get_lasers().size(); ray++)
    for (int ray=45; ray < 46; ray++)
    {
        std::tuple<float, float, float, float> laser_points = this->lidar->get_laser_points(ray, this->x(), this->y(), this->theta);
        laser.set_points(std::get<0>(laser_points), std::get<1>(laser_points), std::get<2>(laser_points), std::get<3>(laser_points));
        std::cout << "Ray " << ray << "\n";
        std::cout << "Laser:" << laser.tostring()<<"\n";
        for(int o = 0; o < round_obstacles.size(); o++)
        {
            // Check intersection
            intersection = round_obstacles[o].intersects_line(laser);
            std::cout << "obstacle " << o << ": " << round_obstacles[o].tostring();
            std::cout << "intersection " <<  o << " :(" <<
            bool_to_string(std::get<0>(intersection)) << ", " <<
            std::get<1>(intersection) << ", " <<
            std::get<2>(intersection) << ", " <<
            std::get<3>(intersection) << ", " <<
            std::get<4>(intersection) << ")\n";
            // If there was intersection
            if (std::get<0>(intersection))
            {
                // Check if it is in sight
                in_sight = this->lidar->obstacle_in_sight(
                    std::get<1>(intersection),
                    std::get<2>(intersection),
                    std::get<3>(intersection),
                    std::get<4>(intersection),
                    round_obstacles[o].get_xc(),
                    round_obstacles[o].get_yc()
                );
                if (in_sight)
                {
                    distance = distance_between_points(
                        std::get<0>(laser_points),
                        std::get<1>(laser_points),
                        0,0
                    );
                }

            }
        }
        std::cout << "-----------\n";
    }
    
}

// Lidar
Lidar::Lidar(float frequency, Point2d* position){
    this->frequency = frequency;
    this->position = position;
    this->lasers.assign(N_LASERS, MAX_DISTANCE);
    this->min_distance = MIN_DISTANCE;
    this->max_distance = MAX_DISTANCE;
}

std::string Lidar::tostring(){
    return "(Lidar) frequency: " + std::to_string(this->frequency)
    + ", position: " + this->position->tostring() 
    + ", min_distance: " + std::to_string(this->min_distance)
    + ", max_distance: " + std::to_string(this->max_distance)
    +  "\n";
}

void Lidar::display_lasers(){
    std::cout << "Lasers: [ ";
    for(int i = 0; i < this->lasers.size(); i++){
        std::cout << this->lasers[i] << " ";
    }
    std::cout << "]" << std::endl;
}

bool Lidar::in_between(float xi, float xm, float xf){
    return (xi <= xm <= xf) || (xf <= xm <= xi);
}
/*
template <typename T> bool Lidar::in_sight(float x_sight, float y_sight, float x_forward, float y_forward, float x_object, float y_object, T object){
    return true;
}
*/
bool Lidar::obstacle_in_sight(float x_min, float y_min, float x_max, float y_max, float x_obs, float y_obs){
    // Check after if it is needed to add the last condition
    return this->in_between(x_min, x_obs, x_max) && this->in_between(y_min, y_obs, y_max);
}


std::tuple<float,float,float,float> Lidar::get_laser_points(float angle, float x, float y, float theta){
    float angle_rad = angle * TO_RAD;
    float cos_ = cos(angle_rad + theta);
    float sin_ = sin(angle_rad + theta);
    return 
    {
        x + this->min_distance * cos_,
        y + this->min_distance * sin_,
        x + this->max_distance * cos_,
        y + this->max_distance * sin_
    };
}

