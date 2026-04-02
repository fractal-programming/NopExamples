#!/usr/bin/env python3

import numpy as np
import argparse

np.set_printoptions(formatter = {'float': '{:.3f}'.format})

cont = True

i = 2.00
x = np.zeros((6, 1))

a = 0.25;
b = 1 - a;
c = 1.00 if cont else 0.00
e = 1.00 - c
A = np.array([[   c, 0.00, 0.00, 0.00, 0.00, 0.00],
              [1.00, 0.00,    a, 0.00, 0.00, 0.00],
              [0.00, 1.00, 0.00,    a, 0.00, 0.00],
              [0.00, 0.00,    b, 0.00,    a, 0.00],
              [0.00, 0.00, 0.00,    b, 0.00, 0.00],
              [0.00, 0.00, 0.00, 0.00,    b,    e]])

nDef = 1

def matrix_power(n):

	print(f"")

	x[0] = i
	print(f"x0")
	print(np.transpose(x))
	print(f"")

	print(f"A")
	print(A)

	sums = np.sum(A, axis = 0)
	print(f"Sums A")
	print(f" {sums}")

	print(f"")

	R = np.linalg.matrix_power(A, n)
	print(f"A^{n}")
	print(R)

	sums = np.sum(R, axis = 0)
	print(f"Sums A^{n}")
	print(f" {sums}")

	print(f"")

	xn = R @ x
	print(f"x{n} = A^{n} * x0")
	print(np.transpose(xn))
	print(f"             ^                 ^")
	print(f"             |     Resistor    |")
	print(f"             |                 |")
	print(f"         Beginning            End")

	print(f"")

	return xn

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'Calculating A^n')
	parser.add_argument('n', type = int, nargs = '?', default = nDef, help = f"Exponent for A^n (default: {nDef})")

	args = parser.parse_args()
	matrix_power(args.n)

