#include <cmath>
#include <random>
#include <vector>
#include <iostream>
//#include <glm/glm.hpp>
#include <omp.h>

using namespace std;
//using namespace glm;

int samplingCount[180][180];

void RandomSampling(int maxCount)
{
		std::random_device generator;
		std::uniform_real_distribution<double> distributionX(-90.0, 90.0);
		std::uniform_real_distribution<double> distributionY(-90.0, 90.0);

		double x, y;

#pragma omp parallel for
	for (int i = 0; i < maxCount; i++) {
		x = distributionX(generator);
		y = distributionY(generator);

		samplingCount[(int)floor(x) + 90][(int)floor(y) + 90]++;

		//cout << x << " , " << y << endl;
	}
}

int main() {
	int maxCount = 100000000;

	RandomSampling(maxCount);
	
	double average = (double)maxCount / (double)(180 * 180);
	double uniformity = 0.0;

	for (int i = 0; i < 180; i++)
		for (int j = 0; j < 180; j++) {
			if(i == 179 && j >= 170)
				cout << "x : " << i - 90 << " , y : " << j - 90 << " => " << samplingCount[i][j] << " times." << endl;

			uniformity += abs((average - (double)samplingCount[i][j]) / average);
		}

	uniformity = 100.0 - (uniformity / (double)(180 * 180));

	cout << endl << "Uniformity : " << uniformity << endl;

	return 0;
}