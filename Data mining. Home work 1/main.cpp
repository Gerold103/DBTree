#include <iostream>
#include <cmath>
using namespace std;

int main()
{
	double sum = 0;
	for (int i = 0; i < 13; ++i) {
		double x;
		cin >> x;
		sum += x * x;
	}
	sum = sqrt(sum);
	cout << sum << endl;
	return 0;
}