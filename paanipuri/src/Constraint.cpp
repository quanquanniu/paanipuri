#include "Constraint.h"
#include "scene.h"
#include <iostream>
#include <tbb/tbb.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <SVD>

typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> Matrix;

using namespace tbb;

Constraint::Constraint() {
}


Constraint::~Constraint() {

}

void Constraint::Solve(glm::vec3& position, const SparseMatrix& invMass) {
    //so check if there is a contact on the particle. if so, resolve it
    
}

//Contact Constraint -------------------------------------------------------------
ContactConstraint::ContactConstraint() {

}

ContactConstraint::ContactConstraint(int particleIndex) {
	_particleIndex = particleIndex;
}

ContactConstraint::~ContactConstraint() {
    
}

//TODO: Implement
void ContactConstraint::Solve(glm::vec3& position, const SparseMatrix& invMass) {
	//so check if there is a contact on the particle. if so, resolve it

}
//---------------------------------------------------------------------------------


//Shape Matching Constraint -------------------------------------------------------
ShapeMatchingConstraint::ShapeMatchingConstraint() {
    
}

ShapeMatchingConstraint::ShapeMatchingConstraint(int particleIndex) {
    _particleIndex = particleIndex;
}

ShapeMatchingConstraint::~ShapeMatchingConstraint() {
    
}

void ShapeMatchingConstraint::Solve(glm::vec3& position, const SparseMatrix& invMass){
    
}

//void ShapeMatchingConstraint::Solve(std::vector<Particle> particleGroup){
//    
//    //first we have to find the center of mass of the particles in the deformed configuration.
//    //that should just be the average position of all the particles?
//    glm::vec3 positionSum = glm::vec3(0.0f);
//    for(int i = 0; i < particleGroup.size(); i++) {
//        positionSum += particleGroup[i].getPredictedPosition(); //should be predicted position right? not position...
//    }
//    glm::vec3 c = positionSum / (float) particleGroup.size();
//    
//    //Imagine we have the matrix A already
//    Matrix A;
//    //Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
//    Eigen::JacobiSVD<Matrix> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
//    //solve for Q (rotation matrix
//    Matrix Q = svd.matrixU() * svd.matrixV().transpose();
//}

    //now solve delta x for particle i. so this will be a loop over all particles in the group?
void ShapeMatchingConstraint::Solve(std::vector<Particle>& particles)
{
    glm::vec3 centerMassDeformed, centerMassRestPose;
    
    for(int i=0; i<particles.size(); i++)
    {
        centerMassDeformed += particles.at(i).getPredictedPosition();
        centerMassRestPose += particles.at(i).getPosition();
    }
    centerMassDeformed /= particles.size();
    centerMassRestPose /= particles.size();
    
    glm::vec3 r, p;
    glm::mat3 A(0);
    
    for(int i=0;i<particles.size(); i++)
    {
        p = particles.at(i).getPredictedPosition() - centerMassDeformed;
        r = particles.at(i).getPosition() - centerMassDeformed;
        A += p*r;
    }
    
    Eigen::Matrix<float, 3, 3> Aeigen;
    
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            Aeigen(i,j) = A[i][j];
        }
    }
    
    Eigen::JacobiSVD<Matrix> svd(Aeigen, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Matrix Q = svd.matrixU() * svd.matrixV().transpose();
    
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            A[i][j] = Q(i,j);
        }
    }
    
    for(int i = 0; i < particles.size(); i++) {
        //neet to convert from glm to euler to glm
        particles.at(i).setDeltaPi((A * particles.at(i).getRestOffset() + centerMassDeformed) - particles.at(i).getPredictedPosition());
    }
}

int ShapeMatchingConstraint::getParticleIndex()
{
    return _particleIndex;
}

//---------------------------------------------------------------------------------


//Density Constraint --------------------------------------------------------------

DensityConstraint::DensityConstraint() {

}

DensityConstraint::DensityConstraint(int particleIndex) {
	_particleIndex = particleIndex;
}

DensityConstraint::~DensityConstraint() {
	
}

