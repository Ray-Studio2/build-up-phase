#include <iostream>
#include <random>
#include <numbers>

struct vec3 {
    double x, y, z;
};

vec3 sample_on_hemisphere() {
    vec3 result{ 0.0 };

    std::random_device rd;
    std::mt19937 gen(rd()); // Mersenne Twister
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // phi: 0 ~ 2*pi
    double phi = 2.0 * std::numbers::pi * dist(gen);
    // theta: 0 ~ pi/2
    double theta = std::acos(dist(gen));

    // x,y,z transformation
    result.x = std::sin(theta) * std::cos(phi);
    result.y = std::sin(theta) * std::sin(phi);
    result.z = std::cos(theta);

    //std::cout << "Random point on sphere: "
    //    << "(" << result.x << ", " << result.y << ", " << result.z << ")\n";

    //std::cout << "check if it's on sphere: " 
    //    << std::pow(result.x, 2) + std::pow(result.y, 2) + std::pow(result.z, 2) << "\n";

    return result;
}

bool test()
{
    const int numSamples = 1000000;
    const int numTest = 10;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    vec3 acc = { 0.0 };

    for (int t = 0; t < numTest; ++t) {
        for (int i = 0; i < numSamples; ++i)
        {
            vec3 sample = sample_on_hemisphere();

            if (dist(gen) < 0.5)
                sample.z *= -1.0;

            acc.x += sample.x;
            acc.y += sample.y;
            acc.z += sample.z;
        }
        printf("x: %lf\n", acc.x);
        printf("y: %lf\n", acc.y);
        printf("z: %lf\n", acc.z);
        printf("---------------------\n");
    }

    return 0;
}

int main()
{
    test();
}
