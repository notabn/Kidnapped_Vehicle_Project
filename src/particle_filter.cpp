/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
    //Set the number of particles. Initialize all particles to first position (based on estimates of
    //   x, y, theta and their uncertainties from GPS) and all weights to 1.
    // Add random Gaussian noise to each particle.
    // NOTE: Consult particle_filter.h for more information about this method (and others in this file).
    
    num_particles = 500;
    
    default_random_engine gen;
    
    normal_distribution<double> dist_x(x,std[0]);
    normal_distribution<double> dist_y(y,std[1]);
    normal_distribution<double> dist_theta(theta,std[2]);
    
    //cout<<"Old particles "<<endl;
    
    for (int i = 0; i < num_particles; ++i){
        
        Particle p ;
        p.x = dist_x(gen);
        p.y = dist_y(gen);
        p.theta = dist_theta(gen);
        p.id = i;
        p.weight = 1;
        //     cout<<"p.x"<<p.x<<" p.y "<<p.y<<" theta "<<p.theta<<endl;
        particles.push_back(p);
    }
    
    is_initialized = true;
    
    
    
}


void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
    // Add measurements to each particle and add random Gaussian noise.
    // NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
    //  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
    //  http://www.cplusplus.com/reference/random/default_random_engine/
    
    std::vector<Particle>::iterator it;
    
    default_random_engine gen;
    
    
    
    for (it=particles.begin(); it != particles.end();++it){
        
        normal_distribution<double> dx(0.0, std_pos[0]);
        normal_distribution<double> dy(0.0, std_pos[1]);
        normal_distribution<double> dt(0.0, std_pos[2]);
        
        
        if (fabs(yaw_rate) > 0.00001) {
            (*it).x  += velocity/yaw_rate * (sin((*it).theta + yaw_rate*delta_t)-sin((*it).theta));
            (*it).y += velocity/yaw_rate * (cos((*it).theta) - cos((*it).theta + yaw_rate*delta_t));
            (*it).theta += yaw_rate*delta_t;
            
        }else{
            (*it).x  += velocity *delta_t *  cos((*it).theta);
            (*it).y += velocity * delta_t * sin((*it).theta);
        }
        
        
        
        (*it).x += dx(gen);
        (*it).y += dy(gen);
        (*it).theta += dt(gen);
        
        
        
    }
    
    /*
     cout<<"New Particles --------------- "<<endl;
    for (int i = 0; i < num_particles; ++i){
        cout<<"p.x"<<particles[i].x<<" p.y "<<particles[i].y<<" theta "<<particles[i].theta<<endl;
    }
    */
}