void DensityConstraint::Solve(glm::vec3& position, const SparseMatrix& invMass) {
	
}

//TODO: setup density constraint to find lambda
void DensityConstraint::Solve(std::vector<Particle>& particles) {
    
    glm::vec3 deltaPi = findDeltaPosition(_particleIndex, particles); //constraintDelta*lambda
    Particle& currParticle = particles.at(_particleIndex);
    currParticle.setDeltaPi(deltaPi);
//        particleCollision(i);
}

void DensityConstraint::findLambda(std::vector<Particle>& particles){

    int index = _particleIndex;
    Particle& currParticle = particles.at(index);
    
    std::vector<int> neighbors = currParticle.getNeighborIndices();
    
    int numNeighbors = static_cast<int>(neighbors.size());
    
    float sumGradientAtParticle = 0.0f;
    float gradientNeighbor = 0.0f;
    
    for (int i=0; i<numNeighbors; i++) {
        gradientNeighbor = glm::length(gradientConstraintForNeighbor(index, i, particles));
        sumGradientAtParticle += gradientNeighbor * gradientNeighbor / currParticle.getMass();
    }
    
    float gradientParticle = glm::length(gradientConstraintAtParticle(index, particles));
    
    //Different Masses
    sumGradientAtParticle += (gradientParticle * gradientParticle);// / currParticle.getMass();
    
    float currDensity = this->getDensity(index, particles);
    float densityContraint = (currDensity/restDensity) - 1.0f; //density constraint
    
    float lambdaI = -1.0f * (densityContraint/(sumGradientAtParticle+relaxation)); //findLambda using density constraint
    
//    if(std::isnan(lambdaI)||std::isinf(fabs(lambdaI)))
//    {
//        std::cout<<"[ERROR] findLambda";
//    }
    // do these have to be atomic?
    currParticle.setDensity(currDensity);
    currParticle.setLambda(lambdaI);
}

float DensityConstraint::getDensity(int index, std::vector<Particle>& particles)
{
    float density = 0;
    int i;
    Particle& currParticle = particles.at(index);
    
    std::vector<int> neighbors = currParticle.getNeighborIndices();
    
    //Kernel function implementation
    //  Using poly6 kernel function
    
    //TODO
    //  Currently mass is 1 for all particles
    //  If mass is changed, change the for loop to multiply by mass
    for(i=0; i<neighbors.size(); i++)
    {
        glm::vec3 temp = (currParticle.getPredictedPosition() - particles.at(neighbors.at(i)).getPredictedPosition());
        
//        if(glm::any(glm::isnan(currParticle.getPredictedPosition())))
//        {
//            std::cout<<"[ERROR] getDensity at Particles";
//        }
//        if(glm::any(glm::isnan(particles.at(neighbors.at(i)).getPredictedPosition())) || glm::any(glm::isinf(particles.at(neighbors.at(i)).getPredictedPosition())))
//        {
//            utilityCore::printVec3(particles.at(neighbors.at(i)).getPredictedPosition());
//            std::cout<<"[ERROR] getDensity at Neighbours";
//        }
        
        density +=  particles.at(neighbors.at(i)).getMass() * wPoly6Kernel(temp, smoothingRadius);
    }
    
    if(density < EPSILON)
    {
        density = EPSILON;
    }
    else if(density > restDensity-1)
    {
        density = restDensity - 1;
    }
    
    return density;
}

float DensityConstraint::wPoly6Kernel(glm::vec3 distance, float smoothingRadius)
{
//    if(glm::any(glm::isnan(distance)) || glm::any(glm::isinf(distance)))
//    {
//        std::cout<<"[ERROR] wPoly6Kernel";
//    }
     
    
    float d = glm::length(distance);
    float x = (smoothingRadius*smoothingRadius) - d*d;
    float x_3 = x*x*x;
    
    float t = poly6Const * x_3/s_9;
    
    if(isinf(fabs(t)) || isnan(t))
    {
        t = EPSILON;
    }
    return t;
}

