#include <iostream>
#include <random>
#include <numbers>

void sample_on_half_sphere() {
    std::random_device rd;
    std::mt19937 gen(rd()); // Mersenne Twister
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // phi: 0 ~ 2*pi
    double phi = 2.0 * std::numbers::pi * dist(gen);

    // cos(theta): 0 ~ 1
    double u = dist(gen);           // 0 ~ 1
    double theta = std::acos(u);    // 0 ~ pi/2

    // x,y,z transformation
    double x = std::sin(theta) * std::cos(phi);
    double y = std::sin(theta) * std::sin(phi);
    double z = std::cos(theta);

    std::cout << "Random point on sphere: "
        << "(" << x << ", " << y << ", " << z << ")\n";

    std::cout << "check if it's on sphere: " 
        << std::pow(x, 2) + std::pow(y, 2) + std::pow(z, 2) << "\n";
}

bool test() {
    return false;
}

int main()
{
    sample_on_half_sphere();
    test();
}
