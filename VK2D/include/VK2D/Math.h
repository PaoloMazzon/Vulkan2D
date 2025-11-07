/// \file Math.h
/// \author Paolo Mazzon (Not really)
/// \brief Defines some math things, only include once
///
/// Paolo Mazzon added the C++ include guards and comments, otherwise this
/// is as it was originally

/* This code was originall written by Ali Emre Gülcü under the MIT license 
 * (specifically the math in this file, it was originally in a differnt file).
 * Since CGLM does not like Vulkan and the maintainer has expressed that Vulkan
 * support is not in the near future we use Ali's matrix code for Vulkan. Many
 * thanks to Ali Emre Gülcü.
 * 
 * MIT License
 * 
 * Copyright (c) 2018 Ali Emre Gülcü
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <math.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

void directionVector(float v[], float t[])
{
	v[0] = t[0];
	v[1] = t[1];
	v[2] = t[2];
	v[3] = 0.0f;
}

void positionVector(float v[], float t[])
{
	v[0] = t[0];
	v[1] = t[1];
	v[2] = t[2];
	v[3] = 1.0f;
}

void identityMatrix(float m[])
{
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void normalize(float v[])
{
	const float eps = 0.0009765625f;
	float mag = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	if(mag > eps && fabsf(1.0f - mag) > eps)
	{
		v[0] /= mag;
		v[1] /= mag;
		v[2] /= mag;
	}
}

float dot(float a[], float b[])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void cross(float a[], float b[], float c[])
{
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}

void multiplyVector(float a[], float b[], float c[])
{
	for(int row = 0; row < 4; row++)
		for(int itr = 0; itr < 4; itr++)
			c[row] += a[4 * row + itr] * b[itr];
}

void multiplyMatrix(float a[], float b[], float c[])
{
	for(int row = 0; row < 4; row++)
		for(int col = 0; col < 4; col++)
			for(int itr = 0; itr < 4; itr++)
				c[4 * row + col] += a[4 * row + itr] * b[4 * itr + col];
}

void scaleVector(float v[], float t)
{
	v[0] *= t;
	v[1] *= t;
	v[2] *= t;
}

void scaleMatrix(float m[], float v[])
{
	float k[] = {
		v[0], 0.0f, 0.0f, 0.0f,
		0.0f, v[1], 0.0f, 0.0f,
		0.0f, 0.0f, v[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	}, t[16] = {0};

	multiplyMatrix(k, m, t);
	memcpy(m, t, sizeof(t));
}

void translateVector(float v[], float t[])
{
	v[0] += t[0] * v[3];
	v[1] += t[1] * v[3];
	v[2] += t[2] * v[3];
}

void translateMatrix(float m[], float v[])
{
	float k[] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		v[0], v[1], v[2], 1.0f
	}, t[16] = {0};

 	multiplyMatrix(k, m, t);
	memcpy(m, t, sizeof(t));
}

void rotate(float f[], float w[], float r, int m)
{
	normalize(w);

	float x = w[0], y = w[1], z = w[2], s = sinf(r), c = cosf(r), d = 1 - cosf(r);
	float xx = x * x * d, xy = x * y * d, xz = x * z * d, yy = y * y * d, yz = y * z * d, zz = z * z * d;

	float k[] = {
		xx + c,     xy + s * z, xz - s * y, 0.0f,
		xy - s * z, yy + c,     yz + s * x, 0.0f,
		xz + s * y, yz - s * x, zz + c,     0.0f,
		0.0f,       0.0f,       0.0f,       1.0f
	};

	if(!m)
	{
		float t[4] = {0};
		multiplyVector(k, f, t);
		memcpy(f, t, sizeof(t));
	}

	else
	{
		float t[16] = {0};
		multiplyMatrix(k, f, t);
		memcpy(f, t, sizeof(t));
	}
}

void rotateVector(float v[], float w[], float r)
{
	rotate(v, w, r, 0);
}

void rotateMatrix(float m[], float w[], float r)
{
	rotate(m, w, r, 1);
}

void cameraMatrix(float m[], float eye[], float cent[], float top[])
{
	float fwd[] = {cent[0] - eye[0], cent[1] - eye[1], cent[2] - eye[2]};
	normalize(fwd);

	float left[3];
	cross(fwd, top, left);
	normalize(left);

	float up[3];
	cross(left, fwd, up);

	float k[] = {
		 left[0],         up[0],          -fwd[0],          0.0f,
		 left[1],         up[1],          -fwd[1],          0.0f,
		 left[2],         up[2],          -fwd[2],          0.0f,
		-dot(left, eye), -dot(up, eye),    dot(fwd, eye),   1.0f
	};

	memcpy(m, k, sizeof(k));
}

void orthographicMatrix(float m[], float h, float r, float n, float f)
{
	float k[] = {
		2 / (h * r),  0.0f,         0.0f,         0.0f,
		0.0f,        -2 / h,        0.0f,         0.0f,
		0.0f,         0.0f,         1 / (n - f),  0.0f,
		0.0f,         0.0f,         n / (n - f),  1.0f
	};

	memcpy(m, k, sizeof(k));
}

void frustumMatrix(float m[], float h, float r, float n, float f)
{
	float k[] = {
		2 * n / (h * r),  0.0f,             0.0f,             0.0f,
		0.0f,            -2 * n / h,        0.0f,             0.0f,
		0.0f,             0.0f,             f / (n - f),     -1.0f,
		0.0f,             0.0f,             n * f / (n - f),  0.0f
	};

	memcpy(m, k, sizeof(k));
}

void perspectiveMatrix(float m[], float fov, float asp, float n, float f)
{
	float t = tanf(fov / 2);

	float k[] = {
		1 / (t * asp),    0.0f,             0.0f,             0.0f,
		0.0f,            -1 / t,            0.0f,             0.0f,
		0.0f,             0.0f,             f / (n - f),     -1.0f,
		0.0f,             0.0f,             n * f / (n - f),  0.0f
	};

	memcpy(m, k, sizeof(k));
}

#ifdef __cplusplus
};
#endif