glm::vec3 DensityConstraint::gradientWSpikyKernel(glm::vec3 distance, float smoothingRadius)
{
    float distanceLength = glm::length(distance);
    
    float x = (smoothingRadius-distanceLength)*(smoothingRadius-distanceLength);
    
    float gradientW = spikyConst * (1.0f/s_6) * x * 1.0f/(distanceLength+EPSILON);
    
    glm::vec3 retVal = gradientW*distance;
    
//    if(glm::any(glm::isinf(retVal)))
//    {
//        retVal = glm::vec3(EPSILON);
//    }
    
    return retVal;
}

glm::vec3 DensityConstraint::gradientConstraintAtParticle(int index, std::vector<Particle>& particles)
{
    glm::vec3 gradientReturn(0,0,0);
    float restDensityInverse = 1.0/restDensity;
    
    std::vector<int> neighbors = particles.at(index).getNeighborIndices();
    
    for(int i=0; i<neighbors.size(); i++)
    {
        gradientReturn += restDensityInverse *
        gradientWSpikyKernel((particles.at(index).getPosition() - particles.at(neighbors.at(i)).getPosition()), smoothingRadius);
    }
    
    return gradientReturn;
}

glm::vec3 DensityConstraint::gradientConstraintForNeighbor(int index, int neighborIndex, std::vector<Particle>& particles)
{
    glm::vec3 gradientReturn(0,0,0);
    float restDensityInverse = 1.0/restDensity;
    
    
    gradientReturn = restDensityInverse * gradientWSpikyKernel(particles.at(index).getPosition() - particles[neighborIndex].getPosition(), smoothingRadius);
    
    return (-1.0f * gradientReturn);
}

glm::vec3 DensityConstraint::findDeltaPosition(int index, std::vector<Particle>& particles)
{
    glm::vec3 deltaPi(0,0,0);
    
    Particle& currParticle = particles.at(index);
    
    std::vector<int> neighbors = currParticle.getNeighborIndices();
    float lambda_i = currParticle.getLambda();
    float sCorr = 0, k = 0.1, deltaQ = 0.1 * smoothingRadius;
    //    int n = 4; //avoid using pow
    float artificialTerm;
    
    // constraintDelta * lambda
    for(int i=0; i<neighbors.size(); i++)
    {
//        if (glm::any(glm::isinf(particles.at(neighbors.at(i)).getPredictedPosition()))){
//            std::cout<<neighbors.at(i)<<std::endl;
//        }

        float temp = wPoly6Kernel(glm::vec3(deltaQ, 0, 0), smoothingRadius);
        
        artificialTerm = wPoly6Kernel((currParticle.getPredictedPosition() - particles.at(neighbors.at(i)).getPredictedPosition()), smoothingRadius) / (temp+EPSILON);

//        if (glm::any(glm::isinf(particles.at(neighbors.at(i)).getPredictedPosition()))){
//            std::cout<<neighbors.at(i)<<std::endl;
//        }
        
        sCorr = -1.0 * k * artificialTerm * artificialTerm * artificialTerm * artificialTerm;
        
        deltaPi += (particles.at(neighbors[i]).getLambda() + lambda_i + sCorr) *
        gradientWSpikyKernel((currParticle.getPredictedPosition() - particles.at(neighbors.at(i)).getPredictedPosition()), smoothingRadius);
    }
    
    return (deltaPi/restDensity);
}

void DensityConstraint::viscosity(int index, std::vector<Particle>& particles)
{
    Particle& currParticle = particles.at(index);
    std::vector<int> neighbors = currParticle.getNeighborIndices();
    
    glm::vec3 newVelocity = currParticle.getVelocity();
    
    for(int i=0; i<neighbors.size(); i++)
    {
        newVelocity += (1/particles.at(neighbors[i]).getDensity()) * (particles.at(neighbors[i]).getVelocity() - currParticle.getVelocity()) * wPoly6Kernel( (currParticle.getPredictedPosition() - particles.at(neighbors.at(i)).getPredictedPosition()), smoothingRadius);
    }
    
    currParticle.setVelocity(newVelocity);
}
