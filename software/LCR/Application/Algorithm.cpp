#include "Algorithm.hpp"

#include <cmath>

Algorithm::complex Algorithm::Goertzel(int16_t* data, uint16_t N,
		uint32_t sampleFreq, uint32_t targetFreq) {
	const int k = (int) (0.5f + (N * targetFreq) / sampleFreq);
	const double w = (2 * M_PI / N) * k;
	const double sine = sin(w);
	const double cosine = cos(w);
	const double a = 2 * cosine;
	double q0, q1 = 0.0, q2 = 0.0;
	for (uint16_t i = 0; i < N; i++) {
		q0 = data[i] + a * q1 - q2;
		q2 = q1;
		q1 = q0;
	}
	complex ret;
	ret.real = q1 - q2 * cosine;
	ret.imag = q2 * sine;

	return ret;
}

void Algorithm::RemoveDC(int16_t* data, uint16_t n) {
	int32_t sum = 0;
	for (uint16_t i = 0; i < n; i++) {
		sum += data[i];
	}
	sum /= n;
	for (uint16_t i = 0; i < n; i++) {
		data[i] -= sum;
	}
}

double Algorithm::RMS(int16_t* data, uint16_t n) {
	uint64_t sum = 0;
	for (uint16_t i = 0; i < n; i++) {
		sum += data[i] * data[i];
	}
	return sqrt(sum)/sqrt(n);
}

int16_t Algorithm::PeakToPeak(int16_t* data, uint16_t n) {
	int16_t min = INT16_MAX;
	int16_t max = INT16_MIN;
	for (uint16_t i = 0; i < n; i++) {
		if (data[i] < min) {
			min = data[i];
		} else if (data[i] > max) {
			max = data[i];
		}
	}
	return max - min;
}

double Algorithm::PhaseDiff(int16_t *data1, int16_t *data2, uint16_t n) {
	int32_t real = 0, imag = 0;
	for(uint16_t i=0;i<n;i++) {
		real += data1[i] * data2[i];
		uint16_t i90 = (i + n/4) % n;
		imag += data1[i] * data2[i90];
	}
	double diff = atan2(imag, real);

	if (diff < -M_PI) {
		diff += 2 * M_PI;
	} else if (diff > M_PI) {
		diff -= 2 * M_PI;
	}

	return diff;
}

double Algorithm::Phase(int16_t* data, uint16_t n) {
	double real = 0, imag = 0;
	double radPerSample = 2.0 * M_PI / n;
	for(uint16_t i=0;i<n;i++) {
		double sine = sin(i * radPerSample);
		double cosine = cos(i * radPerSample);
		real += sine * data[i];
		imag += cosine * data[i];
	}
	return atan2(imag, real);
}
