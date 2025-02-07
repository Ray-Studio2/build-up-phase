#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

vector<float> vertex;
vector<float> normal;

float* vertexData;

bool readMesh(const char* filename);

int main()
{
	if (!readMesh("teapot.obj"))
		cout << "Could not read file." << endl;

	delete vertexData;
	return 0;
}

bool readMesh(const char* filename) {	
	ifstream is(filename);

	if (is.fail())
		return false;

	string code;
	float temp;

	/*for (int i = 0; i < 4; i++) {
		is.getline(aa, 256);
		cout << aa << endl;
	}*/

	while ((is >> code) && !(code == "v")) {
		continue;
	}

	if (code == "v") {
		for (int i = 0; i < 3; i++) {
			is >> temp;
			vertex.push_back(temp);
		}
	}

	while ((is >> code) && (code == "v")) {
		for (int i = 0; i < 3; i++) {
			is >> temp;
			vertex.push_back(temp);
		}
	}

	int vertexSize = vertex.size();

	while ((is >> code) && !(code == "vn")) {
		continue;
	}

	if (code == "vn") {
		for (int i = 0; i < 3; i++) {
			is >> temp;
			normal.push_back(temp);
		}
	}

	while ((is >> code) && (code == "vn")) {
		for (int i = 0; i < 3; i++) {
			is >> temp;
			normal.push_back(temp);
		}
	}

	int normalSize = normal.size();	

	vertexData = new float[vertexSize + normalSize];

	int dataIndex = 0;
	int vertexIndex = 0;
	int normalIndex = 0;

	for (int i = 0; i < vertexSize / 3; i++) {
		for (int j = 0; j < 3; j++)
			vertexData[dataIndex++] = vertex[vertexIndex++];

		for(int k = 0; k < 3; k++)
			vertexData[dataIndex++] = normal[normalIndex++];
	}

	cout << vertex.size() << " " << normal.size() << endl;

	return true;
}