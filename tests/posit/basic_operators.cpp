//  basic_operators.cpp : functional tests for conversion operators of posit numbers
//
// Copyright (C) 2017 Stillwater Supercomputing, Inc.
//
// This file is part of the universal numbers project, which is released under an MIT Open Source license.

#include "stdafx.h"
#include <sstream>

#include "../../posit/posit.hpp"

using namespace std;
using namespace sw::unum;

template<size_t nbits, size_t es>
void checkSpecialCases(posit<nbits, es> p) {
	cout << "posit is " << (p.isZero() ? "zero " : "non-zero ") << (p.isPositive() ? "positive " : "negative ") << (p.isInfinite() ? "+-infinite" : "not infinite") << endl;
}

void BasicOperators() {
	posit<16, 1> p1, p2, p3, p4, p5, p6;

	double minpos = p1.minpos_value();
	double maxpos = p1.maxpos_value();

	p1 = 0;  checkSpecialCases(p1);
	p1 = 1;  checkSpecialCases(p1);
	p2 = 2;  checkSpecialCases(p2);

	p3 = p1 + p2;
	p4 = p2 - p1;
	p5 = p2 * p3;
	p6 = p5 / p3;

	cout << "p1: " << p1 << "\n";
	cout << "p2: " << p2 << "\n";
	cout << "p3: " << p3 << "\n";
	cout << "p4: " << p4 << "\n";
	cout << "p5: " << p5 << "\n";
	cout << "p6: " << p6 << "\n";

	cout << "p1++ " << p1++ << " " << p1 << "\n";
	cout << "++p1 " << ++p1 << "\n";
	cout << "p1-- " << p1-- << " " << p1 << "\n";
	cout << "--p1 " << --p1 << "\n";

	// negative regime
	p1 = -1; checkSpecialCases(p1);
}


int main()
try {
	int nrOfFailedTestCases = 0;

	BasicOperators();
    
	return (nrOfFailedTestCases > 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
catch (char* msg) {
	cerr << msg << endl;
	return EXIT_FAILURE;
}



