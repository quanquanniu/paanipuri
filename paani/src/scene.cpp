//
//  scene.cpp
//  paani
//
//  Created by Sanchit Garg on 3/12/15.
//  Copyright (c) 2015 Debanshu. All rights reserved.
//

#include "scene.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include <iostream>

using namespace std;

Scene scene;
ParticleSystem particleSystem;

void Scene::init()
{
    //need to add particles
    // create box
    
    scene.cube.setCenter(glm::vec3(0,0,0));
    scene.cube.setDimension(glm::vec3(3,3,3));
    scene.numberOfParticles = 100;
    
    int i;
    glm::vec3 position;
    
    for(i=0; i<scene.numberOfParticles; i++)
    {
        position = utilityCore::randomVec3() * scene.cube.getDimensions() - scene.cube.getHalfDimensions();
        particleSystem.addParticle(Particle(position));
    }
    
}

//Cube class functions
Cube::Cube(glm::vec3 c, glm::vec3 d)
{
    center = c;
    dimensions = d;
}

glm::vec3 Cube::getCenter()
{
    return center;
}

glm::vec3 Cube::getDimensions()
{
    return dimensions;
}

glm::vec3 Cube::getHalfDimensions()
{
    return dimensions/2.0f;
}

void Cube::setCenter(glm::vec3 c)
{
    center = c;
}

void Cube::setDimension(glm::vec3 d)
{
    dimensions = d;
}