#include <cmath>
#include <random>
#include <vector>
#include <iostream>

int main() {
	std::default_random_engine generator;
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	std::vector<int> a(40);

	for (int i = 0; i < a.size(); i++)
		a[i] = 0;

	for (int i = 0; i < 1000000000; i++) {
		double temp = distribution(generator);
		a[temp / (1.0 / (double)a.size())]++;
	}

	for (int i = 0; i < a.size(); i++)
		std::cout << i << " : " << a[i] << " times." << std::endl;

	return 0;
}