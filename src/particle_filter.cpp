/*
 * particle_filter.cpp
 *
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <map>

#include <math.h> 
#include <sstream>
#include <string>
#include <iterator>
#include <map>

#include "particle_filter.h"
using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// Sets the number of particles. Initializes all particles to first position (based on estimates of
  // x, y, theta and their uncertainties from GPS) and all weights to 1.
	// Adds random Gaussian noise to each particle.
  	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	num_particles = 100;
  
  std::default_random_engine gen;
  
  std::normal_distribution<double> N_x(x, std[0]);
  std::normal_distribution<double> N_y(y, std[1]);
  std::normal_distribution<double> N_theta(theta, std[2]);
  
  for(int i = 0;i < num_particles; i++){
    Particle particle;
    particle.id = i;
    particle.x = N_x(gen);
    particle.y = N_y(gen);
    particle.theta = N_theta(gen);
    particle.weight = 1;
    
    this->particles.push_back(particle);
    this->weights.push_back(1);
  }
  
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
		// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
  	default_random_engine gen;
  //cout<<"velocity: "<< velocity<<endl;
  //cout<<"yaw rate: "<< yaw_rate<<endl;
  for(int i  = 0; i < num_particles; i++){
    double new_x;
    double new_y;
    double new_theta;
    if(yaw_rate == 0){
      new_x = particles[i].x + velocity * delta_t * cos(particles[i].theta);
      new_y = particles[i].y + velocity * delta_t * sin(particles[i].theta);
      new_theta = particles[i].theta;
    }
    else{
      new_x = particles[i].x + velocity / yaw_rate * (sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta)); 
      new_y = particles[i].y + velocity / yaw_rate * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t)); 
      new_theta = particles[i].theta + yaw_rate * delta_t;
    }
    
    std::normal_distribution<double> N_x(new_x, std_pos[0]);
    std::normal_distribution<double> N_y(new_y, std_pos[1]);
    std::normal_distribution<double> N_theta(new_theta, std_pos[2]);
    
    particles[i].x = N_x(gen);
    particles[i].y = N_y(gen);
    particles[i].theta = N_theta(gen);
  }
}

vector<int> ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, const std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.
	
  vector<int> associations;
  	//I assume that prediced is the landmark actual positions
  for(int i = 0;i < observations.size();++i){
      associations.push_back(-1);//init value
      double min_ecl_dist = 999999;
      for(LandmarkObs pred : predicted){
        double ecl_dist = dist(observations[i].x, observations[i].y, pred.x, pred.y);
        if(min_ecl_dist > ecl_dist){
          min_ecl_dist = ecl_dist;
          associations[i] = pred.id;
        }
      }
    }
  return associations;
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
  for(int p = 0;p < this->particles.size();++p){
    weights[p] = updateWeightForParticle(particles[p], sensor_range, std_landmark, observations, map_landmarks);  
  }
}

double ParticleFilter::updateWeightForParticle(Particle& particle, double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks){
  //////////////////////////get predictions and landmarks map.
  map<int, LandmarkObs> landmarks_map;
  vector<LandmarkObs> predictions;
  for(Map::single_landmark_s a_single_landmark : map_landmarks.landmark_list){
    if(dist(a_single_landmark.x_f, a_single_landmark.y_f, particle.x, particle.y) > sensor_range)
      continue;
    LandmarkObs l;
    l.x = a_single_landmark.x_f;
    l.y = a_single_landmark.y_f;
    l.id = a_single_landmark.id_i;
    landmarks_map[l.id] = l;
    predictions.push_back(l);
  }
	
  //////////////////////////get transformed observations
  vector<LandmarkObs> trans_observations;
  vector<double> sense_x, sense_y;
  for (LandmarkObs obs : observations){
    LandmarkObs trans_obs;
    //perform the space transformation from vehicle to map
    trans_obs.x = particle.x + (obs.x * cos(particle.theta) - obs.y * sin(particle.theta));
    trans_obs.y = particle.y + (obs.x * sin(particle.theta) + obs.y * cos(particle.theta));
    sense_x.push_back(trans_obs.x);
    sense_y.push_back(trans_obs.y);
    trans_observations.push_back(trans_obs);
  }
  
  //////////////////////////associate transformed observations to landmarks.
  vector<int> associations = dataAssociation(predictions, trans_observations);
  
  //////////////////////////compute multi variate gaussian for this particle's weight.
  double weight = 1;
  for(int i = 0;i < associations.size();++i){
    double sig_x = std_landmark[0];
    double sig_y = std_landmark[1];
    double x = trans_observations[i].x;
    double y = trans_observations[i].y;
    double mu_x = landmarks_map[associations[i]].x;
    double mu_y = landmarks_map[associations[i]].y;
    weight *= getMultiVariateGaussian(sig_x, sig_y, x, mu_x, y, mu_y);
  }
  
  //////////////////////////set particle's data
  particle.weight = weight;
  particle.associations = associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
  return weight;
}

double ParticleFilter::getMultiVariateGaussian(const double sig_x, const double sig_y,const double x, const double mu_x,
			const double y, const double mu_y){
  double const_val = 1.0 / (2.0 * M_PI * sig_x * sig_y);
  double x_diff = x - mu_x;
  double y_diff = y - mu_y;
  double exp_pow = ((x_diff * x_diff) / (2.0 * sig_x * sig_x)) + ((y_diff * y_diff) / (2.0 * sig_y * sig_y));  
  double res = const_val * exp(-exp_pow);
  return res;
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	default_random_engine gen;
    discrete_distribution <int> distribution(weights.begin(), weights.end());
    vector<Particle> resample_particles;
  	for(int i = 0;i < num_particles;i++){
       resample_particles.push_back(particles[distribution(gen)]);
    }
    particles = resample_particles;
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    // particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
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