void ParticleFilter::dataAssociation(std::vector<LandmarkObs>predicted, LandmarkObs * observation, double sensor_range) {
    // Find the predicted measurement that is closest to each observed measurement and assign the
    //   observed measurement to this particular landmark.
    // NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
    //   implement this method and use it as a helper during the updateWeights phase.
    
    double curr_dist,pos;
    double dist_obs_min = sensor_range;
    for (int j = 0 ; j < predicted.size();j++ ){
        curr_dist = dist( observation->x, observation->y,predicted[j].x, predicted[j].y);
        if (curr_dist < dist_obs_min) {
            dist_obs_min = curr_dist;
            //std::cout<<"observation:"<<observations[i].x<<" "<<observations[i].y<<" "<<observations[i].id<<" landmark:"<<predicted[j].x <<" "<<predicted[j].y<<" "<<predicted[j].id <<" DIST: "<< curr_dist<<std::endl;
            pos = j;
        }
    }
    *observation = predicted[pos];
    
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
                                   std::vector<LandmarkObs> observations, Map map_landmarks) {
    // Update the weights of each particle using a mult-variate Gaussian distribution. You can read
    //   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
    // NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
    //   according to the MAP'S coordinate system. You will need to transform between the two systems.
    //   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
    //   The following is a good resource for the theory:
    //   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
    //   and the following is a good resource for the actual equation to implement (look at equation
    //   3.33
    //   http://planning.cs.uiuc.edu/node99.html
    
    std::vector<Particle>::iterator p;
    std::vector<LandmarkObs> map_associated_landmark;
    
    std::vector<int> associations;
    std::vector<double> sense_x;
    std::vector<double> sense_y;
    
    
    
    for (int i =0; i<num_particles;i++){
        
        associations.clear();
        sense_x.clear();
        sense_y.clear();
        map_associated_landmark.clear();
        
        Particle p = particles[i];
        // Set particle weight to 1 to initialize for taking the product of multiple weights.
        p.weight = 1;
        
        vector<LandmarkObs> landmarks;
        vector<Map::single_landmark_s>::iterator l;
        
        for (l = map_landmarks.landmark_list.begin(); l != map_landmarks.landmark_list.end();++l){
            double dist_landmark = dist((*l).x_f, (*l).y_f , p.x, p.y);
            if (dist_landmark < sensor_range){
                LandmarkObs lm;
                lm.x = (*l).x_f;
                lm.y = (*l).y_f;
                lm.id =(*l).id_i;
                landmarks.push_back(lm);
            }
        }
        
        for (int j = 0; j < observations.size(); j++){
            
            LandmarkObs map_obs;
            // Space transformation from vehicle to particle (in map coords).
            map_obs.x = p.x + (observations[j].x  * cos(p.theta) - observations[j].y * sin(p.theta));
            map_obs.y = p.y + (observations[j].x * sin(p.theta) + observations[j].y * cos(p.theta));
            
            
            map_obs.id = observations[j].id;
            
            
            LandmarkObs map_associated_landmark = map_obs;
            dataAssociation(landmarks, &map_associated_landmark,sensor_range);
            
            
            // At this point, we have found the closest landmark for this observation.
            associations.push_back(map_associated_landmark.id);
            sense_x.push_back(map_associated_landmark.x);
            sense_y.push_back(map_associated_landmark.y);
            
            // Update the weight for this particle by adding the probability contribution
            // of this measurement / closest landmark pair.
            double mu_x = map_associated_landmark.x;
            double mu_y = map_associated_landmark.y;
            double c = 1 / (2 * M_PI * std_landmark[0] * std_landmark[1]);
            double c1 = 2 * std_landmark[0] * std_landmark[0];
            double c2 = 2 * std_landmark[1] * std_landmark[1];
            double t1 = pow(map_obs.x - mu_x, 2.0)/c1;
            double t2 = pow(map_obs.y - mu_y, 2.0)/c2;
            double val = t1 + t2;
            
            long double multiplier = c * exp(-val);
            
            // calculate weight using normalization terms and exponent
            if (multiplier>0)
                p.weight *= multiplier;
            //std::cout<<"observation:"<<map_observations[ii].x<<" "<<map_observations[ii].y<<" "<<map_observations[ii].id<<" landmark:"<<map_associated_landmark[0].x <<" "<<map_associated_landmark[0].y<<" "<<map_associated_landmark[ii].id <<" weight: "<< multiplier<<std::endl;
            
        }
        p = SetAssociations(p, associations, sense_x, sense_y);
        particles[i] = p;
        //cout <<"partical weight: "<<p.weight<<endl;
        weights.push_back(p.weight);
        
    }
    
    
}

void ParticleFilter::resample() {
    // Resample particles with replacement with probability proportional to their weight.
    // NOTE: You may find std::discrete_distribution helpful here.
    //   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
    
    
    default_random_engine gen;
    
    std::normal_distribution <float> nd(1,num_particles);
    
    
    std::vector<Particle> p3;
    
    std::discrete_distribution<int> d(weights.begin(), weights.end());
    
    
    for(int n=0; n<num_particles; ++n) {
        auto index = d(gen);
        p3.push_back(std::move(particles[index]));
    }
    
    particles = std::move(p3);
    weights.clear();
    
}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates
    
    //Clear the previous associations
    particle.associations.clear();
    particle.sense_x.clear();
    particle.sense_y.clear();
    
    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
    
    return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
    vector<int> v = best.associations;
    stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
    vector<double> v = best.sense_x;
    stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
    vector<double> v = best.sense_y;
    stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
